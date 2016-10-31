// 
// 
// 
#include "ElectricFurnace.h"

RelayThread::RelayThread(uint8_t controlPin)
{
	ControlPin = controlPin;
	pinMode(ControlPin, OUTPUT);
	digitalWrite(ControlPin, LOW);
	SetTimerStop(IntervalTimer);
	SetTimerStop(SafetyTimer);
}


void RelayThread::Start()
{
	IntervalTimer = millis() + CONTROLINTERVAL;
	SafetyTimer = 0;
	SetTimerStop(SwitchingTimer);
}

void RelayThread::Stop()
{
	delay(RELAYCHANGETIME);
	digitalWrite(ControlPin, LOW);
	delay(RELAYCHANGETIME);
	SimpleTimerThread::Stop();
	SetTimerStop(SafetyTimer);
}

bool RelayThread::Tick()
{
	unsigned long Timer = millis();

	if ( Timer <= SafetyTimer )  //リレーが壊れないようにスイッチが完了するまで待つ。
	{
		return false;
	}

	if ( Timer >= IntervalTimer )
	{
		if ( OnTime > RELAYCHANGETIME )
		{
			digitalWrite(ControlPin, HIGH);
		}
		if ( OnTime > CONTROLINTERVAL - RELAYCHANGETIME )
		{
			SwitchingTimer = IntervalTimer + CONTROLINTERVAL - RELAYCHANGETIME;
		}
		else
		{
			SwitchingTimer = IntervalTimer + OnTime;
		}
		UpdateIntervalTimer();
		SafetyTimer = Timer + RELAYCHANGETIME;
		return true;
	}

	if ( Timer >= SwitchingTimer )
	{
		digitalWrite(ControlPin, LOW);
		SwitchingTimer += CONTROLINTERVAL;
		SafetyTimer = Timer + RELAYCHANGETIME;
	}
	return false;
}

void RelayThread::SetIntervalTimer(unsigned long Time)
{
	IntervalTimer = Time;
	while ( IntervalTimer <= millis() )
	{
		IntervalTimer += CONTROLINTERVAL;
	}
	SwitchingTimer = IntervalTimer + OnTime;
}

void RelayThread::SetOnTime(unsigned long Time)
{
	OnTime = Time;
}

void RelayThread::UpdateIntervalTimer()
{
	unsigned long NowTime = millis();
	while ( IntervalTimer < NowTime )
	{
		IntervalTimer += CONTROLINTERVAL;
	}
}

FurnaceThread::FurnaceThread(uint8_t RelayControlPin, IThermometer &thermometer) : PIDController(AverageTemperature, PIDOutput, TemperatureController.ShowGoalTemperatureReference(), KP, KI, KD, CONTROLMAX*0.5), RelayController(RelayControlPin), Thermometer(thermometer)
{
	OldTemperature = 0.0;
	AverageTemperature = 0.0;
	NowTemperature = 0.0;

	Stop();
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

	PIDController.SetOutputLimits(CONTROLMIN, CONTROLMAX);
	PIDController.SetSampleTime(CONTROLINTERVAL);

	RelayController.Start();

	SetIntervalTimer(millis());
}

bool FurnaceThread::Tick()
{
	if ( millis() >= IntervalTimer )
	{
		GetOrder();

		if ( WorkStatus == FurnaceThreadStatus_StopAll ) return false;

		GetTemperature();

		if ( WorkStatus == FurnaceThreadStatus_StopReray ) return true;

		PIDController.Compute();
		RelayController.SetOnTime((unsigned long)PIDOutput);
		if ( PIDOutput == CONTROLMAX )
		{
			WorkStatus = FurnaceThreadStatus_OutputShortage;
		}

		UpdateIntervalTimer();
		return true;
	}
	RelayController.Tick();
	return false;
}

void FurnaceThread::Stop()
{
	RelayController.Stop();
	SimpleTimerThread::Stop();
	WorkStatus = FurnaceThreadStatus_StopAll;
}

void FurnaceThread::SetIntervalTimer(unsigned long Time)
{
	IntervalTimer = Time + CONTROLINTERVAL;
	RelayController.SetIntervalTimer(IntervalTimer + CONTROLINTERVAL / 2); //温度計測とPIDの計算が結構重いため、リレーを半周期ずらすことでリレー制御への影響を抑えている。
}

bool FurnaceThread::isRunning()
{
	return SimpleTimerThread::isRunning();
}

void FurnaceThread::GetTemperature()
{
	//ここら辺の処理はIThermometer側にやらせるのが正しいのでは？

	//温度の測定
	int i = 0;
	do //失敗したときに備えて五回まではミスを認める
	{
		NowTemperature = Thermometer.Read();
		i++;
	} while ( isnan(NowTemperature) && (i < 5) );

	if ( i = 5 ) //五回温度取得に失敗していたらエラーとみなす
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
			Stop();
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
	switch ( WorkStatus )
	{
		case FurnaceThreadStatus_OutputShortage:
			WorkStatus = FurnaceThreadStatus_OK;
			return FurnaceThreadStatus_OutputShortage;
			break;
		case FurnaceThreadStatus_NotGetTemperature:
			WorkStatus = FurnaceThreadStatus_OK;
			return FurnaceThreadStatus_NotGetTemperature;
			break;
		default:
			return WorkStatus;
			break;
	}
}



void FurnaceThread::SetTemperatureController(ITemperatureController &temeperatureController)
{
	TemperatureController = temeperatureController;
}

void FurnaceThread::UpdateIntervalTimer()
{
	unsigned long NowTime = millis();
	while ( IntervalTimer < NowTime )
	{
		IntervalTimer += CONTROLINTERVAL;
	}

}

