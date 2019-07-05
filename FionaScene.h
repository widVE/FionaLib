//
//  FionaScene.h
//  FionaUT
//
//  Created by Hyun Joon Shin on 6/4/12.
//  Copyright 2012 __MyCompanyName__. All rights reserved.
//


#ifndef __FIONA_SCENE_H__
#define __FIONA_SCENE_H__

// Virtual class for Fiona Scene Modules
#ifdef WIN32
#include <Windows.h>
#endif

#include <GL/glew.h>
#include <GL/gl.h>
#include <Kit3D/jmath.h>

#include "VRAction.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/io.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include "glm/gtx/matrix_interpolation.hpp"

#ifdef ENABLE_VIVE
#include "openvr.h"
#endif

enum WAND_MODE
{
	WAND_NONE=0,
	WAND_MODEL=1,
	WAND_WORLD=2,
	WAND_WORLD_PHYSICS=3,
	KEYBOARD=4,
	WAND_UNITY=5,
	WAND_BOUNDS_PHYSICS=6,
	SECOND_TRACKER_BODY=7,
	CONTROLLER_FREE_MOVE=8
};

class FionaScene
{
public:
	FionaScene(void);
	virtual ~FionaScene();

	virtual void	render(void);
	virtual void	preRender(float value);	//called via frame function callback
	virtual void	postRender(void) {}
	virtual void	buttons(int button, int state);
	virtual void	executeCommand(const char *sCmd);	//generic "command" handling - added here mainly just for VMD
	virtual bool	isRunning(void) const { return true; }
	virtual void	keyboard(unsigned int key, int x, int y);
	virtual void	mouseCallback(int button, int state, int x, int y) {}
	virtual void	mouseMoveCallback(int x, int y) {}
	virtual void	passiveMouseMoveCallback(int x, int y) {}
	virtual void	onExit(void) {}
	virtual void	updateController(unsigned short wB, unsigned char bLT, unsigned char bRT, short sTLX, short sTLY, short sTRX, short sTRY);
	virtual void	updateJoystick(const jvec3& v);
	virtual void	updateLeap(void) {}
	virtual void	updateWand(const jvec3& p, const quat& q);
	virtual void	wallRender(void) {}
	
	static const float cameraMovedThresh;

	bool			cameraMoved(void) const;

	void			getViewDir(jvec3 &viewDir);
	const jvec3 &	getCamPos(void) const { return camPos; }
	const quat &	getCamOri(void) const { return camOri; }

	void			getTrackerWorldSpace(jvec3 &vPos) const;
	void			getSecondTrackerWorldSpace(jvec3 &vPos) const;
	void			getWandWorldSpace(jvec3 &vPos, bool noRot=false) const;
	void			getWandDirWorldSpace(jvec3 &vDir, bool zUp=true, float fWiiFit=0.f) const;

	const jvec3 &	getWandPos(void) const { return wandPos; }
	void			getWandPosWorld(jvec3 &vPos) const;
	const quat &	getWandRot(void) const { return wandOri; }
	void			getWandRotWorldSpace(quat &q, bool zUp=true) const;
	jvec3			getCamPos(){return camPos;}

	enum FionaFrustPlane
	{
		TOP_P=0,
		BOTTOM_P,
		LEFT_P,
		RIGHT_P,
		NEAR_P,
		FAR_P
	};

	float			getFrustumValue(unsigned int whichPlane, int whichWall, int whichViewport);

	WAND_MODE		navMode;

	void			setCamera(const jvec3 &vPos, const quat &q) { camPos = vPos; camOri = q; }
	void			setDrawWand(bool bDraw) { bDrawWand = bDraw; }

protected:
	
	//These three methods where added by Sam Solovy
	//All add command window functionality
	void	setCommandActions();	//Initializes the actions that can be called by the command window
	void	checkCommandQueue();	//Handles the stored inputs from the command window
	void	help(bool checkstate);	//Displays a help prompt in the command window

	bool	buttonDown(unsigned int button) const;
	bool	emgAboveThresh(void) const;
	bool	wiiFitPressed(void) const;

	void	drawViveControllers(const glm::mat4 & mvp);

#ifdef ENABLE_VIVE
	GLuint m_glControllerVertBuffer;
	GLuint m_unControllerVAO;
	unsigned int m_uiControllerVertcount;
	int m_iTrackedControllerCount;
	int m_iTrackedControllerCount_Last;
	GLuint m_unControllerTransformProgramID;
	GLint m_nControllerMatrixLocation;

	vr::TrackedDevicePose_t m_rTrackedDevicePose[vr::k_unMaxTrackedDeviceCount];
	glm::mat4 m_rmat4DevicePose[vr::k_unMaxTrackedDeviceCount];
	bool m_rbShowTrackedDevice[vr::k_unMaxTrackedDeviceCount];
#endif

	bool	bDrawWand;
	float	physicsStep;
	float	physicsTime;
	jvec3	camPos;
	quat	camOri;
	vec4	joystick;
	jvec3	wandPos;
	quat	wandOri;

	jvec3	lastCamPos;
	quat	lastCamOri;

	glm::vec3 angles;
	glm::vec3 position;

	VRActionManager m_actions;
};

#endif
