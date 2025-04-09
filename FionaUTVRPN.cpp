

#include "FionaUTVRPN.h"
#include "FionaUT.h"

vrpn_Tracker_Remote* FionaUTVRPN::tracker=0;
vrpn_Analog_Remote* FionaUTVRPN::analog=0;
vrpn_Button_Remote* FionaUTVRPN::buttons=0;
vrpn_Analog_Remote* FionaUTVRPN::analog2=0;
vrpn_Button_Remote* FionaUTVRPN::buttons2=0;
FILE * FionaUTVRPN::ioFile=0;
bool FionaUTVRPN::recordIO=false;
bool FionaUTVRPN::playbackIO=false;
bool FionaUTVRPN::lastStampPlayed=true;
bool FionaUTVRPN::pausePlayback = false;
float FionaUTVRPN::pauseTime = 0.f;
FionaUTVRPN::VRPNData3 FionaUTVRPN::currData;
unsigned int FionaUTVRPN::currButtons=0;
unsigned int FionaUTVRPN::currState=0;
float* FionaUTVRPN::framesPerSecond = 0;
unsigned int FionaUTVRPN::numheadPositions = 0;

static void VRPN_CALLBACK trackerCallback(void* ud, const vrpn_TRACKERCB t)
{
	if(FionaUTVRPN::recordIO)
	{
		if(t.sensor == 0)
		{
			memcpy(&(FionaUTVRPN::currData.headPos), &(t.pos), sizeof(vrpn_float64)*3);
			memcpy(&(FionaUTVRPN::currData.headQuat), &(t.quat), sizeof(vrpn_float64)*4);
		}
		else if(t.sensor == 1)
		{
			memcpy(&(FionaUTVRPN::currData.wandPos), &(t.pos), sizeof(vrpn_float64)*3);
			memcpy(&(FionaUTVRPN::currData.wandQuat), &(t.quat), sizeof(vrpn_float64)*4);
		}
	}

	jvec3 v(t.pos[1]/*+0.0762f*/,-t.pos[2]/*-CAVE_WALL_SZ/2+0.14425f*/,-t.pos[0]/*+0.0762f*/);
	
	//wtf is going on with this quaternion stuff...it's multiplying by a 0 angle 0,0,1 axis quaternion
	//but the first quaternion is getting its [3] value set in the wrong location...
	quat q= quat(t.quat[0], -t.quat[2], -t.quat[1], t.quat[3])*quat(0,0,0,1);
	if( q.w<0 ) q = -q;

	/*if(t.sensor == 0)
	{
		printf("Position: %f, %f, %f\n", v.x, v.y, v.z);
		printf("Rotation: %f, %f, %f, %f\n", q.x, q.y, q.z, q.w);
	}*/
	
	_FionaUTTracker(t.sensor,v,q);
}

static void VRPN_CALLBACK analogCallback(void* ud, const vrpn_ANALOGCB t)
{
	if(FionaUTVRPN::recordIO)
	{
		FionaUTVRPN::currData.joystick[0] = t.channel[0];
		FionaUTVRPN::currData.joystick[1] = t.channel[1];
		FionaUTVRPN::currData.joystick[2] = t.channel[2];
	}

	static const float JOYSTICK_DEADZONE = 0.5f;
	float horzCheck = t.channel[0];
	if (horzCheck < JOYSTICK_DEADZONE && horzCheck > -JOYSTICK_DEADZONE)
	{
		horzCheck = 0.f;
	}

	_FionaUTJoystick(0, vec4(horzCheck, t.channel[2], t.channel[1], 0.f));
}

static void VRPN_CALLBACK analogCallback2(void* ud, const vrpn_ANALOGCB t)
{
	_FionaUTJoystick(1,vec4(t.channel[0],t.channel[2],t.channel[1], 0.f));
}

static void VRPN_CALLBACK buttonCallback(void* ud, const vrpn_BUTTONCB t)
{
	if(FionaUTVRPN::recordIO)
	{
		FionaUTVRPN::currData.buttons |= (1<<t.button);
		t.state == 1 ? FionaUTVRPN::currData.state |= (1<<t.button) : FionaUTVRPN::currData.state &= ~(1<<t.button);
		//printf("Button %u, State: %u\n", FionaUTVRPN::currData.buttons, FionaUTVRPN::currData.state);
	}

	_FionaUTWandButton(t.button, t.state, 0);
}

static void VRPN_CALLBACK buttonCallback2(void* ud, const vrpn_BUTTONCB t)
{
	_FionaUTWandButton(t.button, t.state, 1);
}

void FionaUTVRPN::Init(const char* hostAddress, const char* trackerN, const char* wandN, const char *inputHost, const char *wandN2)
{
	std::string trackerName	(trackerN);
	std::string wandName	(wandN   );
	std::string buttonName	(wandN   );

	if(strlen(inputHost) > 0)
	{
		tracker = new vrpn_Tracker_Remote((trackerName+"@"+std::string(hostAddress)).c_str());
		analog  = new vrpn_Analog_Remote ((wandName   +"@"+std::string(inputHost)).c_str());
		buttons = new vrpn_Button_Remote ((buttonName +"@"+std::string(inputHost)).c_str());
	}
	else
	{
		tracker = new vrpn_Tracker_Remote((trackerName+"@"+std::string(hostAddress)).c_str());
		analog  = new vrpn_Analog_Remote ((wandName   +"@"+std::string(hostAddress)).c_str());
		buttons = new vrpn_Button_Remote ((buttonName +"@"+std::string(hostAddress)).c_str());
	}

	if(wandN2 != 0)
	{
		printf("Initializing 2nd Intersense Wand!\n");
		std::string strSecondWand(wandN2);
		analog2  = new vrpn_Analog_Remote ((strSecondWand +"@"+std::string(hostAddress)).c_str());
		buttons2 = new vrpn_Button_Remote ((strSecondWand +"@"+std::string(hostAddress)).c_str());
	}

	if(tracker==NULL) printf("No tracker found, or cannot connect to the server\n");
	if(analog ==NULL) printf("No analog device found, or cannot connect to the server\n");
	if(buttons==NULL) printf("No wand found, or cannot connect to the server\n");
	
	if( tracker!=NULL ) tracker->register_change_handler(NULL, trackerCallback);
	if( analog !=NULL ) analog ->register_change_handler(NULL, analogCallback);
	if( buttons!=NULL ) buttons->register_change_handler(NULL, buttonCallback);
	if( analog2 !=NULL ) analog2 ->register_change_handler(NULL, analogCallback2);
	if( buttons2!=NULL ) buttons2->register_change_handler(NULL, buttonCallback2);

	if(playbackIO)
	{
		ioFile = fopen(fionaConf.playbackFileName.c_str(), "rb");
	}
	else if(recordIO)
	{
		ioFile = fopen(fionaConf.playbackFileName.c_str(), "wb");
	}
}

//special function to not start recording until a bit later...
void FionaUTVRPN::StartRecord(void)
{
	ioFile = fopen(fionaConf.playbackFileName.c_str(), "wb");
	recordIO = true;
}

 void FionaUTVRPN::StartPlayback(void)
 {
	 ioFile = fopen(fionaConf.playbackFileName.c_str(), "rb");
	 playbackIO = true;
	 //calculate frames per second for each frame...
	 unsigned int frameCount = 0;
	 size_t amtRead = 1;
	 while (amtRead != 0)
	 {
		 VRPNData3 data;
		 amtRead = fread(&data, sizeof(VRPNData3), 1, ioFile);
		 if (amtRead != 0)
		 {
			 frameCount++;
		 }
	 }

	 fseek(ioFile, 0, SEEK_SET);
	 framesPerSecond = new float[frameCount];
	 memset(framesPerSecond, 0.f, sizeof(float)*frameCount);
	 float *timeStamps = new float[frameCount];
	 memset(timeStamps, 0.f, sizeof(float)*frameCount);

	 frameCount = 0;
	 amtRead = 1;
	 while (amtRead != 0)
	 {
		 VRPNData3 data;
		 amtRead = fread(&data, sizeof(VRPNData3), 1, ioFile);
		 if (amtRead != 0)
		 {
			 timeStamps[frameCount] = data.timeStamp;
			 frameCount++;
		 }
	 }

	 //need to look prior 1 second's worth of time to get fps for each frame..
	 for (unsigned int j = 0; j < frameCount; ++j)
	 {
		 unsigned int fCount = 0;
		 float firstStamp = 0.f;
		 for (unsigned int i = 0; i < j; ++i)
		 {
			 //figure out how many frames existed in a second's timespan.
			 
			 if (firstStamp == 0.f)
			 {
				 firstStamp = timeStamps[j];
			 }

			 if (firstStamp - timeStamps[i] > 1.f)
			 {

			 }
			 else if (firstStamp - timeStamps[i] < 0.f)
			 {
				 break;
			 }
			 else if (firstStamp - timeStamps[i] <= 1.f)
			 {
				 fCount++;
			 }
		 }

		 if (fCount == 0.f)
		 {
			 framesPerSecond[j] = fionaConf.framerate;
		 }
		 else
		 {
			 framesPerSecond[j] = fCount;
		 }
	 }

	/* for (unsigned int k = 0; k < frameCount; ++k)
	 {
		 printf("FPS: %f, ", framesPerSecond[k]);
	 }*/

	 fseek(ioFile, 0, SEEK_SET);
	 delete[] timeStamps;
 }

 void FionaUTVRPN::PausePlayback(void)
 {
	 pausePlayback = true;
	 pauseTime = FionaUTTime();
	 _FionaUTJoystick(0, vec4(0.f, 0.f, 0.f, 0.f));
 }

 void FionaUTVRPN::UnpausePlayback(void)
 {
	 pausePlayback = false;
 }


 void FionaUTVRPN::getPathway(std::vector<FionaUTVRPN::VRPNPathPositions> & pathPositions)
 {
	 size_t elements_read = 0;
	 VRPNData3 vrpn_stream;
	 //std::vector<VRPNPathPositions> pathPositions;

	 //get the file to start reading from
	 FILE * file_stream;
	 if ((file_stream = fopen(fionaConf.pathFileName.c_str(), "rb")) == NULL)
	 {
		 //return pathPositions;
	 };

	 if (!file_stream)
	 {
		 printf("Couldn't load path file\n");
		 return;
	 }
	 //set to begining of file
	 //fseek(ioFile, 0, SEEK_SET);

	 pathPositions.reserve(54000); //to avoid a lot of potential reallocations off the bat

	 while (!feof(file_stream))
	 {
		 //read in one block of vrpn data 2 at a time
		 elements_read += fread(&vrpn_stream, sizeof(VRPNData3), 1, file_stream);

		 //create new struct to hold position
		 struct VRPNPathPositions positions;
		 positions.x = vrpn_stream.mv[12];
		 positions.y = vrpn_stream.mv[13];
		 positions.z = vrpn_stream.mv[14];
		 printf("%f, %f, %f\n", positions.x, positions.y, positions.z);
		 pathPositions.push_back(positions);
	 }

	 FionaUTVRPN::numheadPositions = (unsigned int)elements_read;
 }

void FionaUTVRPN::MainLoop(void)
{
	if(playbackIO)
	{
		static float firstStamp = 0.f;
		static bool first = true;
		static unsigned int fCount = 0;
		if(first)
		{
			first = false;
			firstStamp = FionaUTTime();
		}

		if(fionaConf.appType != FionaConfig::HEADNODE && playbackIO)
		{
			if(ioFile == 0)
			{
				printf("Forcing open of playback file %s\n", fionaConf.playbackFileName.c_str());
				StartPlayback();
			}
		}

		if (!pausePlayback)
		{
			if (lastStampPlayed)
			{
				//memset(&currData, 0, sizeof(VRPNData));
				ResetCurrData();
				fread(&currData, sizeof(VRPNData3), 1, ioFile);
				fionaConf.framerate = framesPerSecond[fCount];
				fCount++;
				//size_t readData = 
				//if(readData == sizeof(VRPNData))
				//{
				/*printf("Reading Playback Data:\n");
				printf("Head Pos: %f %f %f\n", currData.headPos[0], currData.headPos[1], currData.headPos[2]);
				printf("Head Ori: %f %f %f %f\n", currData.headQuat[0], currData.headQuat[1], currData.headQuat[2], currData.headQuat[3]);
				printf("Wand Pos: %f %f %f\n", currData.wandPos[0], currData.wandPos[1], currData.wandPos[2]);
				printf("Wand Ori: %f %f %f %f\n", currData.wandQuat[0], currData.wandQuat[1], currData.wandQuat[2], currData.wandQuat[3]);
				printf("Joystick: %f %f %f\n", currData.joystick[0], currData.joystick[1], currData.joystick[2]);
				printf("Buttons: %u %u\n", currData.buttons, currData.state);
				printf("TimeStamp: %f\n", currData.timeStamp);*/
				//}
			}

			float currTime = pauseTime + (FionaUTTime() - firstStamp);
			if (currTime >= currData.timeStamp)
			{
				jvec3 v(currData.headPos[1], -currData.headPos[2], -currData.headPos[0]);
				if (v.len() != 0.f)
				{
					quat q = quat(currData.headQuat[0], -currData.headQuat[2], -currData.headQuat[1], currData.headQuat[3])*quat(0, 0, 0, 1);
					if (q.w < 0) q = -q;

					_FionaUTTracker(0, v, q);

					v.set(currData.wandPos[1], -currData.wandPos[2], -currData.wandPos[0]);
					quat q2 = quat(currData.wandQuat[0], -currData.wandQuat[2], -currData.wandQuat[1], currData.wandQuat[3])*quat(0, 0, 0, 1);
					if (q2.w < 0) q2 = -q2;

					_FionaUTTracker(1, v, q2);

					_FionaUTJoystick(0, vec4(currData.joystick[0], currData.joystick[2], currData.joystick[1], 0.f));

					for (int i = 0; i < 6; ++i)
					{
						if ((currData.buttons & (1 << i)) != 0)
						{
							//printf("Pressed wand button %d\n", i);
							//printf("Button %u, State: %u\n", currData.buttons, currData.state);
							_FionaUTWandButton(i, (((currData.state & (1 << i)) != 0) ? 1 : 0), 0);
						}
					}

					if (currData.keyboard != 0)
					{
						_FionaUTKeyboard(fionaWinConf[0].window, currData.keyboard);
					}
				}

				lastStampPlayed = true;
			}
			else
			{
				_FionaUTJoystick(0, vec4(0.f, 0.f, 0.f, 0.f));
				lastStampPlayed = false;
			}
		}
		else
		{
			firstStamp = FionaUTTime();
		}
	}
	else
	{
		if( tracker!=NULL ) tracker->mainloop();
		if( analog !=NULL ) analog ->mainloop();
		if( buttons!=NULL ) buttons->mainloop();
		if( analog2 !=NULL ) analog2 ->mainloop();
		if( buttons2!=NULL ) buttons2->mainloop();

		if(recordIO)
		{
			static bool first = true;
			static float firstStamp = 0.f;
			if(first)
			{
				first = false;
				firstStamp = FionaUTTime();
			}

			currData.timeStamp = FionaUTTime()-firstStamp;
			/*printf("Recording Playback Data:\n");
			printf("Head Pos: %f %f %f\n", currData.headPos[0], currData.headPos[1], currData.headPos[2]);
			printf("Head Ori: %f %f %f %f\n", currData.headQuat[0], currData.headQuat[1], currData.headQuat[2], currData.headQuat[3]);
			printf("Wand Pos: %f %f %f\n", currData.wandPos[0], currData.wandPos[1], currData.wandPos[2]);
			printf("Wand Ori: %f %f %f %f\n", currData.wandQuat[0], currData.wandQuat[1], currData.wandQuat[2], currData.wandQuat[3]);
			printf("Joystick: %f %f %f\n", currData.joystick[0], currData.joystick[1], currData.joystick[2]);
			printf("Buttons: %u %u\n", currData.buttons, currData.state);
			printf("TimeStamp: %f\n", currData.timeStamp);*/
			fwrite(&currData, sizeof(VRPNData3), 1, ioFile);
			
			ResetCurrData();
		}
	}
}

void FionaUTVRPN::ResetCurrData(void)
{
	//note: we aren't resetting the joystick value...
	currData.buttons=0;
	currData.state=0;
	currData.keyboard = 0;
	currData.headPos[0] = 0.0;
	currData.headPos[1] = 0.0;
	currData.headPos[2] = 0.0;
	currData.headQuat[0] = 0.0;
	currData.headQuat[1] = 0.0;
	currData.headQuat[2] = 0.0;
	currData.headQuat[3] = 0.0;
	currData.wandPos[0] = 0.0;
	currData.wandPos[1] = 0.0;
	currData.wandPos[2] = 0.0;
	currData.wandQuat[0] = 0.0;
	currData.wandQuat[1] = 0.0;
	currData.wandQuat[2] = 0.0;
	currData.wandQuat[3] = 0.0;
	currData.timeStamp=0.f;
	memset(currData.mv, 0, sizeof(float) * 16);
}

void FionaUTVRPN::Close(void)
{
	if(Initialized())
	{
		if( tracker!=NULL ) delete tracker; tracker = NULL;
		if( analog !=NULL ) delete analog ; analog  = NULL;
		if( buttons!=NULL ) delete buttons; buttons = NULL;
		if( analog2 !=NULL ) delete analog2 ; analog2  = NULL;
		if( buttons2!=NULL ) delete buttons2; buttons2 = NULL;
	}

	if(ioFile != 0)
	{
		fclose(ioFile);
	}
}

bool FionaUTVRPN::Initialized(void)
{
	if( tracker!=NULL ) return true;
	if( analog !=NULL ) return true;
	if( buttons!=NULL ) return true;
	if( ioFile!=0) return true;
	return false;
}

