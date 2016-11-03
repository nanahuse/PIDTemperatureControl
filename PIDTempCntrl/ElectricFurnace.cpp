// 
// 
// 
#include "ElectricFurnace.h"

FurnaceThread::FurnaceThread(IRelayController &relayController, IThermometer &thermometer,double Kp,double Ki,double Kd,double interval) :MainTimer(), RelayController(relayController), Thermometer(thermometer), PIDController(NowTemperature,PIDOutput,GoalTemperature,Kp,Ki,Kd,500.0)
{
	NowTemperature = 0.0;
	MainTimer.SetInterval(interval);
	PIDController.SetOutputLimits(0.0, RelayOutputMax);
	PIDController.SetSampleTime(interval);
	this->Stop();
}

void FurnaceThread::Start()
{
	NowTemperature = Thermometer.Read();
	TemperatureController.InputTemperature(NowTemperature);

	RelayController.Start();

	SetIntervalTimer(millis());
}

bool FurnaceThread::Tick()
{
	if ( MainTimer.Tick() )
	{
		GetOrder();

		if ( WorkStatus == FurnaceThreadStatus_StopAll ) return false;

		GetTemperature();

		if ( WorkStatus == FurnaceThreadStatus_StopReray ) return true;

		PIDController.Compute();
		RelayController.SetOutput(PIDOutput);
		if ( PIDOutput == RelayOutputMax )
		{
			WorkStatus = FurnaceThreadStatus_OutputShortage;
		}

		return true;
	}
	return false;
}

void FurnaceThread::Stop()
{
	RelayController.Stop();
	MainTimer.Stop();
	WorkStatus = FurnaceThreadStatus_StopAll;
}

void FurnaceThread::SetIntervalTimer(unsigned long Time)
{
	MainTimer.SetIntervalTimer(Time);
}

bool FurnaceThread::isRunning()
{
	return MainTimer.isRunning();
}

void FurnaceThread::GetTemperature()
{
	double TempTemperature = Thermometer.Read();

	if ( isnan(TempTemperature) )
	{
		WorkStatus = FurnaceThreadStatus_NotGetTemperature;
	}
	else
	{
		NowTemperature = TempTemperature;
	}

	TemperatureController.InputTemperature(TempTemperature);
}

void FurnaceThread::GetOrder()
{
	switch ( TemperatureController.ShowOrderForFurnaceThread() )
	{
		case FurnaceOrder_StopAll:
			this->Stop();
			break;
		case FurnaceOrder_StopRelay:
			WorkStatus = FurnaceThreadStatus_StopReray;
			RelayController.Stop();
			break;
		default:
			GoalTemperature = TemperatureController.ShowGoalTemeperature();
			break;
	}
}

FurnaceThreadStatus FurnaceThread::ShowStatus()
{
	FurnaceThreadStatus ReturnStatus = WorkStatus;

	switch ( WorkStatus )
	{
		case FurnaceThreadStatus_OutputShortage:
			WorkStatus = FurnaceThreadStatus_OK; //一度実行するとエラー無しが返るようになる。
			break;
		case FurnaceThreadStatus_NotGetTemperature:
			WorkStatus = FurnaceThreadStatus_OK; //一度実行するとエラー無しが返るようになる。
			break;
		default:
			break;
	}
	return ReturnStatus;
}

void FurnaceThread::SetTemperatureController(ITemperatureController &temeperatureController)
{
	TemperatureController = temeperatureController;
}
