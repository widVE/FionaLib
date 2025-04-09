
#include "FionaScene.h"
#include "FionaUT.h"
#include "FionaUTVRPN.h"
#include "Kit3D\glslUtils.h"
#include <glm/gtx/rotate_vector.hpp>

#ifdef ENABLE_VIVE
#include "openvr.h"
#endif
const float FionaScene::cameraMovedThresh = 0.01f;

FionaScene::FionaScene(void): navMode(WAND_NONE), bDrawWand(true), physicsStep(0.f), physicsTime(0.f), camPos(0,0,0), camOri(1,0,0,0),
	joystick(0.f,0.f,0.f,0.f), wandPos(0,0,0), wandOri(1,0,0,0), lastCamPos(0,0,0), lastCamOri(1,0,0,0)
{
	VRActionSet *pCommandMode = new VRActionSet();
	pCommandMode->SetName(std::string("command_mode"));
	m_actions.AddSet(pCommandMode);
	setCommandActions();

	position = glm::vec3(0.f,0.f,0.f);
	angles = glm::vec3(0.f, 0.f, 0.f);

	printf("CAM ORI: %f %f %f %f\n", camOri.x, camOri.y, camOri.z, camOri.w);
#ifdef ENABLE_VIVE
	m_glControllerVertBuffer = 0;
	m_unControllerVAO = 0;
	m_uiControllerVertcount = 0;
	m_iTrackedControllerCount = 0;
	m_iTrackedControllerCount_Last = 0;
	m_nControllerMatrixLocation = 0;
	m_unControllerTransformProgramID = 0;
#endif
}

FionaScene::~FionaScene()
{
	
}

void FionaScene::drawViveControllers(const glm::mat4 & mvp)
{
#ifdef ENABLE_VIVE
	if (fionaConf.m_pHMD)
	{
		if (fionaConf.m_pHMD->IsInputFocusCapturedByAnotherProcess())
			return;

		for (int nDevice = 0; nDevice < vr::k_unMaxTrackedDeviceCount; ++nDevice)
		{
			if (m_rTrackedDevicePose[nDevice].bPoseIsValid)
			{
				//m_iValidPoseCount++;
				glm::mat4 matrixObj(
					m_rTrackedDevicePose[nDevice].mDeviceToAbsoluteTracking.m[0][0], m_rTrackedDevicePose[nDevice].mDeviceToAbsoluteTracking.m[1][0], m_rTrackedDevicePose[nDevice].mDeviceToAbsoluteTracking.m[2][0], 0.0,
					m_rTrackedDevicePose[nDevice].mDeviceToAbsoluteTracking.m[0][1], m_rTrackedDevicePose[nDevice].mDeviceToAbsoluteTracking.m[1][1], m_rTrackedDevicePose[nDevice].mDeviceToAbsoluteTracking.m[2][1], 0.0,
					m_rTrackedDevicePose[nDevice].mDeviceToAbsoluteTracking.m[0][2], m_rTrackedDevicePose[nDevice].mDeviceToAbsoluteTracking.m[1][2], m_rTrackedDevicePose[nDevice].mDeviceToAbsoluteTracking.m[2][2], 0.0,
					m_rTrackedDevicePose[nDevice].mDeviceToAbsoluteTracking.m[0][3], m_rTrackedDevicePose[nDevice].mDeviceToAbsoluteTracking.m[1][3], m_rTrackedDevicePose[nDevice].mDeviceToAbsoluteTracking.m[2][3], 1.0f
					);
				//may have to transpose this...
				m_rmat4DevicePose[nDevice] = matrixObj;
			}
		}

		std::vector<float> vertdataarray;

		m_uiControllerVertcount = 0;
		m_iTrackedControllerCount = 0;

		for (vr::TrackedDeviceIndex_t unTrackedDevice = vr::k_unTrackedDeviceIndex_Hmd + 1; unTrackedDevice < vr::k_unMaxTrackedDeviceCount; ++unTrackedDevice)
		{
			if (!fionaConf.m_pHMD->IsTrackedDeviceConnected(unTrackedDevice))
				continue;

			if (fionaConf.m_pHMD->GetTrackedDeviceClass(unTrackedDevice) != vr::TrackedDeviceClass_Controller)
				continue;

			m_iTrackedControllerCount += 1;

			if (!m_rTrackedDevicePose[unTrackedDevice].bPoseIsValid)
				continue;

			const glm::mat4 & mat = m_rmat4DevicePose[unTrackedDevice];

			glm::vec4 center = mat * glm::vec4(0, 0, 0, 1);

			for (int i = 0; i < 3; ++i)
			{
				glm::vec3 color(0, 0, 0);
				glm::vec4 point(0, 0, 0, 1);
				point[i] += 50.0f;  // offset in X, Y, Z
				color[i] = 1.0;  // R, G, B
				point = mat * point;
				vertdataarray.push_back(center.x);
				vertdataarray.push_back(center.y);
				vertdataarray.push_back(center.z);

				vertdataarray.push_back(color.x);
				vertdataarray.push_back(color.y);
				vertdataarray.push_back(color.z);

				vertdataarray.push_back(point.x);
				vertdataarray.push_back(point.y);
				vertdataarray.push_back(point.z);

				vertdataarray.push_back(color.x);
				vertdataarray.push_back(color.y);
				vertdataarray.push_back(color.z);

				m_uiControllerVertcount += 2;
			}

			glm::vec4 start = mat * glm::vec4(0, 0, -0.02f, 1);
			glm::vec4 end = mat * glm::vec4(0, 0, -39.f, 1);
			glm::vec3 color(.92f, .92f, .71f);

			vertdataarray.push_back(start.x); vertdataarray.push_back(start.y); vertdataarray.push_back(start.z);
			vertdataarray.push_back(color.x); vertdataarray.push_back(color.y); vertdataarray.push_back(color.z);

			vertdataarray.push_back(end.x); vertdataarray.push_back(end.y); vertdataarray.push_back(end.z);
			vertdataarray.push_back(color.x); vertdataarray.push_back(color.y); vertdataarray.push_back(color.z);
			m_uiControllerVertcount += 2;
		}

		// Setup the VAO the first time through.
		if (m_unControllerVAO == 0)
		{
			glGenVertexArrays(1, &m_unControllerVAO);
			glBindVertexArray(m_unControllerVAO);

			glGenBuffers(1, &m_glControllerVertBuffer);
			glBindBuffer(GL_ARRAY_BUFFER, m_glControllerVertBuffer);

			GLuint stride = 2 * 3 * sizeof(float);
			GLuint offset = 0;

			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (const void *)offset);

			offset += sizeof(glm::vec3);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (const void *)offset);

			glBindVertexArray(0);

			 
				// vertex shader
			const char *sVert = "#version 410\n"
				"uniform mat4 matrix;\n"
				"layout(location = 0) in vec4 position;\n"
				"layout(location = 1) in vec3 v3ColorIn;\n"
				"out vec4 v4Color;\n"
				"void main()\n"
				"{\n"
				"	v4Color.xyz = v3ColorIn; v4Color.a = 1.0;\n"
				"	gl_Position = matrix * position;\n"
				"}\n";
			const char *sFrag = 
				// fragment shader
				"#version 410\n"
				"in vec4 v4Color;\n"
				"out vec4 outputColor;\n"
				"void main()\n"
				"{\n"
				"   outputColor = v4Color;\n"
				"}\n";

			m_unControllerTransformProgramID = loadProgram(std::string(sVert), std::string(sFrag), true);

			m_nControllerMatrixLocation = glGetUniformLocation(m_unControllerTransformProgramID, "matrix");
		}

		glBindBuffer(GL_ARRAY_BUFFER, m_glControllerVertBuffer);

		// set vertex data if we have some
		if (vertdataarray.size() > 0)
		{
			//$ TODO: Use glBufferSubData for this...
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertdataarray.size(), &vertdataarray[0], GL_STREAM_DRAW);
		}

		glBindBuffer(GL_ARRAY_BUFFER, 0);

		//now draw the lines...
		glUseProgram(m_unControllerTransformProgramID);
		glUniformMatrix4fv(m_nControllerMatrixLocation, 1, GL_FALSE, glm::value_ptr(mvp));
		glBindVertexArray(m_unControllerVAO);
		glDrawArrays(GL_LINES, 0, m_uiControllerVertcount);
		glBindVertexArray(0);
		glUseProgram(0);
	}
#endif
}

void FionaScene::buttons(int button, int state)
{
#ifdef LINUX_BUILD
	if(button == 0 && state == 1)
	{
		camPos.y -= 0.1f;
	}
	else if(button == 2 && state == 1)
	{
		camPos.y += 0.1f;
	}
#endif

	if(m_actions.GetCurrentSet())
	{
		VRAction *action = m_actions.GetCurrentSet()->GetCurrentAction();

		if(action)
		{
			if(action->GetType() == VRAction::WAND)
			{
				VRWandAction *pWandAction = static_cast<VRWandAction*>(action);
				//if we're not still pressing the same actions' buttons
				if(pWandAction->GetButtons() != fionaConf.currentButtons)
				{
					//what to do w/ the previous action in this case?
					if(state == 0 && pWandAction->IsOnRelease())
					{
						pWandAction->ButtonUp();
					}

					//try to find a new action to start
					VRAction *pFound = m_actions.GetCurrentSet()->FindWandAction(fionaConf.currentButtons, true);
					if(pFound)
					{
						if(pFound->GetType() == VRAction::WAND)
						{
							VRWandAction *pNewWand = static_cast<VRWandAction*>(pFound);
							printf("New Action %s\n", pNewWand->GetName().c_str());
							pNewWand->SetScenePtr(this);
							pNewWand->ButtonDown();
						}
					}
				}
				else
				{
					//if the same action - this isn't working because the buttons function doesn't get called repeatedly, instead need to check this in idle...
					printf("Same Action %s\n", pWandAction->GetName().c_str());
					//pWandAction->ButtonDown();
				}
			}
		}
		else
		{
			//try finding a new action...
			VRAction *pFound = m_actions.GetCurrentSet()->FindWandAction(fionaConf.currentButtons, true);
			if(pFound)
			{
				if(pFound->GetType() == VRAction::WAND)
				{
					VRWandAction *pNewWand = static_cast<VRWandAction*>(pFound);
					printf("New Action %s\n", pNewWand->GetName().c_str());
					pNewWand->SetScenePtr(this);
					pNewWand->ButtonDown();
				}
			}
		}

		if(fionaConf.currentButtons==0)
		{
			m_actions.GetCurrentSet()->SetCurrentAction(0);
		}
	}
}

bool FionaScene::buttonDown(unsigned int button) const
{
	return ((fionaConf.currentButtons & (0x1<<button)) != 0);
}

bool FionaScene::emgAboveThresh(void) const
{
	return (fionaConf.emgAvg > fionaConf.emgThresh);
}

void FionaScene::getTrackerWorldSpace(jvec3 &vPos) const
{
	//vPos = camOri.rot(camPos) + camOri.rot(fionaConf.headPos);
	if(!fionaConf.dualView || _FionaUTIsSingleViewMachine())
	{
		vPos = camPos + camOri.rot(fionaConf.headPos);
	}
	else if(_FionaUTIsDualViewMachine())
	{
		getSecondTrackerWorldSpace(vPos);
	}
}

void FionaScene::getSecondTrackerWorldSpace(jvec3 &vPos) const
{
	vPos = camOri.rot(camPos) + camOri.rot(fionaConf.secondHeadPos);
}

void FionaScene::getWandWorldSpace(jvec3 &vPos, bool bNoRot) const
{
	if(bNoRot)
	{
		vPos = camPos + camOri.rot(wandPos);
	}
	else
	{
		vPos = camOri.rot(camPos) + camOri.rot(wandPos);
	}
}

void FionaScene::getWandDirWorldSpace(jvec3 &vDir, bool zUp, float fWiiFit) const
{
	if(zUp)
	{
		vDir = wandOri.rot(-ZAXIS);
	}
	else
	{
		vDir = wandOri.rot(YAXIS);
	}
	
	vDir = camOri.rot(vDir);
	
	vDir = vDir.normalize();

	/*float toDeg = fWiiFit * PI/180;
	mat3 m(cosf(toDeg), 0.f, -sinf(toDeg), 0.f, 1.f, 0.f, sinf(toDeg), 0.f, cosf(toDeg));
	vDir = m.transpose().inv()  * vDir;

	vDir = vDir.normalize();*/
}

void FionaScene::getWandRotWorldSpace(quat &q, bool zUp) const
{
	q = camOri * wandOri;
}


void FionaScene::getWandPosWorld(jvec3 &vPos) const
{
	vPos = camPos + wandPos;
}

void FionaScene::keyboard(unsigned int key, int x, int y)
{
	FionaUTVRPN::currData.keyboard = key;
	if(key == 192)
	{
		if (navMode == CONTROLLER_FREE_MOVE)
		{
			navMode = WAND_WORLD;
		}
		else
		{
			navMode = CONTROLLER_FREE_MOVE;
		}	
	}
}

bool FionaScene::cameraMoved(void) const
{
	jvec3 vMoved = camPos - lastCamPos;
	if((fabs(vMoved.x) > cameraMovedThresh) || (fabs(vMoved.y) > cameraMovedThresh) || (fabs(vMoved.z) > cameraMovedThresh))
	{
		return true;
	}
	else
	{
		if((fabs(camOri.x - lastCamOri.x) > cameraMovedThresh) || (fabs(camOri.y - lastCamOri.y) > cameraMovedThresh) ||
			(fabs(camOri.z - lastCamOri.z) > cameraMovedThresh) || (fabs(camOri.w - lastCamOri.w) > cameraMovedThresh))
		{
			return true;
		}
	}

	return false;
}

void FionaScene::getViewDir(jvec3 &viewDir)
{
	viewDir = fionaConf.headRot.rot(-ZAXIS);
	viewDir = camOri.rot(viewDir);
	viewDir = viewDir.normalize();
}

void FionaScene::preRender(float value)
{
	//printf("PRE RENDER SCENE: %f %f %f %f\n", camOri.x, camOri.y, camOri.z, camOri.w);
	lastCamPos = camPos;
	lastCamOri = camOri;

	//checkCommandQueue();

	bool update = true;
	if(m_actions.GetCurrentSet())
	{
		if(m_actions.GetCurrentSet()->GetName() != std::string("command_mode"))
		{
			VRAction *pCurrAction = m_actions.GetCurrentSet()->GetCurrentAction();
			if(pCurrAction)
			{
				if(pCurrAction->GetType() == VRAction::WAND)
				{
					VRWandAction *pWand = static_cast<VRWandAction*>(pCurrAction);
					//if we're performing an action that requires we don't update movement, well, don't update movement then..
					update = pWand->IsNoMovement();
				}
			}
			else
			{
				//check for joystick idle action.
				VRAction *pJoystick = m_actions.GetCurrentSet()->GetJoystickAction();
				if(pJoystick)
				{
					if(pJoystick->GetType() == VRAction::WAND)
					{
						//if the joystick is being pressed, don't update camera..
						if (fionaConf.currentJoystick != jvec3(0,0,0))
						{
							update = false;
						}
					}
				}
			}
		}
	}

	if((_FionaUTIsCAVEMachine() || fionaConf.appType == FionaConfig::HEADNODE || fionaConf.appType == FionaConfig::DEVLAB || fionaConf.appType == FionaConfig::DEVLAB_WIN8 || fionaConf.playbackFileName.length() > 0) && !fionaConf.noTracking)
	{
		joystick.y = 0.f;
	}

	//printf("%f %f %f %u\n", joystick.x, joystick.y, joystick.z, update);
	//all of this stuff should be handled in updateJoystick in theory...
	if(update)
	{
		switch(navMode)
		{
			case WAND_WORLD:
			{
				//this allows us to fly up and down, etc.
				camOri =exp(YAXIS*-joystick.x*fionaConf.rotationSpeed)*camOri;
				jvec3 wandDir=wandOri.rot(jvec3(-joystick.y*fionaConf.navigationSpeed,0,-joystick.z*fionaConf.navigationSpeed));
				camPos += camOri.rot(wandDir);
				break;
			}
			case WAND_UNITY:
			{
				camOri =exp(YAXIS*joystick.x*fionaConf.rotationSpeed)*camOri;
				jvec3 wandDir=wandOri.rot(jvec3(-joystick.y*fionaConf.navigationSpeed,0,-joystick.z*fionaConf.navigationSpeed));
				camPos += camOri.rot(wandDir);
				break;
			}
			case WAND_MODEL:
			{
				camOri =exp(YAXIS*-joystick.x*fionaConf.rotationSpeed)*camOri;
				//camPos+=jvec3(0,joystick.y*0.01f,joystick.z*fionaConf.navigationSpeed);
				jvec3 wandDir = wandOri.rot(jvec3(-joystick.y*fionaConf.navigationSpeed,0,-joystick.z*fionaConf.navigationSpeed));
				camPos += wandDir;
				break;
			}
			case WAND_WORLD_PHYSICS:
			{
				//stick to the ground..
				if(fionaConf.layeredStereo)
				{
					glm::vec3 eyeOffsetL(fionaConf.kevinOffset.x+fionaConf.lEyeOffset.x, fionaConf.kevinOffset.y+fionaConf.lEyeOffset.y, fionaConf.kevinOffset.z+fionaConf.lEyeOffset.z);
					glm::vec3 eyeOffsetR(fionaConf.kevinOffset.x+fionaConf.rEyeOffset.x, fionaConf.kevinOffset.y+fionaConf.rEyeOffset.y, fionaConf.kevinOffset.z+fionaConf.rEyeOffset.z);
					glm::vec3 currMove(-joystick.y*fionaConf.navigationSpeed,0,-joystick.z*fionaConf.navigationSpeed);
					
					if((glm::length(currMove) > 0.f))
					{
						glm::vec3 rot = glm::rotate(currMove, angles.y, glm::vec3(0.f, 1.f, 0.f));
						position.x += rot.x;
						position.z -= rot.z;
						fionaConf.mvLeft = glm::rotate(glm::mat4(1.0), angles.y, glm::vec3(0.f, 1.f, 0.f));
						fionaConf.mvLeft = glm::translate(fionaConf.mvLeft, -position);
					}
					else if(joystick.x != 0.f)
					{
						float thisAngle = (joystick.x * fionaConf.rotationSpeed);
						printf("%f\n", thisAngle);
						angles.y += thisAngle;
						fionaConf.mvLeft = glm::rotate(glm::mat4(1.0), angles.y, glm::vec3(0.f, 1.f, 0.f));
						fionaConf.mvLeft = glm::translate(fionaConf.mvLeft, -position);
					}
					//fionaConf.mvRight = fionaConf.mvLeft;
					/*currRot = glm::axisAngleMatrix(glm::vec3(0.f, 1.f, 0.f), joystick.x * fionaConf.rotationSpeed);
					orientation *= glm::mat3(currRot);
					fionaConf.mvLeft[0] = glm::vec4(orientation[0], 0.f);
					fionaConf.mvLeft[1] = glm::vec4(orientation[1], 0.f);
					fionaConf.mvLeft[2] = glm::vec4(orientation[2], 0.f);*/
					//fionaConf.mvLeft[3] = glm::vec4(position.x+eyeOffsetL.x, position.y+eyeOffsetL.y, position.z+eyeOffsetL.z, 1.f);
					fionaConf.mvRight = fionaConf.mvLeft;
					//fionaConf.mvRight[3] = glm::vec4(position.x+eyeOffsetR.x, position.y+eyeOffsetR.y, position.z+eyeOffsetR.z, 1.f);
				}
				else
				{
#ifdef ENABLE_OCULUS
#ifdef ENABLE_DK2 
				camOri =exp(YAXIS*-joystick.x*fionaConf.rotationSpeed)*camOri;
				jvec3 wandDir=fionaConf.headRot.rot(jvec3(-joystick.y*fionaConf.navigationSpeed,0,-joystick.z*fionaConf.navigationSpeed));
				jvec3 vRotWandDir = camOri.rot(wandDir);
				vRotWandDir.y = 0.f;
				camPos+=vRotWandDir;
#endif
#ifdef ENABLE_CV1
				camOri = exp(YAXIS*-joystick.x*fionaConf.rotationSpeed)*camOri;
				jvec3 wandDir = fionaConf.headRot.rot(jvec3(-joystick.y*fionaConf.navigationSpeed, 0, -joystick.z*fionaConf.navigationSpeed));
				jvec3 vRotWandDir = camOri.rot(wandDir);
				vRotWandDir.y = 0.f;
				camPos += vRotWandDir;
#endif
#else
					if(fionaConf.deltaInput)
					{
						float fStep = fionaConf.physicsStep;
						camOri =exp(YAXIS*-joystick.x*fionaConf.rotationSpeed*fStep)*camOri;
						jvec3 wandDir=wandOri.rot(jvec3(-joystick.y*fionaConf.navigationSpeed*fStep,0,-joystick.z*fionaConf.navigationSpeed*fStep));
						jvec3 vRotWandDir = camOri.rot(wandDir);
						vRotWandDir.y = 0.f;
						camPos+=vRotWandDir;		
					}
					else
					{
						if (joystick.len() > 0.f)
						{
							//printf("Joystick: %f %f %f %f\n", joystick.x, joystick.y, joystick.z, joystick.h);
							//printf("Cam Ori Before: %f %f %f %f\n", camOri.x, camOri.y, camOri.z, camOri.w);
							camOri = exp(YAXIS*-joystick.x*fionaConf.rotationSpeed)*camOri;
							//printf("Cam Ori: %f %f %f %f\n", camOri.x, camOri.y, camOri.z, camOri.w);
							jvec3 wandDir = wandOri.rot(jvec3(-joystick.y*fionaConf.navigationSpeed, 0, -joystick.z*fionaConf.navigationSpeed));
							//printf("Wand Dir: %f %f %f\n", wandDir.x, wandDir.y, wandDir.z);
							jvec3 vRotWandDir = camOri.rot(wandDir);
							//printf("vRotWandDir: %f %f %f\n", vRotWandDir.x, vRotWandDir.y, vRotWandDir.z);
							vRotWandDir.y = 0.f;
							camPos += vRotWandDir;
							//printf("Joystick: %f %f %f %f\n", joystick.x, joystick.y, joystick.z, joystick.h);
							//printf("Wand Ori: %f %f %f %f\n", wandOri.x, wandOri.y, wandOri.z, wandOri.w);
							//printf("Cam Pos: %f %f %f\n", camPos.x, camPos.y, camPos.z);
						}
					}
#endif
				}
				break;
			}
			case CONTROLLER_FREE_MOVE:
			{
				jvec3 vCam = camOri.rot(XAXIS);
				vCam = vCam.normalize();

				camOri = exp(vCam*joystick.h*fionaConf.rotationSpeed)*camOri;
				camOri = exp(YAXIS*-joystick.x*fionaConf.rotationSpeed)*camOri;

				//jvec3 wandDir=camOri.rot(jvec3(-joystick.y*fionaConf.navigationSpeed,0.f,-joystick.z*fionaConf.navigationSpeed));
				jvec3 wandDir = wandOri.rot(jvec3(-joystick.y*fionaConf.navigationSpeed, 0.f, -joystick.z*fionaConf.navigationSpeed));
				//jvec3 wandDir2=exp(XAXIS).rot(jvec3(-joystick.y*fionaConf.navigationSpeed,0.f,0.f));
				//jvec3 wandDir = exp(ZAXIS*-joystick.z*fionaConf.rotationSpeed)*camOri;
				//wandDir = wandDir.normalize();
				//wandDir = wandDir * (-joystick.z*fionaConf.navigationSpeed);
				//jvec3 vRotWandDir = camOri.rot(wandDir);
				//camPos+=(wandDir);//+wandDir2);
				camPos += camOri.rot(wandDir);
				break;
			}
			case KEYBOARD:
			{
				//if(joystick.z != 0.f)
				//{
				//	printf("joystick updating\n");
				//}
				//printf("updating via keyboard\n");
				camOri= exp(YAXIS*-joystick.x*fionaConf.rotationSpeed) * camOri;
				camPos += camOri.rot(jvec3(-joystick.y*fionaConf.navigationSpeed,0,-joystick.z*fionaConf.navigationSpeed));
				camPos += jvec3(0.f,joystick.y*fionaConf.navigationSpeed,0.f);
				break;
			}
			case WAND_BOUNDS_PHYSICS:
			{
				if(!_FionaUTIsCAVEMachine() && fionaConf.appType != FionaConfig::HEADNODE && fionaConf.appType != FionaConfig::DEVLAB_WIN8 && fionaConf.playbackFileName.length() == 0)
				{
					camOri = exp(YAXIS*-joystick.x*fionaConf.rotationSpeed)*camOri;
				}
				//jvec3 wandDir=wandOri.rot(jvec3(-joystick.y*fionaConf.navigationSpeed,0,-joystick.z*fionaConf.navigationSpeed));
				//jvec3 vRotWandDir = camOri.rot(wandDir);
				//vRotWandDir.y = 0.f;
				//camPos+=vRotWandDir;
				break;
			}
			case SECOND_TRACKER_BODY:
			{
				//may have to flip the head rot by 90 degrees...
				//glm::mat4 rotX = glm::rotate(-90.f, glm::vec3(1.f, 0.f, 0.f));
				//glm::quat q(rotX);
				//quat bodyRot = fionaCof.secondHeadRot * quat(q.w, q.x, q.y, q.z);
				jvec3 vDir = fionaConf.secondHeadRot.rot(-ZAXIS);
				vDir = vDir.normalize();
				vDir.y = 0.f;
				vDir *= (joystick.z*fionaConf.navigationSpeed);
				camPos += vDir;
				break;
			}
			case WAND_NONE:
			default:
				break;
		}
	}

	if(fionaConf.layeredStereo)
	{
		fionaConf.camPos = camPos;
		fionaConf.camRot = camOri;
	}

#ifdef ENABLE_VIVE
	// Process SteamVR events
	vr::VREvent_t event;
	if (fionaConf.m_pHMD == 0)
	{
		printf("Steam not running, shutting down...\n");
		exit(0);
	}

	while (fionaConf.m_pHMD->PollNextEvent(&event, sizeof(event)))
	{
		switch (event.eventType)
		{
			case vr::VREvent_TrackedDeviceActivated:
			{
				//SetupRenderModelForTrackedDevice(event.trackedDeviceIndex);
				printf("Device %u attached. Setting up render model.\n", event.trackedDeviceIndex);
			}
			break;
			case vr::VREvent_TrackedDeviceDeactivated:
			{
				printf("Device %u detached.\n", event.trackedDeviceIndex);
			}
			break;
			case vr::VREvent_TrackedDeviceUpdated:
			{
				printf("Device %u updated.\n", event.trackedDeviceIndex);
			}
			break;
		}
	}

	// Process SteamVR controller state
	for (vr::TrackedDeviceIndex_t unDevice = 0; unDevice < vr::k_unMaxTrackedDeviceCount; unDevice++)
	{
		vr::VRControllerState_t state;
		memset(&state, 0, sizeof(vr::VRControllerState_t));
		if (fionaConf.m_pHMD->GetControllerState(unDevice, &state, sizeof(vr::VRControllerState_t)))
		{
			m_rbShowTrackedDevice[unDevice] = state.ulButtonPressed == 0;

			if ((vr::ButtonMaskFromId(vr::k_EButton_Grip) & state.ulButtonPressed) != 0)
			{
				//printf("Grip pressed!\n");
			}

			if ((vr::ButtonMaskFromId(vr::k_EButton_SteamVR_Touchpad) & state.ulButtonPressed) != 0)
			{
				//printf("Touch pad pressed!\n");
				/*printf("%f %f\n", state.rAxis[0].x, state.rAxis[0].y);
				printf("%f %f\n", state.rAxis[1].x, state.rAxis[1].y);
				printf("%f %f\n", state.rAxis[2].x, state.rAxis[2].y);
				printf("%f %f\n", state.rAxis[3].x, state.rAxis[3].y);
				printf("%f %f\n", state.rAxis[4].x, state.rAxis[4].y);*/
				if (update)
				{
					//if (navMode == WAND_WORLD)
					{
						glm::mat3 m(fionaConf.hmdPose);
						m = glm::transpose(m);
						glm::vec3 vX(state.rAxis[0].x*fionaConf.navigationSpeed, 0, 0);
						glm::vec3 newX = m * vX;
						glm::vec3 vY(0, 0, -state.rAxis[0].y*fionaConf.navigationSpeed);
						glm::vec3 newY = m * vY;
						//newY = glm::normalize(newY);
						//newX = glm::normalize(newX);
						if (navMode == WAND_WORLD_PHYSICS)
						{
							newX.y = 0.f;
							newY.y = 0.f;
						}
						camPos += camOri.rot(jvec3(newX.x, newX.y, newX.z));
						camPos += camOri.rot(jvec3(newY.x, newY.y, newY.z));
					}
				}
			}

			if ((vr::ButtonMaskFromId(vr::k_EButton_SteamVR_Trigger) & state.ulButtonPressed) != 0)
			{
				//printf("Trigger pressed!\n");
				printf("%f %f %f\n", camPos.x, camPos.y, camPos.z);
				printf("%f %f %f %f\n", camOri.x, camOri.y, camOri.z, camOri.w);
			}

			if ((vr::ButtonMaskFromId(vr::k_EButton_SteamVR_Touchpad) & state.ulButtonTouched) != 0)
			{
				//printf("Touch pad touched!\n");
			}

			/*k_EButton_System = 0,
				k_EButton_ApplicationMenu = 1,
				k_EButton_Grip = 2,
				k_EButton_DPad_Left = 3,
				k_EButton_DPad_Up = 4,
				k_EButton_DPad_Right = 5,
				k_EButton_DPad_Down = 6,
				k_EButton_A = 7,

				k_EButton_ProximitySensor = 31,

				k_EButton_Axis0 = 32,
				k_EButton_Axis1 = 33,
				k_EButton_Axis2 = 34,
				k_EButton_Axis3 = 35,
				k_EButton_Axis4 = 36,

				// aliases for well known controllers
				k_EButton_SteamVR_Touchpad = k_EButton_Axis0,
				k_EButton_SteamVR_Trigger = k_EButton_Axis1,

				k_EButton_Dashboard_Back = k_EButton_Grip,

				k_EButton_Max = 64*/
		}
	}

#endif
}

void FionaScene::render(void)
{
	if(fionaConf.layeredStereo)
	{
		/*glm::mat4x4 camPosMat(1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, -camPos.x, -camPos.y, -camPos.z, 1.f);
		quat cr = camOri.inv();
		jvec3 crv = ln(cr)*2.f;
		real cl = len(crv);
		crv=crv.unit();
		glm::mat4x4 camRotMat;
		if(crv.len() > 0.f)
		{
			camRotMat = glm::axisAngleMatrix(glm::vec3(crv.x, crv.y, crv.z), cl*180.f/PI);
		}
		else
		{
			camRotMat = glm::mat4(1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
		}

		switch(navMode)
		{
			case WAND_WORLD:
			case KEYBOARD:
			case WAND_WORLD_PHYSICS:
			case WAND_BOUNDS_PHYSICS:
			case SECOND_TRACKER_BODY:
			case CONTROLLER_FREE_MOVE:
			{
				/*glm::mat4 comb = camPosMat;//glm::mat4(1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);
				//float *rotVals = glm::value_ptr(camRotMat);
				const float *rotVals = glm::value_ptr(camRotMat);
				float *vals = glm::value_ptr(comb);
				vals[0] = rotVals[0];
				vals[1] = rotVals[1];
				vals[2] = rotVals[2];
				vals[4] = rotVals[4];
				vals[5] = rotVals[5];
				vals[6] = rotVals[6];
				vals[8] = rotVals[8];
				vals[9] = rotVals[9];
				vals[10] = rotVals[10];
				
				//comb = camPosMatAdd * comb;
				printf("%f,%f,%f,%f\n%f,%f,%f,%f\n%f,%f,%f,%f\n%f,%f,%f,%f\n", vals[0],vals[1],vals[2],vals[3],vals[4],vals[5],vals[6],vals[7],vals[8],vals[9],vals[10],vals[11],vals[12],vals[13],vals[14],vals[15]);
				fionaConf.mvLeft = glm::make_mat4(vals);*///fionaConf.mvLeft;
				//fionaConf.mvLeft = camPosMat * fionaConf.mvLeft;
				//fionaConf.mvRight = camPosMat * fionaConf.mvRight;
				/*fionaConf.mvLeft = camRotMat * camPosMat * fionaConf.mvLeft;
				fionaConf.mvRight = camRotMat * camPosMat * fionaConf.mvRight;
				break;
			}
			default:
			{
				fionaConf.mvLeft = fionaConf.mvLeft * camRotMat;
				fionaConf.mvRight = fionaConf.mvRight * camRotMat;
				fionaConf.mvLeft = fionaConf.mvLeft * camPosMat;
				fionaConf.mvRight = fionaConf.mvRight * camPosMat;
				break;
			}
		}*/
	}
	else
	{
		switch(navMode)
		{
			case WAND_WORLD:
			case KEYBOARD:
			case WAND_WORLD_PHYSICS:
			case WAND_BOUNDS_PHYSICS:
			case SECOND_TRACKER_BODY:
			case CONTROLLER_FREE_MOVE:
				{
#ifdef ENABLE_OCULUS
#ifdef ENABLE_CV1
					glRotate(fionaConf.headRot.inv());
#endif
#endif
					//glTranslate(fionaCurEyePos);
					glRotate(camOri.inv());
					//glTranslate(-fionaCurEyePos);
					glTranslate(-camPos);
					
					//glTranslate(-(camOri.rot(fionaCurEyePos)));
#ifdef ENABLE_OCULUS
#ifdef ENABLE_CV1
					glTranslate(-(camOri.rot(fionaConf.headPos)));
#endif
#endif
					break;
				}
			default:
				glTranslate(-camPos);
				glRotate(camOri.inv());
				break;
		}

#ifdef ENABLE_VIVE
		wandPos = fionaConf.wandPos;
		glm::mat3 m(fionaConf.hmdPose[0][0], fionaConf.hmdPose[0][1], fionaConf.hmdPose[0][2],
			fionaConf.hmdPose[1][0], fionaConf.hmdPose[1][1], fionaConf.hmdPose[1][2],
			fionaConf.hmdPose[2][0], fionaConf.hmdPose[2][1], fionaConf.hmdPose[2][2]);
		m = glm::transpose(m);
		glm::quat q(m);
		fionaConf.camRot = quat(q.w, q.x, q.y, q.z);
		//camPos = jvec3(fionaConf.hmdPose[3][0], fionaConf.hmdPose[3][1], fionaConf.hmdPose[3][2]);
		//printf("Cam Pos: %f %f %f\n", camPos.x, camPos.y, camPos.z);
#endif
	}
}

void FionaScene::updateController(unsigned short wB, unsigned char bLT, unsigned char bRT, short sTLX, short sTLY, short sTRX, short sTRY)
{
#ifndef LINUX_BUILD
	//printf("Updating controller: %f %f %f %f\n", camOri.x, camOri.y, camOri.z, camOri.w);
	//if(fionaConf.appType == FionaConfig::DEVLAB || fionaConf.appType == FionaConfig::DEVLAB_WIN8 || fionaConf.appType == FionaConfig::WINDOWED || fionaConf.appType == FionaConfig::HEADNODE)
	//{
	//	navMode = CONTROLLER_FREE_MOVE;
	//}


	static const short GAMEPAD_TRIGGER_THRESHOLD = 30;
	static const short LEFT_THUMB_DEADZONE = 7849;
	static const short RIGHT_THUMB_DEADZONE = 8689;

	//tap into action system as well..
	//if(m_actions.GetCurrentSet() == 0)
	//{
	//printf("%d %d %d %d\n", sTLX, sTLY, sTRX, sTRY);
	joystick = vec4(0.f, 0.f, 0.f, 0.f);
	if(sTRX < -RIGHT_THUMB_DEADZONE || sTRX > RIGHT_THUMB_DEADZONE)
	{
		joystick.x = (float)sTRX / (float)SHRT_MAX;
		//camOri = exp(YAXIS*-fVal*fionaConf.rotationSpeed)*camOri;  
	}

	if(sTRY < -RIGHT_THUMB_DEADZONE || sTRY > RIGHT_THUMB_DEADZONE)
	{
		joystick.h = (float)sTRY / (float)SHRT_MAX;
		//camOri = exp(XAXIS*-joystick.h*fionaConf.rotationSpeed) * camOri;
	}

	if(sTLX < -LEFT_THUMB_DEADZONE || sTLX > LEFT_THUMB_DEADZONE)
	{
		joystick.y = (float)-sTLX / (float)SHRT_MAX;;
		//camPos += camOri.rot(jvec3(fVal * fionaConf.navigationSpeed, 0.f, 0.f));
	}

	if(sTLY < -LEFT_THUMB_DEADZONE || sTLY > LEFT_THUMB_DEADZONE)
	{
		joystick.z = (float)sTLY / (float)SHRT_MAX;
		//camPos += camOri.rot(jvec3(0.f, 0.f, -fVal * fionaConf.navigationSpeed));
	}

	//_FionaUTJoystick(0, joystick);

	//TRIGGERS:
	if (bLT > GAMEPAD_TRIGGER_THRESHOLD)	{ /*L-TRIGGER*/	}
	if (bRT > GAMEPAD_TRIGGER_THRESHOLD)	{ /*R-TRIGGER*/	}

	//BUTTONS:
	if (wB & 0x0001)			{ /*DPAD-UP*/ }
	if ((wB & 0x0002) >> 1)		{ /*DPAD-DOWN*/ }
	if ((wB & 0x0004) >> 2)		{ /*DPAD-LEFT*/ }
	if ((wB & 0x0008) >> 3)		{ /*DPAD-RIGHT*/ }
	if ((wB & 0x0010) >> 4)		{ /*START*/ }
	if ((wB & 0x0020) >> 5)		{ /*SELECT*/ }
	if ((wB & 0x0040) >> 6)		{ /*L-CLICK*/ }
	if ((wB & 0x0080) >> 7)		{ /*R-CLICK*/ }
	if ((wB & 0x0100) >> 8)		{ /*L-BUMPER*/ }
	if ((wB & 0x0200) >> 9)		{ /*R-BUMPER*/ }
	if ((wB & 0x1000) >> 12)	{ /*A*/ }
	if ((wB & 0x2000) >> 13)	{ /*B*/ }
	if ((wB & 0x4000) >> 14)	{ /*X*/ }
	if ((wB & 0x8000) >> 15)	{ /*Y*/ }
	//}
#endif

	//printf("End Updating controller: %f %f %f %f\n", camOri.x, camOri.y, camOri.z, camOri.w);
}

void FionaScene::updateJoystick(const vec4& v)
{ 
	//printf("Updating joystick: %f %f %f %f\n", camOri.x, camOri.y, camOri.z, camOri.w);
	if(m_actions.GetCurrentSet() == 0)
	{
		joystick = vec4(v.x, v.y, v.z, v.h);
		return;
	}

	if(m_actions.GetCurrentSet()->GetName() == std::string("command_mode"))
	{
		joystick = vec4(v.x, v.y, v.z, v.h);
		return;
	}
	
	//printf("updating joystick\n");
	float fLen = joystick.len();
	bool joystickWasPressed = (fLen != 0.f);

	float currLen = v.len();
	bool joystickNowPressed = (currLen > 0.01f);

	VRAction *pCurrAction = m_actions.GetCurrentSet()->GetCurrentAction();

	if(pCurrAction)
	{
		if(pCurrAction->GetType() == VRAction::WAND)
		{
			VRWandAction *pWand = static_cast<VRWandAction*>(pCurrAction);
			if(!pWand->IsNoMovement())
			{
				joystick = vec4(v.x, v.y, v.z, v.h);
			}
			
			if(joystickWasPressed)
			{
				if(joystickNowPressed)
				{
					pWand->JoystickMove();
				}
				else
				{
					pWand->JoystickStop();
					
				}
			}
			else
			{
				pWand->JoystickStart();	
			}
		}
	}
	else
	{
		joystick = vec4(v.x, v.y, v.z, v.h); 
		//for testing locally windowed machine...
		VRAction *pJoystickAction = m_actions.GetCurrentSet()->GetJoystickAction();
		if(pJoystickAction != 0)
		{
			if(pJoystickAction->GetType() == VRAction::WAND)
			{
				VRWandAction *pWand = static_cast<VRWandAction*>(pJoystickAction);
				pWand->SetScenePtr(this);
				pWand->JoystickMove();
			}
		}
	}
}

void FionaScene::updateWand(const jvec3& p, const quat& q)
{ 
	if (navMode == CONTROLLER_FREE_MOVE)
	{
		wandPos = fionaConf.secondHeadPos;
		wandOri = fionaConf.secondHeadRot;
	}
	else
	{
		wandPos = p;
		wandOri = q;
	}
	
	if(fionaConf.wandView)
	{
		if( fionaConf.monitorFiltering )
		{
			/*float err = powf(_FionaUTMonitorDistance(p,q),filterExponent);
			fionaConf.monitorCamPos=_FionaUTFiltarP(p,err);
			fionaConf.monitorCamOri=_FionaUTFiltarQ(q,err);*/
		}
		else
		{
			fionaConf.monitorCamPos = p;
			fionaConf.monitorCamOri = q;
		}
	}

	if(m_actions.GetCurrentSet() == 0)
	{
		return;
	}

	if(m_actions.GetCurrentSet()->GetName() == std::string("command_mode"))
	{
		return;
	}

	VRAction *pCurrAction = m_actions.GetCurrentSet()->GetCurrentAction();
	if(pCurrAction)
	{
		if(pCurrAction->GetType() == VRAction::WAND)
		{
			VRWandAction *pWand = static_cast<VRWandAction*>(pCurrAction);
			pWand->WandMove();
			//should we update the joystick here too - this way we can get continuous joystick movement..
			pWand->JoystickMove();
		}
	}
	else
	{
		VRAction *pJoystick = m_actions.GetCurrentSet()->GetJoystickAction();
		if(pJoystick)
		{
			if(pJoystick->GetType() == VRAction::WAND)
			{
				VRWandAction *pWand = static_cast<VRWandAction*>(pJoystick);
				pWand->SetScenePtr(this);
				pWand->JoystickMove();
			}
		}
	}
}

bool FionaScene::wiiFitPressed(void) const
{
	return (fionaConf.wiiTopLeft != 0.f || fionaConf.wiiBottomLeft != 0.f || fionaConf.wiiBottomRight != 0.f || fionaConf.wiiTopRight != 0.f);
}

void FionaScene::setCommandActions()
{
	VRActionSet *currentSet = m_actions.GetCurrentSet();
	m_actions.SetCurrentSet("command_mode");

	VRChangeNavSpeed *pNavSpeed = new VRChangeNavSpeed();
	pNavSpeed->SetScenePtr(this);
	m_actions.GetCurrentSet()->AddAction(pNavSpeed);

	VRChangeRotSpeed *pRotSpeed = new VRChangeRotSpeed();
	pRotSpeed->SetScenePtr(this);
	m_actions.GetCurrentSet()->AddAction(pRotSpeed);

	VRChangeFarPlane *pFarPlane = new VRChangeFarPlane();
	pFarPlane->SetScenePtr(this);
	m_actions.GetCurrentSet()->AddAction(pFarPlane);

	VRChangeNearPlane *pNearPlane = new VRChangeNearPlane();
	pNearPlane->SetScenePtr(this);
	m_actions.GetCurrentSet()->AddAction(pNearPlane);

	//VRChangeDualView is not compatable yet, need to implement hardware changes
	/*VRChangeDualView *pDuelView = new VRChangeDualView();
	pDuelView->SetScenePtr(this);
	m_actions.GetCurrentSet()->AddAction(pDuelView);*/

	VRChangeDisplayGMem *pDisGMem = new VRChangeDisplayGMem();
	pDisGMem->SetScenePtr(this);
	m_actions.GetCurrentSet()->AddAction(pDisGMem);

	VRChangeRiftTracking *pRiftTrack = new VRChangeRiftTracking();
	pRiftTrack->SetScenePtr(this);
	m_actions.GetCurrentSet()->AddAction(pRiftTrack);

	VRChangeShowRenderTime *pShowRenderTime = new VRChangeShowRenderTime();
	pShowRenderTime->SetScenePtr(this);
	m_actions.GetCurrentSet()->AddAction(pShowRenderTime);

	VRChangeRenderHead *pChangeRenderHead = new VRChangeRenderHead();
	pChangeRenderHead->SetScenePtr(this);
	m_actions.GetCurrentSet()->AddAction(pChangeRenderHead);

	VRChangeMode *pChangeMode = new VRChangeMode();
	pChangeMode->SetScenePtr(this);
	m_actions.GetCurrentSet()->AddAction(pChangeMode);

	VRChangeBackGround *pChangeBackGround = new VRChangeBackGround();
	pChangeBackGround->SetScenePtr(this);
	m_actions.GetCurrentSet()->AddAction(pChangeBackGround);

	//VRChangeFrameRate is not compatable yet, fionaconf.framerate doesnt affect FPS
	/*VRChangeFrameRate *pChangeFrameRate = new VRChangeFrameRate();
	pChangeFrameRate->SetScenePtr(this);
	m_actions.GetCurrentSet()->AddAction(pChangeFrameRate);*/

	VRChangeRightEye *pChangeRightEye = new VRChangeRightEye();
	pChangeRightEye->SetScenePtr(this);
	m_actions.GetCurrentSet()->AddAction(pChangeRightEye);

	VRChangeLeftEye *pChangeLeftEye= new VRChangeLeftEye();
	pChangeLeftEye->SetScenePtr(this);
	m_actions.GetCurrentSet()->AddAction(pChangeLeftEye);

	VRChangeKevinOffset *pChangeKevin = new VRChangeKevinOffset();
	pChangeKevin->SetScenePtr(this);
	m_actions.GetCurrentSet()->AddAction(pChangeKevin);

	VRChangeSensorOffest *pChangeSensorOffset = new VRChangeSensorOffest();
	pChangeSensorOffset->SetScenePtr(this);
	m_actions.GetCurrentSet()->AddAction(pChangeSensorOffset);

	m_actions.SetCurrentSet(currentSet);
}

void FionaScene::help(bool checkstate)
{
	puts(".....\n");
	if(checkstate)
		puts("Current State:");
	else
	{
		puts("Valid Actions:");
		puts("Letters count as 0 if used for a number parameter.");
	}
	std::vector<VRAction*> *actions = m_actions.GetCurrentSet()->GetActions();
	for(unsigned int i = 0; i < actions->size(); i++)
	{
		if(checkstate)
		printf("%s = %s\n", actions->at(i)->GetName().c_str(), actions->at(i)->getValue().c_str());
		else
			printf("%s\n", actions->at(i)->GetName().c_str());
	}
	puts(".....\n");
}

//Handles the command queue where actions are inputed from the terminal.
//Commands are pushed from FionaUTWin32.cpp
////The correct format for commands is action=value
void FionaScene::checkCommandQueue()
{
	std::string sCommand, sAction, sValue;

	if(!fionaCommandQueue.empty())
	{
		sCommand = fionaCommandQueue.front();
		fionaCommandQueue.pop();
		VRActionSet *currentSet = m_actions.GetCurrentSet();
		bool noCurrent = false;
		if(currentSet == 0)
		{
			noCurrent = true;
		}
		m_actions.SetCurrentSet("command_mode");		//Only looks at the command action set, and sets it
#ifndef LINUX_BUILD
		sCommand.erase(std::remove(sCommand.begin(), sCommand.end(), ' '), sCommand.end());
#endif
		sAction = sCommand.substr(0,sCommand.find("="));//as the active set until the the code is done checking
#ifndef LINUX_BUILD
		std::transform(sAction.begin(), sAction.end(), sAction.begin(), ::tolower); //the command queue
#endif
		if(sAction == "help" || sAction == "state")
		{
			help(true);
			m_actions.SetCurrentSet(currentSet);
			return;
		}
		else
		{
			sValue = sCommand.substr(sCommand.find("=")+1, sCommand.npos);

#ifdef _DEBUG
		printf("Action Popped: %s\n", sAction.c_str());
		printf("Action Set: %s\n",m_actions.GetCurrentSet()->GetName().c_str());
#endif
		
			VRConsoleAction *command = m_actions.GetCurrentSet()->FindConsoleAction(sAction);
			if(command !=0 )
			{
#ifdef _DEBUG
				printf("Command: %s\n", command->GetName().c_str());
				printf("Value: %s\n", sValue.c_str());
#endif

				bool b = command->Trigger(sValue);
				if(b)
				{
					//Send string sCommand to slaves 
					if(fionaNetMaster)
					{
						//SEND DATA
						_FionaUTSyncSendVMD(sCommand.c_str(), sCommand.size());
					}	
				}
				m_actions.GetCurrentSet()->SetCurrentAction(0);
			}		
			else
			{
				printf("Action: %s is invalid\n", sAction.c_str());
				help(false);
			}
		}

		if(noCurrent)
		{
			m_actions.SetCurrentSet(0);
		}
	}
}

void FionaScene::executeCommand(const char *sCmd)
{
	fionaCommandQueue.push(sCmd);
}

//t = (vu * vc) * n/d; where n is normal of wall, d is -(vn dot va (va is vector from eye to corner of wall)
//vu is local space up vector of wall and vc is vector from eye to upper corner of wall) - see FionaWall / FionaViewport
float FionaScene::getFrustumValue(unsigned int whichPlane, int whichWall, int whichViewport)
{
	FionaWall &wall = fionaWinConf[0].walls[whichWall];
	FionaViewport &vp = wall.viewports[whichViewport];

	jvec3 vWallLB = camOri.rot(wall.vLB) + camPos;
	jvec3 vWallLT = camOri.rot(wall.vLT) + camPos;
	jvec3 vWallRB = camOri.rot(wall.vRB) + camPos;

	jvec3 vWallUp = (vWallLT - vWallLB);
	jvec3 vWallRight = (vWallRB - vWallLB);
	
	jvec3 vWallUpNorm = vWallUp;
	vWallUpNorm = vWallUpNorm.normalize();
	jvec3 vWallRightNorm = vWallRight;
	vWallRightNorm = vWallRightNorm.normalize();

	jvec3 vWallNorm = vWallRightNorm * vWallUpNorm;
	vWallNorm = vWallNorm.normalize();

	jvec3 vProjLB = vWallLB + vWallUp * vp.sy;
	jvec3 vProjLT = vProjLB + vWallUp * vp.sh;
	jvec3 vProjRB = vProjLB + vWallRight * vp.sw;
	jvec3 vProjRT = vProjRB + vWallUp * vp.sh;
	jvec3 vExactEye = camPos + camOri.rot(fionaConf.headPos);
	
	jvec3 vToLB = (vProjLB - vExactEye).normalize();
	jvec3 vToLT = (vProjLT - vExactEye).normalize();
	jvec3 vToRB = (vProjRB - vExactEye).normalize();
	jvec3 vToRT = (vProjRT - vExactEye).normalize();

	jvec3 vToT = (((vProjLT + vProjRT) * 0.5f) - vExactEye).normalize();
	jvec3 vToR = (((vProjRT + vProjRB) * 0.5f) - vExactEye).normalize();

	float d = -(vWallNorm % vToT);
	float d2 = -(vWallNorm % vToR);

	float value = 0.f;// fionaConf.nearClip / d;
	//float value2 = fionaConf.nearClip / d2;

	switch(whichPlane)
	{
		case TOP_P:
		{
			value = acos(d);// (vWallUp % vToT) * value;// vToLT) * value;
			//printf("D: %f, Degrees: %f\n", d, value * 360.f / 3.14159f);
			break;
		}
		case BOTTOM_P:
		{
			value = acos(d);// (vWallUp % vToLB) * value;
			break;
		}
		case LEFT_P:
		{
			value = acos(d2);// (vWallRight % vToLB) * value2;
			break;
		}
		case RIGHT_P:
		{
			value = acos(d2);// (vWallRight % vToRB) * value2;
			break;
		}
		case NEAR_P:
		{
			break;
		}
		case FAR_P:
		{
			break;
		}
	}

	return value;
}
