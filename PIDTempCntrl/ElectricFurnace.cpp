// 
// 
// 
#include "ElectricFurnace.h"

FurnaceThread::FurnaceThread(IRelayController &relayController, IThermometer &thermometer,IPID &pidController) :MainTimer(), PIDController(pidController), RelayController(relayController), Thermometer(thermometer)
{
	OldTemperature = 0.0;
	AverageTemperature = 0.0;
	NowTemperature = 0.0;

	MainTimer.SetInterval(CONTROLINTERVAL);

	this->Stop();
}

void FurnaceThread::Start()
{
	AverageTemperature = 0.0;
	for ( uint8_t i = 0; i < 5; i++ )
	{
		double TmpTemperature = 0.0;
		do
		{
			TmpTemperature = Thermometer.Read();
		} while ( isnan(TmpTemperature) );
		AverageTemperature += TmpTemperature;
		delay(200);
		//なんかエラー判別か何か入れないと事故りそう
	}
	AverageTemperature *= 0.2;

	OldTemperature = AverageTemperature;

	TemperatureController.InputTemperature(OldTemperature);

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
		RelayController.SetOutput((unsigned long)PIDOutput);
		if ( PIDOutput == CONTROLMAX )
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
	NowTemperature = Thermometer.Read();

	if ( isnan(NowTemperature) )
	{
		NowTemperature = OldTemperature;	//エラーが出たときは古い値を使う。
		WorkStatus = FurnaceThreadStatus_NotGetTemperature;
	}
	else
	{
		OldTemperature = NowTemperature;
	}

	//PIDに突っ込む温度。均すために平均っぽくしてる。
	AverageTemperature = (AverageTemperature + NowTemperature) * 0.5;

	TemperatureController.InputTemperature(AverageTemperature);
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
