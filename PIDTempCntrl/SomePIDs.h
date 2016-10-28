// SomePIDs.h
// Arduino-PID-Library( https://github.com/br3ttb/Arduino-PID-Library )を参考にして作ってます

#ifndef _SOMEPIDS_h
#define _SOMEPIDS_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

/*
いろんなPID
Setpointは目標値
*/
class IPID
{
public:
	IPID(double* Input, double* Output, double* Setpoint, double kp, double ki, double kd);

	virtual void Compute() = 0;

	void SetTunings(double Kp, double Ki, double Kd);

	void SetOutputLimits(double Min, double Max);
	void SetSampleTime(int NewSampleTime);

protected:
	virtual void Initialize()=0;

	double kp, ki, kd;

	double *MyInput, *MyOutput, *MySetpoint;

	unsigned long SampleTime;
	double OutputMin, OutputMax;

};

//位置型
class PositionPID :public IPID
{
public:
	PositionPID(double* Input, double* Output, double* Setpoint, double kp, double ki, double kd, double saturator);

	void Compute();
	void SetSaturator(double saturator);

private:
	void Initialize();

	double ITerm, LastInput;
	double Saturator; //積分要素(ITerm)の発散を防ぐためのサチュレーター
};

//速度型
class VelocityPID :public IPID
{
public:
	VelocityPID(double* Input, double* Output, double* Setpoint, double kp, double ki, double kd);

	void Compute();

private:
	void Initialize();

	double LastError, SecondLastError;
};

//微分先行型
class VelocityPID_IPd : public IPID
{
public:
	VelocityPID_IPd(double* Input, double* Output, double* Setpoint, double kp, double ki, double kd);

	void Compute();

private:
	void Initialize();

	double LastError;
	double LastInput, SecondLastInput;

};

//比例微分先行型
class VelocityPID_Ipd : public IPID
{
public:
	VelocityPID_Ipd(double* Input, double* Output, double* Setpoint, double kp, double ki, double kd);

	void Compute();

private:
	void Initialize();

	double LastInput, SecondLastInput;

};

#endif

