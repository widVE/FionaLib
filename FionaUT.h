//
//  FionaUT.h
//  FionaUT
//
//  Created by Hyun Joon Shin on 5/7/12.
//  Updated and work continued by Ross Tredinnick 2012-2019
//  Living Environments Laboratory, Virtual Environments Groups - Wisconsin Institutes for Discovery
//

#ifndef FIONA_UT_H__
#define FIONA_UT_H__
#define GLEW_STATIC

#ifndef NULL
#define NULL 0x0
#endif

#ifdef ENABLE_VIVE
#include "openvr.h"
#pragma comment(lib, "openvr_api.lib")
#endif

#ifdef ENABLE_OCULUS
#ifdef ENABLE_DK2
#include "Oculus2/OVR/OVR_CAPI_GL.h"
#endif
#ifdef ENABLE_CV1
#include "Oculus3/OVR/OVR_CAPI_GL.h"
#endif
#endif

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <Windows.h>
#include <stdlib.h>
#include <GL/glew.h>
#include <GL/GL.h>
#include <GLUT/glut.h>
#include <Xinput.h>
typedef HWND  WIN;
typedef HGLRC CTX;
#endif

#ifdef ENABLE_OCULUS
#ifdef _DEBUG
#ifdef _WIN64
#ifdef ENABLE_DK2
#pragma comment(lib, "libovr64dk2d.lib")
#else
#pragma comment(lib, "libovr64d.lib")
#endif
#else
#ifdef ENABLE_DK2
#pragma comment(lib, "libovrdk2d.lib")
#else
#pragma comment(lib, "libovrd.lib")
#endif
#endif
#else
#ifdef _WIN64
#ifdef ENABLE_DK2
#pragma comment(lib, "libovr64dk2.lib")
#endif
#ifdef ENABLE_DK1
#pragma comment(lib, "libovr64.lib")
#endif
#ifdef ENABLE_CV1
#pragma comment(lib, "libovr64cv1.lib")
#endif
#else
#ifdef ENABLE_DK2
#pragma comment(lib, "libovrdk2.lib")
#else
#pragma comment(lib, "libovr.lib")
#endif
#endif
#endif
#endif

#ifdef APPLE
#include <OpenGL/GL.h>
#include <GLUT/glut.h>
typedef void* CTX;
typedef void* WIN;
#endif

#ifdef LINUX_BUILD
#include <stdlib.h>
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glut.h>
#include <string.h>
#include <stdio.h>
//not sure which x11 to add
#include <GL/glx.h>
#include <GL/glext.h>
#include <GL/glxext.h>
#include <X11/extensions/xf86vmode.h>

typedef void* CTX;
typedef void* WIN;
struct FionaUTWin
{
	Display *dsp;
	XVisualInfo *vi;
	GLXContext cxt;
	Colormap	cmap;
	int screen;
	Window xwin;
	GLXWindow             glxWin;
	GLXFBConfig          *fbConfigs;
	XSetWindowAttributes attr;
	const char *name;
	int x,y;
	unsigned int w,h;
	FionaUTWin(const char *name, int x, int y, unsigned int w, unsigned int h)
	{
		this->name=name;
		this->x=x; this->y=y; this->w=w;this->h=h;
		screen=0;
	}
};
#endif

const CTX	FIONA_INVALID_CTX =NULL;
const WIN	FIONA_INVALID_WIN=NULL;

#include <Kit3D/jmath.h>
#include <vector>
#include <queue>
#include "leap/Leap.h"
#include "FionaNetwork.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/io.hpp>

#ifdef ENABLE_OCULUS
#ifdef ENABLE_DK2
#include "Oculus2/OVR/OVR.h"
#include "Oculus2/OVR/OVR_CAPI_GL.h"
#endif
#ifdef ENABLE_DK1
#include "Oculus/OVR/OVR.h"
#endif
#ifdef ENABLE_CV1
#include "Oculus3/OVR/OVR_CAPI.h"
#include "Oculus3/OVR/OVR_CAPI_GL.h"


struct DepthBuffer
{
	GLuint        texId;

	DepthBuffer(ovrSizei size, int sampleCount)
	{
		UNREFERENCED_PARAMETER(sampleCount);

		assert(sampleCount <= 1); // The code doesn't currently handle MSAA textures.

		glGenTextures(1, &texId);
		glBindTexture(GL_TEXTURE_2D, texId);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		GLenum internalFormat = GL_DEPTH_COMPONENT32F;
		GLenum type = GL_FLOAT;

		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, size.w, size.h, 0, GL_DEPTH_COMPONENT, type, NULL);
	}

	~DepthBuffer()
	{
		if (texId)
		{
			glDeleteTextures(1, &texId);
			texId = 0;
		}
	}
};

//--------------------------------------------------------------------------
struct TextureBuffer
{
	ovrSession          Session;
	ovrTextureSwapChain  TextureChain;
	GLuint              texId;
	GLuint              fboId;
	ovrSizei               texSize;

	TextureBuffer(ovrSession session, bool rendertarget, bool displayableOnHmd, ovrSizei size, int mipLevels, unsigned char * data, int sampleCount) :
		Session(session),
		TextureChain(nullptr),
		texId(0),
		fboId(0)
	{
		UNREFERENCED_PARAMETER(sampleCount);
		texSize.w = 0;
		texSize.h = 0;
		assert(sampleCount <= 1); // The code doesn't currently handle MSAA textures.

		texSize = size;

		if (displayableOnHmd)
		{
			// This texture isn't necessarily going to be a rendertarget, but it usually is.
			assert(session); // No HMD? A little odd.
			assert(sampleCount == 1); // ovr_CreateSwapTextureSetD3D11 doesn't support MSAA.

			ovrTextureSwapChainDesc desc = {};
			desc.Type = ovrTexture_2D;
			desc.ArraySize = 1;
			desc.Width = size.w;
			desc.Height = size.h;
			desc.MipLevels = 1;
			desc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
			desc.SampleCount = 1;
			desc.StaticImage = ovrFalse;

			ovrResult result = ovr_CreateTextureSwapChainGL(Session, &desc, &TextureChain);

			int length = 0;
			ovr_GetTextureSwapChainLength(session, TextureChain, &length);

			if (OVR_SUCCESS(result))
			{
				for (int i = 0; i < length; ++i)
				{
					GLuint chainTexId;
					ovr_GetTextureSwapChainBufferGL(Session, TextureChain, i, &chainTexId);
					glBindTexture(GL_TEXTURE_2D, chainTexId);

					if (rendertarget)
					{
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
					}
					else
					{
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
					}
				}
			}
		}
		else
		{
			glGenTextures(1, &texId);
			glBindTexture(GL_TEXTURE_2D, texId);

			if (rendertarget)
			{
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			}
			else
			{
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			}

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, texSize.w, texSize.h, 0, GL_RGBA, GL_FLOAT, data);
			//glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, texSize.w, texSize.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		}

		if (mipLevels > 1)
		{
			glGenerateMipmap(GL_TEXTURE_2D);
		}

		glGenFramebuffers(1, &fboId);
	}

	~TextureBuffer()
	{
		if (TextureChain)
		{
			ovr_DestroyTextureSwapChain(Session, TextureChain);
			TextureChain = nullptr;
		}
		if (texId)
		{
			glDeleteTextures(1, &texId);
			texId = 0;
		}
		if (fboId)
		{
			glDeleteFramebuffers(1, &fboId);
			fboId = 0;
		}
	}

	ovrSizei GetSize() const
	{
		return texSize;
	}

	void SetAndClearRenderSurface(DepthBuffer* dbuffer)
	{
		GLuint curTexId;
		if (TextureChain)
		{
			int curIndex;
			ovr_GetTextureSwapChainCurrentIndex(Session, TextureChain, &curIndex);
			ovr_GetTextureSwapChainBufferGL(Session, TextureChain, curIndex, &curTexId);
		}
		else
		{
			curTexId = texId;
		}

		glBindFramebuffer(GL_FRAMEBUFFER, fboId);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, curTexId, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, dbuffer->texId, 0);

		glViewport(0, 0, texSize.w, texSize.h);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		//glEnable(GL_FRAMEBUFFER_SRGB);
	}

	void UnsetRenderSurface()
	{
		glDisable(GL_FRAMEBUFFER_SRGB);
		glBindFramebuffer(GL_FRAMEBUFFER, fboId);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
	}

	void Commit()
	{
		if (TextureChain)
		{
			ovr_CommitTextureSwapChain(Session, TextureChain);
		}
	}
};

#endif
#endif

#ifdef ENABLE_VIVE

struct FramebufferDesc
{
	GLuint m_nDepthBufferId;
	GLuint m_nRenderTextureId;
	GLuint m_nRenderFramebufferId;
	GLuint m_nResolveTextureId;
	GLuint m_nResolveFramebufferId;
};

extern bool CreateFrameBuffer(int nWidth, int nHeight, FramebufferDesc &framebufferDesc);

#endif

#define MAX_WINDOW 5

// Per Window Function Pointers & Callbacks
typedef void (*FIONA_DISPLAY_FUNC)		(void);
typedef void (*FIONA_RESHAPE_FUNC)		(int width, int height);
typedef void (*FIONA_KEYBOARD_FUNC)		(unsigned int key, int x, int y);
typedef void (*FIONA_MOUSE_FUNC)		(int button, int state, int x, int y);
typedef void (*FIONA_MOTION_FUNC)		(int x, int y);
typedef void (*FIONA_PASSIVE_FUNC)		(int x, int y);
typedef void (*FIONA_TABLET_BUTTON_FUNC)(int button, int state, int x, int y);
typedef void (*FIONA_TABLET_MOTION_FUNC)(int x, int y);
typedef void (*FIONA_POST_RENDER_FUNC)  (void);
typedef void (*FIONA_WALL_FUNC)  (void);

extern void		FionaUTDisplayFunc		(FIONA_DISPLAY_FUNC func);
extern void		FionaUTReshapeFunc		(FIONA_RESHAPE_FUNC func);
extern void		FionaUTKeyboardFunc		(FIONA_KEYBOARD_FUNC func);
extern void		FionaUTMouseFunc		(FIONA_MOUSE_FUNC func);
extern void		FionaUTMotionFunc		(FIONA_MOTION_FUNC func);
extern void		FionaUTPassiveMotionFunc(FIONA_PASSIVE_FUNC func);
extern void		FionaUTPostRenderFunc	(FIONA_POST_RENDER_FUNC func);
extern void		FionaUTWallFunc			(FIONA_WALL_FUNC func);

// Per App Function Pointers & Callbacks
typedef void (*FIONA_IDLE_FUNC)			(void);
typedef void (*FIONA_FRAME_FUNC)		(float t);
typedef void (*FIONA_TRACKER_FUNC)		(int i, const jvec3& p, const quat& o);
typedef void (*FIONA_WAND_BUTTON_FUNC)	(int i, int s, int idx);
typedef void (*FIONA_JOYSTIK_FUNC)		(int i, const vec4& t);
typedef void (*FIONA_CONTROLLER_FUNC)	(unsigned short, unsigned char, unsigned char, short, short, short, short);
typedef void (*FIONA_CLEANUP_FUNC)		(int e);

extern void		FionaUTFrameFunc		(FIONA_FRAME_FUNC func);
extern void		FionaUTIdle				(FIONA_IDLE_FUNC func);
extern void		FionaUTTrackerFunc		(FIONA_TRACKER_FUNC func);
extern void		FionaUTWandButtonFunc	(FIONA_WAND_BUTTON_FUNC func);
extern void		FionaUTJoystickFunc		(FIONA_JOYSTIK_FUNC func);
extern void		FionaUTCleanupFunc		(FIONA_CLEANUP_FUNC func);
extern void		FionaUTControllerFunc	(FIONA_CONTROLLER_FUNC func);

//Utilities / Misc Windows Functions
extern void		FionaUTInit					(int* argc, char* argv[]);	//performs main config file parsing
extern void		FionaUTInitDisplayMode		(unsigned int mode);
extern void		FionaUTInitWindowSize		(int w, int h);
extern int		FionaUTCreateWindow			(const char* name);			//parent function which when called, creates a win32 window
extern void		FionaUTFullScreen			(void);
extern void		FionaUTMainLoop				(void);						//main application loop
extern int		FionaUTGetWindow			(void);
extern void		FionaUTSetWindow			(int);
extern int		FionaUTGet					(int);
extern jvec3		FionaUTGetJoystick			(void);
extern short	FionaUTGetWandButtons		(void);


// Special Native window property access functions
//  Those values are available only in the Display callback
extern WIN		FionaUTGetNativeWindow	(void);
extern CTX		FionaUTGetContext		(void);	//this isn't used much... and may not be setup to work with multiple GPU contexts...
extern float	FionaUTTime				(void);


// Fiona specific utility functions
extern void		FionaUTExit			(int errorCode=0);	//check that items are being deleted on shutdown appropriately..
extern void		FionaUTCleanupGraphics (void);
extern void		FionaUTSleep		(float time);

//Fiona Config - the main application configuration structure read / written to disk
struct FionaConfig
{
	enum APP_TYPE { WINDOWED = 0, HEADNODE, DEVLAB, DEVLAB2, CAVE1, CAVE2, CAVE3, CAVE4, CAVE5, CAVE6, 
		CAVE1_WIN8, CAVE2_WIN8, CAVE3_WIN8, CAVE4_WIN8, CAVE5_WIN8, CAVE6_WIN8, SLAVE, VUZIX, OCULUS, HDTV, 
		UNITY_DL, UNITY_CAVE, UNITY_DSCVR, DEVLAB_WIN8, TEST_STEREO, VIVE, DEVLAB_DUALPIPE, DEVLAB_DUALPIPE_SLAVE, 
		DEVLAB_DUALPIPE_ONEMACHINE, DEVLAB_DUALVIEW_DUALPIPE, DEVLAB_DUALVIEW_DUALPIPE_SLAVE, CAVE1_DUALPIPE, CAVE2_DUALPIPE,
		CAVE3_DUALPIPE, CAVE4_DUALPIPE, CAVE5_DUALPIPE, CAVE6_DUALPIPE, NEW_DEVLAB, NEW_DEVLAB2, CAVE1_SS, CAVE2_SS, CAVE3_SS, CAVE4_SS, CAVE5_SS, CAVE6_SS,
	CAVE_NEW_DOOR, CAVE_NEW_RIGHT, CAVE_NEW_FRONT, CAVE_NEW_LEFT, CAVE_NEW_FLOOR, CAVE_NEW_CEILING};

	APP_TYPE				appType;

	// Derived data from APP_TYPE
	bool					stereo;
	short					multisample;
	short					fullscreen;
	bool					splitStereo;
	bool					slave;
	bool					master;
	bool					masterSlave;
	jvec3					trackerOffset;

	jvec3					sensorOffset;
	jvec3					kevinOffset;
	jvec3					lEyeOffset;
	jvec3					rEyeOffset;
	float					framerate;
	glm::vec3				backgroundColor;

	FIONA_FRAME_FUNC		frameFunc;
	FIONA_TRACKER_FUNC		trackerFunc;
	FIONA_WAND_BUTTON_FUNC	wandButtonFunc;
	FIONA_JOYSTIK_FUNC		joystickFunc;
	FIONA_IDLE_FUNC			idleFunc;
	FIONA_CLEANUP_FUNC		cleanupFunc;
	FIONA_POST_RENDER_FUNC	postFunc;
	FIONA_WALL_FUNC			wallFunc;
	FIONA_CONTROLLER_FUNC	controllerFunc;

	// Monitor mode.. Display image from the position of tracker
	bool					monitorView;
	bool					monitorFiltering;
	float					monitorStepBack;

	//display image from the position of wand
	bool					wandView;
	bool					dontClear;	//flag to temporarily not clear the color/depth buffers
	bool					deltaInput;	//flag for using time in accordance with input
	bool					desktopProjection;
	float					desktopFOV;

	std::string				playbackFileName;
	std::string				pathFileName;

	// Renderer related config
	bool					useFBO;
	bool					drawFBO;	//whether we actually display the FBO...
	int						FBOWidth;
	int						FBOHeight;

	// Tracker config
	std::string				trackerServer;
	std::string				trackerName;
	std::string				wandName;
	std::string				wandName2;
	std::string				inputHost;

	// Module specific config
	std::string				OgreMediaBasePath;
	std::string				VoreenRoot;

	// Runtime varying config
	jvec3					camPos;
	quat					camRot;

	jvec3					secondCamPos;
	quat					secondCamRot;

	jvec3					monitorCamPos;
	quat					monitorCamOri;

	jvec3					headPos;
	quat					headRot;

	jvec3					secondHeadPos;	//second head tracker position
	quat					secondHeadRot;	//second head tracker orientaiton

	jvec3					wandPos;
	quat					wandRot;

	jvec3					secondWandPos;
	quat					secondWandRot;

	float					physicsStep;
	float					physicsTime;

	// Some values from interface devices to convinient use
	jvec3					currentJoystick;
	short					currentButtons;

	//wii-fit run-time varying stuff
	float					wiiTopLeft;
	float					wiiBottomLeft;
	float					wiiBottomRight;
	float					wiiTopRight;

	//emg stuff
	unsigned int			emgRaw;
	unsigned int			emgAvg;
	unsigned int			emgBinary;
	unsigned int			emgThresh;

	//speech color
	float					speechVal;

	//Sync config
	short					port;		//master needs same port as slaves
	short					masterSlavePort;	//port for master and 2ndary machine to communicate over
	short					numSlaves;	//default number of slaves (3) - 4 if we wanted dev lab wall as well?
	std::string				masterIP;	//slaves need this ip filled out - which master they should connect to
	std::string				masterSlaveIP;	//
	jvec3					projectorCalibx[4];	// cave projector calibration
	jvec3					projectorCaliby[4];	// matrix [ pcx[i].x pcx[i].y 0 pcx[i].z ]
	//        [ pcy[i].x pcy[i].y 0 pcy[i].z ]
	jvec3					projectorCalibz[4];	//        [        0        0 1        0 ]
	//        [ pcz[i].x pcz[i].y 0 pcz[i].z ]

	// Aux Data which can be configured and used later..
	std::string				wallDescriptionFilename;

	float					navigationSpeed;
	float					rotationSpeed;
	float					nearClip;
	float					farClip;

	bool					riftTracking;
	bool					displayGraphicsMem;
	bool					dualView;
	bool					showRenderTime;
	bool					renderHeadNode;

	int						tvFileIndex;
	int						tvBufferIndex;

	bool					graphicsThread;
	bool					noTracking;
	bool					noHeadTracking;
	bool					multiGPU;
	bool					useSecondViewerNodes;
	bool					useSecondGPU;
	bool					twoWindows;	//experimenting with two windows...
	bool					hardwareSync;
	GLuint					fbo;
	GLuint					img;
	GLuint					depth;
	bool					forceOculusFBOSizeMatch;
	bool					fboSameAsWindow;
	bool					layeredStereo;
	bool					borderlessWindow;
	bool					singlePassStereo;
	float					oculusResMultiplier;
#ifdef ENABLE_OCULUS
#ifdef ENABLE_DK2
	ovrSwapTextureSet *		pTextureSet;
#endif

#ifdef ENABLE_CV1
	long long				frameIndex;
	TextureBuffer *			eyeRenderTexture[2];
	DepthBuffer   *			eyeDepthBuffer[2];
	ovrMirrorTexture		mirrorTexture;
	GLuint					mirrorFBO;
	double					sensorSampleTime;
	ovrSession				session;
	ovrSessionStatus		sessionStatus;
	ovrHmdDesc				hmdDesc;
	ovrEyeRenderDesc		oculusEyeRenderDesc[2];
	ovrPosef				eyeRenderPose[2];
#endif
#endif
#ifdef ENABLE_VIVE
	vr::IVRSystem *m_pHMD;
	FramebufferDesc leftEyeDesc;
	FramebufferDesc rightEyeDesc;
	glm::mat4				hmdPose;
#endif
	glm::mat4				mvRight;
	glm::mat4				mvLeft;
	glm::mat4				mvCenter;
	glm::mat4				projRight;
	glm::mat4				projLeft;
	glm::mat4				projCenter;

#ifdef ENABLE_OCULUS
#ifdef ENABLE_DK2 
	ovrHmd					oculusHmd;
	ovrEyeRenderDesc		oculusEyeRenderDesc[2];
	ovrLayerEyeFov			layer;
	//ovrGLTexture			oculusEyeTexture[2];
#endif
#ifdef ENABLE_DK1
	OVR::SensorFusion *		sFusion;
	OVR::Ptr<OVR::SensorDevice> pSensor;
#endif
	//OVR::Util::Render::StereoConfig	stereoConfig;
#endif

#ifndef LINUX_BUILD
	LeapData				leapData;
#endif
	KinectData				kinectData;
	KinectVideoData			kinectVideoData;
#ifndef LINUX_BUILD
	TCHAR					oldConsoleTitle[MAX_PATH];
#endif
};


//****************************************************************************
// 
//  Viewport class..
//  which corresponds to a projector in cave
//  
//  would be smallest unit of display pipeline
//  It handles the final rendering of the scene after CAVE projection
//  It also handles projector calibration
// 
//****************************************************************************

struct FionaViewport
{
	float	sx, sy, sw, sh;		// Source range screen
	float	lx, ly, lw, lh;		// Destination range in screen for left eye
	float	rx, ry, rw, rh;		// Destination range in screen for right eye
								// r is the same to l unless split stereo mode
	float	cScaleX, cScaleY;
	float	cOffsetX, cOffsetY;
	FionaViewport(void)
	{
		rx=lx=ry=ly=sx=sy=0; rw=rh=lw=lh=sw=sh=1;
		cScaleX=cScaleY=1;
		cOffsetX=cOffsetY=0;
	}
	FionaViewport(float _sx, float _sy, float _sw, float _sh,
				  float _lx=0, float _ly=0, float _lw=1, float _lh=1,
				  float _rx=-1, float _ry=-1, float _rw=-1, float _rh=-1)
	{
		sx=_sx; sy=_sy; sw=_sw; sh=_sh;
		lx=_lx; ly=_ly; lw=_lw; lh=_lh;
		rx=_rx<0?lx:_rx; ry=_ry<0?ly:_ry; rw=_rw<0?lw:_rw; rh=_rh<0?lh:_rh;
		cScaleX=cScaleY=1;
		cOffsetX=cOffsetY=0;
	}
};

//****************************************************************************
//
//  Wall: corresponding to a image from same viewpoint
//
//   A wall is a collection of viewports share same CAVE projection
//      which means all mutually planar screen components at the same depth.
//
//   It also corresponds to a FBO
//
//****************************************************************************

struct FionaWall
{
	std::vector<FionaViewport> viewports;
	jvec3	cntr;
	quat	preRot;
	jvec3	sz;
	jvec3	vLB;
	jvec3	vRB;
	jvec3	vLT;
	FionaWall(const jvec3& pp, const jvec3& xx, const jvec3& yy)
	{
		//pp is lower left corner (in tracker space)
		//xx is horizontal vector that defines physical wall (in tracker space) (not normalized)
		//yy is vertical vector that defines physical wall (in tracker space) (not normalized)
		vLB = pp;
		vLT = pp+yy;
		vRB = pp+xx;
		
		jvec3 p=pp, x=xx,y=yy,z=x.unit()*y.unit();
		//z is calculated to be the normal vector of the wall (in tracker space) - i.e. direction wall is facing in the cave
		//get length in x and y dimension and calculate 
		float lx=x.len(), ly=y.len(), lz=-(p%z);	//lz is the distance to the wall from center of cave
		
		//multiplying length of y by z cross x..
		//results in vertical vector of wall
		//but then this isn't used after this...?
		//y = ly*z*x.unit();
		
		//preRot becomes a rotation between the wall normal and 0,0,1
		//this enables rotating the wall on the vertical axis
		preRot = quat(z,ZAXIS);
		//and then this allows rolled over x axis to account for tilt forward or backward..
		preRot = quat(preRot.rot(x.unit()),XAXIS)*preRot;

		//x=preRot.rot(x.unit()); //it appears this isn't doing anything
		//y=preRot.rot(y.unit()); //same here...
		
		//center of wall after it's been rotated
		cntr = preRot.rot(p)+XAXIS*lx/2+YAXIS*ly/2+lz*ZAXIS;
		//and it's dimensions...
		sz = jvec3(lx,ly,lz*2);
	}
};

struct FionaWindowConfig
{
	enum WALL_TYPE{
		NORMAL=0, FULLSCREEN, WIDE, };
	WALL_TYPE				type;
	WIN						window;
	int						winw, winh;
	int						winx, winy;
	int						lastx, lasty;
	FIONA_DISPLAY_FUNC		displayFunc;
	FIONA_RESHAPE_FUNC		reshapeFunc;
	FIONA_KEYBOARD_FUNC		keyboardFunc;
	FIONA_MOUSE_FUNC		mouseFunc;
	FIONA_MOTION_FUNC		motionFunc;
	FIONA_PASSIVE_FUNC		passiveFunc;
	
	std::vector<FionaWall>	walls;
};

extern FionaConfig			fionaConf;
extern FionaWindowConfig	fionaWinConf[MAX_WINDOW];
extern unsigned int			fionaGlobalMode;

extern int					fionaNextWindow;
extern int					fionaCurWindow;
extern int					fionaCurWall;
extern int					fionaCurViewport;
extern jvec3					fionaCurEyePos;

extern WIN					fionaActiveWindow;
extern CTX					fionaActiveContext;
extern int					fionaRenderCycleCount;
extern bool					fionaRenderCycleCenter;
extern bool					fionaRenderCycleLeft;
extern bool					fionaRenderCycleRight;
extern bool					fionaDone; 
extern bool					fionaRenderDone;

extern std::queue<std::string> fionaCommandQueue;


// Following is FionaBridge between FionaUT and Native Windowing System

// We need Native Windowing Function as follows
extern WIN		__FionaUTCreateWindow	(const char* name,int x,int y,int w,int h,int mode);
extern void		__FionaUTMakeFillScreen	(WIN win,int i);
extern void		__FionaUTMakeWideScreen	(WIN win);
extern void		__FionaUTSwapBuffer		(WIN win);
extern void		__FionaUTMainLoop		(int framerate=-1);
extern void		__FionaUTCloseWindow	(void);
extern void		__FionaUTInitNative		(void);
//extern void	__FionaUTRequestRedraw	(WIN win);
extern CTX		__FionaUTGetContext		(WIN win, bool secondGPU=false);
#ifndef LINUX_BUILD
extern HDC		__FionaUTGetDeviceContext (WIN win, bool secondGPU=false);
#else
extern Display *	__FionaUTGetDeviceContext (WIN win, bool secondGPU=false);
#endif
extern void		__FionaUTMakeCurrent	(WIN win, bool secondGPU=false);
extern BOOL		__FionaUTSetContext		(WIN win, bool secondContext=false);
extern int		__FionaUTGetW			(WIN win);
extern int		__FionaUTGetH			(WIN win);
extern int		__FionaUTGetVideoMemorySizeBytes(bool total, GLuint memFlag);

// Callback functions from Native Windowing System
extern void		 _FionaUTCreated		(WIN win);
extern void		 _FionaUTKeyboard		(WIN win, unsigned int k);
extern void		 _FionaUTMouseDown		(WIN win, int b, int x, int y);
extern void		 _FionaUTMouseUp		(WIN win, int b, int x, int y);
extern void		 _FionaUTMouseMove		(WIN win, int x, int y);
extern void		 _FionaUTMouseDrag		(WIN win, int x, int y);

extern void		 _FionaUTDisplay		(WIN win, CTX cntx);
extern void		 _FionaUTDisplaySecondWindow (WIN win, CTX cntx);

extern void		 _FionaUTReshape		(WIN win, int w, int h);
extern void		 _FionaUTIdle			(void);


// Fiona Specific..
// Global (Internal) Callbacks
extern void		 _FionaUTFrame			(void);
extern void		 _FionaUTTracker		(int i, const jvec3& p, const quat& q);
extern void		 _FionaUTWandButton		(int i, int b, int idx=0);
extern void		 _FionaUTJoystick		(int i, const vec4& p);

// Utility functions accesed by internal functions
extern int		 _FionaUTFindWindow		(WIN win);
extern WIN		 _FionaUTFirstWindow	(void);
extern void		 _FionaUTInitConfig		(int argc, char* argv[]);
extern void		 _FionaUTCreateAppTypeWindow(const char* name);
extern void		 _FionaUTCreateGeneralWindow(const char* name);
extern bool		 _FionaUTIsInFBO		(void);
extern int		 _FionaUTGetFBOWidth	(void);
extern int		 _FionaUTGetFBOHeight	(void);
extern void		 _FionaUTCalcMatrices(int eye, int wini, const FionaWall& wall, const FionaViewport& vp, const jvec3& _ep, int w, int h);
extern jvec3	 _FionaUTCalcEyePosition(int eye, const FionaWall& wall);

extern int		 _FionaUTIsUsingExtFBO	(void);
extern void		 _FionaUTUseExtFBO		(bool);
extern void		 _FionaUTSetExtFBOTexture(GLuint tex);
extern GLuint	 _FionaUTGetExtFBOTexture(void);
extern void		 _FionaUTCleanupRendering (void);

inline bool FionaIsFirstOfCycle(void){ return fionaRenderCycleCount==0; }
inline bool FionaRenderCycleLeft (void){ return fionaRenderCycleLeft; }
inline bool FionaRenderCycleRight(void){ return fionaRenderCycleRight; }

inline bool _FionaUTIsSingleWindow(void)
{
	return ((fionaConf.appType == FionaConfig::HEADNODE) || (fionaConf.appType == FionaConfig::HDTV) || (fionaConf.appType == FionaConfig::WINDOWED));
}

inline bool _FionaUTIsSingleMachine(void)
{
	return ((fionaConf.appType == FionaConfig::HEADNODE) || (fionaConf.appType == FionaConfig::HDTV) || (fionaConf.appType == FionaConfig::WINDOWED) || (fionaConf.appType == FionaConfig::DEVLAB) || (fionaConf.appType == FionaConfig::DEVLAB_WIN8));
}

inline bool _FionaUTIsSingleViewMachine(void)
{
	return ((fionaConf.appType == FionaConfig::CAVE1_WIN8) || (fionaConf.appType == FionaConfig::CAVE2_WIN8) || (fionaConf.appType == FionaConfig::CAVE3_WIN8) || (fionaConf.appType == FionaConfig::CAVE4_WIN8) || (fionaConf.appType == FionaConfig::CAVE5_WIN8) || (fionaConf.appType == FionaConfig::CAVE6_WIN8) || (fionaConf.appType == FionaConfig::DEVLAB) || (fionaConf.appType == FionaConfig::HEADNODE) || (fionaConf.appType == FionaConfig::CAVE1) || (fionaConf.appType == FionaConfig::CAVE2) || (fionaConf.appType == FionaConfig::CAVE3)); 
}

inline bool _FionaUTIsDualViewMachine(void)
{
	return ((fionaConf.appType == FionaConfig::DEVLAB2) ||(fionaConf.appType == FionaConfig::CAVE4) || (fionaConf.appType == FionaConfig::CAVE5) || (fionaConf.appType == FionaConfig::CAVE6)); 
}

inline bool _FionaUTIsCAVEMachine(void)
{
	return (fionaConf.appType == FionaConfig::CAVE1_DUALPIPE || fionaConf.appType == FionaConfig::CAVE2_DUALPIPE || fionaConf.appType == FionaConfig::CAVE3_DUALPIPE ||
			fionaConf.appType == FionaConfig::CAVE4_DUALPIPE || fionaConf.appType == FionaConfig::CAVE5_DUALPIPE || fionaConf.appType == FionaConfig::CAVE6_DUALPIPE ||
			fionaConf.appType == FionaConfig::CAVE1_WIN8 || fionaConf.appType == FionaConfig::CAVE2_WIN8 || fionaConf.appType == FionaConfig::CAVE3_WIN8 || 
			fionaConf.appType == FionaConfig::CAVE4_WIN8 || fionaConf.appType == FionaConfig::CAVE5_WIN8 || fionaConf.appType == FionaConfig::CAVE6_WIN8 ||
			fionaConf.appType == FionaConfig::CAVE1 || fionaConf.appType == FionaConfig::CAVE2 || fionaConf.appType == FionaConfig::CAVE3 ||
			fionaConf.appType == FionaConfig::CAVE4 || fionaConf.appType == FionaConfig::CAVE5 || fionaConf.appType == FionaConfig::CAVE6 ||
			fionaConf.appType == FionaConfig::CAVE1_SS || fionaConf.appType == FionaConfig::CAVE2_SS || fionaConf.appType == FionaConfig::CAVE3_SS ||
			fionaConf.appType == FionaConfig::CAVE4_SS || fionaConf.appType == FionaConfig::CAVE5_SS || fionaConf.appType == FionaConfig::CAVE6_SS || 
			fionaConf.appType == FionaConfig::CAVE_NEW_DOOR || fionaConf.appType == FionaConfig::CAVE_NEW_FRONT || fionaConf.appType == FionaConfig::CAVE_NEW_RIGHT ||
			fionaConf.appType == FionaConfig::CAVE_NEW_LEFT || fionaConf.appType == FionaConfig::CAVE_NEW_CEILING || fionaConf.appType == FionaConfig::CAVE_NEW_FLOOR);
}

// Synchronization socket information
#include "FionaNetwork.h"
extern CaveMaster*		fionaNetMaster;
extern CaveSlave*		fionaNetSlave;
extern CaveSlave*		fionaNetMasterSlave;

#include "FionaScene.h"
extern FionaScene*		scene;

#endif

#ifdef WIN32
extern bool Fionais360ControllerPresent();
extern XINPUT_STATE FionaGetControllerState(void);
extern void _FionaUTJoypad(unsigned short wB, unsigned char bLT, unsigned char bRT, short sTLX, short sTLY, short sTRX, short sTRY);
#endif

#ifndef FIONA_UT_IMPLEMENTATION

#ifdef glutInit
#undef glutInit
#endif
#define glutInit				FionaUTInit

#define glutInitDisplayMode		FionaUTInitDisplayMode
#define glutInitWindowSize		FionaUTInitWindowSize
#ifdef glutCreateWindow
#undef glutCreateWindow
#endif
#define glutCreateWindow		FionaUTCreateWindow
#define glutFullScreen			FionaUTFullScreen
#define glutMainLoop			FionaUTMainLoop
#define glutGetWindow			FionaUTGetWindow
#define glutSetWindow			FionaUTSetWindow
#define glutGet					FionaUTGet
#define glutGetJoystick			FionaUTGetJoystick
#define glutGetWandButtons		FionaUTGetWandButtons


#define glutDisplayFunc			FionaUTDisplayFunc
#define glutReshapeFunc			FionaUTReshapeFunc
#define glutKeyboardFunc		FionaUTKeyboardFunc
#define glutMouseFunc			FionaUTMouseFunc
#define glutMotionFunc			FionaUTMotionFunc
#define glutIdleFunc			FionaUTIdle
#define glutPassiveMotionFunc	FionaUTPassiveMotionFunc
#define glutWallFunc			FionaUTWallFunc

#define glutFrameFunc			FionaUTFrameFunc
#define glutTrackerFunc			FionaUTTrackerFunc
#define glutWandButtonFunc		FionaUTWandButtonFunc
#define glutJoystickFunc		FionaUTJoystickFunc
#define glutCleanupFunc			FionaUTCleanupFunc
#define glutPostRender			FionaUTPostRenderFunc
#define glutControllerFunc		FionaUTControllerFunc

#define glutGetNativeWindow		FionaUTGetNativeWindow
#define	glutGetContext			FionaUTGetContext

#endif
