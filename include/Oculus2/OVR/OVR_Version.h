/********************************************************************************//**
\file      OVR_Version.h
\brief     This header provides LibOVR version identification.
\copyright Copyright 2014 Oculus VR, LLC All Rights reserved.
*************************************************************************************/

#ifndef OVR_Version_h
#define OVR_Version_h






/// Conventional string-ification macro.
#if !defined(OVR_STRINGIZE)
    #define OVR_STRINGIZEIMPL(x) #x
    #define OVR_STRINGIZE(x)     OVR_STRINGIZEIMPL(x)
#endif


// We are on major version 6 of the beta pre-release SDK. At some point we will
// transition to product version 1 and reset the major version back to 1 (first
// product release, version 1.0).
#define OVR_PRODUCT_VERSION 0
#define OVR_MAJOR_VERSION   6
#define OVR_MINOR_VERSION   0
#define OVR_PATCH_VERSION   1
#define OVR_BUILD_NUMBER    0


/// "Product.Major.Minor.Patch"
#if !defined(OVR_VERSION_STRING)
    #define OVR_VERSION_STRING  OVR_STRINGIZE(OVR_PRODUCT_VERSION.OVR_MAJOR_VERSION.OVR_MINOR_VERSION.OVR_PATCH_VERSION)
#endif


/// "Product.Major.Minor.Patch.Build"
#if !defined(OVR_DETAILED_VERSION_STRING)
    #define OVR_DETAILED_VERSION_STRING OVR_STRINGIZE(OVR_PRODUCT_VERSION.OVR_MAJOR_VERSION.OVR_MINOR_VERSION.OVR_PATCH_VERSION.OVR_BUILD_NUMBER)
#endif


// This is the firmware version for the DK2 headset sensor board.
//#if !defined(OVR_DK2_LATEST_FIRMWARE_MAJOR_VERSION)
    #define OVR_DK2_LATEST_FIRMWARE_MAJOR_VERSION 2
    #define OVR_DK2_LATEST_FIRMWARE_MINOR_VERSION 12
//#endif

// This is the product version for the Oculus Display Driver. A continuous
// process will propagate this value to all dependent files
#define OVR_DISPLAY_DRIVER_PRODUCT_VERSION "1.2.6.0"

// This is the product version for the Oculus Position Tracker Driver. A
// continuous process will propagate this value to all dependent files
#define OVR_POSITIONAL_TRACKER_DRIVER_PRODUCT_VERSION "1.0.10.0"

/// \brief file description for version info
/// This appears in the user-visible file properties. It is intended to convey publicly
/// available additional information such as feature builds.
#if !defined(OVR_FILE_DESCRIPTION_STRING)
    #if defined(_DEBUG)
        #define OVR_FILE_DESCRIPTION_STRING "dev build debug"
    #else
        #define OVR_FILE_DESCRIPTION_STRING "dev build"
    #endif
#endif


#endif // OVR_Version_h
