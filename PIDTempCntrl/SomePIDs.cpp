// 
// 
// 

#include "SomePIDs.h"


IPID::IPID(const double &Input, double &Output, const double &Setpoint, double kp, double ki, double kd, double saturator) : MyInput(Input), MyOutput(Output), MySetpoint(Setpoint)
{
	IPID::SetOutputLimits(0, 255);
	SampleTime = 100.0;
	Saturator = saturator;

	IPID::SetTunings(kp, ki, kd);

	Initialize();
}

void IPID::SetTunings(double Kp, double Ki, double Kd)
{
	if ( Kp < 0 || Ki < 0 || Kd < 0 ) return;
	double SampleTimeInSec = ((double)SampleTime) / 1000;
	kp = Kp;
	ki = Ki * SampleTimeInSec;
	kd = Kd / SampleTimeInSec;
}


void IPID::SetOutputLimits(double Min, double Max)
{
	if ( Min >= Max ) return;
	OutputMin = Min;
	OutputMax = Max;

	if ( MyOutput > OutputMax ) MyOutput = OutputMax;
	else if ( MyOutput < OutputMin ) MyOutput = OutputMin;
}

void IPID::SetSaturator(double saturator)
{
	Saturator = saturator;
}

void IPID::SetSampleTime(int NewSampleTime)
{
	if ( NewSampleTime > 0 )
	{
		double ratio = (double)NewSampleTime / (double)SampleTime;
		ki *= ratio;
		kd /= ratio;
		SampleTime = (unsigned long)NewSampleTime;
	}
}

void IPID::Initialize() { }

PositionPID::PositionPID(const double &Input, double &Output, const double &Setpoint, double kp, double ki, double kd, double saturator) : IPID(Input, Output, Setpoint, kp, ki, kd, saturator)
{ }

void PositionPID::Compute()
{
	double error = MySetpoint - MyInput;
	ITerm += (ki * error);
	if ( ITerm > OutputMax + Saturator ) ITerm = OutputMax + Saturator; //積分要素の発散を防ぐため、サチュレーターを入れてる。
	else if ( ITerm < OutputMin - Saturator ) ITerm = -OutputMin - Saturator;
	double dInput = (MyInput - LastInput);

	double output = kp*error + ITerm - kd *dInput;

	if ( output > OutputMax ) output = OutputMax;
	if ( output < OutputMin ) output = OutputMin;
	MyOutput = output;

	LastInput = MyInput;
}

void PositionPID::Initialize()
{
	ITerm = MyOutput;
	LastInput = MyInput;
	if ( ITerm > Saturator ) ITerm = Saturator;
	else if ( ITerm < -Saturator ) ITerm = -Saturator;
}

VelocityPID::VelocityPID(const double &Input, double &Output, const double &Setpoint, double kp, double ki, double kd, double saturator) : IPID(Input, Output, Setpoint, kp, ki, kd, saturator) { }
void VelocityPID::Compute()
{
	double error = MySetpoint - MyInput;

	ProvisionalOutput += kp * (error - LastError) + ki * error - kd * (error - 2.0* LastError + SecondLastError);

	if ( ProvisionalOutput > OutputMax+Saturator ) ProvisionalOutput = OutputMax + Saturator;
	if ( ProvisionalOutput < OutputMin - Saturator ) ProvisionalOutput = OutputMin - Saturator;

	if ( ProvisionalOutput > OutputMax )
	{
		MyOutput = OutputMax;
	}
	else if ( ProvisionalOutput < OutputMin )
	{
		MyOutput = OutputMin;
	}
	else
	{
		MyOutput = ProvisionalOutput;
	}

	SecondLastError = LastError;
	LastError = error;
}

void VelocityPID::Initialize()
{
	LastError = 0.0;
	SecondLastError = 0.0;
}

VelocityPID_IPd::VelocityPID_IPd(const double &Input, double &Output, const double &Setpoint, double kp, double ki, double kd,double saturator) : IPID(Input, Output, Setpoint, kp, ki, kd,saturator) { }

void VelocityPID_IPd::Compute()
{
	double error = MySetpoint - MyInput;

	ProvisionalOutput += kp * (error - LastError) + ki * error - kd * (MyInput - 2.0* LastInput + SecondLastInput);

	if ( ProvisionalOutput > OutputMax + Saturator ) ProvisionalOutput = OutputMax + Saturator;
	if ( ProvisionalOutput < OutputMin - Saturator ) ProvisionalOutput = OutputMin - Saturator;

	if ( ProvisionalOutput > OutputMax )
	{
		MyOutput = OutputMax;
	}
	else if ( ProvisionalOutput < OutputMin )
	{
		MyOutput = OutputMin;
	}
	else
	{
		MyOutput = ProvisionalOutput;
	}

	SecondLastInput = LastInput;
	LastInput = MyInput;
	LastError = error;
}

void VelocityPID_IPd::Initialize()
{
	LastError = 0.0;
	LastInput = 0.0;
	SecondLastInput = 0.0;
}

VelocityPID_Ipd::VelocityPID_Ipd(const double &Input, double &Output, const double &Setpoint, double kp, double ki, double kd,double saturator) : IPID(Input, Output, Setpoint, kp, ki, kd,saturator) { }

void VelocityPID_Ipd::Compute()
{
	double error = MySetpoint - MyInput;

	ProvisionalOutput += -kp * (MyInput - LastInput) + ki * error - kd * (MyInput - 2.0* LastInput + SecondLastInput);

	if ( ProvisionalOutput > OutputMax + Saturator ) ProvisionalOutput = OutputMax + Saturator;
	if ( ProvisionalOutput < OutputMin - Saturator ) ProvisionalOutput = OutputMin - Saturator;

	if ( ProvisionalOutput > OutputMax )
	{
		MyOutput = OutputMax;
	}
	else if ( ProvisionalOutput < OutputMin )
	{
		MyOutput = OutputMin;
	}
	else
	{
		MyOutput = ProvisionalOutput;
	}

	SecondLastInput = LastInput;
	LastInput = MyInput;
}

void VelocityPID_Ipd::Initialize()
{
	LastInput = 0.0;
	SecondLastInput = 0.0;
}
