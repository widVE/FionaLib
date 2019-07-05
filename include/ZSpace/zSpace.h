//////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2007-2013 zSpace, Inc.  All Rights Reserved.
//
//  File:       zSpace.h
//  Content:    The zSpace SDK public C API.
//  SVN Info:   $Id$
//
//////////////////////////////////////////////////////////////////////////

#ifndef __ZSPACE_H__
#define __ZSPACE_H__


//////////////////////////////////////////////////////////////////////////
// Defines
//////////////////////////////////////////////////////////////////////////

#define ZSPACE_API_VERSION 1


//////////////////////////////////////////////////////////////////////////
// Basic Types
//////////////////////////////////////////////////////////////////////////

#if (defined(_MSC_VER) && (_MSC_VER >= 1300))

typedef signed __int8       ZSInt8;
typedef signed __int16      ZSInt16;
typedef signed __int32      ZSInt32;
typedef signed __int64      ZSInt64;

typedef unsigned __int8     ZSUInt8;
typedef unsigned __int16    ZSUInt16;
typedef unsigned __int32    ZSUInt32;
typedef unsigned __int64    ZSUInt64;

#else

// From ISO/IEC 988:1999 spec
// 7.18.1.1 Exact-width integer types
typedef signed char         ZSInt8;
typedef short               ZSInt16;
typedef int                 ZSInt32;
typedef long long           ZSInt64;

typedef unsigned char       ZSUInt8;
typedef unsigned short      ZSUInt16;
typedef unsigned int        ZSUInt32;
typedef unsigned long long  ZSUInt64;

#endif

typedef ZSInt8              ZSBool;
typedef float               ZSFloat;
typedef double              ZSDouble;
typedef void*               ZSContext;
typedef void*               ZSHandle;


//////////////////////////////////////////////////////////////////////////
// Enums
//////////////////////////////////////////////////////////////////////////

/// @ingroup General
/// Defines the error codes returned by all Core SDK functions.
typedef enum ZSError
{
  ZS_ERROR_OKAY                 = 0,
  ZS_ERROR_NOT_IMPLEMENTED      = 1,
  ZS_ERROR_NOT_INITIALIZED      = 2,
  ZS_ERROR_ALREADY_INITIALIZED  = 3,
  ZS_ERROR_INVALID_PARAMETER    = 4,
  ZS_ERROR_INVALID_CONTEXT      = 5,
  ZS_ERROR_INVALID_HANDLE       = 6,
  ZS_ERROR_RUNTIME_INCOMPATIBLE = 7,
  ZS_ERROR_RUNTIME_NOT_FOUND    = 8,
  ZS_ERROR_SYMBOL_NOT_FOUND     = 9,
  ZS_ERROR_DISPLAY_NOT_FOUND    = 10,
  ZS_ERROR_DEVICE_NOT_FOUND     = 11,
  ZS_ERROR_TARGET_NOT_FOUND     = 12,
  ZS_ERROR_CAPABILITY_NOT_FOUND = 13,
  ZS_ERROR_BUFFER_TOO_SMALL     = 14
} ZSError;


/// @ingroup Display
/// Defines the types of displays for the Display APIs.
typedef enum ZSDisplayType
{
  ZS_DISPLAY_TYPE_UNKNOWN = -1,
  ZS_DISPLAY_TYPE_GENERIC =  0,
  ZS_DISPLAY_TYPE_ZSPACE  =  1
} ZSDisplayType;


/// @ingroup Display
/// Defines the attributes that you can query for the display.
/// See zsGetDisplayAttribute().
typedef enum ZSDisplayAttribute
{
  ZS_DISPLAY_ATTRIBUTE_ADAPTER_NAME       = 0,  ///< The graphics adapter name.
  ZS_DISPLAY_ATTRIBUTE_ADAPTER_STRING     = 1,  ///< The graphics adapter context string.
  ZS_DISPLAY_ATTRIBUTE_ADAPTER_ID         = 2,  ///< The entire ID string of the graphics adapter.
  ZS_DISPLAY_ATTRIBUTE_ADAPTER_VENDOR_ID  = 3,  ///< The vendor ID of the graphics adapter.
  ZS_DISPLAY_ATTRIBUTE_ADAPTER_DEVICE_ID  = 4,  ///< The device ID of the graphics adapter.
  ZS_DISPLAY_ATTRIBUTE_ADAPTER_KEY        = 5,  ///< Reserved.
  ZS_DISPLAY_ATTRIBUTE_MONITOR_NAME       = 6,  ///< The monitor name.
  ZS_DISPLAY_ATTRIBUTE_MONITOR_STRING     = 7,  ///< The monitor context string.
  ZS_DISPLAY_ATTRIBUTE_MONITOR_ID         = 8,  ///< The entire ID string of the monitor.
  ZS_DISPLAY_ATTRIBUTE_MONITOR_VENDOR_ID  = 9,  ///< The vendor ID of the monitor.
  ZS_DISPLAY_ATTRIBUTE_MONITOR_DEVICE_ID  = 10, ///< The device ID of the monitor.
  ZS_DISPLAY_ATTRIBUTE_MONITOR_KEY        = 11, ///< Reserved.
  ZS_DISPLAY_ATTRIBUTE_MANUFACTURER_NAME  = 12, ///< The display's manufacturer name.
  ZS_DISPLAY_ATTRIBUTE_PRODUCT_CODE       = 13, ///< The display's product code.
  ZS_DISPLAY_ATTRIBUTE_SERIAL_NUMBER      = 14, ///< The display's serial number.
  ZS_DISPLAY_ATTRIBUTE_VIDEO_INTERFACE    = 15  ///< The display's video interface.
} ZSDisplayAttribute;


/// @ingroup StereoBuffer
/// Defines the renderer used by the Stereo Buffer API.
typedef enum ZSRenderer
{
  ZS_RENDERER_QUAD_BUFFER_GL      = 0,
  ZS_RENDERER_QUAD_BUFFER_D3D9    = 1,
  ZS_RENDERER_QUAD_BUFFER_D3D10   = 2,
  ZS_RENDERER_QUAD_BUFFER_D3D11   = 3,
  ZS_RENDERER_QUAD_BUFFER_D3D11_1 = 4
} ZSRenderer;


/// @ingroup StereoFrustum 
/// Defines the eyes for the Stereo Frustum API.
/// This enum is also used by the Stereo Buffer API.
typedef enum ZSEye
{
  ZS_EYE_LEFT   =  0,
  ZS_EYE_RIGHT  =  1,
  ZS_EYE_CENTER =  2
} ZSEye;


/// @ingroup CoordinateSpace
/// Defines the coordinate spaces used by the zSpace Core SDK.
/// This enum is used by both the Coordinate Space API and 
/// the Stereo Frustum API.
typedef enum ZSCoordinateSpace
{
  ZS_COORDINATE_SPACE_TRACKER   =  0,
  ZS_COORDINATE_SPACE_DISPLAY   =  1,
  ZS_COORDINATE_SPACE_VIEWPORT  =  2,
  ZS_COORDINATE_SPACE_CAMERA    =  3
} ZSCoordinateSpace;


/// @ingroup StereoFrustum
/// Defines the attributes that you can set and query for the StereoFrustum.
/// These attributes are important for comfortable viewing of stereoscopic 3D.
typedef enum ZSFrustumAttribute
{
  /// The physical separation, or inter-pupillary distance, between the eyes in meters.
  /// An IPD of 0 will effectively disable stereo since the eyes are assumed
  /// to be at the same location. (Default: 0.06)
  ZS_FRUSTUM_ATTRIBUTE_IPD            = 0,

  /// Viewer scale adjusts the display and head tracking for larger and smaller scenes. (Default: 1)
  /// Use larger values for scenes with large models and smaller values for smaller models.
  ZS_FRUSTUM_ATTRIBUTE_VIEWER_SCALE   = 1,

  /// Field of view scale for the frustum. (Default: 1)
  /// A value greater than 1 causes a wide angle effect, while a value less than 1 causes a zoom effect.
  /// However, large changes to the field of view scale will interfere 
  /// with the mapping between the physical and virtual stylus. 
  ZS_FRUSTUM_ATTRIBUTE_FOV_SCALE      = 2,

  /// Uniform scale factor applied to the frustum's incoming head pose. (Default: 1)
  ZS_FRUSTUM_ATTRIBUTE_HEAD_SCALE     = 3,

  /// Near clipping plane for the frustum in meters. (Default: 0.1)
  ZS_FRUSTUM_ATTRIBUTE_NEAR_CLIP      = 4,

  /// Far clipping plane for the frustum in meters. (Default: 1000)
  ZS_FRUSTUM_ATTRIBUTE_FAR_CLIP       = 5,

  /// Distance between the bridge of the glasses and the bridge of the nose in meters. (Default: 0.01)
  ZS_FRUSTUM_ATTRIBUTE_GLASSES_OFFSET = 6,

  /// Maximum pixel disparity for crossed images (negative parallax) in the coupled zone. (Default: -100)
  /// The coupled zone refers to the area where our eyes can both comfortably converge and focus on an object. 
  ZS_FRUSTUM_ATTRIBUTE_CC_LIMIT       = 7,

  /// Maximum pixel disparity for uncrossed images (positive parallax) in the coupled zone. (Default: 100)
  ZS_FRUSTUM_ATTRIBUTE_UC_LIMIT       = 8,

  /// Maximum pixel disparity for crossed images (negative parallax) in the uncoupled zone. (Default: -200)
  ZS_FRUSTUM_ATTRIBUTE_CU_LIMIT       = 9,

  /// Maximum pixel disparity for uncrossed images (positive parallax) in the uncoupled zone. (Default: 250)
  ZS_FRUSTUM_ATTRIBUTE_UU_LIMIT       = 10,

  /// Maximum depth in meters for negative parallax in the coupled zone. (Default: 0.13)
  ZS_FRUSTUM_ATTRIBUTE_CC_DEPTH       = 11,

  /// Maximum depth in meters for positive parallax in the coupled zone. (Default: -0.30)
  ZS_FRUSTUM_ATTRIBUTE_UC_DEPTH       = 12
} ZSFrustumAttribute;

/// @ingroup StereoFrustum
/// Defines options for positioning the scene relative to the physical display or relative to the viewport.
typedef enum ZSPortalMode
{
  ZS_PORTAL_MODE_NONE     = 0x00000000, ///< The scene is positioned relative to the viewport.
  ZS_PORTAL_MODE_ANGLE    = 0x00000001, ///< The scene's orientation is fixed relative to the physical desktop.
  ZS_PORTAL_MODE_POSITION = 0x00000002, ///< The scene's position is fixed relative to the center of the display.
  ZS_PORTAL_MODE_ALL      = 0xFFFFFFFF  ///< All portal modes except "none" are enabled.
} ZSPortalMode;


/// @ingroup TrackerTarget
/// Defines the types of tracker targets.
typedef enum ZSTargetType
{
  ZS_TARGET_TYPE_HEAD       =  0,  ///< The tracker target corresponding to the user's head.
  ZS_TARGET_TYPE_PRIMARY    =  1,  ///< The tracker target corresponding to the user's primary hand.
  ZS_TARGET_TYPE_SECONDARY  =  2   ///< The tracker target corresponding to the user's secondary hand. (Reserved for future use.)
} ZSTargetType;


/// @ingroup TrackerEvent
/// Defines event types that you can use with a tracker event handler.
typedef enum ZSTrackerEventType
{
  // Target Events
  ZS_TRACKER_EVENT_MOVE             = 0x0101,

  // Button Events
  ZS_TRACKER_EVENT_BUTTON_PRESS     = 0x0201,
  ZS_TRACKER_EVENT_BUTTON_RELEASE   = 0x0202,

  // Tap Events
  ZS_TRACKER_EVENT_TAP_PRESS        = 0x0301,
  ZS_TRACKER_EVENT_TAP_RELEASE      = 0x0302,
  ZS_TRACKER_EVENT_TAP_HOLD         = 0x0303,
  ZS_TRACKER_EVENT_TAP_SINGLE       = 0x0304,
  ZS_TRACKER_EVENT_TAP_DOUBLE       = 0x0305,

  // All Events
  ZS_TRACKER_EVENT_ALL              = 0xFFFF
} ZSTrackerEventType;


/// @ingroup MouseEmulation
/// Defines mouse buttons to be used when mapping a tracker target's buttons to a mouse.
typedef enum ZSMouseButton
{
  ZS_MOUSE_BUTTON_UNKNOWN = -1,
  ZS_MOUSE_BUTTON_LEFT    =  0,
  ZS_MOUSE_BUTTON_RIGHT   =  1,
  ZS_MOUSE_BUTTON_CENTER  =  2
} ZSMouseButton;


/// @ingroup MouseEmulation
/// Determines how the stylus and mouse control the cursor when both are used.
typedef enum ZSMouseMovementMode
{
  /// The stylus uses absolute positions.  
  /// In this mode, the mouse and stylus can fight for control of the cursor if both are in use.
  /// This is the default mode. 
  ZS_MOUSE_MOVEMENT_MODE_ABSOLUTE = 0,

  /// The stylus applies delta positions to the mouse cursor's current position.
  /// Movements by the mouse and stylus are compounded without fighting. 
  ZS_MOUSE_MOVEMENT_MODE_RELATIVE = 1
} ZSMouseMovementMode;


//////////////////////////////////////////////////////////////////////////
// Compound Types
//////////////////////////////////////////////////////////////////////////

// Ensure 8 byte packing.
#pragma pack( push, 8 )

/// @ingroup General
/// @brief Union representing a vector of 3 floats.
typedef union ZSVector3
{
  ZSFloat f[3];
  struct 
  {
    ZSFloat x;
    ZSFloat y;
    ZSFloat z;
  };
} ZSVector3;


/// @ingroup General 
/// @brief Union representing 4x4 matrix (right-handed OpenGL column-major format).
////
/// This structure is used by both the Stereo Frustum and Coordinate Space APIs.
typedef union ZSMatrix4
{
  ZSFloat f[16];
  struct
  {
    ZSFloat m00, m10, m20, m30;
    ZSFloat m01, m11, m21, m31;
    ZSFloat m02, m12, m22, m32;
    ZSFloat m03, m13, m23, m33;
  };
} ZSMatrix4;


/// @ingroup General
/// @brief Union representing an axis-aligned bounding box (AABB).
typedef union ZSBoundingBox
{
  ZSFloat f[6];
  struct 
  {
    ZSVector3 lower;  ///< The minimum extent, or back lower left, corner of the bounding box.
    ZSVector3 upper;  ///< The maximum extent, or front upper right, corner of the bounding box. 
  };
} ZSBoundingBox;


/// @ingroup Display
/// @brief Struct representing display intersection information.
typedef struct ZSDisplayIntersectionInfo
{
  ZSBool  hit;                ///< Whether or not the display was intersected.
  ZSInt32 x;                  ///< The x pixel coordinate on the virtual desktop.
  ZSInt32 y;                  ///< The y pixel coordinate on the virtual desktop.
  ZSInt32 nx;                 ///< The normalized absolute x pixel coordinate on the virtual desktop.
  ZSInt32 ny;                 ///< The normalized absolute y pixel coordinate on the virtual desktop.
  ZSFloat distance;           ///< The distance from origin of the raycast to the point of intersection on the display in meters.
} ZSDisplayIntersectionInfo;


/// @ingroup StereoFrustum
/// @brief Union representing frustum bounds.
typedef union ZSFrustumBounds
{
  ZSFloat f[6];
  struct
  {
    ZSFloat left;
    ZSFloat right;
    ZSFloat bottom;
    ZSFloat top;
    ZSFloat nearClip;
    ZSFloat farClip;
  };
} ZSFrustumBounds;


/// @ingroup TrackerTarget
/// @brief Struct representing tracker pose information.
///
/// This structure is used by the Tracker Target, Display, and Stereo Frustum APIs.
typedef struct ZSTrackerPose
{
  ZSDouble  timestamp;  ///< The time that the pose was captured (represented in seconds since last system reboot).
  ZSMatrix4 matrix;     ///< The tracker-space position and orientation in 4x4 matrix format.
} ZSTrackerPose;


/// @ingroup TrackerEvent
/// @brief The tracker event data type.
typedef struct ZSTrackerEventData
{
  ZSInt32            size;            ///< The event data's size in bytes.
  ZSTrackerEventType type;            ///< The event's type.
  ZSDouble           timestamp;       ///< The time that the event was generated (represented in seconds since last system reboot).
  ZSMatrix4          poseMatrix;      ///< The tracker-space position and orientation in 4x4 matrix format.
  ZSInt32            buttonId;        ///< The integer id corresponding to the target's physical button.
} ZSTrackerEventData;

#pragma pack( pop )


//////////////////////////////////////////////////////////////////////////
// Function Pointer Types
//////////////////////////////////////////////////////////////////////////

/// @ingroup TrackerEvent
/// Handler for tracker events.
/// 
/// @param[in] targetHandle  The handle to the tracker target that the event was generated for.
/// @param[in] eventData     A pointer to the event data.
/// @param[in] userData      A pointer to the user defined data that was passed in to the zsAddTrackerEventHandler() function. 
typedef void (*ZSTrackerEventHandler)(ZSHandle targetHandle, const ZSTrackerEventData* eventData, const void* userData);


//////////////////////////////////////////////////////////////////////////
// zSpace APIs
//////////////////////////////////////////////////////////////////////////

#ifndef ZSPACE_TYPES_ONLY

#ifdef __cplusplus
extern "C" {
#endif

// General API
ZSError zsInitialize(ZSContext* context);
ZSError zsUpdate(ZSContext context);
ZSError zsShutdown(ZSContext context);

ZSError zsSetTrackingEnabled(ZSContext context, ZSBool isEnabled);
ZSError zsIsTrackingEnabled(ZSContext context, ZSBool* isEnabled);
ZSError zsGetRuntimeVersion(ZSContext context, ZSInt32* major, ZSInt32* minor, ZSInt32* patch);
ZSError zsGetErrorString(ZSError error, char* buffer, ZSInt32 bufferSize);

// Display API
ZSError zsRefreshDisplays(ZSContext context);
ZSError zsGetNumDisplays(ZSContext context, ZSInt32* numDisplays);
ZSError zsGetNumDisplaysByType(ZSContext context, ZSDisplayType displayType, ZSInt32* numDisplays);
ZSError zsFindDisplay(ZSContext context, ZSInt32 x, ZSInt32 y, ZSHandle* displayHandle);
ZSError zsFindDisplayByIndex(ZSContext context, ZSInt32 index, ZSHandle* displayHandle);
ZSError zsFindDisplayByType(ZSContext context, ZSDisplayType displayType, ZSInt32 index, ZSHandle* displayHandle);
ZSError zsGetDisplayType(ZSHandle displayHandle, ZSDisplayType* displayType);
ZSError zsGetDisplayNumber(ZSHandle displayHandle, ZSInt32* number);
ZSError zsGetDisplayAdapterIndex(ZSHandle displayHandle, ZSInt32* adapterIndex);
ZSError zsGetDisplayMonitorIndex(ZSHandle displayHandle, ZSInt32* monitorIndex);
ZSError zsGetDisplayAttribute(ZSHandle displayHandle, ZSDisplayAttribute attribute, char* buffer, ZSInt32 bufferSize);
ZSError zsGetDisplaySize(ZSHandle displayHandle, ZSFloat* width, ZSFloat* height);
ZSError zsGetDisplayPosition(ZSHandle displayHandle, ZSInt32* x, ZSInt32* y);
ZSError zsGetDisplayNativeResolution(ZSHandle displayHandle, ZSInt32* x, ZSInt32* y);
ZSError zsGetDisplayAngle(ZSHandle displayHandle, ZSFloat* x, ZSFloat* y, ZSFloat* z);
ZSError zsGetDisplayVerticalRefreshRate(ZSHandle displayHandle, ZSFloat* refreshRate);
ZSError zsIsDisplayHardwarePresent(ZSHandle displayHandle, ZSBool* isHardwarePresent);
ZSError zsIntersectDisplay(ZSHandle displayHandle, const ZSTrackerPose* pose, ZSDisplayIntersectionInfo* intersectionInfo);

// StereoBuffer API
ZSError zsCreateStereoBuffer(ZSContext context, ZSRenderer renderer, void* reserved, ZSHandle* bufferHandle);
ZSError zsDestroyStereoBuffer(ZSHandle bufferHandle);
ZSError zsSetStereoBufferFullScreen(ZSHandle bufferHandle, ZSBool isFullScreen);
ZSError zsBeginStereoBufferFrame(ZSHandle bufferHandle);
ZSError zsSyncStereoBuffer(ZSHandle bufferHandle);

// StereoViewport API
ZSError zsCreateViewport(ZSContext context, ZSHandle* viewportHandle);
ZSError zsDestroyViewport(ZSHandle viewportHandle);
ZSError zsSetViewportPosition(ZSHandle viewportHandle, ZSInt32 x, ZSInt32 y);
ZSError zsGetViewportPosition(ZSHandle viewportHandle, ZSInt32* x, ZSInt32* y);
ZSError zsSetViewportSize(ZSHandle viewportHandle, ZSInt32 width, ZSInt32 height);
ZSError zsGetViewportSize(ZSHandle viewportHandle, ZSInt32* width, ZSInt32* height);

// Coordinate Space API
ZSError zsGetCoordinateSpaceTransform(ZSHandle viewportHandle, ZSCoordinateSpace a, ZSCoordinateSpace b, ZSMatrix4* transform);
ZSError zsTransformMatrix(ZSHandle viewportHandle, ZSCoordinateSpace a, ZSCoordinateSpace b, ZSMatrix4* matrix);

// StereoFrustum API
ZSError zsFindFrustum(ZSHandle viewportHandle, ZSHandle* frustumHandle);
ZSError zsSetFrustumAttribute(ZSHandle frustumHandle, ZSFrustumAttribute attribute, ZSFloat value);
ZSError zsGetFrustumAttribute(ZSHandle frustumHandle, ZSFrustumAttribute attribute, ZSFloat* value);
ZSError zsSetFrustumPortalMode(ZSHandle frustumHandle, ZSInt32 portalModeFlags);
ZSError zsGetFrustumPortalMode(ZSHandle frustumHandle, ZSInt32* portalModeFlags);
ZSError zsSetFrustumCameraOffset(ZSHandle frustumHandle, const ZSVector3* cameraOffset);
ZSError zsGetFrustumCameraOffset(ZSHandle frustumHandle, ZSVector3* cameraOffset);
ZSError zsSetFrustumHeadPose(ZSHandle frustumHandle, const ZSTrackerPose* headPose);
ZSError zsGetFrustumHeadPose(ZSHandle frustumHandle, ZSTrackerPose* headPose);
ZSError zsGetFrustumViewMatrix(ZSHandle frustumHandle, ZSEye eye, ZSMatrix4* viewMatrix);
ZSError zsGetFrustumProjectionMatrix(ZSHandle frustumHandle, ZSEye eye, ZSMatrix4* projectionMatrix);
ZSError zsGetFrustumBounds(ZSHandle frustumHandle, ZSEye eye, ZSFrustumBounds* bounds);
ZSError zsGetFrustumEyePosition(ZSHandle frustumHandle, ZSEye eye, ZSCoordinateSpace coordinateSpace, ZSVector3* eyePosition);
ZSError zsGetFrustumCoupledBoundingBox(ZSHandle frustumHandle, ZSBoundingBox* boundingBox);
ZSError zsCalculateFrustumFit(ZSHandle frustumHandle, const ZSBoundingBox* boundingBox, ZSFloat* viewerScale, ZSMatrix4* lookAtMatrix);
ZSError zsCalculateFrustumDisparity(ZSHandle frustumHandle, const ZSVector3* point, ZSFloat* disparity);
ZSError zsCalculateFrustumFovScale(ZSHandle frustumHandle, ZSFloat fov, ZSFloat* fovScale);

// TrackerDevice API
ZSError zsGetNumTrackerDevices(ZSContext context, ZSInt32* numDevices);
ZSError zsFindTrackerDevice(ZSContext context, ZSInt32 index, ZSHandle* deviceHandle);
ZSError zsFindTrackerDeviceByName(ZSContext context, const char* deviceName, ZSHandle* deviceHandle);
ZSError zsSetTrackerDeviceEnabled(ZSHandle deviceHandle, ZSBool isEnabled);
ZSError zsIsTrackerDeviceEnabled(ZSHandle deviceHandle, ZSBool* isEnabled);
ZSError zsGetTrackerDeviceName(ZSHandle deviceHandle, char* buffer, ZSInt32 bufferSize);

// TrackerTarget API
ZSError zsGetNumTargets(ZSHandle deviceHandle, ZSInt32* numTargets);
ZSError zsGetNumTargetsByType(ZSContext context, ZSTargetType targetType, ZSInt32* numTargets);
ZSError zsFindTarget(ZSHandle deviceHandle, ZSInt32 index, ZSHandle* targetHandle);
ZSError zsFindTargetByName(ZSHandle deviceHandle, const char* targetName, ZSHandle* targetHandle);
ZSError zsFindTargetByType(ZSContext context, ZSTargetType targetType, ZSInt32 index, ZSHandle* targetHandle);
ZSError zsGetTargetName(ZSHandle targetHandle, char* buffer, ZSInt32 bufferSize);
ZSError zsSetTargetEnabled(ZSHandle targetHandle, ZSBool isEnabled);
ZSError zsIsTargetEnabled(ZSHandle targetHandle, ZSBool* isEnabled);
ZSError zsIsTargetVisible(ZSHandle targetHandle, ZSBool* isVisible);
ZSError zsSetTargetMoveEventThresholds(ZSHandle targetHandle, ZSFloat time, ZSFloat distance, ZSFloat angle);
ZSError zsGetTargetMoveEventThresholds(ZSHandle targetHandle, ZSFloat* time, ZSFloat* distance, ZSFloat* angle);
ZSError zsGetTargetPose(ZSHandle targetHandle, ZSTrackerPose* pose);
ZSError zsSetTargetPoseBufferingEnabled(ZSHandle targetHandle, ZSBool isPoseBufferingEnabled);
ZSError zsIsTargetPoseBufferingEnabled(ZSHandle targetHandle, ZSBool* isPoseBufferingEnabled);
ZSError zsGetTargetPoseBuffer(ZSHandle targetHandle, ZSFloat minDelta, ZSFloat maxDelta, ZSTrackerPose* buffer, ZSInt32* bufferSize);
ZSError zsResizeTargetPoseBuffer(ZSHandle targetHandle, ZSInt32 bufferSize);

// TrackerTarget Button API
ZSError zsGetNumTargetButtons(ZSHandle targetHandle, ZSInt32* numButtons);
ZSError zsIsTargetButtonPressed(ZSHandle targetHandle, ZSInt32 buttonId, ZSBool* isButtonPressed);

// TrackerTarget Led API
ZSError zsSetTargetLedEnabled(ZSHandle targetHandle, ZSBool isLedEnabled);
ZSError zsIsTargetLedEnabled(ZSHandle targetHandle, ZSBool* isLedEnabled);
ZSError zsIsTargetLedOn(ZSHandle targetHandle, ZSBool* isLedOn);
ZSError zsSetTargetLedColor(ZSHandle targetHandle, ZSFloat r, ZSFloat g, ZSFloat b);
ZSError zsGetTargetLedColor(ZSHandle targetHandle, ZSFloat* r, ZSFloat* g, ZSFloat* b);

// TrackerTarget Vibration API
ZSError zsSetTargetVibrationEnabled(ZSHandle targetHandle, ZSBool isVibrationEnabled);
ZSError zsIsTargetVibrationEnabled(ZSHandle targetHandle, ZSBool* isVibrationEnabled);
ZSError zsIsTargetVibrating(ZSHandle targetHandle, ZSBool* isVibrating);
ZSError zsStartTargetVibration(ZSHandle targetHandle, ZSFloat onPeriod, ZSFloat offPeriod, ZSInt32 numTimes);
ZSError zsStopTargetVibration(ZSHandle targetHandle);

// TrackerTarget Tap API
ZSError zsIsTargetTapPressed(ZSHandle targetHandle, ZSBool* isTapPressed);
ZSError zsSetTargetTapHoldThreshold(ZSHandle targetHandle, ZSFloat seconds);
ZSError zsGetTargetTapHoldThreshold(ZSHandle targetHandle, ZSFloat* seconds);

// Tracker Event API
ZSError zsAddTrackerEventHandler(ZSHandle targetHandle, ZSTrackerEventType trackerEventType, ZSTrackerEventHandler trackerEventHandler, const void* userData);
ZSError zsRemoveTrackerEventHandler(ZSHandle targetHandle, ZSTrackerEventType trackerEventType, ZSTrackerEventHandler trackerEventHandler);

// Mouse Emulation API
ZSError zsSetMouseEmulationEnabled(ZSContext context, ZSBool isEnabled);
ZSError zsIsMouseEmulationEnabled(ZSContext context, ZSBool* isEnabled);
ZSError zsSetMouseEmulationTarget(ZSContext context, ZSHandle targetHandle);
ZSError zsGetMouseEmulationTarget(ZSContext context, ZSHandle* targetHandle);
ZSError zsSetMouseEmulationMovementMode(ZSContext context, ZSMouseMovementMode movementMode);
ZSError zsGetMouseEmulationMovementMode(ZSContext context, ZSMouseMovementMode* movementMode);
ZSError zsSetMouseEmulationMaxDistance(ZSContext context, ZSFloat maxDistance);
ZSError zsGetMouseEmulationMaxDistance(ZSContext context, ZSFloat* maxDistance);
ZSError zsSetMouseEmulationButtonMapping(ZSContext context, ZSInt32 buttonId, ZSMouseButton mouseButton);
ZSError zsGetMouseEmulationButtonMapping(ZSContext context, ZSInt32 buttonId, ZSMouseButton* mouseButton);

#ifdef __cplusplus
}
#endif

#endif // ZSPACE_TYPES_ONLY

#endif // __ZSPACE_H__
