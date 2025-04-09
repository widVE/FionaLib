//
//  FionaUTConfig.cpp
//  FionaUT
//
//  Created by Hyun Joon Shin on 5/17/12.
//  Updated and work continued by Ross Tredinnick 2012-2019
//  Living Environments Laboratory, Virtual Environments Groups - Wisconsin Institutes for Discovery
//  Copyright 2012 __MyCompanyName__. All rights reserved.
//

//#include "Oculus2/OVR/OVR.h"

#define FIONA_UT_IMPLEMENTATION
#include "FionaUT.h"
#include "FionaUTVRPN.h"
#include <queue>
#ifdef ENABLE_OCULUS
#ifdef ENABLE_DK2
#include "Oculus2/OVR/OVR.h"
#include "Oculus2/OVR/OVR_CAPI_GL.h"
#include "Oculus2/OVR/Kernel/OVR_System.h"
#endif
#ifdef ENABLE_DK1
#include "Oculus/OVR/OVR.h"
#endif
#ifdef ENABLE_CV1
#include "Oculus3/OVR/OVR_CAPI.h"
#include "Oculus3/OVR/OVR_CAPI_GL.h"
#include "Oculus3/OVR/Kernel/OVR_System.h"
#endif
#endif

//#include <sstream>
#include <fstream>

#ifdef LINUX_BUILD
#include <unistd.h>
#endif

#ifndef NULL
#define NULL 0x0
#endif

//#define WIDE_CAVE

const float CV_SZ_BIG = 3.47472;	//size of whole cave wall
const float CV_SZ=2.8956;	//size of whole cave wall
const float CV_W_BIG = 1.73736;
const float CV_W=1.4478;	//half size of cave wall
const float Q=1016.f/1920.f;	//unintuitive variable here - this is being used to calculate the height of a source viewport
//this coincidentally is also the aspect ratio of a screen in the CAVE or Dev Lab.
//but this value is actually used in the below FionaViewport definitions to define the height of a source viewport
//the logic here is that the full screen of a cave / dev lab wall is 1920x1920 pixels.  
//but there is an edge blend going on, so despite the 2 projectors per wall, they don't match to 50/50 exactly, instead they overlap
//the amount of overlap happens to correspond to the Q fraction defined above because the projectors are projecting 1016 pixels in height
//rather than 1920/2 = 960.  So the Q fraction corresponds to the starting points of the overlaps in terms of viewports.
//i.e. 0,0,1,0.52917 for bottom viewport
//and 0, 1-0.52917 (0.47083), 1, 0.52917 for top viewport

//first four values are "source", last 4 value are "desitination"
//destination = the viewport size relative to the window from 0->1
//i.e. so for the CAVE, the widths are all 0.25 since we split amongst 4 viewports on a single machine
//each destination x pos shifts by 0.25 also do to the split amongst 4 viewports on a single machine
//source = the viewport in terms of physical space - i.e. the full wall, also values from 0->1
//so for a single wall, we have two viewports amongst it, and due to edge blending it isn't 50/50 in terms of size
const FionaViewport VP_CAVE_PRIM_LO(0,  0,1,Q,    0,0,0.25f,1);
const FionaViewport VP_CAVE_PRIM_HI(0,1-Q,1,Q,0.25f,0,0.25f,1);
const FionaViewport VP_CAVE_SECN_LO(0,  0,1,Q, 0.5f,0,0.25f,1);
const FionaViewport VP_CAVE_SECN_HI(0,1-Q,1,Q,0.75f,0,0.25f,1);
const FionaViewport VP_NEWDL_PRIM_LO(0, 0, 0.5, 0.3333, 0, 0, 0.25f, 1);
const FionaViewport VP_NEWDL_PRIM_HI(0.5, 0, 0.5, 0.3333, 0.25f, 0, 0.25f, 1);
const FionaViewport VP_NEWDL_SECN_LO(0, 0.3333, 0.5, 0.3333, 0.5f, 0, 0.25f, 1);
const FionaViewport VP_NEWDL_SECN_HI(0.5, 0.3333, 0.5, 0.3333, 0.75f, 0, 0.25f, 1);
const FionaViewport VP_DEVL_LO(0,  0,1,Q,   0,0,0.5,1);
const FionaViewport VP_DEVL_HI(0,1-Q,1,Q, 0.5,0,0.5,1);

const FionaViewport VP_NEWDL_TOPLEFT(0, 0.6666, 0.5, 0.3333, 0, 0, 0.5, 1);
const FionaViewport VP_NEWDL_TOPRIGHT(0.5, 0.6666, 0.5, 0.3333, 0.5, 0, 0.5, 1);

const FionaViewport VP_SPLT_STEREO(0,0,1,1, 0,0,0.5,1, 0.5,0,0.5,1);
const FionaViewport VP_DESK(0,0,1,1,0,0,1,1);

//assuming 0,0,0 in CAVE is middle of the CAVE (not on floor)

//CAVE arrangement: assuming when facing the front wall in cave, left is negative x, right positive x, forward is negative z, backward is positive z, up is positive y, down is negative y

#ifdef WIDE_CAVE
const FionaWall WL_CV1(jvec3(-CV_W_BIG, -CV_W_BIG, -CV_W), jvec3(CV_SZ_BIG, 0, 0), jvec3(0, CV_SZ_BIG, 0));	//front wall
const FionaWall WL_CV2(jvec3(CV_W, -CV_W_BIG, -CV_W_BIG), jvec3(0, 0, CV_SZ_BIG), jvec3(0, CV_SZ_BIG, 0)); //right wall
const FionaWall WL_CV3(jvec3(CV_W_BIG, -CV_W_BIG, CV_W), jvec3(-CV_SZ_BIG, 0, 0), jvec3(0, CV_SZ_BIG, 0)); //door 
const FionaWall WL_CV4(jvec3(-CV_W, -CV_W_BIG, CV_W_BIG), jvec3(0, 0, -CV_SZ_BIG), jvec3(0, CV_SZ_BIG, 0)); //left wall
const FionaWall WL_CV5(jvec3(-CV_W_BIG, CV_W, CV_W_BIG), jvec3(0, 0, -CV_SZ_BIG), jvec3(CV_SZ_BIG, 0, 0)); //ceiling
const FionaWall WL_CV6(jvec3(CV_W_BIG, -CV_W, CV_W_BIG), jvec3(0, 0, -CV_SZ_BIG), jvec3(-CV_SZ_BIG, 0, 0)); //floor
const FionaWall WL_DEVL(jvec3(-CV_W_BIG, -CV_W_BIG, -CV_W), jvec3(CV_SZ_BIG, 0, 0), jvec3(0, CV_SZ_BIG, 0));
#else
const FionaWall WL_CV1 (jvec3(-CV_W,-CV_W,-CV_W),jvec3( CV_SZ,0,0),jvec3(0, CV_SZ,0));	//front wall
const FionaWall WL_CV2 (jvec3( CV_W,-CV_W,-CV_W),jvec3(0,0, CV_SZ),jvec3(0, CV_SZ,0)); //right wall
const FionaWall WL_CV3 (jvec3( CV_W,-CV_W, CV_W),jvec3(-CV_SZ,0,0),jvec3(0, CV_SZ,0)); //door 
const FionaWall WL_CV4 (jvec3(-CV_W,-CV_W, CV_W),jvec3(0,0,-CV_SZ),jvec3(0, CV_SZ,0)); //left wall
const FionaWall WL_CV5 (jvec3(-CV_W, CV_W, CV_W),jvec3(0,0,-CV_SZ),jvec3( CV_SZ,0,0)); //ceiling
const FionaWall WL_CV6 (jvec3( CV_W,-CV_W, CV_W),jvec3(0,0,-CV_SZ),jvec3(-CV_SZ,0,0)); //floor
const FionaWall WL_DEVL(jvec3(-CV_W, -CV_W, -CV_W), jvec3(CV_SZ, 0, 0), jvec3(0, CV_SZ, 0));
const FionaWall WL_NEW_DEVLAB(jvec3(-CV_W, -CV_W, -CV_W), jvec3(CV_SZ, 0, 0), jvec3(0, CV_SZ*.666666f, 0));	//currently an approximation.
const FionaWall WL_NEW_DEVLAB2(jvec3(-CV_W, 0.2413, -CV_W), jvec3(CV_SZ, 0, 0), jvec3(0, CV_SZ*.333333f, 0));	//currently an approximation.
#endif
//const FionaWall WL_DESK(jvec3(-1.2,-1.5,-1.5),jvec3(2.4,0,0),jvec3(0,3,0));
const FionaWall WL_DESK(jvec3(-1.5, -1.5, -1.5), jvec3(3, 0, 0), jvec3(0, 3, 0));
const FionaWall WL_VUZIX(jvec3(-0.68072,-0.51054,-3.048),jvec3(1.36144,0,0),jvec3(0,1.02108,0));
const FionaWall WL_OCULUS(jvec3(0,0,0),jvec3(0,0,0),jvec3(0,0,0));	//not actually used but just added to follow same path as split stereo

const int		DEFAULT_W=1280;
const int		DEFAULT_H=800;
const int		DEFAULT_X=20;
const int		DEFAULT_Y=5;

#ifdef ENABLE_DK2
const int		OCULUS_DEFAULT_WIDTH = 1920;
const int		OCULUS_DEFAULT_HEIGHT = 1080;
#endif
#ifdef ENABLE_DK1
const int		OCULUS_DEFAULT_WIDTH = 1280;
const int		OCULUS_DEFAULT_HEIGHT = 800;
#endif
#ifdef ENABLE_CV1
const int		OCULUS_DEFAULT_WIDTH = 2160;
const int		OCULUS_DEFAULT_HEIGHT = 1200;
#endif

const float		DEFAULT_DPI=0.0002479f;
const float		DEFAULT_FAR=0.6f;
//const jvec3	WIN_Sz(DEFAULT_W*DEFAULT_DPI,DEFAULT_H*DEFAULT_DPI,DEFAULT_FAR);
//const jvec3	DL_TO(-0.019f,-0.014f-1.4478f-0.1222f,-0.003f);
const jvec3		DL_TO(0, -0.014f - 1.4478f - 0.1222f, -0.008);
const jvec3		C6_TO(0.002, -1.4478f, -0.025);
//const jvec3	C6_TO(0.005,0.018-1.4478,0.038);
//const jvec3	C6_TO(0,-1.4478f,0);
//const jvec3	DEFAULT_SO(0,-0.0266f,0.041f);
const jvec3		DL_SO(0.003,0,0);
//const jvec3	C6_SO(0.005,0,-0.015);
const jvec3		C6_SO(0.0f,0.0f,0.0f);

#ifdef DSCVR
const jvec3		DEFAULT_KO(0,0,0.0);
const jvec3		DEFAULT_LO(-0.03,0,0);
const jvec3		DEFAULT_RO( 0.03,0,0);

#else
//Original Settings that appear to make more sense
const jvec3	DEFAULT_KO(0,-0.0266f,0.041f);
const jvec3	DEFAULT_LO(-0.033f,0,0);
const jvec3	DEFAULT_RO( 0.033f,0,0);

//Hyun Joon's Previous Settings
/*const jvec3		DEFAULT_KO(0,-0.01f,0.145f);
const jvec3		DEFAULT_LO(-0.0292f,.004,0);
const jvec3		DEFAULT_RO( 0.0521f,-.003,0);*/
#endif

const float		DEFAULT_MONITOR_STEP_BACK = 0.f;// 0.3f;
const float		DEFAULT_DESKTOP_FOV=60.0f;

const float		DEFAULT_FR=0.f;
const glm::vec3	DEFAULT_BK_COLOR(0,0,0);
const short		DEFAULT_PORT=7563;
const short		DEFAULT_MASTER_SLAVE_PORT=7568;
const short		DEFAULT_NUM_SLAVES=3;
const std::string DEFAULT_MASTER_IP("10.129.24.140");
const std::string DEFAULT_MASTER_SLAVE_IP("10.129.24.213");

const int		DEFAULT_FBOW=1920;
const int		DEFAULT_FBOH=1920;
const int		DEFAULT_MULTISAMPLE=0;

const float DEFAULT_FAR_CLIP = 50.f;
const float DEFAULT_NEAR_CLIP = 0.0025f;
const float DEFAULT_NAV_SPEED = 0.08f;
const float DEFAULT_ROT_SPEED = 0.018f;

#ifdef APPLE
const std::string DEFAULT_OGRE_MEDIA_PATH("/Users/joony/Dropbox/OgreTest/Resources/");
const std::string DEFAULT_VOREEN_ROOT("/Users/joony/Library/Frameworks/Voreen/");
const std::string DEFAULT_CONFIG_FILE="/Users/joony/Desktop/FionaConfig.txt";
#elif defined LINUX_BUILD
//note...we need to change this!!!!!!!
#ifndef DSCVR
const std::string DEFAULT_OGRE_MEDIA_PATH("/home/rtredinnick/dev/FionaOgre/SDK/linux/Samples/Media/");
#else
const std::string DEFAULT_OGRE_MEDIA_PATH("/mnt/dscvr/apps/FionaOgre/SDK/linux/install/share/OGRE/Media/");
#endif
const std::string DEFAULT_VOREEN_ROOT("/usr/local/etc/voreen/");
#ifndef DSCVR
const std::string DEFAULT_CONFIG_FILE="/home/rosstnick/code/FionaLib/FionaConfig.txt";
const std::string DEFAULT_CONFIG_PATH="/home/rosstnick/code/FionaLib/";
#else
const std::string DEFAULT_CONFIG_FILE="/mnt/dscvr/conf/Fiona/FionaConfig.txt";
const std::string DEFAULT_CONFIG_PATH="/mnt/dscvr/conf/Fiona/";
#endif

#else
const std::string DEFAULT_OGRE_MEDIA_PATH("..\\..\\Media\\");	//assuming SDK\\bin is the working directory...(which is true in all currently active setups)
const std::string DEFAULT_VOREEN_ROOT("..\\..\\include\\");//need the trailing slashes for > 4.0.1!

#ifdef ENABLE_OCULUS
//windows 7 desktop location is different than XP
const std::string DEFAULT_CONFIG_FILE="C:\\Users\\Admin2\\Desktop\\FionaConfig.txt";
//const std::string DEFAULT_CONFIG_FILE="C:\\Users\\rtredinnick\\Desktop\\FionaConfig.txt";
#else
const std::string DEFAULT_CONFIG_FILE="C:\\Users\\Admin2\\Desktop\\FionaConfig.txt";
//const std::string DEFAULT_CONFIG_FILE="C:\\Documents and settings\\Administrator\\Desktop\\FionaConfig.txt";
#endif
#endif	//end windows define (not apple or linux)

const std::string DEFAULT_TRACKER_SERVER("");
const std::string DEFAULT_TRACKER_NAME("Isense900");
const std::string DEFAULT_WAND_NAME("Wand0");
const std::string DEFAULT_WAND_NAME2("Wand1");

const jvec3 CAVE_CALIBX(1,0,0);
const jvec3 CAVE_CALIBY(0,1,0);
const jvec3 CAVE_CALIBZ(0,0,1);


FionaConfig fionaConf=	{
	FionaConfig::WINDOWED,
	false,				// Stereo
	DEFAULT_MULTISAMPLE,// multisample
	0,					// Fullscreen mode: -1 for fill all, 0 for windowed, >0 for designated monitor
	false,				// Split Stereo (cross eye or cheap HMD)
	false,				// slave mode
	false,				// master mode
	false,				// master is a slave for a secondary machine 
	jvec3(0,0,0),		// Tracker   offset
	jvec3(0,0,0),		// Sensor    offset
	DEFAULT_KO,			// Kevin Offset
	DEFAULT_LO,			// Left  eye offset
	DEFAULT_RO,			// Right eye offset
	DEFAULT_FR,			// framerate
	DEFAULT_BK_COLOR,	// Clear color
	NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL, // Callback Functions

	false,						// Monitor view: place camera at the tracker
	false,						// Monitor filtering
	DEFAULT_MONITOR_STEP_BACK,	// Monitor step back
	false,						// wand view: place camera at the wand..
	false,						// dont clear
	false,						// delta input...
	false,						// Desktop projection mode
	DEFAULT_DESKTOP_FOV,		// Monitor field of view
	std::string(""),			// playback file
	std::string(""),			// pathway file
	false,						// Use FBO
	true,						// Draw FBO
	DEFAULT_FBOW, DEFAULT_FBOH,	// FBO size
	
	DEFAULT_TRACKER_SERVER,		// Host Address
	DEFAULT_TRACKER_NAME,		// Tracker Name
	DEFAULT_WAND_NAME,			// Wand Name
	DEFAULT_WAND_NAME2,			// possible 2nd wand for dual view
	std::string(""),			// Input host
	
	DEFAULT_OGRE_MEDIA_PATH,	// Ogre Media base path
	DEFAULT_VOREEN_ROOT,		// Voreen root path
	
	// Runtime config.. Set for initial config
	jvec3(0,0,0),	// Initial camera position
	quat(1,0,0,0),	// Initial camera orientation
	jvec3(0,0,0),	// Initial second camera position
	quat(1,0,0,0),	// Initial second camera orientation
	jvec3(0,0,0),	// Initial monitor camera position
	quat(1,0,0,0),	// Initial monitor camera orientation
	jvec3(0,0,0),	// Initial head position
	quat(1,0,0,0),	// Initial head orientation
	jvec3(0,0,0),	// Initial second tracker head position
	quat(1,0,0,0),	// Initial second tracker head orientation
	jvec3(0,0,0),	// Initial wand tracker position
	quat(1,0,0,0),	// Initial wand tracker orientation
	jvec3(0,0,0),	// Initial second wand tracker position
	quat(1,0,0,0),	// Initial second wand tracker orientation
	0.f,
	0.f,
	jvec3(0,0,0),	// Current joystick state
	0,				// Current button state
	0.f,			// wii-fit top left
	0.f,			// wii-fit bottom left
	0.f,			// wii-fit bottom right
	0.f,			// wii-fit top right
	0,				// emg raw
	0,				// emg averaged
	0,				// emg binary
	0,				// emg threshold
	0.f,
	//sync config values
	DEFAULT_PORT,
	DEFAULT_MASTER_SLAVE_PORT,
	DEFAULT_NUM_SLAVES,
	DEFAULT_MASTER_IP,
	DEFAULT_MASTER_SLAVE_IP,
	{CAVE_CALIBX,CAVE_CALIBX,CAVE_CALIBX,CAVE_CALIBX,},
	{CAVE_CALIBY,CAVE_CALIBY,CAVE_CALIBY,CAVE_CALIBY,},
	{CAVE_CALIBZ,CAVE_CALIBZ,CAVE_CALIBZ,CAVE_CALIBZ,},
	"",
	DEFAULT_NAV_SPEED,	//default navigation speed
	DEFAULT_ROT_SPEED,	//default rotation speed
	DEFAULT_NEAR_CLIP, //default near clip
	DEFAULT_FAR_CLIP,		//default far clip
	false,	//oculus rift tracking rather than our tracking
	false,	//display graphics memory
	false,	//dual view
	false,	//show render time
	true,	//render on the head node
	0,		//time varying file index...
	0,		//time varying buffer index...
	false,	//separate thread for rendering..
	false,	//noTracking
	false,
	false,	//multi-gpu
	false,	//use second viewer nodes for single viewer...
	false,	//use second GPU for rendering...
	false,	//render to two windows of 3840 (testing for multi-gpu rendering)
	false,
	0,		//frame buffer object
	0,		//frame buffer object color texture
	0,		//frame buffer object depth texture
	false,	//whether we force the oculus FBO to match the default oculus resoltion (i.e. no oculus downsampling from a higher FBO resolution)
	false,	//whether we force FBO size to match window size
	false,	//whether we use layered framebuffer stereo..
	false,	//borderless window
	false,	//single pass stereo
	1.f	//oculus res multiplier
#ifdef ENABLE_OCULUS
#ifdef ENABLE_DK1
	,0
#endif
#ifdef ENABLE_CV1
	,0,0, 0,
	0, 0,
	0,
	0,
	0.0
#endif
#endif
#ifdef ENABLE_VIVE
	,0
#endif
};

const FionaWindowConfig emptyConfig={
	FionaWindowConfig::NORMAL,	// Wall Type
	NULL,						// Window Handle
	DEFAULT_W, DEFAULT_H,		// Default window size
	DEFAULT_X, DEFAULT_Y,		// Default position
	0,0,0,0,0,0,0,0,			// Per window callbacks
};

unsigned int		fionaGlobalMode =0;
FionaWindowConfig	fionaWinConf[MAX_WINDOW];
int					fionaNextWindow =0, fionaCurWindow=-1;
int					fionaCurWall=0, fionaCurViewport=0;
WIN					fionaActiveWindow =FIONA_INVALID_WIN;
CTX					fionaActiveContext=FIONA_INVALID_CTX;
jvec3				fionaCurEyePos;

bool fionaDone = false;
bool fionaRenderDone = false;

std::queue<std::string> fionaCommandQueue;
// **********************************************
//
//        INTERNAL UTILITY FUNCTIONS
//
// **********************************************

std::string getst(char* argv[], int& i, int argc)
{
	i++;
	std::string ret;
	ret+=argv[i];
	if( argv[i][0]!='\"' ) { i++; return ret; }
	while(i<argc)
	{
		ret+=" ";
		ret+=argv[i];
		if( argv[i][strlen(argv[i])-1]=='\"' ) break;
		i++;
	}
	if( ret[ret.length()-1] == '\"' ) ret = ret.substr(1,ret.length()-2);
	i++;
	return ret;
}


#define IS_WHITE(X) ((X)=='\n'||(X)==' '||(X)=='\t')
#define GETCH(X,Y,Z) {X>>Y; if((X).eof()) return Z;}
std::string getst(std::istream& is)
{
	is >> std::noskipws;
	std::string buf;
	char ch = 0;
EAT_UP_AND_READ_AGAIN:
	while(1){ GETCH(is,ch,buf); if(!IS_WHITE(ch)) break; }
	if(ch=='\"') // Quoted
		while(1) { GETCH(is,ch,buf); if(ch=='\"') return buf; buf+=ch; }
	else if(ch=='/') // Maybe comment?
	{
		is>>ch;
		if( ch=='/' ) // Comment
			while(1) { GETCH(is,ch,buf); if(ch=='\n') goto EAT_UP_AND_READ_AGAIN; }
		else // Single slash
		{
			buf+="/"; buf+=ch;
			while(1) { GETCH(is,ch,buf); if(IS_WHITE(ch)) return buf; buf+=ch; }
		}
	}
	else // normal word
	{
		buf+=ch;
		while(1) { GETCH(is,ch,buf); if(IS_WHITE(ch)) return buf; buf+=ch; }
	}
	return buf;
}

float getf(std::istream& is)
{
	float v;
	sscanf(getst(is).c_str(),"%f",&v);
	return v;
}
jvec3 getv3(std::istream& is)
{
	float x=getf(is), y=getf(is), z=getf(is);
	return jvec3(x,y,z);
}

glm::vec3 getvg3(std::istream& is)
{
	float x=getf(is), y=getf(is), z=getf(is);
	return glm::vec3(x,y,z);
}

bool cmp(const std::string& _a, const std::string& _b)
{
//	printf("comparing %s %s\n",_a.c_str(),_b.c_str());
	std::string a; if( _a[0]=='-' && _a[1]=='-' ) a=_a.substr(2,_a.length()-2); else a=_a;
	std::string b; if( _b[0]=='-' && _b[1]=='-' ) b=_b.substr(2,_b.length()-2); else b=_b;
	if( a.length()!=b.length() ) return 0;
	size_t len = a.length();
	for(size_t i=0; i<len; i++) if(toupper(a[i])!=toupper(b[i])) return 0;
	return 1;
}

void _FionaUTSyncProjectorCalib(FionaWindowConfig& conf)
{
	int cnt=0;
	for(int i=0; i<(int)conf.walls.size(); i++)
	{
		if( cnt>=4 ) break;
		for(int j=0; j<(int)conf.walls[i].viewports.size(); j++)
		{
			if( cnt>=4 ) break;
			FionaViewport& vp = conf.walls[i].viewports[j];
			vp.cOffsetX = fionaConf.projectorCalibx[cnt].z;
			vp.cOffsetY = fionaConf.projectorCaliby[cnt].z;
			vp.cScaleX = fionaConf.projectorCalibx[cnt].x;
			vp.cScaleY = fionaConf.projectorCaliby[cnt].y;
			cnt++;
		}
	}
}

// Fiona Application & Window configuration related functions
void _FionaUTSetAppType(FionaConfig::APP_TYPE type)
{
	if( fionaConf.appType==type) return;
	fionaConf.appType = type;
	switch(fionaConf.appType)
	{
		case FionaConfig::DEVLAB:
		case FionaConfig::DEVLAB_WIN8:
			fionaConf.stereo=true;
			fionaConf.trackerOffset=DL_TO;
			fionaConf.sensorOffset=DL_SO;
			fionaConf.trackerServer="localhost";
			fionaConf.slave=false;
			fionaConf.master=false;
			fionaConf.framerate=1000;
			fionaConf.fullscreen=1;
			break;
		case FionaConfig::CAVE1_WIN8:
		case FionaConfig::CAVE2_WIN8:
		case FionaConfig::CAVE3_WIN8:
		case FionaConfig::CAVE4_WIN8:
		case FionaConfig::CAVE5_WIN8:
		case FionaConfig::CAVE6_WIN8:
			fionaConf.stereo=true;
			fionaConf.trackerOffset=C6_TO;
			fionaConf.sensorOffset=C6_SO;
			fionaConf.master=false; fionaConf.slave=true;
			fionaConf.framerate=1000;
			fionaConf.fullscreen=1;
			break;
		case FionaConfig::CAVE1_SS:
		case FionaConfig::CAVE2_SS:
		case FionaConfig::CAVE3_SS:
		case FionaConfig::CAVE4_SS:
		case FionaConfig::CAVE5_SS:
		case FionaConfig::CAVE6_SS:
			fionaConf.stereo = false;
			fionaConf.splitStereo = true;
			fionaConf.trackerOffset = C6_TO;
			fionaConf.sensorOffset = C6_SO;
			fionaConf.master = false; fionaConf.slave = true;
			fionaConf.framerate = 1000;
			fionaConf.fullscreen = -1;
			fionaWinConf[0].winx = 0;
			fionaWinConf[0].winy = 0;
			fionaWinConf[0].winw = 3840;
			fionaWinConf[0].winh = 1920;
			break;
		case FionaConfig::CAVE1_DUALPIPE:
		case FionaConfig::CAVE2_DUALPIPE:
		case FionaConfig::CAVE3_DUALPIPE:
		case FionaConfig::CAVE4_DUALPIPE:
		case FionaConfig::CAVE5_DUALPIPE:
		case FionaConfig::CAVE6_DUALPIPE:
			fionaConf.stereo = false;
			if (!fionaConf.twoWindows)
			{
				//fionaConf.splitStereo = true;
			}
			fionaConf.trackerOffset = C6_TO;
			fionaConf.sensorOffset = C6_SO;
			fionaConf.master = false; fionaConf.slave = true;
			fionaConf.framerate = 1000;
			fionaConf.fullscreen = -1;
			if (fionaConf.twoWindows)
			{
				fionaWinConf[0].winx = 0;
				fionaWinConf[0].winy = 0;
				fionaWinConf[0].winw = 1920;// 3840;
				fionaWinConf[0].winh = 1920;
				fionaWinConf[1].winx = 1920;
				fionaWinConf[1].winy = 0;
				fionaWinConf[1].winw = 1920;
				fionaWinConf[1].winh = 1920;
			}
			/*else
			{
				fionaWinConf[0].winx = 0;
				fionaWinConf[0].winy = 0;
				fionaWinConf[0].winw = 3840;
				fionaWinConf[0].winh = 1920;
			}*/
			break;
		case FionaConfig::DEVLAB2:
			fionaConf.stereo=true;
			fionaConf.trackerOffset=DL_TO;
			fionaConf.sensorOffset=DL_SO;
			fionaConf.slave=true;
			fionaConf.master=false;
			fionaConf.fullscreen=1;
			break;
		case FionaConfig::CAVE1:
		case FionaConfig::CAVE2:
		case FionaConfig::CAVE3:
			fionaConf.stereo=true;
			fionaConf.trackerOffset=C6_TO;
			fionaConf.sensorOffset=C6_SO;
			fionaConf.master=false; fionaConf.slave=true;
			fionaConf.fullscreen=-1;
			fionaWinConf[0].winx=0;
			fionaWinConf[0].winy=0;
			if(fionaConf.useSecondViewerNodes || fionaConf.twoWindows)
			{
				fionaWinConf[0].winw=1920*2;
				if(fionaConf.twoWindows)
				{
					fionaWinConf[1].winx = 3840;
					fionaWinConf[1].winy = 0;
					fionaWinConf[1].winw = 1920*2;
					fionaWinConf[1].winh = 1016;
				}
			}
			else
			{
				fionaWinConf[0].winw=1920*4;
			}
			fionaWinConf[0].winh=1016;
			break;
		case FionaConfig::CAVE4:
		case FionaConfig::CAVE5:
		case FionaConfig::CAVE6:
			fionaConf.stereo=true;
			fionaConf.trackerOffset=C6_TO;
			fionaConf.sensorOffset=C6_SO;
			fionaConf.master=false; fionaConf.slave=true;
			fionaConf.fullscreen=-1;
			fionaWinConf[0].winy=0;
			if(fionaConf.useSecondViewerNodes)
			{
				fionaWinConf[0].winx=3840;
				fionaWinConf[0].winw=1920*2;
			}
			else
			{
				fionaWinConf[0].winx=0;
				fionaWinConf[0].winw=1920*4;
			}
			fionaWinConf[0].winh=1016;
			break;
		case FionaConfig::HEADNODE:
			fionaConf.stereo=false;
			fionaConf.framerate=1000;
			fionaConf.trackerOffset=C6_TO;
			fionaConf.sensorOffset=C6_SO;
			fionaConf.trackerServer="localhost";
			fionaConf.fullscreen=0;
			fionaConf.desktopProjection=true;
			fionaConf.slave=false; fionaConf.master=true;
			break;
		case FionaConfig::VUZIX:
			fionaConf.stereo=false;
			fionaConf.splitStereo=true;
			fionaConf.fullscreen=2;
			fionaConf.slave=fionaConf.master=false;
			break;
		case FionaConfig::TEST_STEREO:
			fionaConf.stereo=true;
			fionaConf.fullscreen=0;
			fionaConf.trackerOffset=C6_TO;
			fionaConf.sensorOffset=C6_SO;
			fionaConf.slave=false;fionaConf.master=false;
			break;
		case FionaConfig::SLAVE:
			fionaConf.stereo=false;
			fionaConf.fullscreen=0;
			fionaConf.desktopProjection=true;	//this is the case where a machine is a slave (but not a CAVE node)
			fionaConf.slave=true; fionaConf.master=false;
			break;
		case FionaConfig::OCULUS:
			fionaConf.stereo=false;
			fionaConf.splitStereo=true;
			fionaConf.fullscreen=2;
			//fionaConf.trackerOffset=DL_TO;
			//fionaConf.sensorOffset=DL_SO;
			fionaConf.slave=false; 
			fionaConf.master=false;
			break;
		case FionaConfig::VIVE:
			fionaConf.stereo = false;
			fionaConf.splitStereo = true;
			fionaConf.fullscreen = 2;
			fionaConf.slave = false;
			fionaConf.master = false;
			break;
		case FionaConfig::HDTV:
			fionaConf.stereo=false;
			fionaConf.splitStereo=true;
			fionaConf.fullscreen=2;
			fionaConf.slave=false; fionaConf.master=false;
			break;
		case FionaConfig::UNITY_DL:
			//fionaConf.stereo=false;
			fionaConf.stereo=true;
			fionaConf.trackerOffset=DL_TO;
			fionaConf.sensorOffset=DL_SO;
			fionaConf.master = true;	//unity is the slave
			fionaConf.numSlaves = 1;
			fionaConf.desktopProjection=false;
			break;
		case FionaConfig::UNITY_CAVE:
			fionaConf.stereo=true;
			fionaConf.trackerOffset=C6_TO;
			fionaConf.sensorOffset=C6_SO;
			fionaConf.master = true;	//unity is the slave
			fionaConf.numSlaves = 7;//4;	//7 slaves for windows 8.1
			fionaConf.desktopProjection=false;
			break;
		case FionaConfig::UNITY_DSCVR:
			fionaConf.stereo=false;
			fionaConf.splitStereo=true;
			fionaConf.trackerOffset=jvec3(0.f, 0.f, 0.f);
			fionaConf.sensorOffset=jvec3(0.f, 0.f, 0.f);
			//fionaConf.trackerServer = "144.92.182.143";
			//fionaConf.inputHost = "144.92.182.128";
			fionaConf.master = true;	//unity is the slave
			fionaConf.numSlaves = 11;
			fionaConf.desktopProjection=false;
			break;
		case FionaConfig::DEVLAB_DUALPIPE:
			fionaConf.stereo = false;
			fionaConf.trackerOffset = DL_TO;
			fionaConf.sensorOffset = DL_SO;
			fionaConf.kevinOffset = fionaConf.kevinOffset + fionaConf.lEyeOffset;
			fionaConf.master = true;
			fionaConf.numSlaves = 1;
			fionaConf.trackerServer = "localhost";
			fionaConf.fullscreen = 1;
			//fionaConf.desktopProjection = false;
			break;
		case FionaConfig::DEVLAB_DUALPIPE_SLAVE:
			fionaConf.stereo = false;
			fionaConf.trackerOffset = DL_TO;
			fionaConf.sensorOffset = DL_SO;
			fionaConf.kevinOffset = fionaConf.kevinOffset + fionaConf.rEyeOffset;
			fionaConf.master = false;
			fionaConf.slave = true;
			fionaConf.fullscreen = 1;
			//fionaConf.desktopProjection = false;
			//specify ip address for connecting in the config file...
			break;
		case FionaConfig::DEVLAB_DUALPIPE_ONEMACHINE:
			fionaConf.stereo = false;
			fionaConf.trackerOffset = DL_TO;
			fionaConf.sensorOffset = DL_SO;
			fionaConf.master = false;
			fionaConf.slave = false;
			fionaConf.trackerServer = "localhost";
			fionaConf.fullscreen = 1;
			fionaConf.desktopProjection = false;
			break;
		case FionaConfig::DEVLAB_DUALVIEW_DUALPIPE:
			fionaConf.stereo = true;
			fionaConf.trackerOffset = DL_TO;
			fionaConf.sensorOffset = DL_SO;
			fionaConf.master = true;
			fionaConf.numSlaves = 1;
			fionaConf.trackerServer = "localhost";
			fionaConf.fullscreen = 1;
			//fionaConf.desktopProjection = false;
			break;
		case FionaConfig::DEVLAB_DUALVIEW_DUALPIPE_SLAVE:
			fionaConf.stereo = true;
			fionaConf.trackerOffset = DL_TO;
			fionaConf.sensorOffset = DL_SO;
			fionaConf.master = false;
			fionaConf.slave = true;
			fionaConf.fullscreen = 1;
			//fionaConf.desktopProjection = false;
			//specify ip address for connecting in the config file...
			break;
		case FionaConfig::NEW_DEVLAB:
			fionaConf.stereo = true;
			fionaConf.trackerOffset = DL_TO;
			fionaConf.sensorOffset = DL_SO;
			fionaConf.master = true;
			fionaConf.numSlaves = 1;
			fionaConf.trackerServer = "localhost";
			fionaConf.fullscreen = -1;
			fionaWinConf[0].winy = 0;
			fionaWinConf[0].winx = 0;
			fionaWinConf[0].winw = 1280 * 4;
			fionaWinConf[0].winh = 720;
			break;
		case FionaConfig::NEW_DEVLAB2:
			fionaConf.stereo = true;
			fionaConf.trackerOffset = DL_TO;
			fionaConf.sensorOffset = DL_SO;
			fionaConf.master = false;
			fionaConf.slave = true;
			fionaConf.fullscreen = -1;
			fionaWinConf[0].winy = 0;
			fionaWinConf[0].winx = 0;
			fionaWinConf[0].winw = 1280 * 2;
			fionaWinConf[0].winh = 720;
			break;
		case FionaConfig::CAVE_NEW_CEILING:
		case FionaConfig::CAVE_NEW_FLOOR:
		case FionaConfig::CAVE_NEW_LEFT:
		case FionaConfig::CAVE_NEW_RIGHT:
		case FionaConfig::CAVE_NEW_FRONT:
		case FionaConfig::CAVE_NEW_DOOR:
			fionaConf.stereo = true;
			fionaConf.trackerOffset = C6_TO;
			fionaConf.sensorOffset = C6_SO;
			fionaConf.master = false; fionaConf.slave = true;
			fionaConf.framerate = 1000;
			fionaConf.fullscreen = 1;
			fionaWinConf[0].winx = 0;
			fionaWinConf[0].winy = 0;
			fionaWinConf[0].winw = 1920;
			fionaWinConf[0].winh = 1920;
			fionaConf.desktopProjection = false;
			break;
		case FionaConfig::WINDOWED:
		default:
			fionaConf.stereo=false;
			fionaConf.fullscreen=0;
			fionaConf.desktopProjection=true;
			fionaConf.slave=fionaConf.master=false;
			fionaConf.framerate=1000;
			break;
	}
}

void _FionaUTSetAppType(const std::string& str)
{
	printf("APP: %s\n",str.c_str());
		 if( cmp(str,"WINDOWED") ) _FionaUTSetAppType(FionaConfig::WINDOWED);
	else if( cmp(str,"HEADNODE") ) _FionaUTSetAppType(FionaConfig::HEADNODE);
	else if( cmp(str,"DEVLAB"  ) ) _FionaUTSetAppType(FionaConfig::DEVLAB);
	else if( cmp(str,"DEVLAB2"  ) ) _FionaUTSetAppType(FionaConfig::DEVLAB2);
	else if( cmp(str,"CAVE1"   ) ) _FionaUTSetAppType(FionaConfig::CAVE1);
	else if( cmp(str,"CAVE2"   ) ) _FionaUTSetAppType(FionaConfig::CAVE2);
	else if( cmp(str,"CAVE3"   ) ) _FionaUTSetAppType(FionaConfig::CAVE3);
	else if( cmp(str,"CAVE4"   ) ) _FionaUTSetAppType(FionaConfig::CAVE4);
	else if( cmp(str,"CAVE5"   ) ) _FionaUTSetAppType(FionaConfig::CAVE5);
	else if( cmp(str,"CAVE6"   ) ) _FionaUTSetAppType(FionaConfig::CAVE6);
	else if( cmp(str,"SLAVE"   ) ) _FionaUTSetAppType(FionaConfig::SLAVE);
	else if( cmp(str,"VUZIX"   ) ) _FionaUTSetAppType(FionaConfig::VUZIX);
	else if( cmp(str,"OCULUS"  ) ) _FionaUTSetAppType(FionaConfig::OCULUS);
	else if( cmp(str, "TESTSTEREO" ) ) _FionaUTSetAppType(FionaConfig::TEST_STEREO);
	else if( cmp(str,"HDTV"  ) ) _FionaUTSetAppType(FionaConfig::HDTV);
	else if( cmp(str, "UNITY_DL" )) _FionaUTSetAppType(FionaConfig::UNITY_DL);
	else if( cmp(str, "UNITY_CAVE" )) _FionaUTSetAppType(FionaConfig::UNITY_CAVE);
	else if( cmp(str,"UNITY_DSCVR" )) _FionaUTSetAppType(FionaConfig::UNITY_DSCVR);
	else if( cmp(str, "DEVLAB_WIN8" ) ) _FionaUTSetAppType(FionaConfig::DEVLAB_WIN8);
	else if( cmp(str, "CAVE1_WIN8" ) ) _FionaUTSetAppType(FionaConfig::CAVE1_WIN8);
	else if( cmp(str, "CAVE2_WIN8" ) ) _FionaUTSetAppType(FionaConfig::CAVE2_WIN8);
	else if( cmp(str, "CAVE3_WIN8" ) ) _FionaUTSetAppType(FionaConfig::CAVE3_WIN8);
	else if( cmp(str, "CAVE4_WIN8" ) ) _FionaUTSetAppType(FionaConfig::CAVE4_WIN8);
	else if( cmp(str, "CAVE5_WIN8" ) ) _FionaUTSetAppType(FionaConfig::CAVE5_WIN8);
	else if( cmp(str, "CAVE6_WIN8" ) ) _FionaUTSetAppType(FionaConfig::CAVE6_WIN8);
	else if (cmp(str, "CAVE1_DUALPIPE")) _FionaUTSetAppType(FionaConfig::CAVE1_DUALPIPE);
	else if (cmp(str, "CAVE2_DUALPIPE")) _FionaUTSetAppType(FionaConfig::CAVE2_DUALPIPE);
	else if (cmp(str, "CAVE3_DUALPIPE")) _FionaUTSetAppType(FionaConfig::CAVE3_DUALPIPE);
	else if (cmp(str, "CAVE4_DUALPIPE")) _FionaUTSetAppType(FionaConfig::CAVE4_DUALPIPE);
	else if (cmp(str, "CAVE5_DUALPIPE")) _FionaUTSetAppType(FionaConfig::CAVE5_DUALPIPE);
	else if (cmp(str, "CAVE6_DUALPIPE")) _FionaUTSetAppType(FionaConfig::CAVE6_DUALPIPE);
	else if (cmp(str, "CAVE1_SS")) _FionaUTSetAppType(FionaConfig::CAVE1_SS);
	else if (cmp(str, "CAVE2_SS")) _FionaUTSetAppType(FionaConfig::CAVE2_SS);
	else if (cmp(str, "CAVE3_SS")) _FionaUTSetAppType(FionaConfig::CAVE3_SS);
	else if (cmp(str, "CAVE4_SS")) _FionaUTSetAppType(FionaConfig::CAVE4_SS);
	else if (cmp(str, "CAVE5_SS")) _FionaUTSetAppType(FionaConfig::CAVE5_SS);
	else if (cmp(str, "CAVE6_SS")) _FionaUTSetAppType(FionaConfig::CAVE6_SS);
	else if (cmp(str, "VIVE")) _FionaUTSetAppType(FionaConfig::VIVE);
	else if (cmp(str, "DEVLAB_DUALPIPE")) _FionaUTSetAppType(FionaConfig::DEVLAB_DUALPIPE);
	else if (cmp(str, "DEVLAB_DUALPIPE_SLAVE")) _FionaUTSetAppType(FionaConfig::DEVLAB_DUALPIPE_SLAVE);
	else if (cmp(str, "DEVLAB_DUALVIEW_DUALPIPE")) _FionaUTSetAppType(FionaConfig::DEVLAB_DUALVIEW_DUALPIPE);
	else if (cmp(str, "DEVLAB_DUALVIEW_DUALPIPE_SLAVE")) _FionaUTSetAppType(FionaConfig::DEVLAB_DUALVIEW_DUALPIPE_SLAVE);
	else if (cmp(str, "NEW_DEVLAB")) _FionaUTSetAppType(FionaConfig::NEW_DEVLAB);
	else if (cmp(str, "NEW_DEVLAB2")) _FionaUTSetAppType(FionaConfig::NEW_DEVLAB2);
	else if (cmp(str, "CAVE_NEW_CEILING")) _FionaUTSetAppType(FionaConfig::CAVE_NEW_CEILING);
	else if (cmp(str, "CAVE_NEW_FLOOR")) _FionaUTSetAppType(FionaConfig::CAVE_NEW_FLOOR);
	else if (cmp(str, "CAVE_NEW_DOOR")) _FionaUTSetAppType(FionaConfig::CAVE_NEW_DOOR);
	else if (cmp(str, "CAVE_NEW_RIGHT")) _FionaUTSetAppType(FionaConfig::CAVE_NEW_RIGHT);
	else if (cmp(str, "CAVE_NEW_LEFT")) _FionaUTSetAppType(FionaConfig::CAVE_NEW_LEFT);
	else if (cmp(str, "CAVE_NEW_FRONT")) _FionaUTSetAppType(FionaConfig::CAVE_NEW_FRONT);
}

bool _FionaUTCreateWallsFromStream(std::istream& is,int c)
{
	int curWall=-1;
	std::string sbuf;
	std::vector<FionaWall>& walls = fionaWinConf[c].walls;
	while( (sbuf=getst(is)).length()>0 )
	{
		if( is.eof() ) break;
		else if(cmp(sbuf,"wall"))
		{
			jvec3 p=getv3(is), x=getv3(is), y=getv3(is);
			printf("created %d walls\n", walls.size());
			printf("at %f %f %f : %f %f %f : %f %f %f \n", p.x, p.y, p.z, x.x, x.y, x.z, y.x, y.y, y.z);
			walls.push_back(FionaWall(p,x,y)); curWall=(int)walls.size()-1;
		}
		else if(cmp(sbuf,"viewport"))
		{
			printf("create viewport for wall %d\n", curWall);
			if( curWall<0 ) continue;
			float sx=getf(is), sy=getf(is), sw=getf(is), sh=getf(is);
			float lx=getf(is);
			if( lx<0 )
			{
				walls[curWall].viewports.push_back(FionaViewport(sx,sy,sw,sh));
				continue;
			}
			float ly=getf(is), lw=getf(is), lh=getf(is), rx=getf(is);
			if( rx<0 )
			{
				walls[curWall].viewports.push_back(FionaViewport(sx,sy,sw,sh,lx,ly,lw,lh));
				continue;
			}
			float ry=getf(is), rw=getf(is), rh=getf(is);
			walls[curWall].viewports.push_back(FionaViewport(sx,sy,sw,sh,lx,ly,lw,lh,rx,ry,rw,rh));
		}
		if( is.eof() ) break;
	}
	return curWall>=0;
}

bool _FionaUTCreateWallsFromFile(const std::string& filename,int c)
{
	std::ifstream ifs(filename.c_str());
	if( !ifs.is_open()) return 0;
	return _FionaUTCreateWallsFromStream(ifs,c);
}

static void _FionaUTProcessOptions(std::istream& is, const std::string& fn)
{
	std::string sbuf;
	std::string sAppType;

	while( (sbuf=getst(is)).length()>0 )
	{
		if( is.eof() ) break;
		else if(cmp(sbuf,"app"))			_FionaUTSetAppType(getst(is));
		else if(cmp(sbuf,"stereo"))			fionaConf.stereo=true,fionaConf.splitStereo=false;
		else if(cmp(sbuf,"splitstereo"))	fionaConf.stereo=false,fionaConf.splitStereo=true;
		else if(cmp(sbuf,"layeredStereo")) fionaConf.stereo=true,fionaConf.layeredStereo=true;
		else if(cmp(sbuf,"mono"  ))			fionaConf.stereo=fionaConf.splitStereo=false;
		else if(cmp(sbuf,"standAlone"  ))	fionaConf.master=fionaConf.slave=false;
		else if(cmp(sbuf,"master"  ))		fionaConf.master=true, fionaConf.slave=false;
		else if(cmp(sbuf,"slave"  ))		fionaConf.slave=true, fionaConf.master=false;
		else if(cmp(sbuf,"multisample"))	fionaConf.multisample=(short)getf(is);
		else if(cmp(sbuf,"fullscreen"))	
		{
			fionaConf.fullscreen=(short)getf(is);
			if(fionaConf.fullscreen==0)
				fionaWinConf[0].winx=DEFAULT_X,
				fionaWinConf[0].winy=DEFAULT_Y,
				fionaWinConf[0].winw=DEFAULT_W,
				fionaWinConf[0].winh=DEFAULT_H;
		}
		else if(cmp(sbuf,"windowX"))		fionaWinConf[0].winx=getf(is);
		else if(cmp(sbuf,"windowY"))		fionaWinConf[0].winy=getf(is);
		else if(cmp(sbuf,"windowW"))		fionaWinConf[0].winw=getf(is);
		else if(cmp(sbuf,"windowH"))		fionaWinConf[0].winh=getf(is);

		else if(cmp(sbuf,"trackerOffset"))	fionaConf.trackerOffset=getv3(is);
		else if(cmp(sbuf,"sensorOffset"))	fionaConf.sensorOffset=getv3(is);
		else if(cmp(sbuf,"kevinOffset"))	fionaConf.kevinOffset=getv3(is);
		else if(cmp(sbuf,"leftOffset"))		fionaConf.lEyeOffset=getv3(is);
		else if(cmp(sbuf,"rightOffset"))	fionaConf.rEyeOffset=getv3(is);
		else if(cmp(sbuf,"framerate"))		fionaConf.framerate=getf(is);
		else if(cmp(sbuf,"bkColor"))		fionaConf.backgroundColor=getvg3(is);
		else if(cmp(sbuf,"dontClear"))		fionaConf.dontClear = true;
		else if(cmp(sbuf,"monitorView"))	fionaConf.monitorView=true;
		else if(cmp(sbuf,"monitorSmooth"))	fionaConf.monitorFiltering = true;
		else if(cmp(sbuf,"stepBack"))		fionaConf.monitorStepBack=getf(is);
		else if(cmp(sbuf,"desktopProjection"))	fionaConf.desktopProjection = true;
		else if(cmp(sbuf,"vrProjection"))	fionaConf.desktopProjection = false;
		else if(cmp(sbuf,"desktopFOV"))		fionaConf.desktopFOV=getf(is);
		else if(cmp(sbuf,"useFBO"))			fionaConf.useFBO=true;
		else if(cmp(sbuf,"hideFBO"))		fionaConf.drawFBO=false;
		else if(cmp(sbuf,"noFBO"))			fionaConf.useFBO=false;
		else if(cmp(sbuf,"FBOW"))			fionaConf.FBOWidth=(int)getf(is);
		else if(cmp(sbuf,"FBOH"))			fionaConf.FBOHeight=(int)getf(is);
		else if(cmp(sbuf,"ogreMedia"))		fionaConf.OgreMediaBasePath=getst(is);
		else if(cmp(sbuf,"voreenRoot"))		fionaConf.VoreenRoot=getst(is);
		else if(cmp(sbuf,"trackerServer"))	fionaConf.trackerServer=getst(is);
		else if(cmp(sbuf,"trackerName"))	fionaConf.trackerName=getst(is);
		else if(cmp(sbuf,"wandName"))		fionaConf.wandName=getst(is);
		else if(cmp(sbuf,"port"))			fionaConf.port=(short)getf(is);
		else if(cmp(sbuf,"numSlaves"))		fionaConf.numSlaves=(short)getf(is);
		else if(cmp(sbuf,"masterIP"))		fionaConf.masterIP=getst(is);
		else if(cmp(sbuf,"wallFile"))		fionaConf.wallDescriptionFilename=getst(is);
		else if(cmp(sbuf,"wall"))			fionaConf.wallDescriptionFilename=fn;
		else if(cmp(sbuf,"projCalib1x"))	fionaConf.projectorCalibx[0]=getv3(is);
		else if(cmp(sbuf,"projCalib1y"))	fionaConf.projectorCaliby[0]=getv3(is);
		else if(cmp(sbuf,"projCalib1z"))	fionaConf.projectorCalibz[0]=getv3(is);
		else if(cmp(sbuf,"projCalib2x"))	fionaConf.projectorCalibx[1]=getv3(is);
		else if(cmp(sbuf,"projCalib2y"))	fionaConf.projectorCaliby[1]=getv3(is);
		else if(cmp(sbuf,"projCalib2z"))	fionaConf.projectorCalibz[1]=getv3(is);
		else if(cmp(sbuf,"projCalib3x"))	fionaConf.projectorCalibx[2]=getv3(is);
		else if(cmp(sbuf,"projCalib3y"))	fionaConf.projectorCaliby[2]=getv3(is);
		else if(cmp(sbuf,"projCalib3z"))	fionaConf.projectorCalibz[2]=getv3(is);
		else if(cmp(sbuf,"projCalib4x"))	fionaConf.projectorCalibx[3]=getv3(is);
		else if(cmp(sbuf,"projCalib4y"))	fionaConf.projectorCaliby[3]=getv3(is);
		else if(cmp(sbuf,"projCalib4z"))	fionaConf.projectorCalibz[3]=getv3(is);
		else if(cmp(sbuf,"masterSlave"))	fionaConf.masterSlave=true;
		else if(cmp(sbuf,"masterSlaveIP"))	fionaConf.masterSlaveIP=getst(is);
		else if(cmp(sbuf,"masterSlavePort")) fionaConf.masterSlavePort=(short)getf(is);
		else if(cmp(sbuf, "navigationSpeed")) fionaConf.navigationSpeed=getf(is);
		else if(cmp(sbuf, "rotationSpeed")) fionaConf.rotationSpeed=getf(is);
		else if(cmp(sbuf, "riftTracking")) 
		{
			fionaConf.riftTracking=true; 
			fionaConf.slave=false;
		}
		else if(cmp(sbuf, "farClip")) fionaConf.farClip = getf(is);
		else if(cmp(sbuf, "nearClip")) fionaConf.nearClip = getf(is);
		else if(cmp(sbuf, "inputHost")) fionaConf.inputHost = getst(is);
		else if(cmp(sbuf, "graphicsMem")) fionaConf.displayGraphicsMem = true;
		else if(cmp(sbuf, "dualView")) fionaConf.dualView = true;
		else if(cmp(sbuf, "showFPS")) fionaConf.showRenderTime = true;
		else if(cmp(sbuf, "noHeadNode")) fionaConf.renderHeadNode = false;
		else if(cmp(sbuf, "graphicsThread")) fionaConf.graphicsThread = true;
		else if(cmp(sbuf, "noTracking")) fionaConf.noTracking = true;
		else if(cmp(sbuf, "noHeadTracking")) fionaConf.noHeadTracking = true;
		else if(cmp(sbuf, "multiGPU")) fionaConf.multiGPU = true;
		else if(cmp(sbuf, "useSecondViewerNodes")) fionaConf.useSecondViewerNodes = true;
		else if(cmp(sbuf, "useSecondGPU")) fionaConf.useSecondGPU = true;
		else if(cmp(sbuf, "recordMovement")) {FionaUTVRPN::recordIO = true;  fionaConf.playbackFileName = getst(is);}
		else if(cmp(sbuf, "playbackMovement")) {printf("Starting playback...!\n");FionaUTVRPN::playbackIO = true; fionaConf.playbackFileName = getst(is);}
		else if(cmp(sbuf, "twoWindows")) fionaConf.twoWindows = true;
		else if(cmp(sbuf, "noHardwareSync")) fionaConf.hardwareSync = false;
		else if(cmp(sbuf, "hardwareSync")) fionaConf.hardwareSync = true;
		else if(cmp(sbuf, "forceOculusFBOSizeMatch")) fionaConf.forceOculusFBOSizeMatch = true;
		else if(cmp(sbuf, "fboSameAsWindow")) fionaConf.fboSameAsWindow = true;
		else if(cmp(sbuf, "wandView")) fionaConf.wandView = true;
		else if(cmp(sbuf, "deltaInput")) fionaConf.deltaInput = true;
		else if (cmp(sbuf, "loadPath")) fionaConf.pathFileName = getst(is);
		else if (cmp(sbuf, "borderless")) fionaConf.borderlessWindow = true;
		else if (cmp(sbuf, "singlePassStereo")) fionaConf.singlePassStereo = true;
		else if (cmp(sbuf, "oculusResMultiplier")) fionaConf.oculusResMultiplier = getf(is);
		if( is.eof() ) break;
	}
}

void _FionaUTInitConfig (int argc, char* argv[])
{
	for(int i=0; i<MAX_WINDOW; i++ ) 
		fionaWinConf[i]=emptyConfig;

#ifdef LINUX_BUILD
	char hostname[1024];
	hostname[1023] = '\n';
	gethostname(hostname, 1023);	
	std::string configPath=DEFAULT_CONFIG_PATH + hostname + ".txt";
#else
	std::string configPath=DEFAULT_CONFIG_FILE;
#endif

	std::stringstream arg;
	
	// Coin a string stream of arguments
	arg.clear(); for( int i=1; i<argc; i++ ) arg<<argv[i]<<" ";
	
	// Looking for configFile specification
	while( !arg.eof() ) 
	{
		if(cmp(getst(arg),"configFile")) 
		{
			configPath=getst(arg);
		}
	}

	printf("Looking for FionaConfig.txt @ %s\n", configPath.c_str());

	std::ifstream configFile(configPath.c_str());
	if( configFile.is_open() ) 
	{
		_FionaUTProcessOptions(configFile,configPath);
	}
	else
	{
		printf("Couldn't find config file, using default and command line settings\n");
	}
	configFile.close();

	arg.clear(); 
	
	for( int i=1; i<argc; i++ ) 
		arg<<argv[i]<<" ";	

	_FionaUTProcessOptions(arg,"");
}

#ifdef ENABLE_OCULUS
#ifdef ENABLE_CV1
#if defined(_WIN32)
#include <dxgi.h> // for GetDefaultAdapterLuid
#pragma comment(lib, "dxgi.lib")
#endif

static ovrGraphicsLuid GetDefaultAdapterLuid()
{
	ovrGraphicsLuid luid = ovrGraphicsLuid();

#if defined(_WIN32)
	IDXGIFactory* factory = nullptr;

	if (SUCCEEDED(CreateDXGIFactory(IID_PPV_ARGS(&factory))))
	{
		IDXGIAdapter* adapter = nullptr;

		if (SUCCEEDED(factory->EnumAdapters(0, &adapter)))
		{
			DXGI_ADAPTER_DESC desc;

			adapter->GetDesc(&desc);
			memcpy(&luid, &desc.AdapterLuid, sizeof(luid));
			adapter->Release();
		}

		factory->Release();
	}
#endif

	return luid;
}

#endif
#endif

void _FionaUTCreateAppTypeWindow(const char* name)
{
	std::string n(name);
	fionaCurWindow = fionaNextWindow++;
	int c=fionaCurWindow, w=fionaWinConf[c].winw, h=fionaWinConf[c].winh;
	int x=fionaWinConf[c].winx, y=fionaWinConf[c].winy;

	if( fionaConf.wallDescriptionFilename.length()>0 )
	{
		if( _FionaUTCreateWallsFromFile(fionaConf.wallDescriptionFilename,c))
		{
			fionaWinConf[c].window =__FionaUTCreateWindow(n.c_str(),x,y,w,h,fionaConf.fullscreen);
			_FionaUTSyncProjectorCalib(fionaWinConf[c]);
			return;
		}
	}
	switch(fionaConf.fullscreen)
	{
		case 0:		fionaWinConf[c].type=FionaWindowConfig::NORMAL;		break;
		case -1:	fionaWinConf[c].type=FionaWindowConfig::WIDE;		break;
		default:	fionaWinConf[c].type=FionaWindowConfig::FULLSCREEN;	break;
	}
	switch(fionaConf.appType)
	{
		case FionaConfig::DEVLAB:
		case FionaConfig::DEVLAB2:
		{
			fionaWinConf[c].walls.push_back(WL_DEVL);
			fionaWinConf[c].walls[0].viewports.push_back(VP_DEVL_LO);
			fionaWinConf[c].walls[0].viewports.push_back(VP_DEVL_HI);
		}	break;
		case FionaConfig::DEVLAB_WIN8:
		{
			fionaWinConf[c].walls.push_back(WL_DEVL);
			fionaWinConf[c].walls[0].viewports.push_back(VP_DESK);
			break;
		}
		case FionaConfig::DEVLAB_DUALPIPE:
		{
			fionaWinConf[c].walls.push_back(WL_DEVL);
			fionaWinConf[c].walls[0].viewports.push_back(VP_DESK);
			break;
		}
		case FionaConfig::DEVLAB_DUALPIPE_SLAVE:
		{
			fionaWinConf[c].walls.push_back(WL_DEVL);
			fionaWinConf[c].walls[0].viewports.push_back(VP_DESK);
			break;
		}
		case FionaConfig::CAVE1_DUALPIPE:
		{
			fionaWinConf[c].walls.push_back(WL_CV1);
			if (fionaConf.twoWindows)
			{
				fionaWinConf[c].walls[0].viewports.push_back(VP_DESK);
				fionaWinConf[1].walls.push_back(WL_CV1);
				fionaWinConf[1].walls[0].viewports.push_back(VP_DESK);
			}
			else
			{
				fionaWinConf[c].walls[0].viewports.push_back(VP_DESK);//VP_SPLT_STEREO);
			}
			break;
		}
		case FionaConfig::CAVE2_DUALPIPE:
		{
			fionaWinConf[c].walls.push_back(WL_CV3);
			if (fionaConf.twoWindows)
			{
				fionaWinConf[c].walls[0].viewports.push_back(VP_DESK);
				fionaWinConf[1].walls.push_back(WL_CV3);
				fionaWinConf[1].walls[0].viewports.push_back(VP_DESK);
			}
			else
			{
				fionaWinConf[c].walls[0].viewports.push_back(VP_DESK); //VP_SPLT_STEREO);
			}
			break;
		}
		case FionaConfig::CAVE3_DUALPIPE:
		{
			fionaWinConf[c].walls.push_back(WL_CV6);
			if (fionaConf.twoWindows)
			{
				fionaWinConf[c].walls[0].viewports.push_back(VP_DESK);
				fionaWinConf[1].walls.push_back(WL_CV6);
				fionaWinConf[1].walls[0].viewports.push_back(VP_DESK);
			}
			else
			{
				fionaWinConf[c].walls[0].viewports.push_back(VP_DESK); //VP_SPLT_STEREO);
			}
			break;
		}
		case FionaConfig::CAVE4_DUALPIPE:
		{
			fionaWinConf[c].walls.push_back(WL_CV2);
			
			if (fionaConf.twoWindows)
			{
				fionaWinConf[c].walls[0].viewports.push_back(VP_DESK);
				fionaWinConf[1].walls.push_back(WL_CV2);
				fionaWinConf[1].walls[0].viewports.push_back(VP_DESK);
			}
			else
			{
				fionaWinConf[c].walls[0].viewports.push_back(VP_DESK);//VP_SPLT_STEREO);
			}
			break;
		}
		case FionaConfig::CAVE5_DUALPIPE:
		{
			fionaWinConf[c].walls.push_back(WL_CV4);
			if (fionaConf.twoWindows)
			{
				fionaWinConf[c].walls[0].viewports.push_back(VP_DESK);
				fionaWinConf[1].walls.push_back(WL_CV4);
				fionaWinConf[1].walls[0].viewports.push_back(VP_DESK);
			}
			else
			{
				fionaWinConf[c].walls[0].viewports.push_back(VP_DESK);//VP_SPLT_STEREO);
			}
			break;
		}
		case FionaConfig::CAVE6_DUALPIPE:
		{
			fionaWinConf[c].walls.push_back(WL_CV5);
			if (fionaConf.twoWindows)
			{
				fionaWinConf[c].walls[0].viewports.push_back(VP_DESK);
				fionaWinConf[1].walls.push_back(WL_CV5);
				fionaWinConf[1].walls[0].viewports.push_back(VP_DESK);
			}
			else
			{
				fionaWinConf[c].walls[0].viewports.push_back(VP_DESK);// VP_SPLT_STEREO);
			}
			break;
		}
		case FionaConfig::CAVE1_SS:
		{
			fionaWinConf[c].walls.push_back(WL_CV1);
			fionaWinConf[c].walls[0].viewports.push_back(VP_SPLT_STEREO);
			break;
		}
		case FionaConfig::CAVE2_SS:
		{
			fionaWinConf[c].walls.push_back(WL_CV3);
			fionaWinConf[c].walls[0].viewports.push_back(VP_SPLT_STEREO);
			break;
		}
		case FionaConfig::CAVE3_SS:
		{
			fionaWinConf[c].walls.push_back(WL_CV6);
			fionaWinConf[c].walls[0].viewports.push_back(VP_SPLT_STEREO);
			break;
		}
		case FionaConfig::CAVE4_SS:
		{
			fionaWinConf[c].walls.push_back(WL_CV2);
			fionaWinConf[c].walls[0].viewports.push_back(VP_SPLT_STEREO);
			break;
		}
		case FionaConfig::CAVE5_SS:
		{
			fionaWinConf[c].walls.push_back(WL_CV4);
			fionaWinConf[c].walls[0].viewports.push_back(VP_SPLT_STEREO);
			break;
		}
		case FionaConfig::CAVE6_SS:
		{
			fionaWinConf[c].walls.push_back(WL_CV5);
			fionaWinConf[c].walls[0].viewports.push_back(VP_SPLT_STEREO);
			break;
		}
		case FionaConfig::CAVE1_WIN8:
		case FionaConfig::CAVE_NEW_FRONT:
		{
			fionaWinConf[c].walls.push_back(WL_CV1);
			fionaWinConf[c].walls[0].viewports.push_back(VP_DESK);
			break;
		}
		case FionaConfig::CAVE2_WIN8:
		case FionaConfig::CAVE_NEW_DOOR:
		{
			fionaWinConf[c].walls.push_back(WL_CV3);
			fionaWinConf[c].walls[0].viewports.push_back(VP_DESK);
			break;
		}
		case FionaConfig::CAVE3_WIN8:
		case FionaConfig::CAVE_NEW_FLOOR:
		{
			fionaWinConf[c].walls.push_back(WL_CV6);
			fionaWinConf[c].walls[0].viewports.push_back(VP_DESK);
			break;
		}
		case FionaConfig::CAVE4_WIN8:
		case FionaConfig::CAVE_NEW_RIGHT:
		{
			fionaWinConf[c].walls.push_back(WL_CV2);
			fionaWinConf[c].walls[0].viewports.push_back(VP_DESK);
			break;
		}
		case FionaConfig::CAVE5_WIN8:
		case FionaConfig::CAVE_NEW_LEFT:
		{
			fionaWinConf[c].walls.push_back(WL_CV4);
			fionaWinConf[c].walls[0].viewports.push_back(VP_DESK);
			break;
		}
		case FionaConfig::CAVE6_WIN8:
		case FionaConfig::CAVE_NEW_CEILING:
		{
			fionaWinConf[c].walls.push_back(WL_CV5);
			fionaWinConf[c].walls[0].viewports.push_back(VP_DESK);
			break;
		}
		case FionaConfig::CAVE1:
		{
			if(!fionaConf.useSecondViewerNodes && !fionaConf.twoWindows)
			{
				fionaWinConf[c].walls.push_back(WL_CV1);
				fionaWinConf[c].walls[0].viewports.push_back(VP_CAVE_PRIM_LO);
				fionaWinConf[c].walls[0].viewports.push_back(VP_CAVE_PRIM_HI);
				fionaWinConf[c].walls.push_back(WL_CV2);
				fionaWinConf[c].walls[1].viewports.push_back(VP_CAVE_SECN_LO);
				fionaWinConf[c].walls[1].viewports.push_back(VP_CAVE_SECN_HI);
			}
			else
			{
				fionaWinConf[c].walls.push_back(WL_CV1);
				fionaWinConf[c].walls[0].viewports.push_back(VP_DEVL_LO);
				fionaWinConf[c].walls[0].viewports.push_back(VP_DEVL_HI);
				if(fionaConf.twoWindows)
				{
					fionaWinConf[1].walls.push_back(WL_CV2);
					fionaWinConf[1].walls[0].viewports.push_back(VP_DEVL_LO);
					fionaWinConf[1].walls[0].viewports.push_back(VP_DEVL_HI);
				}
			}
			n+="-1";
		}	break;
		case FionaConfig::CAVE2:
		{
			if(!fionaConf.useSecondViewerNodes && !fionaConf.twoWindows)
			{
				fionaWinConf[c].walls.push_back(WL_CV3);
				fionaWinConf[c].walls[0].viewports.push_back(VP_CAVE_PRIM_LO);
				fionaWinConf[c].walls[0].viewports.push_back(VP_CAVE_PRIM_HI);
				fionaWinConf[c].walls.push_back(WL_CV4);
				fionaWinConf[c].walls[1].viewports.push_back(VP_CAVE_SECN_LO);
				fionaWinConf[c].walls[1].viewports.push_back(VP_CAVE_SECN_HI);
			}
			else
			{
				fionaWinConf[c].walls.push_back(WL_CV3);
				fionaWinConf[c].walls[0].viewports.push_back(VP_DEVL_LO);
				fionaWinConf[c].walls[0].viewports.push_back(VP_DEVL_HI);
				if(fionaConf.twoWindows)
				{
					fionaWinConf[1].walls.push_back(WL_CV4);
					fionaWinConf[1].walls[0].viewports.push_back(VP_DEVL_LO);
					fionaWinConf[1].walls[0].viewports.push_back(VP_DEVL_HI);
				}
			}
			n+="-2";
		}	break;
		case FionaConfig::CAVE3:
		{
			if(!fionaConf.useSecondViewerNodes && !fionaConf.twoWindows)
			{
				fionaWinConf[c].walls.push_back(WL_CV5);
				fionaWinConf[c].walls[0].viewports.push_back(VP_CAVE_PRIM_LO);
				fionaWinConf[c].walls[0].viewports.push_back(VP_CAVE_PRIM_HI);
				fionaWinConf[c].walls.push_back(WL_CV6);
				fionaWinConf[c].walls[1].viewports.push_back(VP_CAVE_SECN_LO);
				fionaWinConf[c].walls[1].viewports.push_back(VP_CAVE_SECN_HI);
			}
			else
			{
				fionaWinConf[c].walls.push_back(WL_CV5);
				fionaWinConf[c].walls[0].viewports.push_back(VP_DEVL_LO);
				fionaWinConf[c].walls[0].viewports.push_back(VP_DEVL_HI);
				if(fionaConf.twoWindows)
				{
					fionaWinConf[1].walls.push_back(WL_CV6);
					fionaWinConf[1].walls[0].viewports.push_back(VP_DEVL_LO);
					fionaWinConf[1].walls[0].viewports.push_back(VP_DEVL_HI);
				}
			}
			n+="-3";
		}	break;
		case FionaConfig::CAVE4:
		{
			if(!fionaConf.useSecondViewerNodes)
			{
				fionaWinConf[c].walls.push_back(WL_CV1);
				fionaWinConf[c].walls[0].viewports.push_back(VP_CAVE_PRIM_LO);
				fionaWinConf[c].walls[0].viewports.push_back(VP_CAVE_PRIM_HI);
				fionaWinConf[c].walls.push_back(WL_CV2);
				fionaWinConf[c].walls[1].viewports.push_back(VP_CAVE_SECN_LO);
				fionaWinConf[c].walls[1].viewports.push_back(VP_CAVE_SECN_HI);
			}
			else
			{
				fionaWinConf[c].walls.push_back(WL_CV2);
				fionaWinConf[c].walls[0].viewports.push_back(VP_DEVL_LO);
				fionaWinConf[c].walls[0].viewports.push_back(VP_DEVL_HI);
			}
			
			n+="-4";
		}	break;
		case FionaConfig::CAVE5:
		{
			if(!fionaConf.useSecondViewerNodes)
			{
				fionaWinConf[c].walls.push_back(WL_CV3);
				fionaWinConf[c].walls[0].viewports.push_back(VP_CAVE_PRIM_LO);
				fionaWinConf[c].walls[0].viewports.push_back(VP_CAVE_PRIM_HI);
				fionaWinConf[c].walls.push_back(WL_CV4);
				fionaWinConf[c].walls[1].viewports.push_back(VP_CAVE_SECN_LO);
				fionaWinConf[c].walls[1].viewports.push_back(VP_CAVE_SECN_HI);
			}
			else
			{
				fionaWinConf[c].walls.push_back(WL_CV4);
				fionaWinConf[c].walls[0].viewports.push_back(VP_DEVL_LO);
				fionaWinConf[c].walls[0].viewports.push_back(VP_DEVL_HI);
			}
			n+="-5";
		}	break;
		case FionaConfig::CAVE6:
		{
			if(!fionaConf.useSecondViewerNodes)
			{
				fionaWinConf[c].walls.push_back(WL_CV5);
				fionaWinConf[c].walls[0].viewports.push_back(VP_CAVE_PRIM_LO);
				fionaWinConf[c].walls[0].viewports.push_back(VP_CAVE_PRIM_HI);
				fionaWinConf[c].walls.push_back(WL_CV6);
				fionaWinConf[c].walls[1].viewports.push_back(VP_CAVE_SECN_LO);
				fionaWinConf[c].walls[1].viewports.push_back(VP_CAVE_SECN_HI);
			}
			else
			{
				fionaWinConf[c].walls.push_back(WL_CV6);
				fionaWinConf[c].walls[0].viewports.push_back(VP_DEVL_LO);
				fionaWinConf[c].walls[0].viewports.push_back(VP_DEVL_HI);
			}
			n+="-6";
		}	break;
		case FionaConfig::HEADNODE:
		{
			fionaWinConf[c].walls.push_back(WL_DESK);
			fionaWinConf[c].walls[0].viewports.push_back(VP_DESK);
			n+="-HEAD";
		}	break;
		case FionaConfig::VUZIX:
		{
			fionaWinConf[c].walls.push_back(WL_VUZIX);
			fionaWinConf[c].walls[0].viewports.push_back(VP_SPLT_STEREO);
		}	break;
		case FionaConfig::VIVE:
		{
			fionaWinConf[c].walls.push_back(WL_OCULUS);
			fionaWinConf[c].walls[0].viewports.push_back(VP_SPLT_STEREO);
#ifdef ENABLE_VIVE
			// Loading the SteamVR Runtime
			vr::EVRInitError eError = vr::VRInitError_None;
			fionaConf.m_pHMD = vr::VR_Init(&eError, vr::VRApplication_Scene);

			if (eError != vr::VRInitError_None)
			{
				fionaConf.m_pHMD = NULL;
				printf("Unable to init VR runtime: %s", vr::VR_GetVRInitErrorAsEnglishDescription(eError));
			}

			if (fionaConf.m_pHMD != 0)
			{
				std::string m_strDriver = "No Driver";
				std::string m_strDisplay = "No Display";

				uint32_t unRequiredBufferLen = fionaConf.m_pHMD->GetStringTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String, NULL, 0, 0);
				char *pchBuffer = new char[unRequiredBufferLen];
				unRequiredBufferLen = fionaConf.m_pHMD->GetStringTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String, pchBuffer, unRequiredBufferLen, 0);
				m_strDriver = pchBuffer;
				delete[] pchBuffer;

				unRequiredBufferLen = fionaConf.m_pHMD->GetStringTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SerialNumber_String, NULL, 0, 0);
				pchBuffer = new char[unRequiredBufferLen];
				unRequiredBufferLen = fionaConf.m_pHMD->GetStringTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SerialNumber_String, pchBuffer, unRequiredBufferLen, 0);
				m_strDisplay = pchBuffer;
				delete[] pchBuffer;

				printf("Vive successfully initialized: %s %s\n", m_strDriver.c_str(), m_strDisplay.c_str());
			}
#endif
		} break;
		case FionaConfig::OCULUS:
		{
			fionaWinConf[c].walls.push_back(WL_OCULUS);
			fionaWinConf[c].walls[0].viewports.push_back(VP_SPLT_STEREO);
#ifdef ENABLE_OCULUS
#ifdef ENABLE_DK2
			//OVR::System::Init();
			ovr_Initialize(0);
			ovrHmd_Create(0, &fionaConf.oculusHmd);
			if(fionaConf.oculusHmd == 0)
			{
				printf("Couldn't initialize DK2\n");
				exit(0);
			}

			OVR::Sizei WindowSize;

			//if (fionaConf.oculusHmd->HmdCaps & ovrHmdCap_ExtendDesktop)
			//{
				WindowSize = fionaConf.oculusHmd->Resolution;
			//}
			//else
			//{
				// In Direct App-rendered mode, we can use smaller window size,
				// as it can have its own contents and isn't tied to the buffer.
			//	WindowSize = OVR::Sizei(1100, 618);//Sizei(960, 540); avoid rotated output bug.
			//}
			
			w = WindowSize.w;
			h = WindowSize.h;
			printf("WINDOW SIZE: %u, %u\n", w, h);
			//x = fionaConf.oculusHmd->WindowsPos.x;
			//y = fionaConf.oculusHmd->WindowsPos.y;

			ovrFovPort eyeFov[2];
			eyeFov[0] = fionaConf.oculusHmd->DefaultEyeFov[0];
			eyeFov[1] = fionaConf.oculusHmd->DefaultEyeFov[1];

			printf("Default fov W, Default fov H: %f, %f\n", (atan(eyeFov[0].RightTan)*2.f),  (atan(eyeFov[0].UpTan)*2.f));
			
			fionaConf.desktopFOV = ((atan(eyeFov[0].UpTan)*2.f) * 180.f) / 3.14159;
			
			OVR::Sizei recommenedTex0Size = ovrHmd_GetFovTextureSize(fionaConf.oculusHmd, ovrEye_Left,  eyeFov[0], 1.f);
			OVR::Sizei recommenedTex1Size = ovrHmd_GetFovTextureSize(fionaConf.oculusHmd, ovrEye_Right, eyeFov[1], 1.f);

			//Ogre does NOT like using Oculus' recommended FBO size here.. 

            OVR::Sizei  rtSize;

			if(fionaConf.forceOculusFBOSizeMatch)
			{
				rtSize.w = 1920;
				rtSize.h = 1080;
			}
			else
			{   
				rtSize.w = recommenedTex0Size.w + recommenedTex1Size.w;
				rtSize.h = MAX(recommenedTex0Size.h, recommenedTex1Size.h);
			}

			fionaConf.FBOWidth = rtSize.w;
			fionaConf.FBOHeight = rtSize.h;
			
			printf("FBO Width: %u, FBO Height: %u\n", fionaConf.FBOWidth, fionaConf.FBOHeight);

			//ovrVector2f UVScaleOffset[2][2];
			//ovrRecti           EyeRenderViewport[2];
			//EyeRenderViewport[0].Pos  = OVR::Vector2i(0,0);
			//EyeRenderViewport[0].Size = OVR::Sizei(rtSize.w / 2, rtSize.h);
			//EyeRenderViewport[1].Pos  = OVR::Vector2i((rtSize.w + 1) / 2, 0);
			//EyeRenderViewport[1].Size = EyeRenderViewport[0].Size;
			//ovrHmd_GetRenderScaleAndOffset(eyeFov[0],rtSize, EyeRenderViewport[0], UVScaleOffset[0]);
			//ovrHmd_GetRenderScaleAndOffset(eyeFov[1],rtSize, EyeRenderViewport[1], UVScaleOffset[1]);
			//printf("***%f, %f, %f, %f\n", UVScaleOffset[0][0].x, UVScaleOffset[0][0].y, UVScaleOffset[0][0].x, UVScaleOffset[0][1].y);
			//printf("***%f, %f, %f, %f\n", UVScaleOffset[1][0].x, UVScaleOffset[1][0].y, UVScaleOffset[1][0].x, UVScaleOffset[1][1].y);

            // Use returned size as the actual RT size may be different due to HW limits.
            //rtSize = EnsureRendertargetAtLeastThisBig(Rendertarget_BothEyes, rtSize);
#endif
#ifdef ENABLE_DK1
			OVR::System::Init(OVR::Log::ConfigureDefaultLog(OVR::LogMask_All));
			OVR::Ptr<OVR::DeviceManager> pManager;
			OVR::Ptr<OVR::HMDDevice> pHMD;
			pManager = *OVR::DeviceManager::Create();
			pHMD = *pManager->EnumerateDevices<OVR::HMDDevice>().CreateDevice();

			if(pHMD != 0)
			{
				OVR::HMDInfo hmd;

				if (pHMD->GetDeviceInfo(&hmd))
				{
					char name[32]; memset(name, 0, sizeof(name));
					memcpy(name, hmd.DisplayDeviceName, sizeof(name));
					printf("Oculus Rift Device Detected: %s\n", name);
					fionaConf.stereoConfig.SetHMDInfo(hmd);
					fionaConf.stereoConfig.SetFullViewport(OVR::Util::Render::Viewport(0,0,OCULUS_DEFAULT_WIDTH,OCULUS_DEFAULT_HEIGHT));
					fionaConf.stereoConfig.SetStereoMode(OVR::Util::Render::Stereo_LeftRight_Multipass);
					if (hmd.HScreenSize > 0.0f)
					{
						if (hmd.HScreenSize > 0.140f)  // 7"
							fionaConf.stereoConfig.SetDistortionFitPointVP(-1.0f, 0.0f);        
						else        
							fionaConf.stereoConfig.SetDistortionFitPointVP(0.0f, 1.0f);
					}

					fionaConf.stereoConfig.Set2DAreaFov(OVR::DegreeToRad(85.f));
					//TODO - optional Rift head tracking...
					if(fionaConf.riftTracking)
					{
						fionaConf.pSensor = *pHMD->GetSensor();
						if (fionaConf.pSensor)
						{
							fionaConf.sFusion = new OVR::SensorFusion();
							fionaConf.sFusion->AttachToSensor(fionaConf.pSensor);
						}
					}
					
					::ShowCursor(FALSE);
				}
			}
			else
			{
				printf("Could not detect Oculus Rift Device!\n");
			}
#endif
#ifdef ENABLE_CV1
			
			ovrResult result = ovr_Initialize(nullptr);
			if (OVR_FAILURE(result))
				printf("Couldn't initialize Oculus\n");

			ovrGraphicsLuid luid;
			result = ovr_Create(&fionaConf.session, &luid);

			if (!OVR_SUCCESS(result))
			{
				ovrErrorInfo errorInfo;
				ovr_GetLastErrorInfo(&errorInfo);
				printf("ovr_Create failed: %s\n", errorInfo.ErrorString);
				printf("Couldn't create oculus session\n");
				exit(1);
			}

			if (memcmp(&luid, &GetDefaultAdapterLuid(), sizeof(ovrGraphicsLuid))) // If luid that the Rift is on is not the default adapter LUID...
			{
				printf("OpenGL supports only the default graphics adapter.\n");
			}

			fionaConf.hmdDesc = ovr_GetHmdDesc(fionaConf.session);

			ovrSizei recommenedTex0Size = ovr_GetFovTextureSize(fionaConf.session, ovrEyeType(0), fionaConf.hmdDesc.DefaultEyeFov[0], fionaConf.oculusResMultiplier);
			ovrSizei recommenedTex1Size = ovr_GetFovTextureSize(fionaConf.session, ovrEyeType(1), fionaConf.hmdDesc.DefaultEyeFov[1], fionaConf.oculusResMultiplier);


			ovrSizei  rtSize;

			if (fionaConf.forceOculusFBOSizeMatch)
			{
				rtSize.w = 2160;
				rtSize.h = 1200;
			}
			else
			{
				rtSize.w = recommenedTex0Size.w + recommenedTex1Size.w;
				rtSize.h = MAX(recommenedTex0Size.h, recommenedTex1Size.h);
			}

			//since it's split stereo, we only need our own fbo to be half the recommended size
			fionaConf.FBOWidth = rtSize.w/2;
			//fionaConf.FBOWidth = rtSize.w;
			fionaConf.FBOHeight = rtSize.h;

			//if (fionaConf.FBOWidth > fionaConf.FBOHeight)
			{
				fionaConf.desktopFOV = (atanf(fionaConf.hmdDesc.DefaultEyeFov[0].LeftTan) + atan(fionaConf.hmdDesc.DefaultEyeFov[0].RightTan)) * 180.f / 3.14159;
			}

			printf("\nField of View: %f\n", fionaConf.desktopFOV);

			//else
			{
			//	fionaConf.desktopFOV = ((atan(MAX(fionaConf.hmdDesc.DefaultEyeFov[0].UpTan, fionaConf.hmdDesc.DefaultEyeFov[0].DownTan))*2.f) * 180.f) / 3.14159;
			}
#endif
#endif
			break;
		}
		case FionaConfig::UNITY_DL:
		{
			//pre-win 8.1
			/*fionaWinConf[c].walls.push_back(WL_DEVL);
			fionaWinConf[c].walls[0].viewports.push_back(VP_DEVL_LO);
			fionaWinConf[c].walls[0].viewports.push_back(VP_DEVL_HI);*/
			//windows 8.1 setup
			fionaWinConf[c].walls.push_back(WL_DEVL);
			fionaWinConf[c].walls[0].viewports.push_back(VP_DESK);
			break;
		}
		case FionaConfig::UNITY_CAVE:
		{
			//pre-windows 8.1!
			/*fionaWinConf[c].walls.push_back(WL_CV1);
			fionaWinConf[c].walls.push_back(WL_CV2);
			fionaWinConf[c].walls[0].viewports.push_back(VP_CAVE_PRIM_LO);
			fionaWinConf[c].walls[0].viewports.push_back(VP_CAVE_PRIM_HI);
			fionaWinConf[c].walls[1].viewports.push_back(VP_CAVE_SECN_LO);
			fionaWinConf[c].walls[1].viewports.push_back(VP_CAVE_SECN_HI);
			fionaWinConf[c].walls.push_back(WL_CV3);
			fionaWinConf[c].walls.push_back(WL_CV4);
			fionaWinConf[c].walls[2].viewports.push_back(VP_CAVE_PRIM_LO);
			fionaWinConf[c].walls[2].viewports.push_back(VP_CAVE_PRIM_HI);
			fionaWinConf[c].walls[3].viewports.push_back(VP_CAVE_SECN_LO);
			fionaWinConf[c].walls[3].viewports.push_back(VP_CAVE_SECN_HI);
			fionaWinConf[c].walls.push_back(WL_CV5);
			fionaWinConf[c].walls.push_back(WL_CV6);
			fionaWinConf[c].walls[4].viewports.push_back(VP_CAVE_PRIM_LO);
			fionaWinConf[c].walls[4].viewports.push_back(VP_CAVE_PRIM_HI);
			fionaWinConf[c].walls[5].viewports.push_back(VP_CAVE_SECN_LO);
			fionaWinConf[c].walls[5].viewports.push_back(VP_CAVE_SECN_HI);
			fionaWinConf[c].walls.push_back(WL_DESK);
			fionaWinConf[c].walls[6].viewports.push_back(VP_DESK);
			fionaWinConf[c].walls[6].viewports.push_back(VP_DESK);*/

			//windows 8.1 setup!
			fionaWinConf[c].walls.push_back(WL_CV1);
			fionaWinConf[c].walls[0].viewports.push_back(VP_DESK);
			fionaWinConf[c].walls.push_back(WL_CV3);
			fionaWinConf[c].walls[1].viewports.push_back(VP_DESK);
			fionaWinConf[c].walls.push_back(WL_CV5);
			fionaWinConf[c].walls[2].viewports.push_back(VP_DESK);
			fionaWinConf[c].walls.push_back(WL_CV2);
			fionaWinConf[c].walls[3].viewports.push_back(VP_DESK);
			fionaWinConf[c].walls.push_back(WL_CV4);
			fionaWinConf[c].walls[4].viewports.push_back(VP_DESK);
			fionaWinConf[c].walls.push_back(WL_CV6);
			fionaWinConf[c].walls[5].viewports.push_back(VP_DESK);
			fionaWinConf[c].walls.push_back(WL_DESK);
			fionaWinConf[c].walls[6].viewports.push_back(VP_DESK);
			fionaWinConf[c].walls[6].viewports.push_back(VP_DESK);
			break;
		}
		case FionaConfig::UNITY_DSCVR:
		{
			for(int i = 0; i < 10; ++i)
			{
				char idxChar[4];
				memset(idxChar, 0, sizeof(char)*4);
				sprintf(idxChar, "%d", i);
				std::string strIdx(idxChar);
				std::string configPath;
#ifdef LINUX_BUILD
				configPath=DEFAULT_CONFIG_PATH + "dscvr" + strIdx + ".local.txt";
#else
				configPath=DEFAULT_CONFIG_FILE;
#endif
				printf("%s\n", configPath.c_str());
				_FionaUTCreateWallsFromFile(configPath, 0);
			}
			//last item here is for the head node..
			fionaWinConf[c].walls.push_back(WL_DESK);
			fionaWinConf[c].walls[fionaWinConf[c].walls.size()-1].viewports.push_back(VP_SPLT_STEREO);
			break;
		}
		case FionaConfig::NEW_DEVLAB:
		{
			fionaWinConf[c].walls.push_back(WL_NEW_DEVLAB);
			fionaWinConf[c].walls[fionaWinConf[c].walls.size() - 1].viewports.push_back(VP_NEWDL_PRIM_LO);
			fionaWinConf[c].walls[fionaWinConf[c].walls.size() - 1].viewports.push_back(VP_NEWDL_PRIM_HI);
			fionaWinConf[c].walls[fionaWinConf[c].walls.size() - 1].viewports.push_back(VP_NEWDL_SECN_LO);
			fionaWinConf[c].walls[fionaWinConf[c].walls.size() - 1].viewports.push_back(VP_NEWDL_SECN_HI);
			break;
		}
		case FionaConfig::NEW_DEVLAB2:
		{
			fionaWinConf[c].walls.push_back(WL_NEW_DEVLAB2);
			fionaWinConf[c].walls[fionaWinConf[c].walls.size() - 1].viewports.push_back(VP_NEWDL_TOPLEFT);
			fionaWinConf[c].walls[fionaWinConf[c].walls.size() - 1].viewports.push_back(VP_NEWDL_TOPRIGHT);
			break;
		}
		case FionaConfig::WINDOWED:
		case FionaConfig::HDTV:
		case FionaConfig::SLAVE:
		default:
		{
			fionaWinConf[c].walls.push_back(WL_DESK);
			fionaWinConf[c].walls[0].viewports.push_back(VP_DESK);
		}	break;
	}


	fionaWinConf[c].window =__FionaUTCreateWindow(n.c_str(),x,y,w,h,fionaConf.fullscreen);

#ifdef ENABLE_OCULUS
#ifdef ENABLE_DK2
	/*ovrFovPort eyeFov[2] = { fionaConf.oculusHmd->DefaultEyeFov[0], fionaConf.oculusHmd->DefaultEyeFov[1] };
	//ovrEyeRenderDesc eyeRenderDesc[2];
	
	ovrRecti eyeRenderViewport[2];
	eyeRenderViewport[0].Pos = OVR::Vector2i(fionaWinConf[0].winw, 0);
	eyeRenderViewport[0].Size = OVR::Sizei(fionaConf.FBOWidth / 2, fionaConf.FBOHeight);
	eyeRenderViewport[1].Pos = OVR::Vector2i(fionaWinConf[0].winw + (fionaConf.FBOWidth + 1) / 2, 0);
	eyeRenderViewport[1].Size = eyeRenderViewport[0].Size;*/
    
	//OVR::Sizei  rtSize(fionaConf.FBOWidth, fionaConf.FBOHeight);

	//fionaConf.oculusEyeTexture[0].OGL.Header.API = ovrRenderAPI_OpenGL;
	//fionaConf.oculusEyeTexture[0].OGL.Header.TextureSize = rtSize;
	//fionaConf.oculusEyeTexture[0].OGL.Header.TextureSize = eyeRenderViewport[0].Size;
	//fionaConf.oculusEyeTexture[1].OGL.Header.TextureSize = eyeRenderViewport[1].Size;

	if(fionaConf.fbo == 0)
	{
		printf("Making oculus frame buffer object.\n");
		glGenFramebuffers(1, &fionaConf.fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fionaConf.fbo);
	
		glGenTextures(1, &fionaConf.img);
		glBindTexture(GL_TEXTURE_2D, fionaConf.img);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, fionaConf.FBOWidth, fionaConf.FBOHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );

        /*GLuint renderBuffer;
        glGenRenderbuffers(1, &renderBuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, renderBuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, fionaConf.FBOWidth, fionaConf.FBOHeight);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderBuffer);*/
 
		glGenTextures(1, &fionaConf.depth);
		glBindTexture(GL_TEXTURE_2D, fionaConf.depth);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, fionaConf.FBOWidth, fionaConf.FBOHeight, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fionaConf.img, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, fionaConf.depth, 0);
		
		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

    //fionaConf.oculusEyeTexture[0].OGL.TexId = fionaConf.img;	//needto set this to our fbo...
    //fionaConf.oculusEyeTexture[1] = fionaConf.oculusEyeTexture[0];
    

	fionaConf.oculusEyeRenderDesc[0] = ovrHmd_GetRenderDesc(fionaConf.oculusHmd, ovrEyeType::ovrEye_Left, fionaConf.oculusHmd->DefaultEyeFov[0]);
	fionaConf.oculusEyeRenderDesc[1] = ovrHmd_GetRenderDesc(fionaConf.oculusHmd, ovrEyeType::ovrEye_Right, fionaConf.oculusHmd->DefaultEyeFov[1]);

	/*ovrGLConfig cfg;
	cfg.OGL.Header.API = ovrRenderAPI_OpenGL;
	cfg.OGL.Header.BackBufferSize = OVR::Sizei(fionaConf.FBOWidth, fionaConf.FBOHeight);
	cfg.OGL.Header.Multisample = 1;
	
	ovrHmd_AttachToWindow( fionaConf.oculusHmd, fionaWinConf[c].window, NULL, NULL );
	
	cfg.OGL.Window = fionaWinConf[c].window;
	cfg.OGL.DC = NULL;*/

	//ovrHmd_ConfigureRendering(fionaConf.oculusHmd, &cfg.Config, ovrDistortionCap_Chromatic | ovrDistortionCap_Vignette | ovrDistortionCap_TimeWarp | ovrDistortionCap_Overdrive, eyeFov, eyeRenderDesc);
 
	/*ovrVector2f UVScaleOffset[2];
	for(int eyeNum = 0; eyeNum < 2; ++eyeNum)
	{
		ovrHmd_GetRenderScaleAndOffset(eyeFov[eyeNum], rtSize, eyeRenderViewport[eyeNum], &UVScaleOffset[eyeNum]);
	}*/

	/*OVR::CAPI::HMDState *ourState = ((OVR::CAPI::HMDState*)fionaConf.oculusHmd->Handle);
	OVR::Profile* def = OVR::ProfileManager::GetInstance()->GetDefaultProfile(ourState->OurHMDInfo.HmdType);
	OVR::HmdRenderInfo ourRenderState = OVR::GenerateHmdRenderInfoFromHmdInfo(ourState->OurHMDInfo, def);
	
	fionaConf.stereoConfig.SetHmdRenderInfo(ourRenderState);
	fionaConf.stereoConfig.SetRendertargetSize(rtSize, true);

	OVR::DistortionRenderDesc d1 = OVR::CalculateDistortionRenderDesc(OVR::StereoEye_Left, ourRenderState, 0);
    OVR::DistortionRenderDesc d2 = OVR::CalculateDistortionRenderDesc(OVR::StereoEye_Right, ourRenderState, 0);*/

	ovrHmd_SetEnabledCaps(fionaConf.oculusHmd, ovrHmdCap_LowPersistence | ovrHmdCap_DynamicPrediction);

	// Start the sensor which informs of the Rift's pose and motion
	ovrHmd_ConfigureTracking(fionaConf.oculusHmd,   ovrTrackingCap_Orientation |
									ovrTrackingCap_MagYawCorrection |
									ovrTrackingCap_Position, 0);

#endif
#ifdef ENABLE_CV1

	// Setup Window and Graphics
	// Note: the mirror window can be any size, for this sample we use 1/2 the HMD resolution
	//ovrSizei windowSize = { fionaConf.hmdDesc.Resolution.w / 2, fionaConf.hmdDesc.Resolution.h / 2 };
	/*if (fionaConf.fbo == 0)
	{
		printf("Making oculus frame buffer object.\n");
		glGenFramebuffers(1, &fionaConf.fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fionaConf.fbo);

		glGenTextures(1, &fionaConf.img);
		glBindTexture(GL_TEXTURE_2D, fionaConf.img);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, fionaConf.FBOWidth, fionaConf.FBOHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		glGenTextures(1, &fionaConf.depth);
		glBindTexture(GL_TEXTURE_2D, fionaConf.depth);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, fionaConf.FBOWidth, fionaConf.FBOHeight, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fionaConf.img, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, fionaConf.depth, 0);

		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
	}*/

	ovr_SetTrackingOriginType(fionaConf.session, ovrTrackingOrigin_EyeLevel);// ovrTrackingOrigin_FloorLevel);

	// Make eye render buffers
	/*for (int eye = 0; eye < 2; ++eye)
	{
		ovrSizei idealTextureSize = ovr_GetFovTextureSize(session, ovrEyeType(eye), hmdDesc.DefaultEyeFov[eye], 1);
		eyeRenderTexture[eye] = new TextureBuffer(session, true, true, idealTextureSize, 1, NULL, 1);
		eyeDepthBuffer[eye] = new DepthBuffer(eyeRenderTexture[eye]->GetSize(), 0);

		if (!eyeRenderTexture[eye]->TextureChain)
		{
			if (retryCreate) goto Done;
			VALIDATE(false, "Failed to create texture.");
		}
	}

	ovrMirrorTextureDesc desc;
	memset(&desc, 0, sizeof(desc));
	desc.Width = windowSize.w;
	desc.Height = windowSize.h;
	desc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;

	// Create mirror texture and an FBO used to copy mirror texture to back buffer
	result = ovr_CreateMirrorTextureGL(session, &desc, &mirrorTexture);
	GLuint texId;
    ovr_GetMirrorTextureBufferGL(session, mirrorTexture, &texId);
	glGenFramebuffers(1, &mirrorFBO);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, mirrorFBO);
	glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texId, 0);
	glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	*/
#endif
#endif
	//printf("c %d window = %p\n", c, fionaWinConf[c].window);
	_FionaUTSyncProjectorCalib(fionaWinConf[c]);
	//printf("window 0: %p\n", fionaWinConf[0].window);

	//printf("%d, %d\n", fionaConf.FBOWidth, fionaConf.FBOHeight);
	/*if(fionaConf.fboSameAsWindow)
	{
		fionaConf.FBOWidth = w;
		fionaConf.FBOHeight = h;
	}*/

#ifdef ENABLE_OCULUS
#ifdef ENABLE_DK2
	
	/*if(ovrHmd_CreateSwapTextureSetGL(fionaConf.oculusHmd, GL_RGBA, fionaConf.FBOWidth, fionaConf.FBOHeight, &fionaConf.pTextureSet) == ovrSuccess)
	{
		printf("Made Oculus DK2 Texture Set\n");
	}

	printf("Viewport 0: %u %u %u %u\n", eyeRenderViewport[0].Pos.x, eyeRenderViewport[0].Pos.y, eyeRenderViewport[0].Size.w, eyeRenderViewport[0].Size.h);
	printf("Viewport 1: %u %u %u %u\n", eyeRenderViewport[1].Pos.x, eyeRenderViewport[1].Pos.y, eyeRenderViewport[1].Size.w, eyeRenderViewport[1].Size.h);
	
	fionaConf.layer.Header.Type      = ovrLayerType_EyeFov;
	fionaConf.layer.Header.Flags     = 0;
	fionaConf.layer.ColorTexture[0]  = fionaConf.pTextureSet;
	fionaConf.layer.ColorTexture[1]  = fionaConf.pTextureSet;
	fionaConf.layer.Fov[0]           = fionaConf.oculusEyeRenderDesc[0].Fov;
	fionaConf.layer.Fov[1]           = fionaConf.oculusEyeRenderDesc[1].Fov;
	fionaConf.layer.Viewport[0]      = eyeRenderViewport[0];//Recti(0, 0,                bufferSize.w / 2, bufferSize.h);
	fionaConf.layer.Viewport[1]      = eyeRenderViewport[1];//Recti(bufferSize.w / 2, 0, bufferSize.w / 2, bufferSize.h);*/
#endif
#endif
	//printf("%d, %d\n", fionaConf.FBOWidth, fionaConf.FBOHeight);
}

void _FionaUTCreateGeneralWindow(const char* name)
{
	std::string n(name);
	fionaCurWindow = fionaNextWindow++;
	int c=fionaCurWindow, w=fionaWinConf[c].winw, h=fionaWinConf[c].winh;
	int x=fionaWinConf[c].winx, y=fionaWinConf[c].winy;
	fionaWinConf[c].walls.push_back(WL_DESK);
	fionaWinConf[c].walls[0].viewports.push_back(VP_DESK);
	fionaWinConf[c].window =__FionaUTCreateWindow(n.c_str(),x,y,w,h,0);

	//printf("%d, %d\n", fionaConf.FBOWidth, fionaConf.FBOHeight);
	/*if(fionaConf.fboSameAsWindow)
	{
		fionaConf.FBOWidth = w;
		fionaConf.FBOHeight = h;
	}*/
	//printf("%d, %d\n", fionaConf.FBOWidth, fionaConf.FBOHeight);
}
