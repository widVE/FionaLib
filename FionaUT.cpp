//
//  FionaUT.cpp
//  FionaUT
//
//  Created by Hyun Joon Shin on 5/7/12.
//  Copyright 2012 __MyCompanyName__. All rights reserved.
//


#define FIONA_UT_IMPLEMENTATION
#include "FionaUT.h"
#include "FionaUTVRPN.h"

#ifndef NULL
#define NULL 0x0
#endif

//#define NV_PROFILE

//#ifdef NV_PROFILE
//#include "nvToolsExt.h"
//#endif

//#define PROFILE_TIME


// **********************************************
//
//        INTERNAL UTILITY FUNCTIONS
//
// **********************************************

#ifdef ENABLE_VIVE
bool CreateFrameBuffer(int nWidth, int nHeight, FramebufferDesc &framebufferDesc)
{
	//does the multi-sample stuff here cause issues?

	glGenFramebuffers(1, &framebufferDesc.m_nRenderFramebufferId);
	glBindFramebuffer(GL_FRAMEBUFFER, framebufferDesc.m_nRenderFramebufferId);

	glGenRenderbuffers(1, &framebufferDesc.m_nDepthBufferId);
	glBindRenderbuffer(GL_RENDERBUFFER, framebufferDesc.m_nDepthBufferId);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT, nWidth, nHeight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, framebufferDesc.m_nDepthBufferId);

	glGenTextures(1, &framebufferDesc.m_nRenderTextureId);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, framebufferDesc.m_nRenderTextureId);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA8, nWidth, nHeight, true);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, framebufferDesc.m_nRenderTextureId, 0);

	glGenFramebuffers(1, &framebufferDesc.m_nResolveFramebufferId);
	glBindFramebuffer(GL_FRAMEBUFFER, framebufferDesc.m_nResolveFramebufferId);

	glGenTextures(1, &framebufferDesc.m_nResolveTextureId);
	glBindTexture(GL_TEXTURE_2D, framebufferDesc.m_nResolveTextureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebufferDesc.m_nResolveTextureId, 0);

	// check FBO status
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		return false;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return true;
}

#endif
// Find WindowConfig index from Native Window handle (such as HWND,..)
int	_FionaUTFindWindow	(WIN win)
{
	for(int i=0;i<MAX_WINDOW; i++) if(fionaWinConf[i].window==win) return i;
	return -1;
}

// Find the first Native window created
WIN  _FionaUTFirstWindow (void)
{
	if(fionaWinConf[0].window) return fionaWinConf[0].window;
	return NULL;
}


//***********************************************
//
//        FionaUT Core & Windowing Functions
//
// **********************************************

// Initialize subsystems based on configuration including 
//  Native Windowing system
//  VRPN subsystem
//  and Synchronization system
// This function also handle the configuration parameters in
//  either commandline argument and the configuration file
void FionaUTInit(int* argc, char* argv[])
{
	__FionaUTInitNative();
	
	_FionaUTInitConfig(*argc, argv);
	
	_FionaUTSyncInit();
	
	if(!fionaConf.noTracking)
	{
		if( fionaConf.trackerServer.size() > 0 )
		{
			if(!fionaConf.dualView)
			{
				FionaUTVRPN::Init(fionaConf.trackerServer.c_str(),
								  fionaConf.trackerName.c_str(),
								  fionaConf.wandName.c_str(),
								  fionaConf.inputHost.c_str());
			}
			else
			{
				FionaUTVRPN::Init(fionaConf.trackerServer.c_str(),
								  fionaConf.trackerName.c_str(),
								  fionaConf.wandName.c_str(),
								  fionaConf.inputHost.c_str(),
								  fionaConf.wandName2.c_str());
			}
		}
	}
}

// Create window function
// Note: this function can be called several times if the application is in Windowed mode.
// For further detail about this function, refer _FionaUTCreateAppTypeWindow
int FionaUTCreateWindow(const char* name)
{
	if( fionaNextWindow >= MAX_WINDOW ) return -1;
	if( fionaNextWindow == 0 ) // App Type window is only for the first window
		_FionaUTCreateAppTypeWindow(name);
	else
		_FionaUTCreateGeneralWindow(name);
#ifndef LINUX_BUILD
	GetConsoleTitle(fionaConf.oldConsoleTitle,MAX_PATH);
#endif

	return fionaCurWindow;
}


#ifndef LINUX_BUILD
__int64 BeginTiming(double & freq)
{
	__int64 CounterStart = 0;

	LARGE_INTEGER li;
	QueryPerformanceFrequency(&li);
	freq = double(li.QuadPart)/1000.0;
	QueryPerformanceCounter(&li);
	CounterStart = li.QuadPart;

	return CounterStart;
}
#endif


// Init display mode
// Only configurable mode is stereo; the other mode values are ignored
void FionaUTInitDisplayMode(unsigned int mode)
{
	fionaGlobalMode =GLUT_RGB|GLUT_DOUBLE|GLUT_DEPTH;//|GLUT_STENCIL;
	if(fionaConf.multisample > 0 ) 
	{
		printf("Activating multi-sampling %d...\n", fionaConf.multisample);
		fionaGlobalMode|=GLUT_MULTISAMPLE;
	}

	fionaGlobalMode|=mode;
	if(fionaConf.stereo) fionaGlobalMode|=GLUT_STEREO;
//	printf("FionaStereo: %d\n",fionaConf.stereo);
	if(mode&GLUT_STEREO) fionaConf.stereo=true;
}

// Make window a fullscreen window
void	FionaUTFullScreen		(void)
{
	if(fionaCurWindow<0) return;
	FionaWindowConfig& conf = fionaWinConf[fionaCurWindow];
	if(conf.window) __FionaUTMakeFillScreen(conf.window,0);
}

// Set window size to create.
//  The window size is used only when app is in Windowed mode.
void	FionaUTInitWindowSize(int w, int h)
{
	fionaWinConf[fionaNextWindow].winw=w;
	fionaWinConf[fionaNextWindow].winh=h;
}

// Simulating glutGet() function.
//   only width and height of window are gettable, now.
int		FionaUTGet(int w)
{
	switch( w )
	{
		case GLUT_WINDOW_X:			return 0;
		case GLUT_WINDOW_Y:			return 0;
		case GLUT_WINDOW_WIDTH:		return __FionaUTGetW(fionaWinConf[fionaCurWindow].window);
		case GLUT_WINDOW_HEIGHT:	return __FionaUTGetH(fionaWinConf[fionaCurWindow].window);
	}
	return 0;
}

// Mainloop function
void	FionaUTMainLoop			(void)	{__FionaUTMainLoop();}
// Get current window index
int		FionaUTGetWindow		(void)	{return fionaCurWindow;}
// Set current window index
void	FionaUTSetWindow		(int i)	{fionaCurWindow = i;}
// Get Native window handle of the current window
WIN		FionaUTGetNativeWindow	(void)	{return fionaActiveWindow;}
// Get Native OpenGL context for the current window
CTX		FionaUTGetContext		(void)	{return fionaActiveContext;}

//******************************************
#ifdef ENABLE_VIVE
void ComposeProjection(float fLeft, float fRight, float fTop, float fBottom, float zNear, float zFar, vr::HmdMatrix44_t *pmProj)
{

	/*float projXScale = 2.0f / (fLeft + fRight);
	float projXOffset = (fLeft - fRight) * projXScale * 0.5f;
	float projYScale = 2.0f / (fTop + fBottom);
	float projYOffset = (fTop - fBottom) * projYScale * 0.5f;*/

	//float idx = 1.0f / (fRight - fLeft);
	//float idy = 1.0f / (fBottom - fTop);
	//float idz = 1.0f / (zFar - zNear);
	/*float sx = fLeft - fRight;
	float sy = fTop - fBottom;

	float projXScale = 2.f / (fLeft + fRight);
	float projYScale = 2.f / (fBottom + fTop);*/

	/*float(*p)[4] = pmProj->m;
	p[0][0] = projXScale; p[0][1] = 0;     p[0][2] = -1.f * projXOffset;    p[0][3] = 0;
	p[1][0] = 0;     p[1][1] = projYScale; p[1][2] = -1.f * -projYOffset;    p[1][3] = 0;
	p[2][0] = 0;     p[2][1] = 0;     p[2][2] = (zNear+zFar)/(zNear-zFar); p[2][3] = 2.f*zFar*zNear/(zNear-zFar);
	p[3][0] = 0;     p[3][1] = 0;     p[3][2] = -1.0f;     p[3][3] = 0;*/

	float idx = 1.0f / (fRight - fLeft);
	float idy = -(1.0f / (fBottom - fTop));
	float idz = 1.0f / (zFar - zNear);
	float sx = fRight + fLeft;
	float sy = fBottom + fTop;

	float(*p)[4] = pmProj->m;
	p[0][0] = 2 * idx; p[0][1] = 0;     p[0][2] = sx*idx;    p[0][3] = 0;
	p[1][0] = 0;     p[1][1] = 2 * idy; p[1][2] = sy*idy;    p[1][3] = 0;
	p[2][0] = 0;     p[2][1] = 0;     p[2][2] = -zFar*idz; p[2][3] = -zFar*zNear*idz;
	p[3][0] = 0;     p[3][1] = 0;     p[3][2] = -1.0f;     p[3][3] = 0;
}
#endif

// Windowing related functions called back by Native Windowing system
void _FionaUTCreated	(WIN win) 
{
	glDrawBuffer(GL_BACK);

	//need to make sure these get set in linux..
	glClearColor(fionaConf.backgroundColor.x,
				fionaConf.backgroundColor.y,
				fionaConf.backgroundColor.z,
				0.f);
	
	glMatrixMode(GL_PROJECTION);
#ifdef ENABLE_OCULUS
#ifdef ENABLE_CV1
	// Make eye render buffers
	for (int eye = 0; eye < 2; ++eye)
	{
		ovrSizei idealTextureSize = ovr_GetFovTextureSize(fionaConf.session, ovrEyeType(eye), fionaConf.hmdDesc.DefaultEyeFov[eye], fionaConf.oculusResMultiplier);
		printf("Eye %d, Width: %u, Height %u\n", eye, idealTextureSize.w, idealTextureSize.h);
		fionaConf.eyeRenderTexture[eye] = new TextureBuffer(fionaConf.session, true, true, idealTextureSize, 1, NULL, 1);
		fionaConf.eyeDepthBuffer[eye] = new DepthBuffer(fionaConf.eyeRenderTexture[eye]->GetSize(), 0);

		if (!fionaConf.eyeRenderTexture[eye]->TextureChain)
		{
			printf("Failed to create eye texture for CV1\n");
		}
	}

	ovrMirrorTextureDesc desc;
	memset(&desc, 0, sizeof(desc));
	desc.Width = fionaConf.FBOWidth;
	desc.Height = fionaConf.FBOHeight;
	desc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;

	// Create mirror texture and an FBO used to copy mirror texture to back buffer
	ovrResult result = ovr_CreateMirrorTextureGL(fionaConf.session, &desc, &fionaConf.mirrorTexture);
	if (!OVR_SUCCESS(result))
	{
		printf("Failed to create mirror texture\n");
	}

	// Configure the mirror read buffer
	GLuint texId;
	ovr_GetMirrorTextureBufferGL(fionaConf.session, fionaConf.mirrorTexture, &texId);

	glGenFramebuffers(1, &fionaConf.mirrorFBO);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, fionaConf.mirrorFBO);
	glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texId, 0);
	glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
#endif
#endif

#ifdef ENABLE_VIVE

	uint32_t renderW, renderH;
	if (fionaConf.m_pHMD)
	{
		fionaConf.m_pHMD->GetRecommendedRenderTargetSize(&renderW, &renderH);

		if (fionaConf.singlePassStereo)
		{
			fionaConf.FBOWidth = (int)renderW;
			fionaConf.FBOHeight = (int)renderH;

			float ll, lr, lt, lb = 0.f;
			fionaConf.m_pHMD->GetProjectionRaw(vr::Eye_Left, &ll, &lr, &lt, &lb);
			float rl, rr, rt, rb = 0.f;
			fionaConf.m_pHMD->GetProjectionRaw(vr::Eye_Right, &rl, &rr, &rt, &rb);

			float ll2 = atan(ll);
			float lr2 = atan(lr);
			float lt2 = atan(lt);
			float lb2 = atan(lb);
			float rl2 = atan(rl);
			float rr2 = atan(rr);
			float rt2 = atan(rt);
			float rb2 = atan(rb);

			printf("Left Eye: %f %f %f %f\n", ll2, lr2, lt2, lb2);
			printf("Right Eye: %f %f %f %f\n", rl2, rr2, rt2, rb2);

			float ml = MAX(-ll, -rl);
			float mr = MAX(lr, rr);
			float mt  MAX(-lt, -rt);
			float mb = MAX(lb, rb);

			float ml2 = MAX(-ll2, -rl2);
			float mr2 = MAX(lr2, rr2);
			float mt2  MAX(-lt2, -rt2);
			float mb2 = MAX(lb2, rb2);

			float mtb = MAX(mt, mb);
			float mlr = MAX(ml, mr);

			float mtb2 = MAX(mt2, mb2);
			float mlr2 = MAX(ml2, mr2);

			ml = mr = mlr;
			mt = mb = mtb;

			ml2 = mr2 = mlr2;
			mt2 = mb2 = mtb2;
			printf("Max Eye: %f %f %f %f\n", ml, mr, mt, mb);

			/*ml *= fionaConf.nearClip;
			mr *= fionaConf.nearClip;
			mt *= fionaConf.nearClip;
			mb *= fionaConf.nearClip;*/

			vr::HmdMatrix44_t mat;
			ComposeProjection(-ml, mr, mt, -mb, fionaConf.nearClip, fionaConf.farClip, &mat);

			fionaConf.projLeft = glm::mat4(mat.m[0][0], mat.m[1][0], mat.m[2][0], mat.m[3][0],
				mat.m[0][1], mat.m[1][1], mat.m[2][1], mat.m[3][1],
				mat.m[0][2], mat.m[1][2], mat.m[2][2], mat.m[3][2],
				mat.m[0][3], mat.m[1][3], mat.m[2][3], mat.m[3][3]);
			
			//need to adjust FBOWidth / Height to account for overlapping FOV...
			glm::vec4 textureBounds[2];
			textureBounds[0].x = 0.5f + 0.5f * ll / mlr;
			textureBounds[0].y = 0.5f + 0.5f * lr / mlr;
			textureBounds[0].z = 0.5f - 0.5f * lb / mtb;
			textureBounds[0].w = 0.5f - 0.5f * lt / mtb;

			textureBounds[1].x = 0.5f + 0.5f * rl / mlr;
			textureBounds[1].y = 0.5f + 0.5f * rr / mlr;
			textureBounds[1].z = 0.5f - 0.5f * rb / mtb;
			textureBounds[1].w = 0.5f - 0.5f * rt / mtb;

			//fionaConf.FBOWidth = fionaConf.FBOWidth / MAX(textureBounds[0].y - textureBounds[0].x, textureBounds[1].y - textureBounds[1].x);
			//fionaConf.FBOHeight = fionaConf.FBOHeight / MAX(textureBounds[0].w - textureBounds[0].z, textureBounds[1].w - textureBounds[1].z);

			//printf("**** Adjusted Width / Height to: %d %d\n", fionaConf.FBOWidth, fionaConf.FBOHeight);
			/*fionaConf.projLeft = glm::mat4(mat.m[0][0], mat.m[0][1], mat.m[0][2], mat.m[0][3],
				mat.m[1][0], mat.m[1][1], mat.m[1][2], mat.m[1][3],
				mat.m[2][0], mat.m[2][1], mat.m[2][2], mat.m[2][3],
				mat.m[3][0], mat.m[3][1], mat.m[3][2], mat.m[3][3]);*/

			CreateFrameBuffer(fionaConf.FBOWidth, fionaConf.FBOHeight, fionaConf.leftEyeDesc);
			CreateFrameBuffer(fionaConf.FBOWidth, fionaConf.FBOHeight, fionaConf.rightEyeDesc);

			fionaConf.projRight = fionaConf.projLeft;
		}
		else
		{
			printf("Recommended texture size: %u %u\n", renderW, renderH);
			CreateFrameBuffer(renderW, renderH, fionaConf.leftEyeDesc);
			CreateFrameBuffer(renderW, renderH, fionaConf.rightEyeDesc);

			fionaConf.FBOWidth = (int)renderW;
			fionaConf.FBOHeight = (int)renderH;

			vr::HmdMatrix44_t mat = fionaConf.m_pHMD->GetProjectionMatrix(vr::Eye_Left, fionaConf.nearClip, fionaConf.farClip);

			fionaConf.projLeft = glm::mat4(mat.m[0][0], mat.m[1][0], mat.m[2][0], mat.m[3][0],
				mat.m[0][1], mat.m[1][1], mat.m[2][1], mat.m[3][1],
				mat.m[0][2], mat.m[1][2], mat.m[2][2], mat.m[3][2],
				mat.m[0][3], mat.m[1][3], mat.m[2][3], mat.m[3][3]);

			mat = fionaConf.m_pHMD->GetProjectionMatrix(vr::Eye_Right, fionaConf.nearClip, fionaConf.farClip);

			fionaConf.projRight = glm::mat4(mat.m[0][0], mat.m[1][0], mat.m[2][0], mat.m[3][0],
				mat.m[0][1], mat.m[1][1], mat.m[2][1], mat.m[3][1],
				mat.m[0][2], mat.m[1][2], mat.m[2][2], mat.m[3][2],
				mat.m[0][3], mat.m[1][3], mat.m[2][3], mat.m[3][3]);
		}

		vr::HmdMatrix34_t matEyeLeft = fionaConf.m_pHMD->GetEyeToHeadTransform(vr::Eye_Left);

		fionaConf.mvLeft = glm::mat4(matEyeLeft.m[0][0], matEyeLeft.m[1][0], matEyeLeft.m[2][0], 0.0,
			matEyeLeft.m[0][1], matEyeLeft.m[1][1], matEyeLeft.m[2][1], 0.0,
			matEyeLeft.m[0][2], matEyeLeft.m[1][2], matEyeLeft.m[2][2], 0.0,
			matEyeLeft.m[0][3], matEyeLeft.m[1][3], matEyeLeft.m[2][3], 1.0f);

		fionaConf.mvLeft = glm::inverse(fionaConf.mvLeft);

		vr::HmdMatrix34_t matEyeRight = fionaConf.m_pHMD->GetEyeToHeadTransform(vr::Eye_Right);

		fionaConf.mvRight = glm::mat4(matEyeRight.m[0][0], matEyeRight.m[1][0], matEyeRight.m[2][0], 0.0,
			matEyeRight.m[0][1], matEyeRight.m[1][1], matEyeRight.m[2][1], 0.0,
			matEyeRight.m[0][2], matEyeRight.m[1][2], matEyeRight.m[2][2], 0.0,
			matEyeRight.m[0][3], matEyeRight.m[1][3], matEyeRight.m[2][3], 1.0f);

		fionaConf.mvRight = glm::inverse(fionaConf.mvRight);
	}
	else
	{
		printf("Vive not connected...\n");
	}

	if (!vr::VRCompositor())
	{
		printf("Couldn't initialize Vive Compositor!\n");
	}
#endif
}

void _FionaUTReshape	(WIN win, int w, int h)
{
	int i = _FionaUTFindWindow(win);
	if( i<0 ) i=fionaCurWindow;  // maybe the window is being created
	if( i<0 ) return;
	fionaCurWindow = i;	
	fionaWinConf[i].winw = w; fionaWinConf[i].winh = h;
	// If the application is in Windowed mode,
	//  recompute aspect ratio and set the wall size of the window.
	
	if (fionaConf.desktopProjection)
	{

	}
	else
	{
#ifndef DSCVR
		//make sure below if doesn't get hit for DSCVR
		//**** This code below is dangerous - it screws up the aspect ratio and is basically only assuming it will run on a desktop
		//however, if you don't set fullscreen to 1 or -1, the window config ends up assuming "Normal" (instead of Fullscreen or Wide) so 
		//then it does this stupid correction...***
		if (fionaWinConf[i].type == FionaWindowConfig::NORMAL)
		{
			if (fionaConf.appType != FionaConfig::UNITY_CAVE && fionaConf.appType != FionaConfig::UNITY_DL)
			{
				for (int k = 0; k<(int)fionaWinConf[i].walls.size(); k++)
					if (w>h) fionaWinConf[i].walls[k].sz = jvec3(1.5, 1.5*h / w, 1.5);
					else	fionaWinConf[i].walls[k].sz = jvec3(1.5*w / h, 1.5*w / h, 1.5);
			}
		}
#endif
	}

	if(fionaWinConf[i].reshapeFunc!=NULL) 
	{
		fionaWinConf[i].reshapeFunc(w,h);
	}
}

static float __lastTime = 0;
// This function is called at each frame
// If it is master, it flushes the event queue by sending them to the slaves
// After update, it render a frame.
// Note: before swap, master check all slaves are ready to swap.
//       Once all slaves are ready, it causes swap at all nodes.
#define DEBUG_RENDER_TIME 0

long last_time = 0;
bool init = false;
int frameCount=0;
long frameStart;

void _FionaUTPreFrame(void)
{
	float cur = FionaUTTime();
	float fStep = cur-__lastTime;

	if(!fionaNetSlave)
	{
		fionaConf.physicsStep = fStep;
		fionaConf.physicsTime = cur;
	}

	if(fionaNetMaster)
	{
		_FionaUTSyncSendPhysics(fStep, cur);
	}
	//for slaves, the physics timestep should be set in the Fiona Config - run-time varying section...

#if DEBUG_RENDER_TIME >2
	printf(" %3.1f ",(cur-__lastTime)*1000);
#endif
	__lastTime = cur;

	//std::cout << "[Fiona] cur: " << cur << " fStep: " << fStep << " lastTime: " << __lastTime << std::endl;

	//assemble packet to send to slaves
	if(fionaNetMaster) 
	{
		//Ross 11/28/2012, master can now be an optional slave to an additional computer.
		//this is so we can obtain kinect, wii-fit, etc. data
		if(fionaConf.masterSlave)
		{
			//here we need to grab the packets from the second machine and then pass them over to the our collection of packets here to send onto the cluster..
			//clear out any old leap data...
			_FionaUTSyncMasterSlaveSync();
		}

#ifdef PROFILE_TIME
		__int64 recvTimeStart = 0;
		double recvTimeStartFreq = 0.0;
		if(fionaConf.showRenderTime)
		{
			recvTimeStart = BeginTiming(recvTimeStartFreq);
		}
#endif

		_FionaUTSyncMasterSync();

#ifdef PROFILE_TIME
		if(fionaConf.showRenderTime)
		{
			LARGE_INTEGER li;
			QueryPerformanceCounter(&li);
			printf("Master Send Time: %f\n", double(li.QuadPart-recvTimeStart)/recvTimeStartFreq);
		}
#endif

	}
	else
	{
		if(fionaConf.masterSlave && !fionaConf.master)
		{
			_FionaUTSyncMasterSlaveSync();
		}
	}

	
#if DEBUG_RENDER_TIME >2
	printf("(%.2f, ", (FionaUTTime()-__lastTime)*1000);
#endif
}


void _FionaUTPostFrame(void)
{

	//if(!fionaConf.graphicsThread)
	//{
		//handshake here so we swap at the same time...
		if(fionaNetMaster)
		{
			//wait for slaves to know they are ready to swap
#ifdef PROFILE_TIME
			__int64 recvTimeStart = 0;
			double recvTimeStartFreq = 0.0;
			if(fionaConf.showRenderTime)
			{
				recvTimeStart = BeginTiming(recvTimeStartFreq);
			}
#endif
			fionaNetMaster->Handshake();

#ifdef PROFILE_TIME
			if(fionaConf.showRenderTime)
			{
				LARGE_INTEGER li;
				QueryPerformanceCounter(&li);
				printf("Recv Time: %f\n", double(li.QuadPart-recvTimeStart)/recvTimeStartFreq);
			}

			//send a bit back to slaves to tell them to swap
			__int64 sendTimeStart = 0;
			double sendTimeStartFreq = 0.0;
			if(fionaConf.showRenderTime)
			{
				sendTimeStart = BeginTiming(sendTimeStartFreq);
			}
#endif

			fionaNetMaster->WakeupSlaves();
			
#ifdef PROFILE_TIME
			if(fionaConf.showRenderTime)
			{
				LARGE_INTEGER li;
				QueryPerformanceCounter(&li);
				printf("Send Time: %f\n", double(li.QuadPart-sendTimeStart)/sendTimeStartFreq);
			}
#endif
			if(fionaConf.masterSlave)
			{
				fionaNetMasterSlave->Handshake();
				//wait for master to send signal before proceeding..
				fionaNetMasterSlave->WaitForMaster();
			}
		}
		else
		{
			if(fionaConf.masterSlave && !fionaConf.master)
			{
				fionaNetMasterSlave->Handshake();
				//wait for master to send signal before proceeding..
				fionaNetMasterSlave->WaitForMaster();
			}
		}

		if(fionaNetSlave)
		{

#ifdef PROFILE_TIME
			__int64 recvTimeStart = 0;
			double recvTimeStartFreq = 0.0;
			if(fionaConf.showRenderTime)
			{
				recvTimeStart = BeginTiming(recvTimeStartFreq);
			}
#endif

			//let master know we are ready to swap
			fionaNetSlave->Handshake();

#ifdef PROFILE_TIME
			if(fionaConf.showRenderTime)
			{
				LARGE_INTEGER li;
				QueryPerformanceCounter(&li);
				printf("Send Time: %f\n", double(li.QuadPart-recvTimeStart)/recvTimeStartFreq);
			}

			__int64 sendTimeStart = 0;
			double sendTimeStartFreq = 0.0;
			if(fionaConf.showRenderTime)
			{
				sendTimeStart = BeginTiming(sendTimeStartFreq);
			}
#endif

			//wait for master to send signal before proceeding..
			fionaNetSlave->WaitForMaster();

#ifdef PROFILE_TIME
			if(fionaConf.showRenderTime)
			{
				LARGE_INTEGER li;
				QueryPerformanceCounter(&li);
				printf("Recv Time: %f\n", double(li.QuadPart-sendTimeStart)/sendTimeStartFreq);
			}
#endif
		}
	//}

#if DEBUG_RENDER_TIME >2
	printf("%.2f, ", (FionaUTTime()-__lastTime)*1000);
#endif
}

void _FionaUTShowRenderTime(void)
{
	if(fionaConf.showRenderTime)
	{
		long t = (long)(FionaUTTime()*1000.f);
	#if DEBUG_RENDER_TIME ==1
		printf("%ld ", t-last_time);
	#endif
		if( (t-frameStart)>=1000)
		{
			float fFps = frameCount/(float)(t-frameStart)*1000;
			frameCount=0;
			frameStart=t;
#ifndef LINUX_BUILD
			std::ostringstream buffer;
			buffer << fFps;
			std::string s = "FrameRate = ";
			
			s.append(buffer.str());
			std::string title;

			title.append(fionaConf.oldConsoleTitle);
			title.append(".........");
			title.append(s);
			title.append(buffer.str());
			SetConsoleTitle(title.c_str());
#else
			printf("FrameRate = %f\n", fFps);
#endif
			
		}
		frameCount++;
		last_time=t;
	}
#ifndef LINUX_BUILD
	//Restores original title
	TCHAR currentTitle[MAX_PATH];
	GetConsoleTitle(currentTitle,MAX_PATH);
		if(!fionaConf.showRenderTime  &&  (currentTitle!=fionaConf.oldConsoleTitle))
			SetConsoleTitle(fionaConf.oldConsoleTitle);
#endif
}

void _FionaUTFrame		(void)
{
	//if in our own graphics thread, handle networking in idle function...

#ifdef PROFILE_TIME
	double PCFreq = 0.0;
	__int64 preRenderTimeStart = 0;
	if(fionaConf.showRenderTime)
	{
		preRenderTimeStart = BeginTiming(PCFreq);
	}
#endif

	_FionaUTPreFrame();

#ifdef PROFILE_TIME
	if(fionaConf.showRenderTime)
	{
		LARGE_INTEGER li;
		QueryPerformanceCounter(&li);
		printf("Pre-Render Time: %f\n", double(li.QuadPart-preRenderTimeStart)/PCFreq);
	}

	PCFreq = 0.0;
	__int64 renderTimeStart = 0;
	if(fionaConf.showRenderTime)
	{
		renderTimeStart = BeginTiming(PCFreq);
	}
#endif

	// Call user frame function
	if (fionaConf.frameFunc)
	{
		fionaConf.frameFunc(FionaUTTime());
	}

	if(!fionaConf.graphicsThread)
	{
		// Render to all the windows
		fionaRenderCycleCount=0;

		bool bDraw = true;
		if(fionaConf.appType == FionaConfig::HEADNODE)
		{
			bDraw = fionaConf.renderHeadNode;
		}

		//bDraw = false;

		for(int i=0;i<1/*MAX_WINDOW*/;i++) 
		{
			if(fionaWinConf[i].window!=NULL)
			{
				WIN win = fionaWinConf[i].window;
			
				if(bDraw)
				{
#ifdef LINUX_BUILD
					__FionaUTMakeCurrent(win, true);
#endif

//#ifdef NV_PROFILE
//					::nvtxRangePushA("Display");
//#endif
					_FionaUTDisplay(win,__FionaUTGetContext(win));
//#ifdef NV_PROFILE
//					::nvtxRangePop();
//#endif
				}
				else
				{
					glClearColor(fionaConf.backgroundColor.x,
					 fionaConf.backgroundColor.y,
					 fionaConf.backgroundColor.z,
					 1.f);

					glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
				}
				
				//3/10/2015 - took this glFinish out - need to retest on windows 8 cave
				//would be an optimization but there's a flashing bug on the dev lab wall..
/*#ifdef PROFILE_TIME
				__int64 glFinishTimeStart = 0;
				double glFinishFreq = 0.0;
				if(fionaConf.showRenderTime)
				{
					glFinishTimeStart = BeginTiming(glFinishFreq);
				}
#endif*/
#ifdef ENABLE_VIVE
				glFlush();
				glFinish();
#endif
#ifdef LINUX_BUILD
				//glFinish();
#endif
				//if(_FionaUTIsCAVEMachine())
				//{
					//glFinish();
					//glFlush();
				//}

/*#ifdef PROFILE_TIME
				if(fionaConf.showRenderTime)
				{
					LARGE_INTEGER li;
					QueryPerformanceCounter(&li);
					printf("GL Finish Time: %f\n", double(li.QuadPart-glFinishTimeStart)/glFinishFreq);
				}
#endif*/
				
			}

			break;
		}

#if DEBUG_RENDER_TIME >2
		printf("%.2f, ", (FionaUTTime()-__lastTime)*1000);
#endif
	
		if(fionaConf.postFunc)
		{
			fionaConf.postFunc();
		}
	}

#ifdef PROFILE_TIME
	if(fionaConf.showRenderTime)
	{
		LARGE_INTEGER li;
		QueryPerformanceCounter(&li);
		printf("Frame Time: %f\n", double(li.QuadPart-renderTimeStart)/PCFreq);
	}

	PCFreq = 0.0;
	__int64 postRenderTimeStart = 0;
	if(fionaConf.showRenderTime)
	{
		postRenderTimeStart = BeginTiming(PCFreq);
	}
#endif
	
	_FionaUTPostFrame();

#ifdef PROFILE_TIME
	if(fionaConf.showRenderTime)
	{
		LARGE_INTEGER li;
		QueryPerformanceCounter(&li);
		printf("Post Frame Time: %f\n", double(li.QuadPart-postRenderTimeStart)/PCFreq);
	}
#endif

	/*GLenum error = glGetError();
	while (error != GL_NO_ERROR)
	{
		printf("ErrorID: %i: %s\n", error, gluErrorString(error));
		error = glGetError(); // get next error if any.
	}*/

	if(!fionaConf.graphicsThread)
	{
		//_FionaUTShowRenderTime();

		// Swap the frame buffers
		//for(int i=0;i<MAX_WINDOW;i++) 
		//{
		//	if(fionaWinConf[i].window!=NULL)
		//	{
				//static double PCFreq = 0.0;
				//static __int64 swapTimeStart = 0;
#ifdef PROFILE_TIME
				__int64 glFinishTimeStart = 0;
				double glFinishFreq = 0.0;
				if(fionaConf.showRenderTime)
				{
					glFinishTimeStart = BeginTiming(glFinishFreq);
				}
#endif
				//glFinish();

#ifdef PROFILE_TIME
				if(fionaConf.showRenderTime)
				{
					LARGE_INTEGER li;
					QueryPerformanceCounter(&li);
					printf("GL Flush Time: %f\n", double(li.QuadPart-glFinishTimeStart)/glFinishFreq);
				}
#endif
				
#ifdef PROFILE_TIME
				double PCFreq = 0.0;
				__int64 swapTimeStart = 0;
				if(fionaConf.showRenderTime)
				{
					swapTimeStart = BeginTiming(PCFreq);
				}
#endif
				if (!fionaConf.dontClear)
				{
					__FionaUTSwapBuffer(fionaWinConf[0].window);
				}

				/*if(swapTimeStart == 0)
				{
					swapTimeStart = BeginTiming(PCFreq);
				}
				else
				{
					LARGE_INTEGER li;
					QueryPerformanceCounter(&li);
					printf("Swap Time: %f\n", double(li.QuadPart-swapTimeStart)/PCFreq);
					swapTimeStart = 0;
				}*/


#ifdef PROFILE_TIME
				Sleep(0);
				if(fionaConf.showRenderTime)
				{
					LARGE_INTEGER li;
					QueryPerformanceCounter(&li);
					printf("Swap Time: %f\n", double(li.QuadPart-swapTimeStart)/PCFreq);
				}
#endif
			//}

			//break;
		//}
	}

#if DEBUG_RENDER_TIME >2
	printf("%.2f)", (FionaUTTime()-__lastTime)*1000);
#endif
	
#if DEBUG_RENDER_TIME >1
	if( 1/((FionaUTTime()-__lastTime)) <40 )
		printf("------------------------Drop Frame\n");
#if DEBUG_RENDER_TIME >2
	else printf("\n");
#endif
#endif
}

// Idle function.. which is a function called most often consistently.
//   At the first call of this function after a frame,
//   it waits for another frame from the master, if it is a slave.
void _FionaUTIdle		(void)
{
	// If it is slave, just wait for new frame
	if( fionaNetSlave )
	{
		//clear leap data before we get new
#ifndef LINUX_BUILD
		memset(&fionaConf.leapData, 0, sizeof(LeapData));
		
		fionaConf.leapData.hand1.t = Leap::Gesture::TYPE_INVALID;
		fionaConf.leapData.hand2.t = Leap::Gesture::TYPE_INVALID;
		fionaConf.leapData.hand1.s = Leap::Gesture::STATE_INVALID;
		fionaConf.leapData.hand2.s = Leap::Gesture::STATE_INVALID;
#endif
		
#ifdef PROFILE_TIME
		__int64 recvTimeStart = 0;
		double recvTimeStartFreq = 0.0;
		if(fionaConf.showRenderTime)
		{
			recvTimeStart = BeginTiming(recvTimeStartFreq);
		}
#endif

		_FionaUTSyncSlaveSync();

#ifdef PROFILE_TIME
		if(fionaConf.showRenderTime)
		{
			LARGE_INTEGER li;
			QueryPerformanceCounter(&li);
			printf("Sync Time: %f\n", double(li.QuadPart-recvTimeStart)/recvTimeStartFreq);
		}
#endif

		_FionaUTFrame();
	}

	// Otherwise continuousely collect tracker information
	else
	{
		if(fionaConf.riftTracking)
		{
#ifdef ENABLE_OCULUS
#ifdef ENABLE_DK2
			/*ovrTrackingState t = ovrHmd_GetTrackingState(fionaConf.oculusHmd, (double)fionaConf.physicsTime);
			ovrPoseStatef p = t.HeadPose;
			OVR::Quatf q = p.ThePose.Orientation;
			fionaConf.headRot.set(q.w, q.x, q.y, q.z);
			fionaConf.headPos.set(p.ThePose.Position.x, p.ThePose.Position.y, p.ThePose.Position.z);*/
#endif
#ifdef ENABLE_DK1
			OVR::Quatf q = fionaConf.sFusion->GetOrientation();
			fionaConf.headRot.set(q.w, q.x, q.y, q.z);
#endif
#endif
		}
		else
		{
#ifdef PROFILE_TIME
			__int64 recvTimeStart = 0;
			double recvTimeStartFreq = 0.0;
			if(fionaConf.showRenderTime)
			{
				recvTimeStart = BeginTiming(recvTimeStartFreq);
			}
#endif

			FionaUTVRPN::MainLoop();
#ifdef PROFILE_TIME
			if(fionaConf.showRenderTime)
			{
				LARGE_INTEGER li;
				QueryPerformanceCounter(&li);
				printf("VRPN Time: %f\n", double(li.QuadPart-recvTimeStart)/recvTimeStartFreq);
			}
#endif

		}

#ifdef WIN32
		if(Fionais360ControllerPresent())
		{
			XINPUT_STATE x = FionaGetControllerState();
			_FionaUTJoypad(x.Gamepad.wButtons, x.Gamepad.bLeftTrigger, x.Gamepad.bRightTrigger, x.Gamepad.sThumbLX, x.Gamepad.sThumbLY, x.Gamepad.sThumbRX, x.Gamepad.sThumbRY);
		}
#endif
	}

	if(fionaConf.idleFunc!=NULL) 
	{
		//app-specific idle function - generally implemented in main.cpp for an app
		fionaConf.idleFunc();
	}
}

void FionaUTCleanupGraphics(void)
{
	if(scene)
	{
		glFinish();
		scene->onExit();
		delete scene;
		scene = 0;
		_FionaUTCleanupRendering();

#ifdef ENABLE_OCULUS
#ifdef ENABLE_DK2
		if(fionaConf.appType == FionaConfig::OCULUS)
		{
			if (fionaConf.oculusHmd)
			{
				ovrHmd_Destroy(fionaConf.oculusHmd);
				fionaConf.oculusHmd = 0;
			}

			 ovr_Shutdown();
		}
#endif
#ifdef ENABLE_DK1
		if(fionaConf.appType == FionaConfig::OCULUS)
		{
			OVR::System::Destroy();
		}
#endif
#ifdef ENABLE_CV1
		
		if (fionaConf.mirrorFBO) glDeleteFramebuffers(1, &fionaConf.mirrorFBO);
		if (fionaConf.mirrorTexture) ovr_DestroyMirrorTexture(fionaConf.session, fionaConf.mirrorTexture);
		for (int eye = 0; eye < 2; ++eye)
		{
			delete fionaConf.eyeRenderTexture[eye];
			delete fionaConf.eyeDepthBuffer[eye];
		}
		ovr_Destroy(fionaConf.session);
#endif
#endif
#ifdef ENABLE_VIVE
		fionaConf.m_pHMD = NULL;
		vr::VR_Shutdown();
#endif
	}
}


void FionaUTExit(int errorCode)
{
	printf("Exiting Fiona...\n");
	if(fionaNetMaster)
	{
		printf("CLOSING... Sending end signal\n");
		_FionaUTSyncSendKeyboard(27, 0);
		_FionaUTSyncMasterSync();
	}

	FionaUTSleep(1.f);
	if(fionaNetMaster) delete fionaNetMaster; fionaNetMaster=0;
	if(fionaNetSlave ) delete fionaNetSlave;  fionaNetSlave =0;

	FionaUTVRPN::Close();

	if(!fionaConf.graphicsThread)
	{
		FionaUTCleanupGraphics();
	}

	if(fionaConf.cleanupFunc != 0)
	{
		fionaConf.cleanupFunc(errorCode);
	}

	__FionaUTCloseWindow();

	exit(errorCode);
}

int __FionaUTGetVideoMemorySizeBytes(bool total, GLuint memFlag)
{
	std::string glExtensionsString_ = std::string(reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS)));
	std::string glVendorString_  = std::string(reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
	int availableTexMem = -1;
	bool nVidia = true;

	if(glVendorString_.find("ATI") != std::string::npos)
	{
		nVidia = false;
	}

	if(total)
	{
		if (nVidia) {
			if(glExtensionsString_.find("GL_NVX_gpu_memory_info") != std::string::npos){
#ifndef GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX
#define GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX 0x9049
#endif
				GLint retVal;
				glGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &retVal);
				availableTexMem = static_cast<int>(retVal);
			} else {
				printf("No GL_NVX_gpu_memory_info support!!!");
			}
		}
		else {	//ATI
			if(glExtensionsString_.find("GL_ATI_meminfo") != std::string::npos) {
#ifndef GL_TEXTURE_FREE_MEMORY_ATI
#define GL_TEXTURE_FREE_MEMORY_ATI 0x87FC
#endif
				GLint retVal[4];
				glGetIntegerv(GL_TEXTURE_FREE_MEMORY_ATI, retVal);
				availableTexMem = static_cast<int>(retVal[0]); //0=total 1=availabel
			} else {
				printf("No GL_ATI_meminfo support");
			}
		}

		return availableTexMem;
	}

    if (nVidia) {
        if(glExtensionsString_.find("GL_NVX_gpu_memory_info") != std::string::npos){
#ifndef GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX
#define GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX 0x9047
#endif
            GLint retVal;
            glGetIntegerv(memFlag, &retVal);
			//GL_GPU_MEMORY_INFO_EVICTION_COUNT_NVX
			//GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX
			//GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX

            availableTexMem = static_cast<int>(retVal);
        } else {
            printf("No GL_NVX_gpu_memory_info support!!!");
        }
    }
    else {
        if(glExtensionsString_.find("GL_ATI_meminfo") != std::string::npos) {
#ifndef GL_TEXTURE_FREE_MEMORY_ATI
#define GL_TEXTURE_FREE_MEMORY_ATI 0x87FC
#endif
            GLint retVal[4];
            glGetIntegerv(GL_TEXTURE_FREE_MEMORY_ATI, retVal);
            availableTexMem = static_cast<int>(retVal[1]); //0=total 1=availabel
        } else {
            printf("No GL_ATI_meminfo support");
        }
    }

    return availableTexMem;
}

#ifdef WIN32
bool Fionais360ControllerPresent()
{
   XINPUT_STATE XBOX_CONTROLLER_State;
   //Invoke the memset(); function to zero the XBOX_CONTROLLER_State. 
   memset(&XBOX_CONTROLLER_State, 0, sizeof(XINPUT_STATE)); 
   DWORD result = XInputGetState(0, &XBOX_CONTROLLER_State); 
   return  result == ERROR_DEVICE_NOT_CONNECTED ? false : true;  
}

XINPUT_STATE FionaGetControllerState(void)
{
	XINPUT_STATE x;
	ZeroMemory(&x, sizeof(XINPUT_STATE));

	// Get the state
	XInputGetState(0, &x);

	return x;
}
#endif
