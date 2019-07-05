#include "VRAction.h"

#include "FionaUT.h"

VRActionManager::~VRActionManager()
{
	ClearSets();
}

void VRActionManager::ClearSets(void)
{
	for(unsigned int i = 0; i < m_actionSets.size(); ++i)
	{
		delete m_actionSets[i];
	}

	m_actionSets.clear();
}

void VRActionManager::SetCurrentSet(const std::string &sName)
{
	for(unsigned int i = 0; i < m_actionSets.size(); ++i)
	{
		if(m_actionSets[i]->GetName() == sName)
		{
			m_pCurrentSet = m_actionSets[i];
			break;
		}
	}
}

void VRActionManager::ReadActions()
{


}

void VRActionSet::ClearActions(void)
{
	for(unsigned int i = 0; i < m_actions.size(); ++i)
	{
		delete m_actions[i];
	}

	m_actions.clear();
}

void VRActionSet::IdleDrawCallback(void)
{
	if(m_pDrawAction != 0 && (m_pDrawAction->GetType() == 0))
	{ 
		static_cast<VRWandAction*>(m_pDrawAction)->DrawCallback(); 
	}
}

VRAction *	VRActionSet::FindWandAction(int buttons, bool bSet, bool bRelease)
{
	for(unsigned int i = 0; i < m_actions.size(); ++i)
	{
		if(m_actions[i]->GetType() == VRAction::WAND)
		{
			VRWandAction *wand = static_cast<VRWandAction*>(m_actions[i]);
			if((wand->GetButtons() == buttons)/* && (wand->IsOnRelease() == bRelease)*/)
			{
				if(bSet)
				{
					m_pCurrentAction = m_actions[i];
				}
				return m_actions[i];
			}
		}
	}

	return 0;
}

std::vector<VRAction*> * VRActionSet::GetActions()
{
	return &m_actions;
}

VRConsoleAction * VRActionSet::FindConsoleAction(std::string sName)
{
#ifndef LINUX_BUILD
	std::transform(sName.begin(), sName.end(), sName.begin(), ::tolower);
	for(unsigned int i = 0; i < m_actions.size(); ++i)
	{
		if(m_actions[i]->GetType() == VRAction::CONSOLE)
		{
			VRConsoleAction *console = static_cast<VRConsoleAction*>(m_actions[i]);
			std::string sCommandName = console->GetName();
			std::transform(sCommandName.begin(), sCommandName.end(), sCommandName.begin(), ::tolower);
			if(sCommandName.compare(sName) == 0)
			{
				//m_pCurrentAction = m_actions[i];
				return console;
			}
		}
	}
#endif
	return 0;
}

bool VRChangeNavSpeed::Trigger(std::string value)
{
		float fSpeed = strtod(value.c_str(), NULL);
		if(fSpeed >=0.0f && value!="")
		{
			fionaConf.navigationSpeed = fSpeed;
			printf("Successful triggered, %s =  %s\n", this->GetName().c_str(), this->getValue().c_str());
			return true;
		}
		else
		{
			puts(this->getDiscription().c_str());
		}
	return false;
}

std::string VRChangeNavSpeed::getValue()
{
	std::ostringstream buffer;
	buffer << fionaConf.navigationSpeed;
	return  buffer.str();
}

std::string VRChangeNavSpeed::getDiscription()
{
	std::string s;
	s.append("USAGE::\t NavSpeed = \"number\"");
	s.append("\n Changes the Navigation Speed"); 
	return s;
}

bool VRChangeRotSpeed::Trigger(std::string value)
{
		float fSpeed = strtod(value.c_str(), NULL);
		if(fSpeed >=0.0f && value!="")
		{
			fionaConf.rotationSpeed = fSpeed;
			printf("Successful triggered, %s =  %s\n", this->GetName().c_str(), this->getValue().c_str());
			return true;
		}
		else
		{
			puts(this->getDiscription().c_str());
		}
	return false;
}

std::string VRChangeRotSpeed::getValue()
{
	std::ostringstream buffer;
	buffer << fionaConf.rotationSpeed;
	return  buffer.str();
}

std::string VRChangeRotSpeed::getDiscription()
{
	std::string s;
	s.append("USAGE::\t RotSpeed = \"number\"");
	s.append("\n Changes the Rotation Speed"); 
	return s;
}


bool VRChangeFarPlane::Trigger(std::string value)
{
		float fFarClip = strtod(value.c_str(), NULL);
		if(fFarClip >fionaConf.nearClip)
		{
			fionaConf.farClip = fFarClip;
			printf("Successful triggered, %s =  %s\n", this->GetName().c_str(), this->getValue().c_str());
			return true;
		}
		else
		{
			puts(this->getDiscription().c_str());
		}
	return false;
}

std::string VRChangeFarPlane::getValue()
{
	std::ostringstream buffer;
	buffer << fionaConf.farClip;
	return  buffer.str();
}

std::string VRChangeFarPlane::getDiscription()
{
	std::string s;
	s.append("USAGE::\t FarPlane = \"number\"");
	s.append("\n Changes Far Plane distance, \ncannnot be smaller than Near Plane"); 
	return s;
}

bool VRChangeNearPlane::Trigger(std::string value)
{
		float fNearClip = strtod(value.c_str(), NULL);
		if(fNearClip < fionaConf.farClip && fNearClip > 0.f )
		{
			fionaConf.nearClip = fNearClip;
			printf("Successful triggered, %s =  %s\n", this->GetName().c_str(), this->getValue().c_str());
			return true;
		}
		else
		{
			puts(this->getDiscription().c_str());
		}
		return false;
}

std::string VRChangeNearPlane::getValue()
{
	std::ostringstream buffer;
	buffer << fionaConf.nearClip;
	return  buffer.str();
}

std::string VRChangeNearPlane::getDiscription()
{
	std::string s;
	s.append("USAGE::\t NearPlane = \"number\"");
	s.append("\n Changes Near Plane distance, \ncannnot be bigger than Near Plane");
	s.append("\n and cannot be equal to or less than 0");
	return s;
}

bool VRChangeDualView::Trigger(std::string sValue)
{
		bool b = boolTiggerHandler(this->GetName(), this->getDiscription(), sValue, fionaConf.dualView);
		if(fionaConf.dualView!= b)
		{
			fionaConf.dualView = b;
			printf("Successful triggered, %s =  %s\n", this->GetName().c_str(), this->getValue().c_str());
			return true;
		}
		return false;
}

std::string VRChangeDualView::getValue()
{
	return boolHandler(fionaConf.dualView);
}

std::string VRChangeDualView::getDiscription()
{
	std::string s;
	s.append("USAGE::\t DualView = \"boolean\"");
	s.append("\n Changes if Dual View is enabled");
	return s;
}

bool VRChangeShowRenderTime::Trigger(std::string sValue)
{
		bool b = boolTiggerHandler(this->GetName(), this->getDiscription(), sValue, fionaConf.showRenderTime);
		if(fionaConf.showRenderTime!= b)
		{
			fionaConf.showRenderTime = b;
			printf("Successful triggered, %s =  %s\n", this->GetName().c_str(), this->getValue().c_str());
			return true;
		}
		return false;
}

std::string VRChangeShowRenderTime::getValue()
{
	return boolHandler(fionaConf.showRenderTime);
}

std::string VRChangeShowRenderTime::getDiscription()
{
	std::string s;
	s.append("USAGE::\t ShowRenderTime = \"boolean\"");
	s.append("\n Changes if the FPS is displayed in the command window title or not at all");
	return s;
}

bool VRChangeDisplayGMem::Trigger(std::string sValue)
{
		bool b = boolTiggerHandler(this->GetName(), this->getDiscription(), sValue, fionaConf.displayGraphicsMem);
		if(fionaConf.displayGraphicsMem!= b)
		{
			fionaConf.displayGraphicsMem = b;
			printf("Successful triggered, %s =  %s\n", this->GetName().c_str(), this->getValue().c_str());
		}
		return false;
}

std::string VRChangeDisplayGMem::getValue()
{
	return boolHandler(fionaConf.displayGraphicsMem);
}

std::string VRChangeDisplayGMem::getDiscription()
{
	std::string s;
	s.append("USAGE::\t DisplayGMem");
	s.append("\n Outputs the Graphic Memory usage to standard output");
	return s;
}

bool VRChangeRiftTracking::Trigger(std::string sValue)
{		
		bool b = boolTiggerHandler(this->GetName(), this->getDiscription(), sValue, fionaConf.riftTracking);
		if(fionaConf.riftTracking!= b)
		{
			fionaConf.riftTracking = b;
			printf("Successful triggered, %s =  %s\n", this->GetName().c_str(), this->getValue().c_str());
			return true;
		}
		return false;	
}

std::string VRChangeRiftTracking::getValue()
{
	return boolHandler(fionaConf.riftTracking);
}

std::string VRChangeRiftTracking::getDiscription()
{
	std::string s;
	s.append("USAGE::\t RiftTracking = \"boolean\"");
	s.append("\n Changes if Rift Tracking is enabled");
	return s;
}

bool VRChangeRenderHead::Trigger(std::string sValue)
{		
		bool b = boolTiggerHandler(this->GetName(), this->getDiscription(), sValue, fionaConf.renderHeadNode);
		if(fionaConf.renderHeadNode!= b)
		{
			fionaConf.renderHeadNode = b;
			printf("Successful triggered, %s =  %s\n", this->GetName().c_str(), this->getValue().c_str());
			return true;
		}
	return false;
}

std::string VRChangeRenderHead::getValue()
{
	return boolHandler(fionaConf.renderHeadNode);
}

std::string VRChangeRenderHead::getDiscription()
{
	std::string s;
	s.append("USAGE::\t RenderHead = \"boolean\"");
	s.append("\n Changes if the head node is rendered");
	return s;
}

std::string VRConsoleAction::boolHandler(bool b)
{
	std::ostringstream buffer;
	buffer << b;
	if(buffer.str()=="0")
		return "false";
	else if(buffer.str()=="1")
		return "true";
	else
		return "error";
}

bool VRConsoleAction::boolTiggerHandler(std::string name, const std::string discription, std::string sValue, bool oldValue)
{
#ifndef LINUX_BUILD
	std::transform(sValue.begin(), sValue.end(), sValue.begin(), ::tolower);
	std::transform(name.begin(), name.end(), name.begin(), ::tolower);
#endif
	if(sValue == name)
	{
		return !oldValue;
	}
	else if(sValue == "false")
	{
		return false;
	}
	else if(sValue == "true")
	{
		return true;
	}
	else
	{
		puts(discription.c_str());
		return oldValue;
	}
}

bool VRChangeMode::Trigger(std::string sValue)
{
#ifndef LINUX_BUILD
	std::transform(sValue.begin(), sValue.end(), sValue.begin(), ::toupper);
#endif
	if ( sValue == "WAND_NONE")
		{
			this->m_scene->navMode = WAND_NONE;
			printf("Navigation Mode: %s\n", sValue.c_str());
			return true;
		}
		else if ( sValue == "WAND_MODEL")
		{
			this->m_scene->navMode = WAND_MODEL;
			printf("Navigation Mode: %s\n", sValue.c_str());
			return true;
		}
		else if ( sValue == "WAND_WORLD")
		{
			this->m_scene->navMode = WAND_WORLD;
			printf("Navigation Mode: %s\n", sValue.c_str());
			return true;
		}
		else if ( sValue == "WAND_WORLD_PHYSICS")
		{
			this->m_scene->navMode = WAND_WORLD_PHYSICS;
			printf("Navigation Mode: %s\n", sValue.c_str());
			return true;
		}
		else if ( sValue == "KEYBOARD")
		{
			this->m_scene->navMode = KEYBOARD;
			printf("Navigation Mode: %s\n", sValue.c_str());
			return true;
		}
		else
		{
			puts(getDiscription().c_str());
			return false;
		}
}

std::string VRChangeMode::getValue()
{
		if(this->m_scene->navMode == 0)
		{
			return "WAND_NONE";
		}
		else if (this->m_scene->navMode == 1)
		{
			return "WAND_MODEL";
		}
		else if (this->m_scene->navMode == 2)
		{
			return "WAND_WORLD";
		}
		else if (this->m_scene->navMode == 3)
		{
			return "WAND_WORLD_PHYSICS";
		}
		else if (this->m_scene->navMode == 4)
		{
			return "KEYBOARD";
		}
		else
		{
			return "ERROR IN NAVIGATION MODE";
		}
}

std::string VRChangeMode::getDiscription()
{
	std::string s;
	s.append("USAGE::\t MODE  = \"WAND_MODE\"");
	s.append("\n Changes the WAND_MODE, valid WAND_MODEs are \n");
	s.append("\n\tWAND_NONE, WAND_MODEL, WAND_WORLD,\n");
	s.append("\n\tWAND_WORLD_PHYSICS, KEYBOARD,\n");
	return s;
}

bool VRChangeFrameRate::Trigger(std::string value)
{
		float fFrameRate = strtod(value.c_str(), NULL);
		if(fFrameRate >=0.0f && value!="")
		{
			fionaConf.framerate = fFrameRate;
			printf("Successful triggered, %s =  %s\n", this->GetName().c_str(), this->getValue().c_str());
			return true;
		}
		else
		{
			puts(this->getDiscription().c_str());
		}
	return false;
}

std::string VRChangeFrameRate::getValue()
{
	std::ostringstream buffer;
	buffer << fionaConf.framerate;
	return  buffer.str();
}

std::string VRChangeFrameRate::getDiscription()
{
	std::string s;
	s.append("USAGE::\t FrameRate = \"number\"");
	s.append("\n Changes the desired Frame Rate"); 
	return s;
}

bool VRChangeBackGround::Trigger(std::string value)
{
	if(value.substr(0, value.find(",")).c_str() != value)
	{
		jvec3 j = jvec3TriggerHandler(value);
		if( j.x>1.0f ||  j.y>1.0f ||  j.z>1.0f ||j.x<0.0f || j.y<0.0f || j.z<0.0f)
		{
				puts(this->getDiscription().c_str());
				return false;
		}
		fionaConf.backgroundColor = glm::vec3(j.x, j.y, j.z);
		printf("Successful triggered, %s =  %s\n", this->GetName().c_str(), this->getValue().c_str());
		return true;
		}
		else
		{
			puts(this->getDiscription().c_str());
		}
	return false;
}

std::string VRChangeBackGround::getValue()
{
	std::ostringstream buffer;
	buffer << fionaConf.backgroundColor;
	return  buffer.str();
}

std::string VRChangeBackGround::getDiscription()
{
	std::string s;
	s.append("USAGE::\t Background = \"r, g, b,\"");
	s.append("\n\t r, g, b have to be greater than 0.0 and less than  or equal to 1.0f.");
	s.append("\n Changes the Background Color"); 
	return s;
}

bool VRChangeRightEye::Trigger(std::string value)
{
	if(value.substr(0, value.find(",")).c_str() != value)
	{
		jvec3 j = jvec3TriggerHandler(value);
		fionaConf.rEyeOffset = j;
		printf("Successful triggered, %s =  %s\n", this->GetName().c_str(), this->getValue().c_str());
		return true;
		}
		else
		{
			puts(this->getDiscription().c_str());
		}
	return false;
}

std::string VRChangeRightEye::getValue()
{
	std::ostringstream buffer;
	buffer << fionaConf.rEyeOffset;
	return  buffer.str();
}

std::string VRChangeRightEye::getDiscription()
{
	std::string s;
	s.append("USAGE::\t rEye = \"x, y, z,\"");
	s.append("\n Changes the Right Eye's coordinates"); 
	return s;
}

bool VRChangeLeftEye::Trigger(std::string value)
{
	if(value.substr(0, value.find(",")).c_str() != value)
	{
		jvec3 j = jvec3TriggerHandler(value);
		fionaConf.lEyeOffset = j;
		printf("Successful triggered, %s =  %s\n", this->GetName().c_str(), this->getValue().c_str());
		return true;
		}
		else
		{
			puts(this->getDiscription().c_str());
		}
	return false;
}

std::string VRChangeLeftEye::getValue()
{
	std::ostringstream buffer;
	buffer << fionaConf.lEyeOffset;
	return  buffer.str();
}

std::string VRChangeLeftEye::getDiscription()
{
	std::string s;
	s.append("USAGE::\t lEye = \"x, y, z,\"");
	s.append("\n Changes the Left Eye's coordinates"); 
	return s;
}

bool VRChangeKevinOffset::Trigger(std::string value)
{
	if(value.substr(0, value.find(",")).c_str() != value)
	{
		jvec3 j = jvec3TriggerHandler(value);
		fionaConf.kevinOffset = j;
		printf("Successful triggered, %s =  %s\n", this->GetName().c_str(), this->getValue().c_str());
		return true;
		}
		else
		{
			puts(this->getDiscription().c_str());
		}
	return false;
}

std::string VRChangeKevinOffset::getValue()
{
	std::ostringstream buffer;
	buffer << fionaConf.kevinOffset;
	return  buffer.str();
}

std::string VRChangeKevinOffset::getDiscription()
{
	std::string s;
	s.append("USAGE::\t Kevin = \"x, y, z,\"");
	s.append("\n Changes Kevin's coordinates"); 
	return s;
}

bool VRChangeSensorOffest::Trigger(std::string value)
{
	if(value.substr(0, value.find(",")).c_str() != value)
	{
		jvec3 j = jvec3TriggerHandler(value);
		fionaConf.sensorOffset = j;
		printf("Successful triggered, %s =  %s\n", this->GetName().c_str(), this->getValue().c_str());
		return true;
		}
		else
		{
			puts(this->getDiscription().c_str());
		}
	return false;
}

std::string VRChangeSensorOffest::getValue()
{
	std::ostringstream buffer;
	buffer << fionaConf.sensorOffset;
	return  buffer.str();
}

std::string VRChangeSensorOffest::getDiscription()
{
	std::string s;
	s.append("USAGE::\t Sensor = \"x, y, z,\"");
	s.append("\n Changes the Sensor's coordinates"); 
	return s;
}

jvec3 VRConsoleAction::jvec3TriggerHandler(std::string s)
{
	jvec3 j;
	float x = strtod(s.substr(0, s.find(",")).c_str(), NULL);
	float y = strtod(s.substr(s.find(",")+1, s.find(",")).c_str(), NULL);
	float z = strtod(s.substr(s.find(",",s.find(",")+1)+1, s.npos).c_str(), NULL);
	j.x = x;
	j.y = y;
	j.z=z;
	return j;
}

