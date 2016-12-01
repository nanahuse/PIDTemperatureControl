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
	ErrorStatus = FurnaceThreadError::None;
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
	WorkStatus = FurnaceThreadStatus::StopRelay;
}

bool FurnaceThread::Tick()
{
	if ( MainTimer.Tick() )
	{
		ErrorStatus = FurnaceThreadError::None;
		GetOrder();

		if ( WorkStatus == FurnaceThreadStatus::Stop ) return false;

		GetTemperature();

		if ( WorkStatus == FurnaceThreadStatus::StopRelay ) return true;
		Serial.print(millis());
		Serial.print("\t");
		PIDController.Compute();
		Serial.println(millis());
		RelayController.SetOutput(PIDOutput);
		if ( PIDOutput == RelayOutputMax )
		{
			ErrorStatus = FurnaceThreadError::OutputShortage;
		}

		return true;
	}
	return false;
}

void FurnaceThread::Stop()
{
	RelayController.Stop();
	MainTimer.Stop();
	WorkStatus = FurnaceThreadStatus::Stop;
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
		case FurnaceOrder::Work:
			if ( WorkStatus != FurnaceThreadStatus::Working )
			{
				WorkStatus = FurnaceThreadStatus::Working;
				RelayController.Start();
			}
			GoalTemperature = TemperatureController.ShowGoalTemperature();
			break;
		case FurnaceOrder::StopAll:
			if ( WorkStatus != FurnaceThreadStatus::Stop )
			{
				this->Stop();
			}
			break;
		case FurnaceOrder::StopRelay:
			if ( WorkStatus != FurnaceThreadStatus::StopRelay )
			{
				WorkStatus = FurnaceThreadStatus::StopRelay;
				RelayController.Stop();
			}
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

