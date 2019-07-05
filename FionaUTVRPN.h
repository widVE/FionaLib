#ifndef FionaUT_VRPN_BRIDGE_H__
#define FionaUT_VRPN_BRIDGE_H__

#include "VRPN/vrpn_Tracker.h"
#include "VRPN/vrpn_Analog.h"
#include "VRPN/vrpn_Button.h"

#include <stdio.h>

class FionaUTVRPN
{

private:
	static vrpn_Tracker_Remote* tracker;
	static vrpn_Analog_Remote* analog;
	static vrpn_Button_Remote* buttons;	
	static vrpn_Analog_Remote* analog2;
	static vrpn_Button_Remote* buttons2;

	static bool Initialized(void);
	
	FionaUTVRPN() {}

	struct VRPNData
	{
		float timeStamp;
		vrpn_float64 headPos[3];
		vrpn_float64 headQuat[4];
		vrpn_float64 wandPos[3];
		vrpn_float64 wandQuat[4];
		vrpn_float64 joystick[3];
		unsigned int buttons;
		unsigned int state;
	};

	struct VRPNData2
	{
		float timeStamp;
		vrpn_float64 headPos[3];
		vrpn_float64 headQuat[4];
		vrpn_float64 wandPos[3];
		vrpn_float64 wandQuat[4];
		vrpn_float64 joystick[3];
		unsigned int buttons;
		unsigned int state;
		float mv[16];
	};

	struct VRPNData3
	{
		float timeStamp;
		vrpn_float64 headPos[3];
		vrpn_float64 headQuat[4];
		vrpn_float64 wandPos[3];
		vrpn_float64 wandQuat[4];
		vrpn_float64 joystick[3];
		unsigned int buttons;
		unsigned int state;
		unsigned int keyboard;
		float mv[16];
	};
	static void ResetCurrData(void);

public:

	struct VRPNPathPositions
	{
		vrpn_float64 x;
		vrpn_float64 y;
		vrpn_float64 z;
	};

	static FILE *ioFile;
	static bool recordIO;
	static bool playbackIO;
	static bool lastStampPlayed;
	static bool pausePlayback;
	static float pauseTime;
	static VRPNData3 currData;
	static float *framesPerSecond;
	static unsigned int currButtons;
	static unsigned int currState;
	static unsigned int numheadPositions;

	static void StartRecord(void);
	static void StartPlayback(void);

	static void Init(const char* hostAddress="localhost",
								  const char* trackerN="Isense900",
								  const char* wandN="Wand0",
								  const char* inputHost="",
								  const char* wandN2=0);

	static void MainLoop(void);
	static void Close(void);

	static void PausePlayback(void);
	static void UnpausePlayback(void);
	static void getPathway(std::vector<FionaUTVRPN::VRPNPathPositions> & pathPositions);
	static unsigned int getNumPositions(void) { return numheadPositions; }

};

#endif
