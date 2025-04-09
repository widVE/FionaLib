//
//  FionaUTEvent.cpp
//  FionaUT
//
//  Created by Hyun Joon Shin on 5/8/12.
//  Copyright 2012 __MyCompanyName__. All rights reserved.
//

#include "FionaUT.h"

static const float filterExponent=3;
static const float stepScale=0.25;

inline float _FionaUTMonitorDistance(const jvec3& ip, const quat& iq)
{
	float e1 = sqlen(fionaConf.monitorCamPos-ip);
	float e2 = sqlen(DIFF(fionaConf.monitorCamOri,iq));
	return MIN(1,e1*1.75f+e2*2.0f);
}

inline jvec3 _FionaUTFiltarP(const jvec3& ip, float err)
{
	jvec3 dist = ip-fionaConf.monitorCamPos;
	dist = dist*stepScale*err;
	return fionaConf.monitorCamPos+dist;
}

inline quat _FionaUTFiltarQ(const quat& iq, float err)
{
	jvec3 dist = DIFF(iq,fionaConf.monitorCamOri);
	dist = dist*stepScale*err;
	return exp(dist)*fionaConf.monitorCamOri;
}

void _FionaUTKeyboard	(WIN win, unsigned int k)
{
	int i = _FionaUTFindWindow(win); if(i<0) return; fionaCurWindow = i;
	if(fionaNetMaster)			_FionaUTSyncSendKeyboard(k, 0);
	
	if(k==27)					
	{	
		printf("ESC pressed...\n");
#ifdef LINUX_BUILD
		FionaUTExit();
#else
		fionaDone = true;//FionaUTExit();
#endif
	}

	//for debug
	//printf("key = %d\n", k);

	if (k == 96)
	{
		//toogle stereo
		
		fionaConf.splitStereo=!fionaConf.splitStereo;	
		fionaConf.fullscreen=0;
		printf("stereo = %d\n", fionaConf.splitStereo);
		
	}

	if(fionaWinConf[i].keyboardFunc)
		fionaWinConf[i].keyboardFunc((unsigned char)k,
								  fionaWinConf[i].lastx,fionaWinConf[i].lasty);
}

void _FionaUTMouseDown	(WIN win, int b, int x, int y)
{
	int i = _FionaUTFindWindow(win); if(i<0) return; fionaCurWindow = i;
	fionaWinConf[i].lastx = x, fionaWinConf[i].lasty=y;
	if(fionaWinConf[i].mouseFunc) fionaWinConf[i].mouseFunc(b,GLUT_DOWN,x,y);
}

void _FionaUTMouseUp	(WIN win, int b, int x, int y)
{
	int i = _FionaUTFindWindow(win); if(i<0) return; fionaCurWindow = i;
	fionaWinConf[i].lastx = x, fionaWinConf[i].lasty=y;
	if(fionaWinConf[i].mouseFunc) fionaWinConf[i].mouseFunc(b,GLUT_UP,x,y);
}

void _FionaUTMouseMove	(WIN win, int x, int y)
{
	int i = _FionaUTFindWindow(win); if(i<0) return; fionaCurWindow = i;
	fionaWinConf[i].lastx = x, fionaWinConf[i].lasty=y;
	if(fionaWinConf[i].passiveFunc) fionaWinConf[i].passiveFunc(x,y);
}

void _FionaUTMouseDrag	(WIN win, int x, int y)
{
	int i = _FionaUTFindWindow(win); if(i<0) return; fionaCurWindow = i;
	fionaWinConf[i].lastx = x, fionaWinConf[i].lasty=y;
	if(fionaWinConf[i].motionFunc) fionaWinConf[i].motionFunc(x,y);
}

void _FionaUTTracker	(int i, const jvec3& up, const quat& q)
{
	jvec3 p = up+fionaConf.trackerOffset;
	p = p+q.rot(fionaConf.sensorOffset);
	
	if( i==0 )
	{
		if( fionaConf.monitorFiltering )
		{
			float err = powf(_FionaUTMonitorDistance(p,q),filterExponent);
			fionaConf.monitorCamPos=_FionaUTFiltarP(p,err);
			fionaConf.monitorCamOri=_FionaUTFiltarQ(q,err);
		}
		else
		{
			if(!fionaConf.wandView)
			{
				fionaConf.monitorCamOri=q;
				fionaConf.monitorCamPos=p;
			}
		}
		fionaConf.headPos=p, fionaConf.headRot=q;
#ifdef LINUX_BUILD		
		if(fionaConf.noTracking)
		{
			fionaConf.headPos=jvec3(0.f, 0.f, 0.f);
			fionaConf.headRot=quat(1.f, 0.f, 0.f, 0.f);
		}
#endif

		if(fionaConf.noHeadTracking)
		{
			fionaConf.headPos=jvec3(0.f, 0.f, 0.f);
			fionaConf.headRot=quat(1.f, 0.f, 0.f, 0.f);
		}
	}
	else if(i == 2)
	{
		fionaConf.secondHeadPos = p;
		fionaConf.secondHeadRot = q;
		//printf("Second head tracker: %f, %f, %f\n", fionaConf.secondHeadPos.x, fionaConf.secondHeadPos.y, fionaConf.secondHeadPos.z);
	}

	if(fionaNetMaster)			_FionaUTSyncSendTracker(i,up,q);
	if(fionaConf.trackerFunc)	fionaConf.trackerFunc(i,p,q);
}

void _FionaUTWandButton	(int b, int s, int idx)
{
	if( s== 0 )					fionaConf.currentButtons&=~(0x1<<b);
	else						fionaConf.currentButtons|= (0x1<<b);
	if(fionaNetMaster)			_FionaUTSyncSendWandButton(b, s, idx);
	if(fionaConf.wandButtonFunc)fionaConf.wandButtonFunc(b,s, idx);
}

void _FionaUTJoystick	(int i, const vec4& p)
{
	fionaConf.currentJoystick = p;
	if(fionaNetMaster)			_FionaUTSyncSendJoystick(i,p);
	if(fionaConf.joystickFunc)	fionaConf.joystickFunc(i,p);
}


void _FionaUTJoypad(unsigned short wB, unsigned char bLT, unsigned char bRT, short sTLX, short sTLY, short sTRX, short sTRY)
{
	if(fionaNetMaster)
	{
		_FionaUTSyncSendController( wB, bLT, bRT, sTLX, sTLY, sTRX, sTRY);
	}

	if(fionaConf.controllerFunc)
	{
		fionaConf.controllerFunc( wB, bLT, bRT, sTLX, sTLY, sTRX, sTRY);
	}

	//have some defaults, but then tap into the action system eventually..
	//if(x.Gamepad.wButtons & XINPUT_GAMEPAD_A)
	//{
		//do something..
	//}
}

#define CURWIN() if(fionaCurWindow<0)return;fionaWinConf[fionaCurWindow]

void FionaUTDisplayFunc			(FIONA_DISPLAY_FUNC f)		{ CURWIN().displayFunc=f;}
void FionaUTKeyboardFunc		(FIONA_KEYBOARD_FUNC f)		{ CURWIN().keyboardFunc=f;}
void FionaUTReshapeFunc			(FIONA_RESHAPE_FUNC f)		{ CURWIN().reshapeFunc=f;}
void FionaUTMouseFunc			(FIONA_MOUSE_FUNC f)		{ CURWIN().mouseFunc=f;}
void FionaUTMotionFunc			(FIONA_MOTION_FUNC f)		{ CURWIN().motionFunc=f;}
void FionaUTPassiveMotionFunc	(FIONA_PASSIVE_FUNC f)		{ CURWIN().passiveFunc=f;}
void FionaUTFrameFunc			(FIONA_FRAME_FUNC f)		{ fionaConf.frameFunc = f;}
void FionaUTIdle				(FIONA_IDLE_FUNC f)			{ fionaConf.idleFunc = f;}
void FionaUTTrackerFunc			(FIONA_TRACKER_FUNC f)		{ fionaConf.trackerFunc = f;}
void FionaUTWandButtonFunc		(FIONA_WAND_BUTTON_FUNC f)	{ fionaConf.wandButtonFunc = f;}
void FionaUTJoystickFunc		(FIONA_JOYSTIK_FUNC f)		{ fionaConf.joystickFunc = f;}
void FionaUTCleanupFunc			(FIONA_CLEANUP_FUNC f)		{ fionaConf.cleanupFunc = f;}
void FionaUTPostRenderFunc		(FIONA_POST_RENDER_FUNC f)	{ fionaConf.postFunc = f;}
void FionaUTWallFunc			(FIONA_WALL_FUNC f)			{ fionaConf.wallFunc = f;}
void FionaUTControllerFunc		(FIONA_CONTROLLER_FUNC f)	{ fionaConf.controllerFunc = f;}
