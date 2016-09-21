// SomePIDs.h
// Arduino-PID-Library( https://github.com/br3ttb/Arduino-PID-Library )���Q�l�ɂ��č���Ă܂�

#ifndef _SOMEPIDS_h
#define _SOMEPIDS_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

/*
������PID
Setpoint�͖ڕW�l
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

//�ʒu�^
class PositionPID :public IPID
{
public:
	PositionPID(double* Input, double* Output, double* Setpoint, double kp, double ki, double kd, double saturator);

	void Compute();
	void SetSaturator(double saturator);

private:
	void Initialize();

	double ITerm, LastInput;
	double Saturator; //�ϕ��v�f(ITerm)�̔��U��h�����߂̃T�`�����[�^�[
};

//���x�^
class VelocityPID :public IPID
{
public:
	VelocityPID(double* Input, double* Output, double* Setpoint, double kp, double ki, double kd);

	void Compute();

private:
	void Initialize();

	double LastError, SecondLastError;
};

//������s�^
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

//��������s�^
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

