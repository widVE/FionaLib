#ifndef _FIONANETWORK_H_
#define _FIONANETWORK_H_

#include "Socket.h"
#include <Kit3D/jmath.h>
#include <Kit3D/jtrans.h>
#ifndef LINUX_BUILD
#include "leap/Leap.h"
#endif

class CaveMaster
{
	public:
		CaveMaster() : connections(0) {}
		~CaveMaster();
			
		void ListenForSlaves(void);
		void Handshake(void);
		void SendCavePacket(char *buffer, int len, bool firstView=true);
		void WakeupSlaves(void);

	private:
		Socket **connections;
};

class CaveSlave
{
	public:
		CaveSlave(bool bMasterSlave=false) : masterSlave(bMasterSlave) {}
		~CaveSlave() {}

		void ConnectToMaster(void);
		void Handshake(void);
		void ReceiveCavePacket(char **packetData, int size);
		void ReceivePacketHeader(int &numPackets, int &totalSize);
		void WaitForMaster(void);

	private:
		bool masterSlave;
		Socket masterConnection;
};

struct JoystickData
{
	int wandIdx;		//4 bytes
	float x;			//4 bytes
	float y;			//4 bytes
	float z;			//4 bytes
};

struct WandButtonData
{
	int buttonIdx;			//4 bytes
	int wandIdx;			//4 bytes
	int state;				//4 bytes
};

struct TrackerData
{
	int trackerIdx;		//4 bytes
	float pos[3];		//12 bytes
	float quat[4];		//16 bytes
};

struct KeyboardData
{
	unsigned int key;		//4 bytes
	unsigned int modifer;	//4 bytes
};

struct ConfigData
{
	float	lEye[3];
	float	rEye[3];
	float	trackerOffset[3];
	float	kevinOffset[3];
};

struct PhysicsData
{
	float timeStep;
	float currTime;
};

struct Test
{
	float test;
};

struct PacketHeader
{
	int type;	//type of following packet
	int size;	//size of following packet
};

struct KinectData
{
	// skeleton data
	float values[140];
};

struct KinectVideoData
{	
	unsigned long	colorFrame;
	// rgba
	unsigned char	color[640*480*4];
	
	unsigned long	depthFrame;
	// depth + player id -- look at NuiCameraImage
	unsigned short	depth[320*240];
};


struct WiiFitData
{
	float values[4];
};

struct EMGData
{
	unsigned int values[4];
	float fVoltage;
};

struct SpeechData
{
	float speechVal;
};

struct VMDData
{
	char vmdMsg[512];	//assuming a max length of 512 bytes...
};

struct TimeVaryingData
{
	int fileIndex;
	int bufferIndex;
};

struct FrameCountData
{
	unsigned long long frameCount;
};

#ifndef LINUX_BUILD
struct LeapData
{
	//possible leap hand data
	struct HandData
	{
		//possible leap finger data
		struct FingerData
		{
			bool valid;
			float length;
			float tipPosition[3];
			float tipDirection[3];
			float tipVelocity[3];
		};

		bool valid;
		float sphereRadius;
		float sphereCenter[3];
		float palmNormal[3];
		float handPosition[3];
		float handDirection[3];
		float handVelocity[3];
		
		Leap::Gesture::Type t;	//whether this hand had a gesture this frame (and if so what type?)
		Leap::Gesture::State s;	//the state of the gesture (started, in-progress, stopped)

		FingerData fingers[5];	//we allow up to 5 fingers for a hand
	};

	//two hands max for now..
	HandData hand1;
	HandData hand2;
};
#endif

struct CameraData
{
	float pos[3];
	float rot[4];
};

struct MatrixData
{
	unsigned int index;
	float p[16];
	float v[16];
};

//add more packet types here
struct ControllerPacket
{
    unsigned short                      wButtons;
    unsigned char                       bLeftTrigger;
    unsigned char                       bRightTrigger;
    short                               sThumbLX;
    short                               sThumbLY;
    short                               sThumbRX;
    short                               sThumbRY;
};

class BasePacket 
{
	public:

		typedef enum
		{
			UPDATE_TRACKER=0,
			UPDATE_JOYSTICK,
			UPDATE_WANDBUTTONS,
			UPDATE_KEYBOARD,
			UPDATE_CONFIG,
			UPDATE_PHYSICS,
			UPDATE_KINECT,
			UPDATE_WIIFIT,
			UPDATE_EMG,
			UPDATE_SPEECH,
			UPDATE_VMD,
			UPDATE_LEAP,
			UPDATE_KINECT_VIDEO,
			UPDATE_TIME_VARYING_FILE,
			UPDATE_CAMERA,
			UPDATE_FRAME_NUMBER,
			UPDATE_MATRIX,
			UPDATE_CONTROLLER,
			TEST,
			NUM_COMMAND_TYPES
		} commandType_t;

		typedef enum
		{
			PER_FRAME=0,
			EVENT_DRIVEN,
			APP_SPECIFIC,
			NO_TYPE,
			NUM_PACKET_TYPES
		} packetType_t;

		typedef enum
		{
			SINGLE_VIEW=0,
			DUAL_VIEW,
			BOTH,
			NUM_VIEW_TYPES
		} viewType_t;

		BasePacket() : packetType(NO_TYPE), viewType(BOTH)
		{
			header.type = 0;
			header.size = 0;
		}

		BasePacket(unsigned int commandType, packetType_t p, viewType_t v=BOTH) : packetType(p), viewType(v)
		{	
			header.type = commandType;
			header.size = 0;
		}

		virtual ~BasePacket() {}

		unsigned int			GetCommandType(void) const	{ return header.type; }
		unsigned int			GetSize(void) const			{ return header.size; }
		void *					GetHeader(void)				{ return &header; }
		virtual void *			GetPayload(void)			=0;
		viewType_t				GetViewType(void) const		{ return viewType; }

	protected:
		PacketHeader header;
		packetType_t packetType;
		viewType_t viewType;
};

class UpdateMatrixPacket : public BasePacket
{
public:
	UpdateMatrixPacket(unsigned int commandType=0, viewType_t v=BOTH) : BasePacket(commandType, PER_FRAME, v) { header.size = sizeof(t); }
	virtual ~UpdateMatrixPacket() {}

	virtual void *			GetPayload(void)			{ return (void*)&t; }
	void					SetPayload(unsigned int index, float *p, float *v);
	void					SetPayloadFromBuffer(char * buf, int size);
	const MatrixData&		GetData(void) const { return t; }

protected:
	MatrixData t;
};

class UpdateFramePacket : public BasePacket 
{
public:
	UpdateFramePacket(unsigned int commandType=0, viewType_t v=BOTH) : BasePacket(commandType, PER_FRAME, v) { header.size = sizeof(t); }
	virtual ~UpdateFramePacket() {}

	virtual void *			GetPayload(void)			{ return (void*)&t; }
	void					SetPayload(unsigned long long fc);
	void					SetPayloadFromBuffer(char * buf, int size);
	const FrameCountData&		GetData(void) const { return t; }

protected:
	FrameCountData t;
};

class UpdateTrackerPacket : public BasePacket 
{
public:
	UpdateTrackerPacket(unsigned int commandType=0, viewType_t v=BOTH) : BasePacket(commandType, PER_FRAME, v) { header.size = sizeof(t); }
	virtual ~UpdateTrackerPacket() {}

	virtual void *			GetPayload(void)			{ return (void*)&t; }
	void					SetPayload(short trackerIdx, const jvec3 &v, const quat &q);
	void					SetPayloadFromBuffer(char * buf, int size);
	const TrackerData&		GetData(void) const { return t; }

protected:
	TrackerData t;
};


class UpdateCameraPacket : public BasePacket 
{
public:
	UpdateCameraPacket(unsigned int commandType=0, viewType_t v=BOTH) : BasePacket(commandType, EVENT_DRIVEN, v) { header.size = sizeof(t); }
	virtual ~UpdateCameraPacket() {}

	virtual void *			GetPayload(void)			{ return (void*)&t; }
	void					SetPayload(const jvec3 &v, const quat &q);
	void					SetPayloadFromBuffer(char * buf, int size);
	const CameraData&		GetData(void) const { return t; }

protected:
	CameraData t;
};

class FionaNetwork
{
public:
	
	FionaNetwork();

	~FionaNetwork() {}
#ifdef LINUX_BUILD
	static const unsigned int FIONA_NETWORK_BUFFER_SIZE = 65536;
#else
	static const unsigned int FIONA_NETWORK_BUFFER_SIZE = 262144;
#endif
	char masterBuffer[FIONA_NETWORK_BUFFER_SIZE];
	char slaveBuffer[FIONA_NETWORK_BUFFER_SIZE];
	
	char masterSlaveBuffer[FIONA_NETWORK_BUFFER_SIZE];

	//below are for dual view...
	char masterBuffer2[FIONA_NETWORK_BUFFER_SIZE];
	char slaveBuffer2[FIONA_NETWORK_BUFFER_SIZE];
};


class UpdateJoystickPacket : public BasePacket
{
public:
	UpdateJoystickPacket(unsigned int commandType=0, viewType_t v=BOTH) : BasePacket(commandType, EVENT_DRIVEN, v) { header.size = sizeof(w); }
	virtual ~UpdateJoystickPacket() {}

	virtual void *			GetPayload(void)			{ return (void*)&w; }
	void					SetPayload(short wandIdx, float fX, float fY, float fZ);
	void					SetPayloadFromBuffer(char * buf, int size);
	const JoystickData&		GetData(void) const { return w; }

protected:
	JoystickData w;
};

class UpdateWandButtonPacket : public BasePacket
{
public:
	UpdateWandButtonPacket(unsigned int commandType=0, viewType_t v=BOTH) : BasePacket(commandType, EVENT_DRIVEN, v) { header.size = sizeof(w); }
	virtual ~UpdateWandButtonPacket() {}
	
	virtual void *			GetPayload(void)			{ return (void*)&w; }
	void					SetPayload(short btnIdx, unsigned char btnState, short wandIdx);
	void					SetPayloadFromBuffer(char * buf, int size);
	const WandButtonData&	GetData(void) const { return w; }
	
protected:
	WandButtonData w;
};

class UpdateKeyboardPacket : public BasePacket
{
public:
	UpdateKeyboardPacket(unsigned int commandType=0) : BasePacket(commandType, EVENT_DRIVEN) { header.size = sizeof(w); }
	virtual ~UpdateKeyboardPacket() {}
	
	virtual void *			GetPayload(void)			{ return (void*)&w; }
	void					SetPayload(unsigned int key, unsigned int mod);
	void					SetPayloadFromBuffer(char * buf, int size);
	const KeyboardData&	GetData(void) const { return w; }
	
protected:
	KeyboardData w;
};

class UpdateConfigPacket : public BasePacket
{
public:
	UpdateConfigPacket(unsigned int commandType=0) : BasePacket(commandType, PER_FRAME) { header.size = sizeof(w); }
	virtual ~UpdateConfigPacket() {}
	
	virtual void *			GetPayload(void)			{ return (void*)&w; }
	void					SetPayload(const jvec3& lEye, const jvec3& rEye, const jvec3& tO, const jvec3& kO);
	void					SetPayloadFromBuffer(char * buf, int size);
	const ConfigData&		GetData(void) const { return w; }
	
protected:
	ConfigData w;
};


class UpdatePhysicsSync : public BasePacket
{
public:
	UpdatePhysicsSync(unsigned int commandType=0) : BasePacket(commandType, PER_FRAME) { header.size = sizeof(w); }
	virtual ~UpdatePhysicsSync() {}
	
	virtual void *			GetPayload(void)			{ return (void*)&w; }
	void					SetPayload(float fTimeStep, float fCurrTime);
	void					SetPayloadFromBuffer(char * buf, int size);
	const PhysicsData&		GetData(void) const { return w; }

protected:
	PhysicsData w;
};

class UpdateKinectData : public BasePacket
{
public:
	UpdateKinectData(unsigned int commandType=0) : BasePacket(commandType, PER_FRAME, SINGLE_VIEW) { header.size = sizeof(w); }
	virtual ~UpdateKinectData() {}
	
	virtual void *			GetPayload(void)			{ return (void*)&w; }
	void					SetPayload(float fKinectData[140]);
	void					SetPayloadFromBuffer(char * buf, int size);
	const KinectData&		GetData(void) const { return w; }

protected:
	KinectData w;
};

class UpdateWiiFitData : public BasePacket
{
public:
	UpdateWiiFitData(unsigned int commandType=0) : BasePacket(commandType, PER_FRAME, SINGLE_VIEW) { header.size = sizeof(w); }
	virtual ~UpdateWiiFitData() {}
	
	virtual void *			GetPayload(void)			{ return (void*)&w; }
	void					SetPayload(float fWiiFitData[4]);
	void					SetPayloadFromBuffer(char * buf, int size);
	const WiiFitData&		GetData(void) const			{ return w; }

protected:
	WiiFitData w;
};

class UpdateEMGData : public BasePacket
{
public:
	UpdateEMGData(unsigned int commandType=0) : BasePacket(commandType, PER_FRAME, SINGLE_VIEW) { header.size = sizeof(w); }
	virtual ~UpdateEMGData() {}
	
	virtual void *			GetPayload(void)			{ return (void*)&w; }
	void					SetPayload(unsigned int emg[4], float fVoltage);
	void					SetPayloadFromBuffer(char * buf, int size);
	const EMGData&			GetData(void) const			{ return w; }

protected:
	EMGData w;
};

class UpdateSpeechData : public BasePacket
{
public:
	UpdateSpeechData(unsigned int commandType=0) : BasePacket(commandType, EVENT_DRIVEN) { header.size = sizeof(w); }
	virtual ~UpdateSpeechData() {}
	
	virtual void *			GetPayload(void)			{ return (void*)&w; }
	void					SetPayload(float sV);
	void					SetPayloadFromBuffer(char * buf, int size);
	const SpeechData&		GetData(void) const { return w; }

protected:
	SpeechData w;
};

class UpdateVMDData : public BasePacket
{
public:
	UpdateVMDData(unsigned int commandType=0) : BasePacket(commandType, EVENT_DRIVEN) { header.size = sizeof(w); }
	virtual ~UpdateVMDData() {}
	
	virtual void *			GetPayload(void)			{ return (void*)&w; }
	void					SetPayload(char **vmdMsg, int sizeOfMsg);
	void					SetPayload(const char *vmdMsg, int sizeOfMsg);
	void					SetPayloadFromBuffer(char * buf, int size);
	const VMDData&			GetData(void) const { return w; }

protected:
	VMDData w;
};

#ifndef LINUX_BUILD
class UpdateLeapData : public BasePacket
{
public:
	UpdateLeapData(unsigned int commandType=0) : BasePacket(commandType, PER_FRAME, SINGLE_VIEW) { header.size = sizeof(w); }
	virtual ~UpdateLeapData() {}
	
	virtual void *			GetPayload(void)			{ return (void*)&w; }
	void					SetPayload(const Leap::Hand *pHand1, const Leap::Hand *pHand2, const Leap::Gesture::Type &gestureType1, const Leap::Gesture::State &state1, const Leap::Gesture::Type &gestureType2, const Leap::Gesture::State &state2);
	void					SetPayloadFromBuffer(char * buf, int size);
	const LeapData&			GetData(void) const			{ return w; }

protected:
	void					CopyHand(const Leap::Hand *pHand, LeapData::HandData & ourHand);
	void					CopyFinger(const Leap::Finger &pFinger, LeapData::HandData::FingerData & ourFinger);
	
	LeapData w;
};
#endif

class TestPacket : public BasePacket
{
public:
	TestPacket(unsigned int commandType=0) : BasePacket(commandType, PER_FRAME) { header.size = sizeof(w); }
	virtual ~TestPacket() {}

	virtual void *			GetPayload(void)			{ return (void*)&w; }
	void					SetPayload(float fTest);
	void					SetPayloadFromBuffer(char * buf, int size);
	const Test &			GetPacket(void) const { return w; }

protected:
	Test w;
};


class UpdateKinectVideoData : public BasePacket
{
public:
	UpdateKinectVideoData() : BasePacket(UPDATE_KINECT_VIDEO, PER_FRAME) { header.size = sizeof(KinectVideoData); }
	UpdateKinectVideoData(const KinectVideoData& d) : BasePacket(UPDATE_KINECT_VIDEO, PER_FRAME), data(d) {header.size = sizeof(KinectVideoData); }

	void*					GetPayload() { return reinterpret_cast<void*>(&data); }
	inline KinectVideoData& GetData() { return data; }

	void					SetPayloadFromBuffer(char* buf, int size);


	KinectVideoData			data;
};

class UpdateTimeVaryingData : public BasePacket
{
public:
	UpdateTimeVaryingData(unsigned int commandType=0) : BasePacket(commandType, EVENT_DRIVEN) { header.size = sizeof(w); }
	virtual ~UpdateTimeVaryingData() {}
	
	virtual void *			GetPayload(void)			{ return (void*)&w; }
	void					SetPayload(int fileIndex, int bufferIndex);
	void					SetPayloadFromBuffer(char * buf, int size);
	const TimeVaryingData&		GetData(void) const { return w; }

protected:
	TimeVaryingData w;
};

class UpdateControllerData : public BasePacket
{
public:
	UpdateControllerData(unsigned int commandType=0) : BasePacket(commandType, EVENT_DRIVEN) { header.size = sizeof(w); }
	virtual ~UpdateControllerData() {}
	
	virtual void *			GetPayload(void)			{ return (void*)&w; }
	void					SetPayload(unsigned short wB, unsigned char bLT, unsigned char bRT, short sTLX, short sTLY, short sTRX, short sTRY);
	void					SetPayloadFromBuffer(char * buf, int size);
	const ControllerPacket&		GetData(void) const { return w; }

protected:
	ControllerPacket w;
};


#include "FionaUT.h"


/*********************************************
 
 Following parts are for Fiona Interface
 
 
 ***********************************************/

extern std::vector<BasePacket*> fionaPackets;

extern	void _FionaUTSyncInit(void);
extern	void _FionaUTSyncSendJoystick(int i, const jvec3& p);
extern  void _FionaUTSyncSendCamera(const jvec3 &p, const quat &r);
extern	void _FionaUTSyncSendKeyboard(int key, int mod);
extern	void _FionaUTSyncSendWandButton(int b, int s, short w);
extern	void _FionaUTSyncSendTracker(int i, const jvec3& p, const quat& q);
extern  void _FionaUTSyncSendVMD(char **vmdMsg, int sizeOfMsg);
extern  void _FionaUTSyncSendVMD(const char *vmdMsg, int sizeOfMsg);
extern  void _FionaUTSyncSendFileIndex(int fileIndex, int bufferIndex);
extern  void _FionaUTSyncSendWiiFit(float fValues[4]);
extern	void _FionaUTSyncSendConfig(const jvec3& lEye, const jvec3& rEye, const jvec3& tO, const jvec3& kO);
extern  void _FionaUTSyncSendPhysics(float fTimestep, float fCurrTime);
extern  void _FionaUTSyncSendController(unsigned short wB, unsigned char bLT, unsigned char bRT, short sTLX, short sTLY, short sTRX, short sTRY);
extern  void _FionaUTSyncSendLeap(const Leap::Hand *pHand1, const Leap::Hand *pHand2, const Leap::Gesture::Type &gestureType1, const Leap::Gesture::State & state1, const Leap::Gesture::Type &gestureType2, const Leap::Gesture::State & state2);
extern	int  _FionaUTSyncGetTotalSize(unsigned char viewType);
extern	void _FionaUTSyncPackPackets(char* buf, int sz, bool firstView);
extern	void _FionaUTSyncClearPackets(void);
extern  void _FionaUTSyncSendMatrix(unsigned int index, float *p, float *v);

// Sync functions called within _FionaUTFrame
extern	void _FionaUTSyncMasterSync(void);
// New sync functions for obtaining data from a 2nd machine..
extern  void _FionaUTSyncMasterSlaveSync(void);
extern  void _FionaUTSyncProcessMasterSlavePacket(void);

extern	void _FionaUTSyncSlaveSync(void);


#endif
