#ifndef _VRACTION_H_
#define _VRACTION_H_

#include <Kit3D/jtrans.h>

#include <string>
#include <vector>

class VRAction;
class VRWandAction;
class VRActionSet;
class FionaScene;
class VRConsoleAction;


class VRActionManager
{
	public:
		VRActionManager() : m_pCurrentSet(0) {}
		~VRActionManager();

		void			AddSet(VRActionSet *pSet) { m_actionSets.push_back(pSet); }
		VRActionSet *	GetCurrentSet(void) { return m_pCurrentSet; }
		void			ReadActions();
		void			SetCurrentSet(const std::string &sName);
		void			SetCurrentSet(VRActionSet *pSet) { m_pCurrentSet = pSet; }

	private:
		void ClearSets(void);
		//make diff action sets..
		std::vector<VRActionSet*> m_actionSets;
		VRActionSet *m_pCurrentSet;
};

class VRActionSet
{
	public:
		VRActionSet() : m_pCurrentAction(0), m_pJoystickAction(0), m_pDrawAction(0) {}
		virtual ~VRActionSet() { ClearActions(); }

		void		AddAction(VRAction *pAction) { m_actions.push_back(pAction); }	//temp function before read-in code is done.
		VRAction *	GetCurrentAction(void) { return m_pCurrentAction; }
		VRAction *	GetJoystickAction(void) { return m_pJoystickAction; }
		const std::string & GetName(void) const { return m_name; }
		VRAction *	FindWandAction(int buttons, bool bSet=false, bool bRelease=false);
		VRConsoleAction *  FindConsoleAction(std::string name);		//Can find Console Actions within a set, name is not case sensitive. 
		std::vector<VRAction*> * GetActions();		//returns the list of actions in the set.
		void		SetCurrentAction(VRAction *pAction) { m_pCurrentAction = pAction; }
		void		SetDrawAction(VRAction *pAction) { m_pDrawAction = pAction; }
		void		SetJoystickAction(VRAction *pAction) { m_pJoystickAction = pAction; }
		void		SetName(const std::string &sName ) { m_name = sName; }
		
		virtual void IdleDrawCallback(void);

	protected:
		std::string m_name;
		std::vector<VRAction*> m_actions;
		VRAction * m_pCurrentAction;
		VRAction * m_pJoystickAction;		//an action that can occur when the user only presses the joystick..(no buttons)
		VRAction * m_pDrawAction;			//idle draw action..

	private:
		void ClearActions(void);
};


class VRAction
{
	public:
		typedef enum
		{
			WAND=0,
			//added console type
			CONSOLE=1,
			NUM_ACTION_TYPES
		} VRActionType;

		VRAction() {}
		VRAction(const std::string &name, unsigned int type) : m_type(type), m_name(name), m_scene(0) {}
		virtual ~VRAction() {}

		const std::string & GetName(void) const { return m_name; }
		virtual std::string getValue(){ return "";}
		virtual std::string getDiscription(){return "";}
		unsigned int GetType(void) const { return m_type; }
		virtual void SetScenePtr(FionaScene *scene) { m_scene = scene; }
	protected:
		FionaScene * m_scene;

	private:
		std::string m_name;
		unsigned int m_type;
};

//VRConsoleActions added by Sam Solovy
//These Actions should be treated as singletons that only activate when triggered\
//All of these actions change variables in fiona.conf
class VRConsoleAction : public VRAction
{
public:
	VRConsoleAction() : VRAction(){}
	VRConsoleAction(const std::string &name) : VRAction(name, VRAction::CONSOLE){}
	virtual ~VRConsoleAction() {}

	virtual bool Trigger(std::string) {return false;}
	virtual std::string getValue(){return "";}
	virtual std::string getDiscription(){return "";}

	std::string boolHandler(bool b);
	virtual bool boolTiggerHandler(std::string name, std::string discription, std::string sValue, bool oldValue);
	virtual jvec3 jvec3TriggerHandler(std::string value);

protected:
};

//Changes the navagation speed
class VRChangeNavSpeed : public VRConsoleAction
{
public:
	VRChangeNavSpeed() : VRConsoleAction("NavSpeed") {}
	virtual ~VRChangeNavSpeed() {}

	virtual bool Trigger(std::string);
	virtual std::string getValue();
	virtual std::string getDiscription();
	protected:

	private:
};

//Changes the rotation speed
class VRChangeRotSpeed : public VRConsoleAction
{
public:
	VRChangeRotSpeed() : VRConsoleAction("RotSpeed") {}
	virtual ~VRChangeRotSpeed() {}

	virtual bool Trigger(std::string);
	virtual std::string getValue();
	virtual std::string getDiscription();

	protected:

	private:
};

//Changes the far plane
class VRChangeFarPlane : public VRConsoleAction
{
public:
	VRChangeFarPlane() : VRConsoleAction("FarPlane") {}
	virtual ~VRChangeFarPlane() {}

	virtual bool Trigger(std::string);
	virtual std::string getValue();
	virtual std::string getDiscription();
	protected:

	private:
};

//Changes the near plane
class VRChangeNearPlane : public VRConsoleAction
{
public:
	VRChangeNearPlane() : VRConsoleAction("NearPlane") {}
	virtual ~VRChangeNearPlane() {}

	virtual bool Trigger(std::string);
	virtual std::string getValue();
	virtual std::string getDiscription();
	
	protected:

	private:
};

//Enables/Disables rift tracking
class VRChangeRiftTracking : public VRConsoleAction
{
public:
	VRChangeRiftTracking() : VRConsoleAction("RiftTracking") {}
	virtual ~VRChangeRiftTracking() {}

	virtual bool Trigger(std::string);
	virtual std::string getValue();
	virtual std::string getDiscription();

	protected:

	private:
};
//Displays the graphic memory
class VRChangeDisplayGMem : public VRConsoleAction
{
public:
	VRChangeDisplayGMem() : VRConsoleAction("DisplayGMem") {}
	virtual ~VRChangeDisplayGMem() {}

	virtual bool Trigger(std::string);
	virtual std::string getValue();
	virtual std::string getDiscription();

	protected:

	private:
};
//Enables/Disables dual View
class VRChangeDualView : public VRConsoleAction
{
public:
	VRChangeDualView() : VRConsoleAction("DualView") {}
	virtual ~VRChangeDualView() {}

	virtual bool Trigger(std::string);
	virtual std::string getValue();
	virtual std::string getDiscription();

	protected:

	private:
};
//Enables/Disables showing the FPS
class VRChangeShowRenderTime : public VRConsoleAction
{
public:
	VRChangeShowRenderTime() : VRConsoleAction("ShowRenderTime") {}
	virtual ~VRChangeShowRenderTime() {}

	virtual bool Trigger(std::string);
	virtual std::string getValue();
	virtual std::string getDiscription();

	protected:

	private:
};
//Enables/Disables rendering on the headnode
class VRChangeRenderHead : public VRConsoleAction
{
public:
	VRChangeRenderHead() : VRConsoleAction("RenderHead") {}
	virtual ~VRChangeRenderHead() {}

	virtual bool Trigger(std::string);
	virtual std::string getValue();
	virtual std::string getDiscription();

	protected:

	private:
};
//Changes the wand mode 
class VRChangeMode : public VRConsoleAction
{
public :
	VRChangeMode() : VRConsoleAction("MODE") {}
	virtual ~VRChangeMode() {}

	virtual bool Trigger(std::string);
	virtual std::string getValue();
	virtual std::string getDiscription();

	protected:

	private:
};
//Changes the designated framerate
class VRChangeFrameRate : public VRConsoleAction
{
public :
	VRChangeFrameRate() : VRConsoleAction("FrameRate") {}
	virtual ~VRChangeFrameRate() {}

	virtual bool Trigger(std::string);
	virtual std::string getValue();
	virtual std::string getDiscription();

	protected:

	private:
};
//Changes the background color
class VRChangeBackGround : public VRConsoleAction
{
public :
	VRChangeBackGround() : VRConsoleAction("BackGround") {}
	virtual ~VRChangeBackGround() {}

	virtual bool Trigger(std::string);
	virtual std::string getValue();
	virtual std::string getDiscription();

	protected:

	private:
};

//Changes the right eye position
class VRChangeRightEye : public VRConsoleAction
{
public :
	VRChangeRightEye() : VRConsoleAction("RightEye") {}
	virtual ~VRChangeRightEye() {}

	virtual bool Trigger(std::string);
	virtual std::string getValue();
	virtual std::string getDiscription();

	protected:

	private:
};

//Changes the left eye position
class VRChangeLeftEye : public VRConsoleAction
{
public :
	VRChangeLeftEye() : VRConsoleAction("LeftEye") {}
	virtual ~VRChangeLeftEye() {}

	virtual bool Trigger(std::string);
	virtual std::string getValue();
	virtual std::string getDiscription();

	protected:

	private:
};

//Changes kevin's position
class VRChangeKevinOffset : public VRConsoleAction
{
public :
	VRChangeKevinOffset() : VRConsoleAction("Kevin") {}
	virtual ~VRChangeKevinOffset() {}

	virtual bool Trigger(std::string);
	virtual std::string getValue();
	virtual std::string getDiscription();

	protected:

	private:
};

//Changes the sensor's position
class VRChangeSensorOffest : public VRConsoleAction
{
public :
	VRChangeSensorOffest() : VRConsoleAction("Sensor") {}
	virtual ~VRChangeSensorOffest() {}

	virtual bool Trigger(std::string);
	virtual std::string getValue();
	virtual std::string getDiscription();

	protected:

	private:
};
//End of Sam Solovy's code

class VRWandAction : public VRAction
{
	public:
		VRWandAction() : VRAction(), m_buttons(0) {}
		VRWandAction(const std::string &name) : VRAction(name, VRAction::WAND), m_buttons(0), m_bOnRelease(false), m_bCaptured(false), m_bNoMovement(false) {}

		virtual ~VRWandAction() {}

		unsigned int	GetButtons(void) const { return m_buttons; }
		bool			IsCaptureWand(void) const { return !m_bCaptured; }
		bool			IsOnRelease(void) const { return m_bOnRelease; }
		bool			IsNoMovement(void) const { return m_bNoMovement; }
		void			SetButton(unsigned int button) { m_buttons |= (1<<button); }
		void			SetCaptureWand(bool bCapture) { m_bCaptured = bCapture; }
		void			SetOnRelease(bool release) { m_bOnRelease = release; }
		void			SetNoMovement(bool noMove) { m_bNoMovement = noMove; }

		virtual	void	ButtonDown(void) {}
		virtual void	ButtonUp(void) {}
		virtual void	WandMove(void) {}
		virtual void	JoystickStart(void) {}
		virtual void	JoystickMove(void) {}
		virtual void	JoystickStop(void) {}
		virtual void	DrawCallback(void) {}

	protected:
		

	private:
		unsigned int m_buttons;
		bool		m_bOnRelease;	//whether this action should occur on button release
		bool		m_bCaptured;	//whether to hide drawing the wand beam
		bool		m_bNoMovement;	//whether navigation should stop updating when executing this action..
};

#endif