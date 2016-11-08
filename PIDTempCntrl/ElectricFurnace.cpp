// 
// 
// 
#include "ElectricFurnace.h"

FurnaceThread::FurnaceThread(IRelayController &relayController, IThermometer &thermometer, ITemperatureController &temperatureController,double Kp,double Ki,double Kd,double interval) :MainTimer(), RelayController(relayController), Thermometer(thermometer),TemperatureController(temperatureController), PIDController(NowTemperature,PIDOutput,GoalTemperature,Kp,Ki,Kd,500.0)
{
	NowTemperature = 0.0;
	MainTimer.SetInterval(interval);
	PIDController.SetOutputLimits(0.0, RelayOutputMax);
	PIDController.SetSampleTime(interval);
	ErrorStatus = FurnaceThreadError_None;
	this->Stop();
}

void FurnaceThread::Start()
{
	Start(millis());
}

void FurnaceThread::Start(unsigned long intervalTimer)
{ 
	GetTemperature();
	MainTimer.Start(intervalTimer);
	WorkStatus = FurnaceThreadStatus_StopRelay;
}

bool FurnaceThread::Tick()
{
	if ( MainTimer.Tick() )
	{
		ErrorStatus = FurnaceThreadError_None;
		GetOrder();

		if ( WorkStatus == FurnaceThreadStatus_Stop ) return false;

		GetTemperature();

		if ( WorkStatus == FurnaceThreadStatus_StopRelay ) return true;

		PIDController.Compute();
		RelayController.SetOutput(PIDOutput);
		if ( PIDOutput == RelayOutputMax )
		{
			ErrorStatus = FurnaceThreadError_OutputShortage;
		}

		return true;
	}
	return false;
}

void FurnaceThread::Stop()
{
	RelayController.Stop();
	MainTimer.Stop();
	WorkStatus = FurnaceThreadStatus_Stop;
}

bool FurnaceThread::isRunning()
{
	return MainTimer.isRunning();
}

void FurnaceThread::GetTemperature()
{
	NowTemperature = Thermometer.Read();
	TemperatureController.InputTemperature(NowTemperature);
}

void FurnaceThread::GetOrder()
{
	switch ( TemperatureController.ShowOrderForFurnaceThread() )
	{
		case FurnaceOrder_Work:
			if ( WorkStatus != FurnaceThreadStatus_Working )
			{
				WorkStatus = FurnaceThreadStatus_Working;
				RelayController.Start();
			}
			GoalTemperature = TemperatureController.ShowGoalTemperature();
			break;
		case FurnaceOrder_StopAll:
			this->Stop();
			break;
		case FurnaceOrder_StopRelay:
			WorkStatus = FurnaceThreadStatus_StopRelay;
			RelayController.Stop();
			break;
	}
}

FurnaceThreadStatus FurnaceThread::ShowStatus()
{
	return WorkStatus;
}

FurnaceThreadError FurnaceThread::CheckError()
{
	return ErrorStatus;
}

