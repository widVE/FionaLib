/************************************************************************************

PublicHeader:   OVR.h
Filename    :   OVR_SensorFusion.h
Content     :   Methods that determine head orientation from sensor data over time
Created     :   October 9, 2012
Authors     :   Michael Antonov, Steve LaValle, Max Katsev

Copyright   :   Copyright 2013 Oculus VR, Inc. All Rights reserved.

Licensed under the Oculus VR SDK License Version 2.0 (the "License"); 
you may not use the Oculus VR SDK except in compliance with the License, 
which is provided at the time of installation or download, or which 
otherwise accompanies this software in either electronic or hard copy form.

You may obtain a copy of the License at

http://www.oculusvr.com/licenses/LICENSE-2.0 

Unless required by applicable law or agreed to in writing, the Oculus VR SDK 
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*************************************************************************************/

#ifndef OVR_SensorFusion_h
#define OVR_SensorFusion_h

#include "OVR_Device.h"
#include "OVR_SensorFilter.h"
#include <time.h>

namespace OVR {

//-------------------------------------------------------------------------------------
// ***** SensorFusion

// SensorFusion class accumulates Sensor notification messages to keep track of
// orientation, which involves integrating the gyro and doing correction with gravity.
// Magnetometer based yaw drift correction is also supported; it is usually enabled
// automatically based on loaded magnetometer configuration.
// Orientation is reported as a quaternion, from which users can obtain either the
// rotation matrix or Euler angles.
//
// The class can operate in two ways:
//  - By user manually passing MessageBodyFrame messages to the OnMessage() function. 
//  - By attaching SensorFusion to a SensorDevice, in which case it will
//    automatically handle notifications from that device.


class SensorFusion : public NewOverrideBase
{
    enum
    {
        MagMaxReferences = 1000
    };        

public:
    SensorFusion(SensorDevice* sensor = 0);
    ~SensorFusion();


    // *** Setup
    
    // Attaches this SensorFusion to a sensor device, from which it will receive
    // notification messages. If a sensor is attached, manual message notification
    // is not necessary. Calling this function also resets SensorFusion state.
    bool        AttachToSensor(SensorDevice* sensor);

    // Returns true if this Sensor fusion object is attached to a sensor.
    bool        IsAttachedToSensor() const  { return Handler.IsHandlerInstalled(); }



    // *** State Query

    // Obtain the current accumulated orientation. Many apps will want to use GetPredictedOrientation
    // instead to reduce latency.
    Quatf       GetOrientation() const      { return lockedGet(&Q); }

    // Get predicted orientaion in the near future; predictDt is lookahead amount in seconds.
    Quatf       GetPredictedOrientation(float predictDt);
    Quatf       GetPredictedOrientation()   { return GetPredictedOrientation(PredictionDT); }

    // Obtain the last absolute acceleration reading, in m/s^2.
    Vector3f    GetAcceleration() const     { return lockedGet(&A); }
    // Obtain the last angular velocity reading, in rad/s.
    Vector3f    GetAngularVelocity() const  { return lockedGet(&AngV); }

    // Obtain the last raw magnetometer reading, in Gauss
    Vector3f    GetMagnetometer() const     { return lockedGet(&RawMag); }   
    // Obtain the calibrated magnetometer reading (direction and field strength)
    Vector3f    GetCalibratedMagnetometer() const  { OVR_ASSERT(MagCalibrated); return lockedGet(&CalMag); }


    // Resets the current orientation.
    void        Reset();



    // *** Configuration

    void        EnableMotionTracking(bool enable = true)    { MotionTrackingEnabled = enable; }
    bool        IsMotionTrackingEnabled() const             { return MotionTrackingEnabled;   }



    // *** Prediction Control

    // Prediction functions.
    // Prediction delta specifes how much prediction should be applied in seconds; it should in
    // general be under the average rendering latency. Call GetPredictedOrientation() to get
    // predicted orientation.
    float       GetPredictionDelta() const                  { return PredictionDT; }
    void        SetPrediction(float dt, bool enable = true) { PredictionDT = dt; EnablePrediction = enable; }
    void		SetPredictionEnabled(bool enable = true)    { EnablePrediction = enable; }    
    bool		IsPredictionEnabled()                       { return EnablePrediction; }


    // *** Accelerometer/Gravity Correction Control

    // Enables/disables gravity correction (on by default).
    void        SetGravityEnabled(bool enableGravity)       { EnableGravity = enableGravity; }   
    bool        IsGravityEnabled() const                    { return EnableGravity;}

    // Gain used to correct gyro with accel. Default value is appropriate for typical use.
    float       GetAccelGain() const                        { return Gain; }
    void        SetAccelGain(float ag)                      { Gain = ag; }


    // *** Magnetometer and Yaw Drift Correction Control

    // Methods to load and save a mag calibration.  Calibrations can optionally
    // be specified by name to differentiate multiple calibrations under different conditions
    // If LoadMagCalibration succeeds, it will override YawCorrectionEnabled based on
    // saved calibration setting.
    bool        SaveMagCalibration(const char* calibrationName = NULL) const;
    bool        LoadMagCalibration(const char* calibrationName = NULL);

    // Enables/disables magnetometer based yaw drift correction. Must also have mag calibration
    // data for this correction to work.
	void        SetYawCorrectionEnabled(bool enable)    { EnableYawCorrection = enable; }
    // Determines if yaw correction is enabled.
    bool        IsYawCorrectionEnabled() const          { return EnableYawCorrection;}

    // Store the calibration matrix for the magnetometer
    void        SetMagCalibration(const Matrix4f& m)
    {
        MagCalibrationMatrix = m;
        time(&MagCalibrationTime);   // time stamp the calibration
        MagCalibrated = true;
    }

    // Retrieves the magnetometer calibration matrix
    Matrix4f    GetMagCalibration() const        { return MagCalibrationMatrix; }
    // Retrieve the time of the calibration
    time_t      GetMagCalibrationTime() const    { return MagCalibrationTime; }

    // True only if the mag has calibration values stored
    bool        HasMagCalibration() const        { return MagCalibrated;}  
    // Force the mag into the uncalibrated state
    void        ClearMagCalibration()            { MagCalibrated = false; }

	// These refer to reference points that associate mag readings with orientations
	void        ClearMagReferences()             { MagNumReferences = 0; }


    Vector3f    GetCalibratedMagValue(const Vector3f& rawMag) const;



    // *** Message Handler Logic

    // Notifies SensorFusion object about a new BodyFrame message from a sensor.
    // Should be called by user if not attaching to a sensor.
    void        OnMessage(const MessageBodyFrame& msg)
    {
        OVR_ASSERT(!IsAttachedToSensor());
        handleMessage(msg);
    }

    void        SetDelegateMessageHandler(MessageHandler* handler)
    { pDelegate = handler; }



private:

    SensorFusion* getThis()  { return this; }

    // Helper used to read and return value within a Lock.
    template<class C>
    C lockedGet(const C* p) const
    {
        Lock::Locker lockScope(Handler.GetHandlerLock());
        return *p;
    }

    // Internal handler for messages; bypasses error checking.
    void        handleMessage(const MessageBodyFrame& msg);

    // Set the magnetometer's reference orientation for use in yaw correction
    // The supplied mag is an uncalibrated value
    void        setMagReference(const Quatf& q, const Vector3f& rawMag);
    // Default to current HMD orientation
    void        setMagReference()  { setMagReference(Q, RawMag); }

	class BodyFrameHandler : public MessageHandler
    {
        SensorFusion* pFusion;
    public:
        BodyFrameHandler(SensorFusion* fusion) : pFusion(fusion) { }
        ~BodyFrameHandler();

        virtual void OnMessage(const Message& msg);
        virtual bool SupportsMessageType(MessageType type) const;
    };   

    SensorInfo        CachedSensorInfo;
    
    Quatf             Q;
	Quatf			  QUncorrected;
    Vector3f          A;    
    Vector3f          AngV;
    Vector3f          CalMag;
    Vector3f          RawMag;
    unsigned int      Stage;
	float             RunningTime;
	float             DeltaT;
    BodyFrameHandler  Handler;
    MessageHandler*   pDelegate;
    float             Gain;
    volatile bool     EnableGravity;

    bool              EnablePrediction;
    float             PredictionDT;
	float             PredictionTimeIncrement;

    SensorFilter      FRawMag;
    SensorFilter      FAngV;

    Vector3f          GyroOffset;
    SensorFilterBase<float> TiltAngleFilter;


    bool              EnableYawCorrection;
    bool              MagCalibrated;
    Matrix4f          MagCalibrationMatrix;
    time_t            MagCalibrationTime;    
    int               MagNumReferences;
    Vector3f          MagRefsInBodyFrame[MagMaxReferences];
    Vector3f          MagRefsInWorldFrame[MagMaxReferences];
    int               MagRefIdx;
    int               MagRefScore;

    bool              MotionTrackingEnabled;
};


} // namespace OVR

#endif
