#include "FionaNetwork.h"

#define DEBUG_PRINT 0

////////////////////////////CAVE MASTER////////////////////////////////
#include "FionaUT.h"

CaveMaster			*fionaNetMaster=0;
CaveSlave			*fionaNetMasterSlave=0;
CaveSlave			*fionaNetSlave=0;
std::vector<BasePacket*> fionaPackets;
static FionaNetwork fionaNetwork;


FionaNetwork::FionaNetwork(void)
{
	memset(masterBuffer, 0, FIONA_NETWORK_BUFFER_SIZE); 
	memset(masterBuffer2, 0, FIONA_NETWORK_BUFFER_SIZE);
	memset(slaveBuffer, 0, FIONA_NETWORK_BUFFER_SIZE); 
	memset(slaveBuffer2, 0, FIONA_NETWORK_BUFFER_SIZE);
	memset(masterSlaveBuffer, 0, FIONA_NETWORK_BUFFER_SIZE);
}

CaveMaster::~CaveMaster(void)
{
	for(int i = 0; i < fionaConf.numSlaves; ++i)
	{
		delete connections[i];
		connections[i] = 0;
	}

	delete[] connections;
}

void CaveMaster::ListenForSlaves(void)
{
	//TODO - check that the connection receive'd IP matches the ones listed above...?
	Socket listener;
	
	listener .Create();
	listener .Listen(fionaConf.port);

	connections = new Socket*[fionaConf.numSlaves];

	for(int i = 0; i < fionaConf.numSlaves; ++i)
	{
#if DEBUG_PRINT > 0
		printf("Master waiting for connection from slave %i\n", i);			
#endif
		Socket *pAccept = listener.Accept();
		connections[i] = new Socket();
		connections[i]->InitFromSocket(pAccept->_socket);
		
#ifndef LINUX_BUILD
		sockaddr client_info = {0};
		int addr_size = sizeof(client_info);
		getpeername(pAccept->_socket, &client_info, &addr_size);
		sockaddr_in *ipAddr = (struct sockaddr_in*)&client_info;
		char *sIP = inet_ntoa(ipAddr->sin_addr);
		
		printf("Received slave connection %d from %s\n", i, sIP);
#endif
		if(fionaConf.dualView)
		{
#ifndef LINUX_BUILD
			//this IP call isn't working right now - returns weird number ordering...
			
			printf("Dual View IP: %s\n", sIP);
			//compare this IP to the 3 dual view machines
			if(strcmp(sIP, "10.129.24.131")==0)
			{
				printf("got connection from dual view machine!\n");
				connections[i]->firstView = false;
			} 
			else if(strcmp(sIP, "10.129.24.132")==0)
			{
				printf("got connection from dual view machine!\n");
				connections[i]->firstView = false;
			}
			else if(strcmp(sIP, "10.129.24.133")==0)
			{
				printf("got connection from dual view machine!\n");
				connections[i]->firstView = false;
			}
			else if(strcmp(sIP, "10.129.24.141")==0)	//dev lab 2
			{
				printf("got connection from dual view machine!\n");
				connections[i]->firstView = false;
			}
#endif
		}
#if DEBUG_PRINT > 0
		printf("Master accepted connection from slave %i\n", i);			
#endif
	}
}

#ifndef LINUX_BUILD
__int64 BeginTime(double & freq)
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

void CaveMaster::Handshake(void)
{
	
	for(int i = 0; i < fionaConf.numSlaves; ++i)
	{
#ifndef LINUX_BUILD
		double pcFreq = 0.0;
		__int64 t = 0;

		if(fionaConf.showRenderTime)
		{
			t = BeginTime(pcFreq);
		}
#endif
		//this receive blocks..
		//when all slaves contact the master, we continue
		char wakeUpBit;

#if DEBUG_PRINT > 1
		printf("waiting for handshake from slave # %d\n",i);
#endif
		connections[i]->Receive(&wakeUpBit, 1, 0);

#ifndef LINUX_BUILD
		LARGE_INTEGER li;
		QueryPerformanceCounter(&li);
		if(fionaConf.showRenderTime)
		{
			printf("Master Handshake %d, %f\n", i, double(li.QuadPart-t)/pcFreq);
		}
#endif
	}
}

void CaveMaster::SendCavePacket(char *buffer, int len, bool firstView)
{
#if DEBUG_PRINT > 1
	printf("finally sending data: %d\n",len);
#endif
	for(int i = 0; i < fionaConf.numSlaves; ++i)
	{
		//send the current packet to the slaves
		if(connections[i]->firstView == firstView)
		{
			connections[i]->Send(buffer, len, 0);
		}
	}
}

void CaveMaster::WakeupSlaves(void)
{
	char wakeUpBit = 'a';
	for(int i = 0; i < fionaConf.numSlaves; ++i)
	{
#if DEBUG_PRINT > 1
		printf("waking up slaves: slave # %d\n",i);
#endif
		//send the current packet to the slaves
		connections[i]->Send(&wakeUpBit, 1);
	}
}

//////////////////////////////CAVE SLAVE//////////////////////////////////

//char *CaveSlave::sMasterIP = "10.129.24.140";
//char *CaveSlave::sMasterIP = "10.129.29.225";
//char *CaveSlave::sDualMasterIP = "10.129.24.130";

void CaveSlave::ConnectToMaster(void)
{
	short port = masterSlave ? fionaConf.masterSlavePort : fionaConf.port;
#if DEBUG_PRINT > 0
	printf("Slave connecting to master on %s:%d\n", fionaConf.masterIP.c_str(), port);			
#endif
	for( int i=0; i<100; i++ )
	{
		masterConnection.Create();
		bool connected = false;
		if(!masterSlave)
		{
			connected = masterConnection.Connect((char*)fionaConf.masterIP.c_str(), port);
		}
		else
		{
			printf("Connecting to secondary machine...%s\n", fionaConf.masterSlaveIP.c_str());
			connected = masterConnection.Connect((char*)fionaConf.masterSlaveIP.c_str(), port);
		}

		if( connected )
		{
			printf("Connected to server, succesfully\n");
			return;
		}
		else
			printf("Retrying .. trial (%d)\n",i);
		masterConnection.ShutDown();
		FionaUTSleep(0.1);
	}
	printf("Fail to connect to server..(too many trial..).. exiting\n");
}

void CaveSlave::Handshake(void)
{
#if DEBUG_PRINT > 1
	printf("Sending handshake signal to master\n");
#endif
	char wakeUpBit = 'a';
	masterConnection.Send(&wakeUpBit, 1);
}

void CaveSlave::WaitForMaster(void)
{
#if DEBUG_PRINT > 1
	printf("Waiting handshake signal from master\n");
#endif

	/*double pcFreq = 0.0;
	__int64 t = 0;
	if(fionaConf.showRenderTime)
	{
		t = BeginTime(pcFreq);
	}*/

	char wakeUpBit;
	masterConnection.Receive(&wakeUpBit, 1);

	/*LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	if(fionaConf.showRenderTime)
	{
		printf("Slave Wait %f\n", double(li.QuadPart-t)/pcFreq);
	}*/
}

void CaveSlave::ReceiveCavePacket(char **packetData, int size)
{
	masterConnection.Receive(*packetData, size, 0);
}

void CaveSlave::ReceivePacketHeader(int &numPackets, int &totalSize)
{
	masterConnection.Receive((char*)&numPackets, sizeof(int), 0);
	masterConnection.Receive((char*)&totalSize, sizeof(int), 0);
#if DEBUG_PRINT > 1
	printf("Received packet header - num packets %d, total size %d\n", numPackets, totalSize);			
#endif
}

///////////////////////////////////////CAVE PACKET//////////////////////////////////////

void UpdateMatrixPacket::SetPayload(unsigned int index, float *p, float *v)
{
	t.index = index;
	memcpy(t.v, v, sizeof(float)*16);
	memcpy(t.p, p, sizeof(float)*16);
}

void UpdateMatrixPacket::SetPayloadFromBuffer(char * buf, int size)
{
	memcpy((void*)&t, buf, size);
}

void UpdateFramePacket::SetPayload(unsigned long long fc)
{
	t.frameCount = fc;
}

void UpdateFramePacket::SetPayloadFromBuffer(char * buf, int size)
{
	memcpy((void*)&t, buf, size);
}

void UpdateTrackerPacket::SetPayload(short trackerIdx, const jvec3 &v, const quat &q)
{
	t.trackerIdx = trackerIdx;
	v.get(t.pos);
	q.get(t.quat);
}

void UpdateTrackerPacket::SetPayloadFromBuffer(char * buf, int size)
{
	memcpy((void*)&t, buf, size);
}

void UpdateJoystickPacket::SetPayload(short wandIdx, float fX, float fY, float fZ, float fW)
{
	w.wandIdx = wandIdx;
	w.x = fX;
	w.y = fY;
	w.z = fZ;
	w.w = fW;
}

void UpdateCameraPacket::SetPayload(const jvec3 &v, const quat &q)
{
	v.get(t.pos);
	q.get(t.rot);
}

void UpdateCameraPacket::SetPayloadFromBuffer(char * buf, int size)
{
	memcpy((void*)&t, buf, size);
}

void UpdateJoystickPacket::SetPayloadFromBuffer(char * buf, int size)
{
	memcpy((void*)&w, (void*)buf, size);
}

void UpdateWandButtonPacket::SetPayload(short btnIdx, unsigned char btnState, short wandIdx)
{
	w.buttonIdx = btnIdx;
	w.state = btnState;
	w.wandIdx = wandIdx;
}

void UpdateWandButtonPacket::SetPayloadFromBuffer(char * buf, int size)
{
	memcpy((void*)&w, (void*)buf, size);
}

void UpdateKeyboardPacket::SetPayload(unsigned int key, unsigned int mod)
{
	w.key = key;
	w.modifer = mod;
}

void UpdateKeyboardPacket::SetPayloadFromBuffer(char * buf, int size)
{
	memcpy((void*)&w, (void*)buf, size);
}

void UpdateConfigPacket::SetPayload(const jvec3& lEye, const jvec3& rEye, const jvec3& tO, const jvec3& kO)
{
	lEye.get(w.lEye);
	rEye.get(w.rEye);
	tO.get(w.trackerOffset);
	kO.get(w.kevinOffset);
}

void UpdateConfigPacket::SetPayloadFromBuffer(char * buf, int size)
{
	memcpy((void*)&w, (void*)buf, size);
}

void TestPacket::SetPayload(float fTest)
{
	w.test = fTest;
}

void TestPacket::SetPayloadFromBuffer(char *buf, int size)
{
	memcpy((void*)&w, (void*)buf, size);
}

void UpdatePhysicsSync::SetPayload(float fTimeStep, float fCurrTime)
{
	w.timeStep = fTimeStep;
	w.currTime = fCurrTime;
}

void UpdatePhysicsSync::SetPayloadFromBuffer(char * buf, int size)
{
	memcpy((void*)&w, (void*)buf, size);
}

void UpdateKinectData::SetPayload(float kinectData[60])
{
	memcpy((void*)&w, (void*)kinectData, 60 * sizeof(float));
}

void UpdateKinectData::SetPayloadFromBuffer(char * buf, int size)
{
	memcpy((void*)&w, (void*)buf, size);
}

void UpdateWiiFitData::SetPayload(float wiiFitData[4])
{
	memcpy((void*)&w, (void*)wiiFitData, 4 * sizeof(float));
}

void UpdateWiiFitData::SetPayloadFromBuffer(char * buf, int size)
{
	memcpy((void*)&w, (void*)buf, size);
}

void UpdateEMGData::SetPayload(unsigned int emgData[4], float fVoltage)
{
	for(int i = 0; i < 4; ++i)
	{
		w.values[i] = emgData[i];
	}
	w.fVoltage = fVoltage;
}

void UpdateEMGData::SetPayloadFromBuffer(char * buf, int size)
{
	memcpy((void*)&w, (void*)buf, size);
}

void UpdateSpeechData::SetPayload(float sV)
{
	w.speechVal = sV;
}

void UpdateSpeechData::SetPayloadFromBuffer(char * buf, int size)
{
	memcpy((void*)&w, (void*)buf, size);
}

#ifndef LINUX_BUILD
void UpdateLeapData::CopyHand(const Leap::Hand *pHand, LeapData::HandData & ourHand)
{
	Leap::Vector handDir = pHand->direction();
	Leap::Vector handPos = pHand->palmPosition();
	Leap::Vector handVel = pHand->palmVelocity();
	Leap::Vector palmDir = pHand->palmNormal();
	Leap::Vector sphereC = pHand->sphereCenter();

	static const float toMeters = 0.001f;

	ourHand.valid = true;
	ourHand.handDirection[0] = handDir.x;
	ourHand.handDirection[1] = handDir.y;
	ourHand.handDirection[2] = handDir.z;
	ourHand.handPosition[0] = handPos.x * toMeters;
	ourHand.handPosition[1] = handPos.y * toMeters;
	ourHand.handPosition[2] = handPos.z * toMeters;
	ourHand.handVelocity[0] = handVel.x;
	ourHand.handVelocity[1] = handVel.y;
	ourHand.handVelocity[2] = handVel.z;
	ourHand.palmNormal[0] = palmDir.x;
	ourHand.palmNormal[1] = palmDir.y;
	ourHand.palmNormal[2] = palmDir.z;
	ourHand.sphereCenter[0] = sphereC.x * toMeters;
	ourHand.sphereCenter[1] = sphereC.y * toMeters;
	ourHand.sphereCenter[2] = sphereC.z * toMeters;

	ourHand.sphereRadius = pHand->sphereRadius() * toMeters;

	//now copy optional fingers...
	int fingerCount = pHand->fingers().count();
	if(fingerCount > 0)
	{
		for(int i = 0; i < fingerCount; ++i)
		{
			CopyFinger(pHand->fingers()[i], ourHand.fingers[i]);
		}
	}
}

void UpdateLeapData::CopyFinger(const Leap::Finger &pFinger, LeapData::HandData::FingerData & ourFinger)
{
	Leap::Vector fingerDir = pFinger.direction();
	Leap::Vector fingerPos = pFinger.tipPosition();
	Leap::Vector fingerVel = pFinger.tipVelocity();

	static const float toMeters = 0.001f;

	ourFinger.valid = true;
	ourFinger.tipDirection[0] = fingerDir.x;
	ourFinger.tipDirection[1] = fingerDir.y;
	ourFinger.tipDirection[2] = fingerDir.z;
	ourFinger.length = pFinger.length() * toMeters;
	ourFinger.tipPosition[0] = fingerPos.x * toMeters;
	ourFinger.tipPosition[1] = fingerPos.y * toMeters;
	ourFinger.tipPosition[2] = fingerPos.z * toMeters;
	ourFinger.tipVelocity[0] = fingerVel.x;
	ourFinger.tipVelocity[1] = fingerVel.y;
	ourFinger.tipVelocity[2] = fingerVel.z;
}

void UpdateLeapData::SetPayload(const Leap::Hand *pHand1, const Leap::Hand *pHand2, const Leap::Gesture::Type &gestureType1, const Leap::Gesture::State &state1, const Leap::Gesture::Type &gestureType2, const Leap::Gesture::State &state2)
{
	memset(&w, 0, sizeof(LeapData));
	
	if(pHand1 != 0)
	{
		w.hand1.t = gestureType1;
		w.hand1.s = state1;
		CopyHand(pHand1, w.hand1);
	}

	if(pHand2 != 0)
	{
		w.hand2.t = gestureType2;
		w.hand2.s = state2;
		CopyHand(pHand2, w.hand2);
	}
}

void UpdateLeapData::SetPayloadFromBuffer(char * buf, int size)
{
	memcpy((void*)&w, (void*)buf, size);
}
#endif

void UpdateVMDData::SetPayload(char **vmdMsg, int sizeOfMsg)
{
	memset(w.vmdMsg, 0, sizeof(w.vmdMsg));
	memcpy(w.vmdMsg, *vmdMsg, sizeOfMsg);
}

void UpdateVMDData::SetPayload(const char *vmdMsg, int sizeOfMsg)
{
	memset(w.vmdMsg, 0, sizeof(w.vmdMsg));
	memcpy(w.vmdMsg, vmdMsg, sizeOfMsg);
}

void UpdateVMDData::SetPayloadFromBuffer(char * buf, int size)
{
	memcpy(w.vmdMsg, (void*)buf, size);
}

void UpdateKinectVideoData::SetPayloadFromBuffer(char* buf, int sz)
{
	memcpy(&data, buf, sz);
}

void UpdateTimeVaryingData::SetPayload(int fileIndex, int bufferIndex)
{
	w.fileIndex = fileIndex;
	w.bufferIndex = bufferIndex;
}

void UpdateTimeVaryingData::SetPayloadFromBuffer(char * buf, int size)
{
	memcpy(&w, (void*)buf, size);
}

void UpdateControllerData::SetPayload(unsigned short wB, unsigned char bLT, unsigned char bRT, short sTLX, short sTLY, short sTRX, short sTRY)
{
	w.wButtons = wB;
	w.bLeftTrigger = bLT;
	w.bRightTrigger = bRT;
	w.sThumbLX = sTLX;
	w.sThumbLY = sTLY;
	w.sThumbRX = sTRX;
	w.sThumbRY = sTRY;
}

void UpdateControllerData::SetPayloadFromBuffer(char * buf, int size)
{
	memcpy(&w, (void*)buf, size);
}

void UpdateVolumePacket::SetPayload(VolumeData const& data)
{
	w = data;
}

void UpdateVolumePacket::SetPayloadFromBuffer(char * buf, int size)
{
	memcpy((void*)&w, (void*)buf, size);
}

//*******************************************************
//
//    FionaUT interface functions
//
//*******************************************************


void _FionaUTSyncInit(void)
{
	if(fionaConf.master)
	{
		fionaNetMaster = new CaveMaster();
		printf("Waiting for %d slave connections...\n", fionaConf.numSlaves);
		fionaNetMaster->ListenForSlaves();
		if(fionaConf.masterSlave)
		{
			printf("Setting up secondary connection...\n");
			fionaNetMasterSlave = new CaveSlave(true);
			fionaNetMasterSlave->ConnectToMaster();
		}
	}
	else if(fionaConf.slave)
	{
		fionaNetSlave = new CaveSlave();
		fionaNetSlave->ConnectToMaster();
	}
	else if(fionaConf.masterSlave)
	{
		printf("Setting up secondary connection in dev lab...\n");
		fionaNetMasterSlave = new CaveSlave(true);
		fionaNetMasterSlave->ConnectToMaster();
	}
}

void _FionaUTSyncSendJoystick(int i, const vec4& p)
{
	UpdateJoystickPacket *packet = new UpdateJoystickPacket(BasePacket::UPDATE_JOYSTICK);
	packet->SetPayload(i, p.x, p.z, p.y, p.h);
#if DEBUG_PRINT > 1
	printf("Adding joystick packet %f %f %f %f\n", p.x, p.y, p.z, p.h);
#endif
	fionaPackets.push_back(packet);
}

void _FionaUTSyncSendKeyboard(int key, int mod)
{
	UpdateKeyboardPacket *packet = new UpdateKeyboardPacket(BasePacket::UPDATE_KEYBOARD);
	packet->SetPayload(key,mod);
#if DEBUG_PRINT > 1
	printf("Adding keyboard packet\n");
#endif
	fionaPackets.push_back(packet);
}

void _FionaUTSyncSendCamera(const jvec3 &p, const quat &r)
{
	UpdateCameraPacket *packet = new UpdateCameraPacket(BasePacket::UPDATE_CAMERA);
	packet->SetPayload(p, r);
#if DEBUG_PRINT > 1
	printf("Adding camera packet\n");
#endif
	fionaPackets.push_back(packet);
}

void _FionaUTSyncSendWandButton(int b, int s, short w)
{
	UpdateWandButtonPacket *packet = new UpdateWandButtonPacket(BasePacket::UPDATE_WANDBUTTONS);
	packet->SetPayload(b,s, w);
#if DEBUG_PRINT > 1
	printf("Adding wand button packet\n");
#endif
	fionaPackets.push_back(packet);
}
void _FionaUTSyncSendTracker(int i, const jvec3& p, const quat& q)
{
	UpdateTrackerPacket *packet = new UpdateTrackerPacket(BasePacket::UPDATE_TRACKER);
	packet->SetPayload(i, p, q);
#if DEBUG_PRINT > 1
	printf("Adding tracker packet\n");
#endif
	fionaPackets.push_back(packet);
}

void _FionaUTSyncSendWiiFit(float fValues[4])
{
	UpdateWiiFitData *packet = new UpdateWiiFitData(BasePacket::UPDATE_WIIFIT);
	packet->SetPayload(fValues);
#if DEBUG_PRINT > 1
	printf("Adding wii-fit packet\n");
#endif
	fionaPackets.push_back(packet);
}

void _FionaUTSyncSendVMD(char **vmdMsg, int sizeOfMsg)
{
	UpdateVMDData *packet = new UpdateVMDData(BasePacket::UPDATE_VMD);
	packet->SetPayload(vmdMsg, sizeOfMsg);
#if DEBUG_PRINT > 1
	printf("Adding VMD packet\n");
#endif
	fionaPackets.push_back(packet);
}

void _FionaUTSyncSendVMD(const char *vmdMsg, int sizeOfMsg)
{
	UpdateVMDData *packet = new UpdateVMDData(BasePacket::UPDATE_VMD);
	packet->SetPayload(vmdMsg, sizeOfMsg);
#if DEBUG_PRINT > 1
	printf("Adding VMD packet\n");
#endif
	fionaPackets.push_back(packet);
}

void _FionaUTSyncSendFileIndex(int fileIndex, int bufferIndex)
{
	UpdateTimeVaryingData *p = new UpdateTimeVaryingData(BasePacket::UPDATE_TIME_VARYING_FILE);
	p->SetPayload(fileIndex, bufferIndex);
	fionaPackets.push_back(p);
}

void _FionaUTSyncVolume(VolumeData const& data)
{
	UpdateVolumePacket *pUpdateMatrix = new UpdateVolumePacket(BasePacket::UPDATE_VOLUME);
	pUpdateMatrix->SetPayload(data);
	fionaPackets.push_back(pUpdateMatrix);
}

#ifndef LINUX_BUILD
void _FionaUTSyncSendLeap(const Leap::Hand *pHand1, const Leap::Hand *pHand2, const Leap::Gesture::Type &gestureType1, const Leap::Gesture::State & state1, const Leap::Gesture::Type &gestureType2, const Leap::Gesture::State & state2)
{
	if(pHand1 != 0)
	{
		UpdateLeapData *pUpdateLeap = new UpdateLeapData(BasePacket::UPDATE_LEAP);
		pUpdateLeap->SetPayload(pHand1, pHand2, gestureType1, state1, gestureType2, state2);
		fionaPackets.push_back(pUpdateLeap);
	}
}
#endif

void _FionaUTSendConfig(const jvec3& lEye, const jvec3& rEye, const jvec3& tO, const jvec3& kO)
{

}

void _FionaUTSyncSendMatrix(unsigned int index, float *p, float *v)
{
	UpdateMatrixPacket *pUpdateMatrix = new UpdateMatrixPacket(BasePacket::UPDATE_MATRIX);
	pUpdateMatrix->SetPayload(index, p, v);
	fionaPackets.push_back(pUpdateMatrix);
}

void _FionaUTSyncSendPhysics(float fTimestep, float fCurrTime)
{
	UpdatePhysicsSync *packet = new UpdatePhysicsSync(BasePacket::UPDATE_PHYSICS);
	packet->SetPayload(fTimestep, fCurrTime);
#if DEBUG_PRINT > 1
	printf("Adding physics packet\n");
#endif
	fionaPackets.push_back(packet);
}

void _FionaUTSyncSendController(unsigned short wB, unsigned char bLT, unsigned char bRT, short sTLX, short sTLY, short sTRX, short sTRY)
{
	UpdateControllerData *packet = new UpdateControllerData(BasePacket::UPDATE_CONTROLLER);
	packet->SetPayload(wB, bLT, bRT, sTLX, sTLY, sTRX, sTRY);
#if DEBUG_PRINT > 1
	printf("Adding physics packet\n");
#endif
	fionaPackets.push_back(packet);
}

int _FionaUTSyncGetTotalSize(unsigned char viewType)
{
	//assemble packet - current test code does this in the VPRN call back
	//assemble packet from queue
	//figure out total size and number of packets we're sending over...
	size_t numPackets = fionaPackets.size();
	
	int totalSize = sizeof(int) * 2;	//for num packets and total size
#if DEBUG_PRINT > 1
	printf("Counting size of %d packets ", numPackets);
#endif
	for(int i=0; i<(int)numPackets; i++ )
	{
		if(fionaPackets[i]->GetViewType() == (BasePacket::viewType_t)viewType ||
			fionaPackets[i]->GetViewType() == BasePacket::BOTH)
		{
			totalSize+=sizeof(PacketHeader)+fionaPackets[i]->GetSize();
		}
	}
#if DEBUG_PRINT > 1
	printf("total size %d\n", totalSize);
#endif
	return totalSize;
}

int _FionaUTGetNumViewPackets(unsigned char viewType)
{
	int numPackets = 0;
	size_t nPacket = fionaPackets.size();
	for (size_t i = 0; i < nPacket; ++i)
	{
		if(fionaPackets[i]->GetViewType() == (BasePacket::viewType_t)viewType ||
			fionaPackets[i]->GetViewType() == BasePacket::BOTH)
		{
			numPackets++;
		}
	}

	return numPackets;
}

void _FionaUTSyncPackPackets(char* buf, int sz, bool firstView)
{
	//int nPacket = fionaPackets.size();
	BasePacket::viewType_t t = firstView ? BasePacket::SINGLE_VIEW : BasePacket::DUAL_VIEW;
	int nPacket = _FionaUTGetNumViewPackets(firstView ? 0 : 1);
	int bufSize = 0;
	memcpy(buf, &nPacket, sizeof(int));
	bufSize += sizeof(int);
	memcpy(buf+bufSize, &sz, sizeof(int));
	bufSize += sizeof(int);
	
	for(int i=0; i<(int)nPacket; i++ )
	{
		if(fionaConf.dualView)
		{
			if(fionaPackets[i]->GetViewType() == t || fionaPackets[i]->GetViewType() == BasePacket::BOTH)
			{
				memcpy(buf + bufSize, (char*)fionaPackets[i]->GetHeader(), sizeof(PacketHeader));
				bufSize += sizeof(PacketHeader);
				memcpy(buf + bufSize, (char*)fionaPackets[i]->GetPayload(), fionaPackets[i]->GetSize());
				bufSize += fionaPackets[i]->GetSize();
			}
		}
		else
		{
			memcpy(buf + bufSize, (char*)fionaPackets[i]->GetHeader(), sizeof(PacketHeader));
			bufSize += sizeof(PacketHeader);
			memcpy(buf + bufSize, (char*)fionaPackets[i]->GetPayload(), fionaPackets[i]->GetSize());
			bufSize += fionaPackets[i]->GetSize();
		}
	}
#if DEBUG_PRINT > 1
	printf("Buf size %d\n", bufSize);
#endif
}

void _FionaUTSyncClearPackets(void)
{
	for(int i=0; i<(int)fionaPackets.size(); i++){ delete fionaPackets[i]; fionaPackets[i]=NULL; }
	fionaPackets.clear();
}

// Sync functions called within _FionaUTFrame
void _FionaUTSyncMasterSync(void)
{
	//for dual view, we need to adjust this function...
	//need knowledge of which set of slave machines to send particular packets to
	//basically just need two buffers and each packet should have a flag for single view, dual view or both
	//then adjust size for each.
	//this also needs to be special cased for dev lab dual view - we don't want to send to any other machine other than the
	//dual view machine...
	int totalSize = 0;
	if(fionaConf.appType != FionaConfig::DEVLAB)	//special case dev lab dual view...
	{
		totalSize = _FionaUTSyncGetTotalSize(0);	//first viewer..
		//memset(fionaNetwork.masterBuffer, 0, FionaNetwork::FIONA_NETWORK_BUFFER_SIZE);
		_FionaUTSyncPackPackets(fionaNetwork.masterBuffer, totalSize, true);
		fionaNetMaster->SendCavePacket(fionaNetwork.masterBuffer, totalSize);
	}

	if(fionaConf.dualView)
	{
		totalSize = _FionaUTSyncGetTotalSize(1);	//second viewer
		//memset(fionaNetwork.masterBuffer2, 0, FionaNetwork::FIONA_NETWORK_BUFFER_SIZE);
		_FionaUTSyncPackPackets(fionaNetwork.masterBuffer2, totalSize, false);
		fionaNetMaster->SendCavePacket(fionaNetwork.masterBuffer2, totalSize, false);
	}

	_FionaUTSyncClearPackets();
}

void _FionaUTPrintPacketType(int type, int sz)
{
	switch(type)
	{
		case BasePacket::UPDATE_TRACKER:
			printf("Received packet tracker UPDATE HEAD size: %d\n", sz);
			break;
		case BasePacket::UPDATE_JOYSTICK:
			printf("Received packet header UPDATE JOYSTICK size: %d\n", sz);
			break;
		case BasePacket::UPDATE_WANDBUTTONS:
			printf("Received packet header UPDATE WANDBUTTON size: %d\n", sz);
			break;
		case BasePacket::UPDATE_KEYBOARD:
			printf("Received packet header KEYBOARD_PRESS size: %d\n", sz);
			break;
		case BasePacket::UPDATE_PHYSICS:
			printf("Received packet header PHYSICS_SYNC, size %d\n", sz);
			break;
		case BasePacket::UPDATE_EMG:
			printf("Received packet header UPDATE_EMG, size %d\n", sz);
			break;
		case BasePacket::UPDATE_KINECT:
			printf("Received packet header UPDATE_KINECT, size %d\n", sz);
			break;
		case BasePacket::UPDATE_WIIFIT:
			printf("Received packet header UPDATE_WIIFIT, size %d\n", sz);
			break;
		case BasePacket::UPDATE_CONFIG:
			printf("Received packet header UPDATE_CONFIG, size %d\n", sz);
			break;
		case BasePacket::UPDATE_SPEECH:
			printf("Received packet header UPDATE_SPEECH, size %d\n", sz);
			break;
		case BasePacket::UPDATE_VMD:
			printf("Received packet header UPDATE_VMD, size %d\n", sz);
			break;
		case BasePacket::UPDATE_LEAP:
			printf("Received packet head UPDATE_LEAP, size %d\n", sz);
			break;
		case BasePacket::UPDATE_KINECT_VIDEO:
			printf("Received packet header UPDATE_KINECT_VIDEO %d\n", sz);
			break;
		case BasePacket::UPDATE_CAMERA:
			printf("Received packet head UPDATE_CAMERA, size %d\n", sz);
			break;
		case BasePacket::UPDATE_MATRIX:
			printf("Received packet head UPDATE_MATRIX, size %d\n", sz);
			break;
		case BasePacket::UPDATE_CONTROLLER:
			printf("Received packet header UPDATE_CONTROLLER, size %d\n", sz);
			break;
		case BasePacket::UPDATE_VOLUME:
			printf("Received packet header UPDATE_VOLUME, size %d\n", sz);
			break;
		default:
			printf("Received unknown packet header\n");
	}
}


char* _FionaUTSyncProcessPacket(int type, int sz, char* bufPtr)
{	
	char* ptr = bufPtr;
	switch(type)
	{
		case BasePacket::UPDATE_TRACKER:
		{
			UpdateTrackerPacket p(type);
			p.SetPayloadFromBuffer(ptr, sz);
			const TrackerData& data = (TrackerData&)p.GetData();
#if DEBUG_PRINT > 2
			printf("Received update head packet.");		
			printf("pos - %f, %f, %f\n", data.pos[0], data.pos[1], data.pos[2]);
			printf("quat - %f, %f, %f, %f\n", data.quat[3], data.quat[0], data.quat[1],data.quat[2]);
#endif
			_FionaUTTracker(data.trackerIdx, jvec3(data.pos), quat(data.quat));
			break;
		} 
		case BasePacket::UPDATE_JOYSTICK:
		{
			UpdateJoystickPacket p(BasePacket::UPDATE_JOYSTICK);
			p.SetPayloadFromBuffer(ptr, sz);
			const JoystickData& data = (JoystickData&)p.GetData();
			//printf("Receiving: %f %f %f %f\n", data.x, data.y, data.z, data.w);
			_FionaUTJoystick(data.wandIdx, vec4(data.x,data.z,data.y, data.w));
			 break;
		}
		case BasePacket::UPDATE_WANDBUTTONS:
		{
			UpdateWandButtonPacket p(BasePacket::UPDATE_WANDBUTTONS);
			p.SetPayloadFromBuffer(ptr, sz);
			const WandButtonData& data = (WandButtonData&)p.GetData();
			_FionaUTWandButton(data.buttonIdx,data.state, data.wandIdx);
			 break;
		}
		case BasePacket::UPDATE_KEYBOARD:
		{
			UpdateKeyboardPacket p(BasePacket::UPDATE_KEYBOARD);
			p.SetPayloadFromBuffer(ptr, sz);
			const KeyboardData& data = (KeyboardData&)p.GetData();
			_FionaUTKeyboard(_FionaUTFirstWindow(),data.key);
			break;
		} 
		case BasePacket::UPDATE_PHYSICS:
		{
			UpdatePhysicsSync p(BasePacket::UPDATE_PHYSICS);
			p.SetPayloadFromBuffer(ptr, sz);
			const PhysicsData & data = (PhysicsData&)p.GetData();
			//set the physics timestamp..
			fionaConf.physicsStep = data.timeStep;
			fionaConf.physicsTime = data.currTime;
			break;
		}
		case BasePacket::UPDATE_KINECT:
		{
			UpdateKinectData p(BasePacket::UPDATE_KINECT);
			p.SetPayloadFromBuffer(ptr, sz);
			const KinectData & data = (KinectData&)p.GetData();
			//for(int i = 3; i < 4; i++) 
			//{
			//	printf("Upper extremity: (%f, %f, %f)\n", data.values[i*7+0], data.values[i*7+1], data.values[i*7+2]);
			//}
			//TODO - update the kinect data
			//const LeapData & data = p.GetData();
			//we don't necesssarily get leap data every frame (if no hands are tracked), so we should clear out the existing leap data after it's used in the render loop..
			memcpy(&(fionaConf.kinectData), &(data), sizeof(KinectData));
			//printf("Receiving Kinect data !\n");
			break;
		}
		case BasePacket::UPDATE_WIIFIT:
		{
			UpdateWiiFitData p(BasePacket::UPDATE_WIIFIT);
			p.SetPayloadFromBuffer(ptr, sz);
			const WiiFitData & data = (WiiFitData&)p.GetData();
			fionaConf.wiiTopLeft = data.values[2];
			fionaConf.wiiBottomLeft = data.values[0];
			fionaConf.wiiBottomRight = data.values[1];
			fionaConf.wiiTopRight = data.values[3];
			//printf("Received wii-fit packet: %f, %f, %f, %f\n", fionaConf.wiiTopLeft, fionaConf.wiiTopRight, fionaConf.wiiBottomLeft, fionaConf.wiiBottomRight);
			break;
		} 
		case BasePacket::UPDATE_EMG:
		{
			UpdateEMGData p(BasePacket::UPDATE_EMG);
			p.SetPayloadFromBuffer(ptr, sz);
			const EMGData & data = (EMGData&)p.GetData();
			fionaConf.emgRaw = data.values[0];
			fionaConf.emgAvg = data.values[1];
			fionaConf.emgBinary = data.values[2];
			fionaConf.emgThresh = data.values[3];
			//printf("Received EMG packet: %d, %d, %d, %d\n", fionaConf.emgRaw, fionaConf.emgAvg, fionaConf.emgBinary, fionaConf.emgThresh);
			break;
		} 
		case BasePacket::UPDATE_SPEECH:
		{
			UpdateSpeechData p(BasePacket::UPDATE_SPEECH);
			p.SetPayloadFromBuffer(ptr, sz);
			const SpeechData & data = p.GetData();
			//lookup color value..
			fionaConf.speechVal = data.speechVal;
			printf("Received Speech Value: %f\n", fionaConf.speechVal);
			break;
		}
		case BasePacket::UPDATE_VMD:
		{
			UpdateVMDData v(BasePacket::UPDATE_VMD);
			v.SetPayloadFromBuffer(ptr, sz);
			const VMDData &data = v.GetData();
			//this is where we need to make the VMD TCL call...
			//need to get to the vmd scene somehow from here...
			if(scene != 0)
			{
				scene->executeCommand(data.vmdMsg);
			}
			break;
		}
		case BasePacket::UPDATE_TIME_VARYING_FILE:
		{
			UpdateTimeVaryingData v(BasePacket::UPDATE_TIME_VARYING_FILE);
			v.SetPayloadFromBuffer(ptr, sz);
			fionaConf.tvFileIndex = v.GetData().fileIndex;
			fionaConf.tvBufferIndex = v.GetData().bufferIndex;
		}
		case BasePacket::UPDATE_CONTROLLER:
		{
			UpdateControllerData v(BasePacket::UPDATE_CONTROLLER);
			v.SetPayloadFromBuffer(ptr, sz);
			//perform callback w/ controller info we've obtained..
			//perhaps hook into scene class?
			const ControllerPacket &d = v.GetData();
#ifndef LINUX_BUILD
			_FionaUTJoypad(d.wButtons, d.bLeftTrigger, d.bRightTrigger, d.sThumbLX, d.sThumbLY, d.sThumbRX, d.sThumbRY);
#endif
			break;
		}
		case BasePacket::UPDATE_CAMERA:
		{
			UpdateCameraPacket p(BasePacket::UPDATE_CAMERA);
			p.SetPayloadFromBuffer(ptr, sz);
			const CameraData& data = (CameraData&)p.GetData();
			if(scene != 0)
			{
				//printf("Updating camera\n");
				scene->setCamera(data.pos, data.rot);
			}
			break;
		}
		case BasePacket::UPDATE_LEAP:
		{
#ifndef LINUX_BUILD
			UpdateLeapData p(BasePacket::UPDATE_LEAP);
			p.SetPayloadFromBuffer(ptr, sz);
			const LeapData & data = p.GetData();
			//we don't necesssarily get leap data every frame (if no hands are tracked), so we should clear out the existing leap data after it's used in the render loop..
			memcpy(&(fionaConf.leapData.hand1), &(data.hand1), sizeof(LeapData));
			memcpy(&(fionaConf.leapData.hand2), &(data.hand2), sizeof(LeapData));
#if DEBUG_PRINT > 2
			printf("Received Leap Data\n");
#endif
#endif
			break;
		}
		case BasePacket::UPDATE_VOLUME:
		{
			UpdateVolumePacket p(BasePacket::UPDATE_VOLUME);
			p.SetPayloadFromBuffer(ptr, sz);
			scene->updateVolumeState(p.GetData());
			break;
		}
		case BasePacket::TEST:
		{
			TestPacket w(type);
			w.SetPayloadFromBuffer(ptr, sz);
#if DEBUG_PRINT > 0
			float fTest = w.GetPacket().test;
			printf("Received test packet:\n");
			printf("Test Value: %f\n", fTest);
#endif
			break;
		} 

		case BasePacket::UPDATE_KINECT_VIDEO:
			{
				//printf("Received kinect video data\n");

				const KinectVideoData* vd = reinterpret_cast<KinectVideoData*>(ptr);
				
				fionaConf.kinectVideoData.colorFrame = vd->colorFrame;
				fionaConf.kinectVideoData.depthFrame = vd->depthFrame;

				memcpy(fionaConf.kinectVideoData.color, vd->color, sizeof(fionaConf.kinectVideoData.color));
				memcpy(fionaConf.kinectVideoData.depth, vd->depth, sizeof(fionaConf.kinectVideoData.depth));
			}
			break;


	}
	ptr += sz;
	return ptr;
}

char* _FionaUTSyncProcessMasterSlavePacket(int type, int sz, char* bufPtr)
{
	char* ptr = bufPtr;
	switch(type)
	{
		case BasePacket::UPDATE_WIIFIT:
		{
			UpdateWiiFitData *p = new UpdateWiiFitData(BasePacket::UPDATE_WIIFIT);
			p->SetPayloadFromBuffer(ptr, sz);
			fionaPackets.push_back(p);
			break;
		} 
		case BasePacket::UPDATE_KINECT:
		{
			UpdateKinectData *p = new UpdateKinectData(BasePacket::UPDATE_KINECT);
			p->SetPayloadFromBuffer(ptr, sz);
			fionaPackets.push_back(p);
			break;
		} 
		case BasePacket::UPDATE_CAMERA:
		{
			UpdateCameraPacket *p = new UpdateCameraPacket(BasePacket::UPDATE_CAMERA);
			p->SetPayloadFromBuffer(ptr, sz);
			fionaPackets.push_back(p);
			break;
		} 
		case BasePacket::UPDATE_EMG:
		{	
			UpdateEMGData *p = new UpdateEMGData(BasePacket::UPDATE_EMG);
			p->SetPayloadFromBuffer(ptr, sz);
			fionaPackets.push_back(p);
			break;
		} 
		case BasePacket::UPDATE_SPEECH:
		{
			UpdateSpeechData *p = new UpdateSpeechData(BasePacket::UPDATE_SPEECH);
			p->SetPayloadFromBuffer(ptr, sz);
			fionaPackets.push_back(p);
			break;
		}
		case BasePacket::UPDATE_VMD:
		{
			UpdateVMDData *p = new UpdateVMDData(BasePacket::UPDATE_VMD);
			p->SetPayloadFromBuffer(ptr, sz);
			fionaPackets.push_back(p);
			break;
		}
		case BasePacket::UPDATE_LEAP:
		{
#ifndef LINUX_BUILD
			UpdateLeapData *p = new UpdateLeapData(BasePacket::UPDATE_LEAP);
			p->SetPayloadFromBuffer(ptr, sz);
			fionaPackets.push_back(p);
#endif
			break;		
		}

		case BasePacket::UPDATE_KINECT_VIDEO:
		{
			std::clog << "KinectVideoData size: " << sizeof(KinectVideoData) << std::endl;

			UpdateKinectVideoData* p = new UpdateKinectVideoData();
			p->SetPayloadFromBuffer(ptr, sz);
			fionaPackets.push_back( p );
			break;
		}
		
		case BasePacket::UPDATE_MATRIX:
		{
			UpdateMatrixPacket *p = new UpdateMatrixPacket();
			p->SetPayloadFromBuffer(ptr, sz);
			fionaPackets.push_back(p);
			break;
		}
		
		case BasePacket::UPDATE_CONTROLLER:
		{
			UpdateControllerData *p = new UpdateControllerData();
			p->SetPayloadFromBuffer(ptr, sz);
			fionaPackets.push_back(p);
			break;
		}

		case BasePacket::UPDATE_VOLUME:
		{
			UpdateVolumePacket *p = new UpdateVolumePacket();
			p->SetPayloadFromBuffer(ptr, sz);
			fionaPackets.push_back(p);
			break;
		}
		case BasePacket::TEST:
		{
			TestPacket w(type);
			w.SetPayloadFromBuffer(ptr, sz);
#if DEBUG_PRINT > 0
			float fTest = w.GetPacket().test;
			printf("Received test packet:\n");
			printf("Test Value: %f\n", fTest);
#endif
			break;
		} 
	}
	ptr += sz;
	return ptr;
}

void _FionaUTSyncSlaveSync(void)
{
	// We do not need this..
	// Shake hand only before sync
	//	slave->Handshake();
	
	//todo - clean up
	//receive packet
	int numPackets = 0;
	int totalSize = 0;
	fionaNetSlave->ReceivePacketHeader(numPackets, totalSize);

#if DEBUG_PRINT > 1
	printf("total size: %d (of %d packets)\n",totalSize,numPackets);
#endif

	if(numPackets <= 0 || totalSize<=0 ) return;
	
	int mainHeaderSize = sizeof(numPackets) + sizeof(totalSize);

	//memset(fionaNetwork.slaveBuffer, 0, totalSize-mainHeaderSize);

	char *buf = fionaNetwork.slaveBuffer;
	fionaNetSlave->ReceiveCavePacket(&buf, totalSize-mainHeaderSize);
	
	//now process each packet we get..
	//todo - factory type system to process data..
	char *bufPtr = buf;
	for(int j = 0; j < numPackets; ++j)
	{
		PacketHeader head;
		memcpy((void*)&head, bufPtr, sizeof(PacketHeader));
		bufPtr += sizeof(PacketHeader);

#if DEBUG_PRINT > 0
		_FionaUTPrintPacketType(head.type,head.size);
#endif
		bufPtr = _FionaUTSyncProcessPacket(head.type, head.size, bufPtr);
	}
}

void _FionaUTSyncMasterSlaveSync(void)
{
	//pass all packets obtained from the 2ndary machine onto the slaves.
	int numPackets = 0;
	int totalSize = 0;
	fionaNetMasterSlave->ReceivePacketHeader(numPackets, totalSize);
	//printf("MASTER SLAVE: total size: %d (of %d packets)\n",totalSize,numPackets);
	//todo - could make this a static buffer w/ a larger size that we know our messages are never going to be larger than
			 //to avoid dynamic allocations
	// JOONY
	if(numPackets <= 0 || totalSize<=0 ) return;
	
	int mainHeaderSize = sizeof(numPackets) + sizeof(totalSize);
	//char *buf = new char[totalSize-mainHeaderSize];
	//memset(buf, 0, totalSize-mainHeaderSize);
	
	char *buf = fionaNetwork.masterSlaveBuffer;
	//gApps[i]->slave->masterConnection.Receive(buf,totalSize-mainHeaderSize, 0); 
	fionaNetMasterSlave->ReceiveCavePacket(&buf, totalSize-mainHeaderSize);
	
	//now process each packet we get..
	//todo - factory type system to process data..
	char *bufPtr = buf;
	for(int j = 0; j < numPackets; ++j)
	{
		PacketHeader head;
		memcpy((void*)&head, bufPtr, sizeof(PacketHeader));
		bufPtr += sizeof(PacketHeader);

#if DEBUG_PRINT > 0
		_FionaUTPrintPacketType(head.type,head.size);

		if (head.type == BasePacket::UPDATE_KINECT_VIDEO)
		{	
			std::clog << "Received kinect video data!\n";
			std::clog << "KinectVideoData size: " << sizeof(KinectVideoData) << std::endl;
			std::clog << "Received: " << head.size << std::endl;

		}

#endif
		//std::clog << "Received " << head.type << ", size: " << head.size << std::endl;


		if(fionaConf.masterSlave && !fionaConf.master)
		{
			
			//if just in dev lab process the packet right here instead of preparing it to send on
			bufPtr = _FionaUTSyncProcessPacket(head.type, head.size, bufPtr);
		}
		else if(fionaConf.masterSlave && fionaConf.master)
		{
			//also process on the head node as well...(to obtain color and stuff)?
			char *ptr = _FionaUTSyncProcessPacket(head.type, head.size, bufPtr);
			bufPtr = _FionaUTSyncProcessMasterSlavePacket(head.type, head.size, bufPtr);
		}
	}
	//delete[] buf;
}


