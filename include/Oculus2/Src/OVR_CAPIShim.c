/************************************************************************************

Filename    :   OVR_CAPIShim.c
Content     :   CAPI DLL user library
Created     :   November 20, 2014
Copyright   :   Copyright 2014 Oculus VR, LLC All Rights reserved.

Licensed under the Oculus VR Rift SDK License Version 3.2 (the "License");
you may not use the Oculus VR Rift SDK except in compliance with the License,
which is provided at the time of installation or download, or which
otherwise accompanies this software in either electronic or hard copy form.

You may obtain a copy of the License at

http://www.oculusvr.com/licenses/LICENSE-3.2

Unless required by applicable law or agreed to in writing, the Oculus VR SDK
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

************************************************************************************/


#include "OVR_CAPI.h"
#include "OVR_Version.h"
#include "OVR_ErrorCode.h"
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#if defined(_WIN32)
    #if defined(_MSC_VER)
        #pragma warning(push, 0)
    #endif
    #include <Windows.h>
    #if defined(_MSC_VER)
        #pragma warning(pop)
    #endif

    #include "../Include/OVR_CAPI_D3D.h"
#else
    #if defined(__APPLE__)
        #include <mach-o/dyld.h>
        #include <sys/syslimits.h>
        #include <libgen.h>
        #include <pwd.h>
        #include <unistd.h>
    #endif
    #include <dlfcn.h>
    #include <sys/stat.h>
    #include <unistd.h>
#endif
#include "../Include/OVR_CAPI_GL.h"


#if defined(_MSC_VER)
    #pragma warning(push)
    #pragma warning(disable: 4996) // 'getenv': This function or variable may be unsafe.
#endif


// -----------------------------------------------------------------------------------
// ***** OVR_ENABLE_DEVELOPER_SEARCH
//
// If defined then our shared library loading code searches for developer build
// directories.
//
#if !defined(OVR_ENABLE_DEVELOPER_SEARCH)
#endif


// -----------------------------------------------------------------------------------
// ***** OVR_BUILD_DEBUG
//
// Defines OVR_BUILD_DEBUG when the compiler default debug preprocessor is set.
//
// If you want to control the behavior of these flags, then explicitly define
// either -DOVR_BUILD_RELEASE or -DOVR_BUILD_DEBUG in the compiler arguments.

#if !defined(OVR_BUILD_DEBUG) && !defined(OVR_BUILD_RELEASE)
    #if defined(_MSC_VER)
        #if defined(_DEBUG)
            #define OVR_BUILD_DEBUG
        #endif
    #else
        #if defined(DEBUG)
            #define OVR_BUILD_DEBUG
        #endif
    #endif
#endif


//-----------------------------------------------------------------------------------
// ***** FilePathCharType, ModuleHandleType, ModuleFunctionType
//
#if defined(_WIN32)                                       // We need to use wchar_t on Microsoft platforms, as that's the native file system character type.
    #define FilePathCharType       wchar_t                // #define instead of typedef because debuggers (VC++, XCode) don't recognize typedef'd types as a string type.
    typedef HMODULE                ModuleHandleType;
    typedef FARPROC                ModuleFunctionType;
#else
    #define FilePathCharType       char
    typedef void*                  ModuleHandleType;
    typedef void*                  ModuleFunctionType;
#endif

#define ModuleHandleTypeNull   ((ModuleHandleType)NULL)
#define ModuleFunctionTypeNull ((ModuleFunctionType)NULL)


//-----------------------------------------------------------------------------------
// ***** OVR_MAX_PATH
//
#if !defined(OVR_MAX_PATH)
    #if defined(_WIN32)
        #define OVR_MAX_PATH  _MAX_PATH
    #elif defined(__APPLE__)
        #define OVR_MAX_PATH  PATH_MAX
    #else
        #define OVR_MAX_PATH  1024
    #endif
#endif



//-----------------------------------------------------------------------------------
// ***** OVR_DECLARE_IMPORT
//
// Creates typedef and pointer declaration for a function of a given signature.
// The typedef is <FunctionName>Type, and the pointer is <FunctionName>Ptr.
//
// Example usage:
//     int MultiplyValues(float x, float y);  // Assume this function exists in an external shared library. We don't actually need to redeclare it.
//     OVR_DECLARE_IMPORT(int, MultiplyValues, (float x, float y)) // This creates a local typedef and pointer for it.

#define OVR_DECLARE_IMPORT(ReturnValue, FunctionName, Arguments)  \
    typedef ReturnValue (OVR_CDECL *FunctionName##Type)Arguments; \
    FunctionName##Type FunctionName##Ptr = NULL;



//-----------------------------------------------------------------------------------
// ***** OVR_GETFUNCTION
//
// Loads <FunctionName>Ptr from hLibOVR if not already loaded.
// Assumes a variable named <FunctionName>Ptr of type <FunctionName>Type exists which is called <FunctionName> in LibOVR.
//
// Example usage:
//     OVR_GETFUNCTION(MultiplyValues)    // Normally this would be done on library init and not before every usage.
//     int result = MultiplyValuesPtr(3.f, 4.f);

#if !defined(OVR_DLSYM)
    #if defined(_WIN32)
        #define OVR_DLSYM(dlImage, name) GetProcAddress(dlImage, name)
    #else
        #define OVR_DLSYM(dlImage, name) dlsym(dlImage, name)
    #endif
#endif

#define OVR_GETFUNCTION(f)             \
    if(!f##Ptr)                        \
    {                                  \
        union                          \
        {                              \
            f##Type p1;                \
            ModuleFunctionType p2;     \
        } u;                           \
        u.p2 = OVR_DLSYM(hLibOVR, #f); \
        f##Ptr = u.p1;                 \
    }


static size_t OVR_strlcpy(char* dest, const char* src, size_t destsize)
{
    const char* s = src;
    size_t      n = destsize;

    if(n && --n)
    {
        do{
            if((*dest++ = *s++) == 0)
                break;
        } while(--n);
    }

    if(!n)
    {
        if(destsize)
            *dest = 0;
        while(*s++)
            { }
    }

    return (size_t)((s - src) - 1);
}


static size_t OVR_strlcat(char* dest, const char* src, size_t destsize)
{
    const size_t d = destsize ? strlen(dest) : 0;
    const size_t s = strlen(src);
    const size_t t = s + d;

    if(t < destsize)
        memcpy(dest + d, src, (s + 1) * sizeof(*src));
    else
    {
        if(destsize)
        {
            memcpy(dest + d, src, ((destsize - d) - 1) * sizeof(*src));
            dest[destsize - 1] = 0;
        }
    }

    return t;
}


#if defined(__APPLE__)
    static ovrBool OVR_strend(const char* pStr, const char* pFind, size_t strLength, size_t findLength)
    {
        if(strLength == (size_t)-1)
            strLength = strlen(pStr);
        if(findLength == (size_t)-1)
            findLength = strlen(pFind);
        if(strLength >= findLength)
            return (strcmp(pStr + strLength - findLength, pFind) == 0);
        return ovrFalse;
    }

    static ovrBool OVR_isBundleFolder(const char* filePath)
    {
        static const char* extensionArray[] = { ".app", ".bundle", ".framework", ".plugin", ".kext" };
        size_t i;

        for(i = 0; i < sizeof(extensionArray)/sizeof(extensionArray[0]); i++)
        {
            if(OVR_strend(filePath, extensionArray[i], (size_t)-1, (size_t)-1))
                return ovrTrue;
        }

        return ovrFalse;
    }
#endif


#if defined(OVR_ENABLE_DEVELOPER_SEARCH)

// Returns true if the path begins with the given prefix.
// Doesn't support non-ASCII paths, else the return value may be incorrect.
static int OVR_PathStartsWith(const FilePathCharType* path, const char* prefix)
{
    while(*prefix)
    {
        if(tolower((unsigned char)*path++) != tolower((unsigned char)*prefix++))
            return ovrFalse;
    }

    return ovrTrue;
}

#endif


static ovrBool OVR_GetCurrentWorkingDirectory(FilePathCharType* directoryPath, size_t directoryPathCapacity)
{
    #if defined(_WIN32)
        DWORD dwSize = GetCurrentDirectoryW((DWORD)directoryPathCapacity, directoryPath);

        if((dwSize > 0) && (directoryPathCapacity > 1)) // Test > 1 so we have room to possibly append a \ char.
        {
            size_t length = wcslen(directoryPath);

            if((length == 0) || ((directoryPath[length - 1] != L'\\') && (directoryPath[length - 1] != L'/')))
            {
                directoryPath[length++] = L'\\';
                directoryPath[length]   = L'\0';
            }

            return ovrTrue;
        }

    #else
        char* cwd = getcwd(directoryPath, directoryPathCapacity);

        if(cwd && directoryPath[0] && (directoryPathCapacity > 1)) // Test > 1 so we have room to possibly append a / char.
        {
            size_t length = strlen(directoryPath);

            if((length == 0) || (directoryPath[length - 1] != '/'))
            {
                directoryPath[length++] = '/';
                directoryPath[length]   = '\0';
            }

            return ovrTrue;
        }
    #endif

    if(directoryPathCapacity > 0)
        directoryPath[0] = '\0';

    return ovrFalse;
}


// The appContainer argument is specific currently to only Macintosh. If true and the application is a .app bundle then it returns the
// location of the bundle and not the path to the executable within the bundle. Else return the path to the executable binary itself.
// The moduleHandle refers to the relevant dynamic (a.k.a. shared) library. The main executable is the main module, and each of the shared
// libraries is a module. This way you can specify that you want to know the directory of the given shared library, which may be different
// from the main executable. If the moduleHandle is NULL then the current application module is used.
static ovrBool OVR_GetCurrentApplicationDirectory(FilePathCharType* directoryPath, size_t directoryPathCapacity, ovrBool appContainer, ModuleHandleType moduleHandle)
{
    #if defined(_WIN32)
        DWORD length = GetModuleFileNameW(moduleHandle, directoryPath, (DWORD)directoryPathCapacity);
        DWORD pos;

        if((length != 0) && (length < (DWORD)directoryPathCapacity)) // If there wasn't an error and there was enough capacity...
        {
            for(pos = length; (pos > 0) && (directoryPath[pos] != '\\') && (directoryPath[pos] != '/'); --pos)
            {
                if((directoryPath[pos - 1] != '\\') && (directoryPath[pos - 1] != '/'))
                   directoryPath[pos - 1] = 0;
            }

            return ovrTrue;
        }

        (void)appContainer; // Not used on this platform.

    #elif defined(__APPLE__)
        uint32_t directoryPathCapacity32 = (uint32_t)directoryPathCapacity;
        int result = _NSGetExecutablePath(directoryPath, &directoryPathCapacity32);

        if(result == 0) // If success...
        {
            char realPath[OVR_MAX_PATH];

            if(realpath(directoryPath, realPath)) // realpath returns the canonicalized absolute file path.
            {
                size_t length = 0;

                if(appContainer) // If the caller wants the path to the containing bundle...
                {
                    char    containerPath[OVR_MAX_PATH];
                    ovrBool pathIsContainer;

                    OVR_strlcpy(containerPath, realPath, sizeof(containerPath));
                    pathIsContainer = OVR_isBundleFolder(containerPath);

                    while(!pathIsContainer && strncmp(containerPath, ".", OVR_MAX_PATH) && strncmp(containerPath, "/", OVR_MAX_PATH)) // While the container we're looking for is not found and while the path doesn't start with a . or /
                    {
                        OVR_strlcpy(containerPath, dirname(containerPath), sizeof(containerPath));
                        pathIsContainer = OVR_isBundleFolder(containerPath);
                    }

                    if(pathIsContainer)
                        length = OVR_strlcpy(directoryPath, containerPath, directoryPathCapacity);
                }

                if(length == 0) // If not set above in the appContainer block...
                    length = OVR_strlcpy(directoryPath, realPath, directoryPathCapacity);

                while(length-- && (directoryPath[length] != '/'))
                    directoryPath[length] = '\0'; // Strip the file name from the file path, leaving a trailing / char.

                return ovrTrue;
            }
        }

        (void)moduleHandle;  // Not used on this platform.

    #else
        ssize_t length = readlink("/proc/self/exe", directoryPath, directoryPathCapacity);
        ssize_t pos;

        if(length > 0)
        {
            for(pos = length; (pos > 0) && (directoryPath[pos] != '/'); --pos)
            {
                if(directoryPath[pos - 1] != '/')
                   directoryPath[pos - 1]  = '\0';
            }

            return ovrTrue;
        }

        (void)appContainer; // Not used on this platform.
        (void)moduleHandle;
    #endif

    if(directoryPathCapacity > 0)
        directoryPath[0] = '\0';

    return ovrFalse;
}


#if defined(_WIN32) || defined(OVR_ENABLE_DEVELOPER_SEARCH) // Used only in these cases

// Get the file path to the current module's (DLL or EXE) directory within the current process.
// Will be different from the process module handle if the current module is a DLL and is in a different directory than the EXE module.
// If successful then directoryPath will be valid and ovrTrue is returned, else directoryPath will be empty and ovrFalse is returned.
static ovrBool OVR_GetCurrentModuleDirectory(FilePathCharType* directoryPath, size_t directoryPathCapacity, ovrBool appContainer)
{
    #if defined(_WIN32)
        HMODULE hModule;
        BOOL result = GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)(uintptr_t)OVR_GetCurrentModuleDirectory, &hModule);
        if(result)
            OVR_GetCurrentApplicationDirectory(directoryPath, directoryPathCapacity, ovrTrue, hModule);
        else
            directoryPath[0] = 0;

        (void)appContainer;

        return directoryPath[0] ? ovrTrue : ovrFalse;
    #else
        return OVR_GetCurrentApplicationDirectory(directoryPath, directoryPathCapacity, appContainer, NULL);
    #endif
}

#endif


static ModuleHandleType OVR_OpenLibrary(const FilePathCharType* libraryPath)
{
    #if defined(_WIN32)
        return LoadLibraryW(libraryPath);
    #else
        // Don't bother trying to dlopen() a file that is not even there.
        if (access(libraryPath, X_OK | R_OK ) != 0)
        {
            return NULL;
        }

        dlerror(); // Clear any previous dlopen() errors

        // Use RTLD_NOW because we don't want unexpected stalls at runtime, and the library isn't very large.
        // Use RTLD_LOCAL to avoid unilaterally exporting resolved symbols to the rest of this process.
        void *lib = dlopen(libraryPath, RTLD_NOW | RTLD_LOCAL);

        if (!lib)
        {
            #if defined(__APPLE__)
            // TODO: Output the error in whatever logging system OSX uses (jhughes)
            #else  // __APPLE__
            fprintf(stderr, "ERROR: Can't load '%s':\n%s\n", libraryPath, dlerror());
            #endif // __APPLE__
        }

        return lib;
    #endif
}


/*  Currently not in use, but expected to be used in the future
static void OVR_CloseLibrary(ModuleHandleType hLibrary)
{
    if (hLibrary)
    {
        #if defined(_WIN32)
            FreeLibrary(hLibrary);
        #else
            dlclose(hLibrary);
        #endif
    }
}
*/


// Returns a valid ModuleHandleType (e.g. Windows HMODULE) or returns ModuleHandleTypeNull (e.g. NULL).
// The caller is required to eventually call OVR_CloseLibrary on a valid return handle.
//
static ModuleHandleType OVR_FindLibraryPath(int requestedProductVersion, int requestedMajorVersion,
                               FilePathCharType* libraryPath, size_t libraryPathCapacity)
{
    ModuleHandleType moduleHandle;
    int printfResult;
    FilePathCharType developerDir[OVR_MAX_PATH] = { '\0' };

    #if defined(_MSC_VER)
        #if defined(_WIN64)
            const char* pBitDepth = "64";
        #else
            const char* pBitDepth = "32";
        #endif
    #elif defined(__APPLE__)
		// For Apple platforms we are using a Universal Binary LibOVRRT dylib which has both 32 and 64 in it.
	#else // Other Unix.
        #if defined(__x86_64__)
            const char* pBitDepth = "64";
        #else
            const char* pBitDepth = "32";
        #endif
    #endif

    moduleHandle = ModuleHandleTypeNull;
    if(libraryPathCapacity)
        libraryPath[0] = '\0';

    // Note: OVR_ENABLE_DEVELOPER_SEARCH is deprecated in favor of the simpler LIBOVR_DLL_DIR, as the edge
    // case uses of the former created some complications that may be best solved by simply using a LIBOVR_DLL_DIR
    // environment variable which the user can set in their debugger or system environment variables.
    #if (defined(_MSC_VER) || defined(_WIN32)) && !defined(OVR_FILE_PATH_SEPARATOR)
        #define OVR_FILE_PATH_SEPARATOR "\\"
    #else
        #define OVR_FILE_PATH_SEPARATOR "/"
    #endif

    {
        const char* pLibOvrDllDir = getenv("LIBOVR_DLL_DIR"); // Example value: /dev/OculusSDK/Main/LibOVR/Mac/Debug/

        if(pLibOvrDllDir)
        {
            char developerDir8[OVR_MAX_PATH];
            size_t length = OVR_strlcpy(developerDir8, pLibOvrDllDir, sizeof(developerDir8)); // If missing a trailing path separator then append one.

            if((length > 0) && (length < sizeof(developerDir8)) && (developerDir8[length - 1] != OVR_FILE_PATH_SEPARATOR[0]))
            {
                length = OVR_strlcat(developerDir8, OVR_FILE_PATH_SEPARATOR, sizeof(developerDir8));

                if(length < sizeof(developerDir8))
                {
                    #if defined(_WIN32)
                        size_t i;
                        for(i = 0; i <= length; ++i) // ASCII conversion of 8 to 16 bit text.
                            developerDir[i] = (FilePathCharType)(uint8_t)developerDir8[i];
                    #else
                        OVR_strlcpy(developerDir, developerDir8, sizeof(developerDir));
                    #endif
                }
            }
        }
    }

    // Support checking for a developer library location override via the OVR_SDK_ROOT environment variable.
    // This pathway is deprecated in favor of using LIBOVR_DLL_DIR instead.
    #if defined(OVR_ENABLE_DEVELOPER_SEARCH)
    if (!developerDir[0]) // If not already set by LIBOVR_DLL_PATH...
    {
        // __FILE__ maps to <sdkRoot>/LibOVR/Src/OVR_CAPIShim.c
        char sdkRoot[OVR_MAX_PATH];
        char* pLibOVR;
        size_t i;

        // We assume that __FILE__ returns a full path, which isn't the case for some compilers.
        // Need to compile with /FC under VC++ for __FILE__ to expand to the full file path.
        // clang expands __FILE__ to a full path by default.
        OVR_strlcpy(sdkRoot, __FILE__, sizeof(sdkRoot));
        for(i = 0; sdkRoot[i]; ++i)
            sdkRoot[i] = (char)tolower(sdkRoot[i]); // Microsoft doesn't maintain case.
        pLibOVR = strstr(sdkRoot, "libovr");
        if(pLibOVR && (pLibOVR > sdkRoot))
            pLibOVR[-1] = '\0';
        else
            sdkRoot[0] = '\0';

        if(sdkRoot[0])
        {
            // We want to use a developer version of the library only if the application is also being executed from
            // a developer location. Ideally we would do this by checking that the relative path from the executable to
            // the shared library is the same at runtime as it was when the executable was first built, but we don't have
            // an easy way to do that from here and it would require some runtime help from the application code.
            // Instead we verify that the application is simply in the same developer tree that was was when built.
            // We could put in some additional logic to make it very likely to know if the EXE is in its original location.
            FilePathCharType modulePath[OVR_MAX_PATH];
            const ovrBool pathMatch = OVR_GetCurrentModuleDirectory(modulePath, OVR_MAX_PATH, ovrTrue) &&
                                        (OVR_PathStartsWith(modulePath, sdkRoot) == ovrTrue);
            if(pathMatch == ovrFalse)
            {
                sdkRoot[0] = '\0'; // The application module is not in the developer tree, so don't try to use the developer shared library.
            }
        }

        if(sdkRoot[0])
        {
            #if defined(OVR_BUILD_DEBUG)
                const char* pConfigDirName = "Debug";
            #else
                const char* pConfigDirName = "Release";
            #endif

            #if defined(_MSC_VER)
                #if defined(_WIN64)
                    const char* pArchDirName = "x64";
                #else
                    const char* pArchDirName = "Win32";
                #endif
            #else
                #if defined(__x86_64__)
                    const char* pArchDirName = "x86_64";
                #else
                    const char* pArchDirName = "i386";
                #endif
            #endif

            #if defined(_MSC_VER) && (_MSC_VER == 1600)
                const char* pCompilerVersion = "VS2010";
            #elif defined(_MSC_VER) && (_MSC_VER == 1700)
                const char* pCompilerVersion = "VS2012";
            #elif defined(_MSC_VER) && (_MSC_VER == 1800)
                const char* pCompilerVersion = "VS2013";
            #elif defined(_MSC_VER) && (_MSC_VER == 1900)
                const char* pCompilerVersion = "VS2014";
            #endif

            #if defined(_WIN32)
                int count = swprintf_s(developerDir, OVR_MAX_PATH, L"%hs\\LibOVR\\Lib\\Windows\\%hs\\%hs\\%hs\\",
                                        sdkRoot, pArchDirName, pConfigDirName, pCompilerVersion);
            #elif defined(__APPLE__)
                // Apple/XCode doesn't let you specify an arch in build paths, which is OK if we build a universal binary.
                (void)pArchDirName;
                int count = snprintf(developerDir, OVR_MAX_PATH, "%s/LibOVR/Lib/Mac/%s/",
                                        sdkRoot, pConfigDirName);
            #else
                int count = snprintf(developerDir, OVR_MAX_PATH, "%s/LibOVR/Lib/Linux/%s/%s/",
                                        sdkRoot, pArchDirName, pConfigDirName);
            #endif

            if((count < 0) || (count >= (int)OVR_MAX_PATH)) // If there was an error or capacity overflow... clear the string.
            {
                developerDir[0] = '\0';
            }
        }
    }
    #endif // OVR_ENABLE_DEVELOPER_SEARCH

    {
        FilePathCharType cwDir[OVR_MAX_PATH]; // Will be filled in below.
        FilePathCharType appDir[OVR_MAX_PATH];
        size_t i;

        #if defined(_WIN32)
            FilePathCharType        moduleDir[OVR_MAX_PATH];
            const FilePathCharType* directoryArray[5];
            directoryArray[0] = cwDir;
            directoryArray[1] = moduleDir;
            directoryArray[2] = appDir;
            directoryArray[3] = developerDir;   // Developer directory.
            directoryArray[4] = L"";            // No directory, which causes Windows to use the standard search strategy to find the DLL.

            OVR_GetCurrentModuleDirectory(moduleDir, sizeof(moduleDir)/sizeof(moduleDir[0]), ovrTrue);

        #elif defined(__APPLE__)
            // https://developer.apple.com/library/mac/documentation/Darwin/Reference/ManPages/man1/dyld.1.html

            FilePathCharType        homeDir[OVR_MAX_PATH];
            FilePathCharType        homeFrameworkDir[OVR_MAX_PATH];
            const FilePathCharType* directoryArray[5];
            size_t                  homeDirLength = 0;

            const char* pHome = getenv("HOME"); // Try getting the HOME environment variable.

            if (pHome)
            {
                homeDirLength = OVR_strlcpy(homeDir, pHome, sizeof(homeDir));
            }
            else
            {
                // https://developer.apple.com/library/mac/documentation/Darwin/Reference/ManPages/man3/getpwuid_r.3.html
                const long pwBufferSize = sysconf(_SC_GETPW_R_SIZE_MAX);

                if (pwBufferSize != -1)
                {
                    char pwBuffer[pwBufferSize];
                    struct passwd  pw;
                    struct passwd* pwResult = NULL;

                    if ((getpwuid_r(getuid(), &pw, pwBuffer, pwBufferSize, &pwResult) == 0) && pwResult)
                        homeDirLength = OVR_strlcpy(homeDir, pw.pw_dir, sizeof(homeDir));
                }
            }

            if (homeDirLength)
            {
                if (homeDir[homeDirLength - 1] == '/')
                    homeDir[homeDirLength - 1] = '\0';
                OVR_strlcpy(homeFrameworkDir, homeDir, sizeof(homeFrameworkDir));
                OVR_strlcat(homeFrameworkDir, "/Library/Frameworks/", sizeof(homeFrameworkDir));
            }
            else
            {
                homeFrameworkDir[0] = '\0';
            }

            directoryArray[0] = cwDir;
            directoryArray[1] = appDir;
            directoryArray[2] = homeFrameworkDir;           // ~/Library/Frameworks/
            directoryArray[3] = "/Library/Frameworks/";     // DYLD_FALLBACK_FRAMEWORK_PATH
            directoryArray[4] = developerDir;               // Developer directory.

        #else
            #define STR1(x) #x
            #define STR(x)  STR1(x)
            #ifdef LIBDIR
                #define TEST_LIB_DIR STR(LIBDIR) "/"
            #else
                #define TEST_LIB_DIR appDir
            #endif

            const FilePathCharType* directoryArray[5];
            directoryArray[0] = cwDir;
            directoryArray[1] = TEST_LIB_DIR;           // Directory specified by LIBDIR if defined.
            directoryArray[2] = developerDir;           // Developer directory.
            directoryArray[3] = "/usr/local/lib/";
            directoryArray[4] = "/usr/lib/";
        #endif

        OVR_GetCurrentWorkingDirectory(cwDir, sizeof(cwDir) / sizeof(cwDir[0]));
        OVR_GetCurrentApplicationDirectory(appDir, sizeof(appDir) / sizeof(appDir[0]), ovrTrue, NULL);

        // Versioned file expectations.
        //     Windows: LibOVRRT<BIT_DEPTH>_<PRODUCT_VERSION>_<MAJOR_VERSION>.dll                                  // Example: LibOVRRT64_1_1.dll -- LibOVRRT 64 bit, product 1, major version 1, minor/patch/build numbers unspecified in the name.
        //     Mac:     LibOVRRT_<PRODUCT_VERSION>.framework/Versions/<MAJOR_VERSION>/LibOVRRT_<PRODUCT_VERSION>   // We are not presently using the .framework bundle's Current directory to hold the version number. This may change.
        //     Linux:   libOVRRT<BIT_DEPTH>_<PRODUCT_VERSION>.so.<MAJOR_VERSION>                                   // The file on disk may contain a minor version number, but a symlink is used to map this major-only version to it.

        // Since we are manually loading the LibOVR dynamic library, we need to look in various locations for a file
        // that matches our requirements. The functionality required is somewhat similar to the operating system's
        // dynamic loader functionality. Each OS has some differences in how this is handled.
        // Future versions of this may iterate over all libOVRRT.so.* files in the directory and use the one that matches our requirements.
        //
        // We need to look for a library that matches the product version and major version of the caller's request,
        // and that library needs to support a minor version that is >= the requested minor version. Currently we
        // don't test the minor version here, as the library is named based only on the product and major version.
        // Currently the minor version test is handled via the initialization of the library and the initialization
        // fails if minor version cannot be supported by the library. The reason this is done during initialization
        // is that the library can at runtime support multiple minor versions based on the user's request. To the
        // external user, all that matters it that they call ovr_Initialize with a requested version and it succeeds
        // or fails.
        //
        // The product version is something that is at a higher level than the major version, and is not something that's
        // always seen in libraries (an example is the well-known LibXml2 library, in which the 2 is essentially the product version).

        for(i = 0; i < sizeof(directoryArray)/sizeof(directoryArray[0]); ++i)
        {
            #if defined(_WIN32)
                printfResult = swprintf(libraryPath, libraryPathCapacity, L"%lsLibOVRRT%hs_%d_%d.dll", directoryArray[i], pBitDepth, requestedProductVersion, requestedMajorVersion);

            #elif defined(__APPLE__)
                // https://developer.apple.com/library/mac/documentation/MacOSX/Conceptual/BPFrameworks/Concepts/VersionInformation.html
                // Macintosh application bundles have the option of embedding dependent frameworks within the application
                // bundle itself. A problem with that is that it doesn't support vendor-supplied updates to the framework.
                printfResult = snprintf(libraryPath, libraryPathCapacity, "%sLibOVRRT_%d.framework/Versions/%d/LibOVRRT_%d", directoryArray[i], requestedProductVersion, requestedMajorVersion, requestedProductVersion);

            #else // Unix
                // Applications that depend on the OS (e.g. ld-linux / ldd) can rely on the library being in a common location
                // such as /usr/lib or can rely on the -rpath linker option to embed a path for the OS to check for the library,
                // or can rely on the LD_LIBRARY_PATH environment variable being set. It's generally not recommended that applications
                // depend on LD_LIBRARY_PATH be globally modified, partly due to potentialy security issues.
                // Currently we check the current application directory, current working directory, and then /usr/lib and possibly others.
                printfResult = snprintf(libraryPath, libraryPathCapacity, "%slibOVRRT%s_%d.so.%d", directoryArray[i], pBitDepth, requestedProductVersion, requestedMajorVersion);
            #endif

            if((printfResult >= 0) && (printfResult < (int)libraryPathCapacity))
            {
                moduleHandle = OVR_OpenLibrary(libraryPath);
                if(moduleHandle != ModuleHandleTypeNull)
                    return moduleHandle;
            }
        }
    }

    return moduleHandle;
}



//-----------------------------------------------------------------------------------
// ***** hLibOVR
//
// global handle to the LivOVR shared library.
//
static ModuleHandleType hLibOVR = NULL;

// This function is currently unsupported.
ModuleHandleType ovr_GetLibOVRRTHandle()
{
    return hLibOVR;
}



//-----------------------------------------------------------------------------------
// ***** Function declarations
//
// To consider: Move OVR_DECLARE_IMPORT and the declarations below to OVR_CAPI.h
//
OVR_DECLARE_IMPORT(ovrBool,          ovr_InitializeRenderingShimVersion, (int requestedMinorVersion))
OVR_DECLARE_IMPORT(ovrResult,        ovr_Initialize, (const ovrInitParams* params))
OVR_DECLARE_IMPORT(ovrBool,          ovr_Shutdown, ())
OVR_DECLARE_IMPORT(const char*,      ovr_GetVersionString, ())
OVR_DECLARE_IMPORT(void,             ovr_GetLastErrorInfo, (ovrErrorInfo* errorInfo))
OVR_DECLARE_IMPORT(ovrResult,        ovrHmd_Detect, ())
OVR_DECLARE_IMPORT(ovrResult,        ovrHmd_Create, (int index, ovrHmd* pHmd))
OVR_DECLARE_IMPORT(void,             ovrHmd_Destroy, (ovrHmd hmd))
OVR_DECLARE_IMPORT(ovrResult,        ovrHmd_CreateDebug, (ovrHmdType type, ovrHmd* pHmd))
OVR_DECLARE_IMPORT(unsigned int,     ovrHmd_GetEnabledCaps, (ovrHmd hmd))
OVR_DECLARE_IMPORT(void,             ovrHmd_SetEnabledCaps, (ovrHmd hmd, unsigned int hmdCaps))
OVR_DECLARE_IMPORT(ovrResult,        ovrHmd_ConfigureTracking, (ovrHmd hmd, unsigned int supportedTrackingCaps, unsigned int requiredTrackingCaps))
OVR_DECLARE_IMPORT(void,             ovrHmd_RecenterPose, (ovrHmd hmd))
OVR_DECLARE_IMPORT(ovrTrackingState, ovrHmd_GetTrackingState, (ovrHmd hmd, double absTime))
OVR_DECLARE_IMPORT(ovrSizei,         ovrHmd_GetFovTextureSize, (ovrHmd hmd, ovrEyeType eye, ovrFovPort fov, float pixelsPerDisplayPixel))
OVR_DECLARE_IMPORT(void,             ovrHmd_SubmitFrameLegacy, (ovrHmd hmd, unsigned int frameIndex, const ovrPosef renderPose[2], const ovrTexture eyeTexture[2]))
OVR_DECLARE_IMPORT(ovrResult,        ovrHmd_SubmitFrame, (ovrHmd hmd, unsigned int frameIndex, const ovrViewScaleDesc* viewScaleDesc, ovrLayerHeader const * const * layerPtrList, unsigned int layerCount))
OVR_DECLARE_IMPORT(ovrEyeRenderDesc, ovrHmd_GetRenderDesc, (ovrHmd hmd, ovrEyeType eyeType, ovrFovPort fov))
OVR_DECLARE_IMPORT(ovrFrameTiming,   ovrHmd_GetFrameTiming, (ovrHmd hmd, unsigned int frameIndex))
OVR_DECLARE_IMPORT(void,             ovrHmd_ResetFrameTiming, (ovrHmd hmd, unsigned int frameIndex))
OVR_DECLARE_IMPORT(double,           ovr_GetTimeInSeconds, ())
OVR_DECLARE_IMPORT(ovrBool,          ovrHmd_GetBool, (ovrHmd hmd, const char* propertyName, ovrBool defaultVal))
OVR_DECLARE_IMPORT(ovrBool,          ovrHmd_SetBool, (ovrHmd hmd, const char* propertyName, ovrBool value))
OVR_DECLARE_IMPORT(int,              ovrHmd_GetInt, (ovrHmd hmd, const char* propertyName, int defaultVal))
OVR_DECLARE_IMPORT(ovrBool,          ovrHmd_SetInt, (ovrHmd hmd, const char* propertyName, int value))
OVR_DECLARE_IMPORT(float,            ovrHmd_GetFloat, (ovrHmd hmd, const char* propertyName, float defaultVal))
OVR_DECLARE_IMPORT(ovrBool,          ovrHmd_SetFloat, (ovrHmd hmd, const char* propertyName, float value))
OVR_DECLARE_IMPORT(unsigned int,     ovrHmd_GetFloatArray, (ovrHmd hmd, const char* propertyName, float values[], unsigned int arraySize))
OVR_DECLARE_IMPORT(ovrBool,          ovrHmd_SetFloatArray, (ovrHmd hmd, const char* propertyName, const float values[], unsigned int arraySize))
OVR_DECLARE_IMPORT(const char*,      ovrHmd_GetString, (ovrHmd hmd, const char* propertyName, const char* defaultVal))
OVR_DECLARE_IMPORT(ovrBool,          ovrHmd_SetString, (ovrHmd hmddesc, const char* propertyName, const char* value))
OVR_DECLARE_IMPORT(int,              ovr_TraceMessage, (int level, const char* message))

#if defined (_WIN32)
OVR_DECLARE_IMPORT(ovrResult, ovrHmd_CreateSwapTextureSetD3D11, (ovrHmd hmddesc, ID3D11Device* device, const D3D11_TEXTURE2D_DESC* desc, ovrSwapTextureSet** outTextureSet))
OVR_DECLARE_IMPORT(ovrResult, ovrHmd_CreateMirrorTextureD3D11,  (ovrHmd hmddesc, ID3D11Device* device, const D3D11_TEXTURE2D_DESC* desc, ovrTexture** outMirrorTexture))
#endif

OVR_DECLARE_IMPORT(ovrResult, ovrHmd_CreateSwapTextureSetGL, (ovrHmd hmddesc, GLuint format, int width, int height, ovrSwapTextureSet** outTextureSet))
OVR_DECLARE_IMPORT(ovrResult, ovrHmd_CreateMirrorTextureGL,  (ovrHmd hmddesc, GLuint format, int width, int height, ovrTexture** outMirrorTexture))

OVR_DECLARE_IMPORT(void, ovrHmd_DestroySwapTextureSet, (ovrHmd hmddesc, ovrSwapTextureSet* textureSet))
OVR_DECLARE_IMPORT(void, ovrHmd_DestroyMirrorTexture, (ovrHmd hmddesc, ovrTexture* textureSet))
OVR_DECLARE_IMPORT(ovrResult, ovrHmd_SetQueueAheadFraction, (ovrHmd hmddesc, float queueAheadFraction))


static ovrResult OVR_LoadSharedLibrary(int requestedMinorVersion, int requestedPatchVersion)
{
    FilePathCharType filePath[OVR_MAX_PATH];

    if(hLibOVR)
        return ovrSuccess;

    hLibOVR = OVR_FindLibraryPath(requestedMinorVersion, requestedPatchVersion,
                             filePath, sizeof(filePath) / sizeof(filePath[0]));
    if(!hLibOVR)
        return ovrError_LibLoad;

    OVR_GETFUNCTION(ovr_InitializeRenderingShimVersion)
    OVR_GETFUNCTION(ovr_Initialize)
    OVR_GETFUNCTION(ovr_Shutdown)
    OVR_GETFUNCTION(ovr_GetVersionString)
    OVR_GETFUNCTION(ovr_GetLastErrorInfo)
    OVR_GETFUNCTION(ovrHmd_Detect)
    OVR_GETFUNCTION(ovrHmd_Create)
    OVR_GETFUNCTION(ovrHmd_Destroy)
    OVR_GETFUNCTION(ovrHmd_CreateDebug)
    OVR_GETFUNCTION(ovrHmd_GetEnabledCaps)
    OVR_GETFUNCTION(ovrHmd_SetEnabledCaps)
    OVR_GETFUNCTION(ovrHmd_ConfigureTracking)
    OVR_GETFUNCTION(ovrHmd_RecenterPose)
    OVR_GETFUNCTION(ovrHmd_GetTrackingState)
    OVR_GETFUNCTION(ovrHmd_GetFovTextureSize)
    OVR_GETFUNCTION(ovrHmd_SubmitFrame)
    OVR_GETFUNCTION(ovrHmd_GetRenderDesc)
    OVR_GETFUNCTION(ovrHmd_GetFrameTiming)
    OVR_GETFUNCTION(ovrHmd_ResetFrameTiming)
    OVR_GETFUNCTION(ovr_GetTimeInSeconds)
    OVR_GETFUNCTION(ovrHmd_GetBool)
    OVR_GETFUNCTION(ovrHmd_SetBool)
    OVR_GETFUNCTION(ovrHmd_GetInt)
    OVR_GETFUNCTION(ovrHmd_SetInt)
    OVR_GETFUNCTION(ovrHmd_GetFloat)
    OVR_GETFUNCTION(ovrHmd_SetFloat)
    OVR_GETFUNCTION(ovrHmd_GetFloatArray)
    OVR_GETFUNCTION(ovrHmd_SetFloatArray)
    OVR_GETFUNCTION(ovrHmd_GetString)
    OVR_GETFUNCTION(ovrHmd_SetString)
    OVR_GETFUNCTION(ovr_TraceMessage)
#if defined (_WIN32)
    OVR_GETFUNCTION(ovrHmd_CreateSwapTextureSetD3D11)
    OVR_GETFUNCTION(ovrHmd_CreateMirrorTextureD3D11)
#endif
    OVR_GETFUNCTION(ovrHmd_CreateSwapTextureSetGL)
    OVR_GETFUNCTION(ovrHmd_CreateMirrorTextureGL)
    OVR_GETFUNCTION(ovrHmd_DestroySwapTextureSet)
    OVR_GETFUNCTION(ovrHmd_DestroyMirrorTexture)
    OVR_GETFUNCTION(ovrHmd_SetQueueAheadFraction)

    return ovrSuccess;
}

static void OVR_UnloadSharedLibrary()
{
    // TBD: Currently there are some CAPI functions and code in the render shim that does
    // not work with unloading the LibOVRRT. We also have the problem that LibOVR returns
    // a string pointer in the GetLastError function.
    //OVR_CloseLibrary(hLibOVR);
    //hLibOVR = NULL;
}



OVR_PUBLIC_FUNCTION(ovrBool) ovr_InitializeRenderingShim()
{
#if 1
    return ovrTrue;
#else
    return ovr_InitializeRenderingShimVersion(OVR_MINOR_VERSION);
#endif
}


OVR_PUBLIC_FUNCTION(ovrBool) ovr_InitializeRenderingShimVersion(int requestedMinorVersion)
{
    // By design we ignore the build version in the library search.
    ovrBool initializeResult;
    ovrResult result = OVR_LoadSharedLibrary(OVR_PRODUCT_VERSION, OVR_MAJOR_VERSION);

    if (result != ovrSuccess)
        return ovrFalse;

    initializeResult = ovr_InitializeRenderingShimVersionPtr(requestedMinorVersion);

    if (initializeResult == ovrFalse)
        OVR_UnloadSharedLibrary();

    return initializeResult;
}


// These defaults are also in CAPI.cpp
static const ovrInitParams DefaultParams = {
    ovrInit_RequestVersion, // Flags
    OVR_MINOR_VERSION,      // RequestedMinorVersion
    0,                      // LogCallback
    0,                      // ConnectionTimeoutSeconds
    OVR_ON64("")            // pad0
};

OVR_PUBLIC_FUNCTION(ovrResult) ovr_Initialize(const ovrInitParams* inputParams)
{
    ovrResult result;
    ovrInitParams params;

    if (!inputParams)
    {
        params = DefaultParams;
    }
    else
    {
        params = *inputParams;

        // If not requesting a particular minor version,
        if (!(params.Flags & ovrInit_RequestVersion))
        {
            // Enable requesting the default minor version.
            params.Flags |= ovrInit_RequestVersion;
            params.RequestedMinorVersion = OVR_MINOR_VERSION;
        }
    }

#if defined(OVR_BUILD_DEBUG)
    // If no debug setting is provided,
    if (!(params.Flags & (ovrInit_Debug | ovrInit_ForceNoDebug)))
    {
        // Set the debug flag in debug mode.
        params.Flags |= ovrInit_Debug;
    }
#endif

    // By design we ignore the build version in the library search.
    result = OVR_LoadSharedLibrary(OVR_PRODUCT_VERSION, OVR_MAJOR_VERSION);
    if (result != ovrSuccess)
       return result;

    result = ovr_InitializePtr(&params);
    if (result != ovrSuccess)
        OVR_UnloadSharedLibrary();

    return result;
}

OVR_PUBLIC_FUNCTION(void) ovr_Shutdown()
{
    if (!ovr_ShutdownPtr)
        return;
    ovr_ShutdownPtr();
    OVR_UnloadSharedLibrary();
}

OVR_PUBLIC_FUNCTION(const char*) ovr_GetVersionString()
{
    if (!ovr_GetVersionStringPtr)
        return "(Unable to load LibOVR)";
    return ovr_GetVersionStringPtr();
}

OVR_PUBLIC_FUNCTION(void) ovr_GetLastErrorInfo(ovrErrorInfo* errorInfo)
{
    if (!ovr_GetLastErrorInfoPtr)
    {
        memset(errorInfo, 0, sizeof(ovrErrorInfo));
        errorInfo->Result = ovrError_LibLoad;
    }
    else
        ovr_GetLastErrorInfoPtr(errorInfo);
}

OVR_PUBLIC_FUNCTION(ovrResult) ovrHmd_Detect()
{
    if (!ovrHmd_DetectPtr)
        return ovrError_NotInitialized;
    return ovrHmd_DetectPtr();
}

OVR_PUBLIC_FUNCTION(ovrResult) ovrHmd_Create(int index, ovrHmd* pHmd)
{
    if (!ovrHmd_CreatePtr)
        return ovrError_NotInitialized;
    return ovrHmd_CreatePtr(index, pHmd);
}

OVR_PUBLIC_FUNCTION(ovrResult) ovrHmd_CreateDebug(ovrHmdType type, ovrHmd* pHmd)
{
    if (!ovrHmd_CreateDebugPtr)
        return ovrError_NotInitialized;
    return ovrHmd_CreateDebugPtr(type, pHmd);
}

OVR_PUBLIC_FUNCTION(void) ovrHmd_Destroy(ovrHmd hmd)
{
    if (!ovrHmd_DestroyPtr)
        return;
    ovrHmd_DestroyPtr(hmd);
}

OVR_PUBLIC_FUNCTION(unsigned int) ovrHmd_GetEnabledCaps(ovrHmd hmd)
{
    if (!ovrHmd_GetEnabledCapsPtr)
        return 0;
    return ovrHmd_GetEnabledCapsPtr(hmd);
}

OVR_PUBLIC_FUNCTION(void) ovrHmd_SetEnabledCaps(ovrHmd hmd, unsigned int hmdCaps)
{
    if (!ovrHmd_SetEnabledCapsPtr)
        return;
    ovrHmd_SetEnabledCapsPtr(hmd, hmdCaps);
}

OVR_PUBLIC_FUNCTION(ovrResult) ovrHmd_ConfigureTracking(ovrHmd hmd, unsigned int supportedTrackingCaps,
                                                         unsigned int requiredTrackingCaps)
{
    if (!ovrHmd_ConfigureTrackingPtr)
        return ovrError_NotInitialized;
    return ovrHmd_ConfigureTrackingPtr(hmd, supportedTrackingCaps, requiredTrackingCaps);
}

OVR_PUBLIC_FUNCTION(void) ovrHmd_RecenterPose(ovrHmd hmd)
{
    if (!ovrHmd_RecenterPosePtr)
        return;
    ovrHmd_RecenterPosePtr(hmd);
}

OVR_PUBLIC_FUNCTION(ovrTrackingState) ovrHmd_GetTrackingState(ovrHmd hmd, double absTime)
{
    if (!ovrHmd_GetTrackingStatePtr)
    {
        ovrTrackingState nullTrackingState;
        memset(&nullTrackingState, 0, sizeof(nullTrackingState));
        return nullTrackingState;
    }

    return ovrHmd_GetTrackingStatePtr(hmd, absTime);
}



OVR_PUBLIC_FUNCTION(ovrSizei) ovrHmd_GetFovTextureSize(ovrHmd hmd, ovrEyeType eye, ovrFovPort fov,
                                             float pixelsPerDisplayPixel)
{
    if (!ovrHmd_GetFovTextureSizePtr)
    {
        ovrSizei nullSize;
        memset(&nullSize, 0, sizeof(nullSize));
        return nullSize;
    }

    return ovrHmd_GetFovTextureSizePtr(hmd, eye, fov, pixelsPerDisplayPixel);
}

#if defined (_WIN32)
OVR_PUBLIC_FUNCTION(ovrResult) ovrHmd_CreateSwapTextureSetD3D11(ovrHmd hmd,
                                                                ID3D11Device* device,
                                                                const D3D11_TEXTURE2D_DESC* desc,
                                                                ovrSwapTextureSet** outTextureSet)
{
    if (!ovrHmd_CreateSwapTextureSetD3D11Ptr)
        return ovrError_NotInitialized;

    return ovrHmd_CreateSwapTextureSetD3D11Ptr(hmd, device, desc, outTextureSet);
}

OVR_PUBLIC_FUNCTION(ovrResult) ovrHmd_CreateMirrorTextureD3D11(ovrHmd hmd,
                                                               ID3D11Device* device,
                                                               const D3D11_TEXTURE2D_DESC* desc,
                                                               ovrTexture** outMirrorTexture)
{
    if (!ovrHmd_CreateMirrorTextureD3D11Ptr)
        return ovrError_NotInitialized;

    return ovrHmd_CreateMirrorTextureD3D11Ptr(hmd, device, desc, outMirrorTexture);
}
#endif

OVR_PUBLIC_FUNCTION(ovrResult) ovrHmd_CreateSwapTextureSetGL(ovrHmd hmd, GLuint format,
                                                             int width, int height,
                                                             ovrSwapTextureSet** outTextureSet)
{
    if (!ovrHmd_CreateSwapTextureSetGLPtr)
        return ovrError_NotInitialized;

    return ovrHmd_CreateSwapTextureSetGLPtr(hmd, format, width, height, outTextureSet);
}

OVR_PUBLIC_FUNCTION(ovrResult) ovrHmd_CreateMirrorTextureGL(ovrHmd hmd, GLuint format,
                                                            int width, int height,
                                                            ovrTexture** outMirrorTexture)
{
    if (!ovrHmd_CreateMirrorTextureGLPtr)
        return ovrError_NotInitialized;

    return ovrHmd_CreateMirrorTextureGLPtr(hmd, format, width, height, outMirrorTexture);
}

OVR_PUBLIC_FUNCTION(void) ovrHmd_DestroySwapTextureSet(ovrHmd hmd, ovrSwapTextureSet* textureSet)
{
    if (!ovrHmd_DestroySwapTextureSetPtr)
        return;

    ovrHmd_DestroySwapTextureSetPtr(hmd, textureSet);
}

OVR_PUBLIC_FUNCTION(void) ovrHmd_DestroyMirrorTexture(ovrHmd hmd, ovrTexture* mirrorTexture)
{
    if (!ovrHmd_DestroyMirrorTexturePtr)
        return;

    ovrHmd_DestroyMirrorTexturePtr(hmd, mirrorTexture);
}

OVR_PUBLIC_FUNCTION(ovrResult) ovrHmd_SetQueueAheadFraction(ovrHmd hmd, float queueAheadFraction)
{
    if (!ovrHmd_SetQueueAheadFractionPtr)
        return ovrError_NotInitialized;

    return ovrHmd_SetQueueAheadFractionPtr(hmd, queueAheadFraction);
}

OVR_PUBLIC_FUNCTION(ovrResult) ovrHmd_SubmitFrame(ovrHmd hmd, unsigned int frameIndex, const ovrViewScaleDesc* viewScaleDesc, ovrLayerHeader const * const * layerPtrList, unsigned int layerCount)
{
    if (!ovrHmd_SubmitFramePtr)
        return ovrError_NotInitialized;

    return ovrHmd_SubmitFramePtr(hmd, frameIndex, viewScaleDesc, layerPtrList, layerCount);
}

OVR_PUBLIC_FUNCTION(ovrEyeRenderDesc) ovrHmd_GetRenderDesc(ovrHmd hmd, ovrEyeType eyeType, ovrFovPort fov)
{
    if (!ovrHmd_GetRenderDescPtr)
    {
        ovrEyeRenderDesc nullEyeRenderDesc;
        memset(&nullEyeRenderDesc, 0, sizeof(nullEyeRenderDesc));
        return nullEyeRenderDesc;
    }
    return ovrHmd_GetRenderDescPtr(hmd, eyeType, fov);
}

OVR_PUBLIC_FUNCTION(ovrFrameTiming) ovrHmd_GetFrameTiming(ovrHmd hmd, unsigned int frameIndex)
{
    if (!ovrHmd_GetFrameTimingPtr)
    {
        ovrFrameTiming nullFrameTiming;
        memset(&nullFrameTiming, 0, sizeof(nullFrameTiming));
        return nullFrameTiming;
    }
    return ovrHmd_GetFrameTimingPtr(hmd, frameIndex);
}

OVR_PUBLIC_FUNCTION(void) ovrHmd_ResetFrameTiming(ovrHmd hmd, unsigned int frameIndex)
{
    if (!ovrHmd_ResetFrameTimingPtr)
        return;
    ovrHmd_ResetFrameTimingPtr(hmd, frameIndex);
}

OVR_PUBLIC_FUNCTION(double) ovr_GetTimeInSeconds()
{
    if (!ovr_GetTimeInSecondsPtr)
        return 0.;
    return ovr_GetTimeInSecondsPtr();
}

OVR_PUBLIC_FUNCTION(ovrBool) ovrHmd_GetBool(ovrHmd hmd, const char* propertyName, ovrBool defaultVal)
{
    if (!ovrHmd_GetBoolPtr)
        return ovrFalse;
    return ovrHmd_GetBoolPtr(hmd, propertyName, defaultVal);
}

OVR_PUBLIC_FUNCTION(ovrBool) ovrHmd_SetBool(ovrHmd hmd, const char* propertyName, ovrBool value)
{
    if (!ovrHmd_SetBoolPtr)
        return ovrFalse;
    return ovrHmd_SetBoolPtr(hmd, propertyName, value);
}

OVR_PUBLIC_FUNCTION(int) ovrHmd_GetInt(ovrHmd hmd, const char* propertyName, int defaultVal)
{
    if (!ovrHmd_GetIntPtr)
        return 0;
    return ovrHmd_GetIntPtr(hmd, propertyName, defaultVal);
}

OVR_PUBLIC_FUNCTION(ovrBool) ovrHmd_SetInt(ovrHmd hmd, const char* propertyName, int value)
{
    if (!ovrHmd_SetIntPtr)
        return ovrFalse;
    return ovrHmd_SetIntPtr(hmd, propertyName, value);
}

OVR_PUBLIC_FUNCTION(float) ovrHmd_GetFloat(ovrHmd hmd, const char* propertyName, float defaultVal)
{
    if (!ovrHmd_GetFloatPtr)
        return 0.f;
    return ovrHmd_GetFloatPtr(hmd, propertyName, defaultVal);
}

OVR_PUBLIC_FUNCTION(ovrBool) ovrHmd_SetFloat(ovrHmd hmd, const char* propertyName, float value)
{
    if (!ovrHmd_SetFloatPtr)
        return ovrFalse;
    return ovrHmd_SetFloatPtr(hmd, propertyName, value);
}

OVR_PUBLIC_FUNCTION(unsigned int) ovrHmd_GetFloatArray(ovrHmd hmd, const char* propertyName,
                                            float values[], unsigned int arraySize)
{
    if (!ovrHmd_GetFloatArrayPtr)
        return 0;
    return ovrHmd_GetFloatArrayPtr(hmd, propertyName, values, arraySize);
}

OVR_PUBLIC_FUNCTION(ovrBool) ovrHmd_SetFloatArray(ovrHmd hmd, const char* propertyName,
                                             const float values[], unsigned int arraySize)
{
    if (!ovrHmd_SetFloatArrayPtr)
        return ovrFalse;
    return ovrHmd_SetFloatArrayPtr(hmd, propertyName, values, arraySize);
}

OVR_PUBLIC_FUNCTION(const char*) ovrHmd_GetString(ovrHmd hmd, const char* propertyName,
                                        const char* defaultVal)
{
    if (!ovrHmd_GetStringPtr)
        return "(Unable to load LibOVR)";
    return ovrHmd_GetStringPtr(hmd, propertyName, defaultVal);
}

OVR_PUBLIC_FUNCTION(ovrBool) ovrHmd_SetString(ovrHmd hmd, const char* propertyName,
                                    const char* value)
{
    if (!ovrHmd_SetStringPtr)
        return ovrFalse;
    return ovrHmd_SetStringPtr(hmd, propertyName, value);
}

OVR_PUBLIC_FUNCTION(int) ovr_TraceMessage(int level, const char* message)
{
    if (!ovr_TraceMessagePtr)
        return -1;

    return ovr_TraceMessagePtr(level, message);
}





#if defined(_MSC_VER)
    #pragma warning(pop)
#endif

