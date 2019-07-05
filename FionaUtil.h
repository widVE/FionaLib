#ifndef _FIONA_UTIL_H_
#define _FIONA_UTIL_H_

#include <Windows.h>
#include <tchar.h>
#include <stdio.h>
#include <strsafe.h>
#include <string>
#include <vector>

//several static functions helpful for various utility operations for Fiona!
//Ross Tredinnick
//6/5/2012

class FionaUtil
{
public:
	//sDir must end with "\\" and sType must be "\\*.<extension"
	static void ListFilesInDirectory(const std::string &sDir, std::vector<std::string> & files, std::string sType=std::string("\\*"), bool includeDirs=false)
	{
		WIN32_FIND_DATA ffd;
		TCHAR szDir[MAX_PATH];
		size_t argLen=0;
		HANDLE hFind= INVALID_HANDLE_VALUE;
		DWORD dwError=0;

		//StringCchLength(sDir, MAX_PATH, &argLen);
		if(argLen > (MAX_PATH-3))
		{
			printf("Path too long\n");
			return;
		}

		std::string sTheDir = sDir;
		sTheDir.append("\\");
		printf("Gathering files of type %s in directory %s\n", sType.c_str(), sTheDir.c_str());
		StringCchCopy(szDir, MAX_PATH, sTheDir.c_str());
		StringCchCat(szDir, MAX_PATH, TEXT(sType.c_str()));

		hFind = FindFirstFile(szDir, &ffd);
		
		if( hFind == INVALID_HANDLE_VALUE )
		{
			printf("Found no files\n");
			return;
		}

		do
		{
			//todo - option for recursive?
			if(((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) || includeDirs)
			{
				files.push_back(sTheDir + std::string(ffd.cFileName));
			}
		}
		while (FindNextFile(hFind, &ffd) != 0);

		dwError = GetLastError();
		if(dwError != ERROR_NO_MORE_FILES)
		{
			printf("Didn't navigate all files?\n");
		}

		FindClose(hFind);
		return;
	}


};

#endif
