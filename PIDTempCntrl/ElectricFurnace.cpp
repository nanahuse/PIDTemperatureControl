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
	SetTimerStop(IntervalTimer);
	SetTimerStop(SafetyTimer);
}

bool RelayThread::Tick()
{
	unsigned long Timer = millis();

	if (Timer <= SafetyTimer)  //リレーが壊れないようにスイッチが完了するまで待つ。
	{
		return false;
	}

	if (Timer >= IntervalTimer)
	{
		if (OnTime > RELAYCHANGETIME )
		{
			digitalWrite(ControlPin, HIGH);
		}
		if (OnTime > CONTROLINTERVAL - RELAYCHANGETIME) {
			SwitchingTimer = IntervalTimer + CONTROLINTERVAL - RELAYCHANGETIME;
		}
		else {
			SwitchingTimer = IntervalTimer + OnTime;
		}
		UpdateIntervalTimer();
		SafetyTimer = Timer + RELAYCHANGETIME;
		return true;
	}

	if (Timer >= SwitchingTimer)
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
	while (IntervalTimer <= millis())
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
	while (IntervalTimer < NowTime)
	{
		IntervalTimer += CONTROLINTERVAL;
	}
}

Furnace::Furnace(uint8_t RelayControlPin, IThermometer &thermometer) : PIDController(AverageTemperature, PIDOutput, GoalTemperature, KP, KI, KD), RelayController(RelayControlPin),Thermometer(thermometer)
{
	OldTemperature = 0.0;
	AverageTemperature = 0.0;
	NowTemperature = 0.0;

	SetTimerStop(IntervalTimer);

	WorkStatus = StatusStop;
	ErrorStatus = ErrorNone;

	Thermometer = thermometer;

}

void Furnace::Start()
{
	AverageTemperature = 0.0;
	for (uint8_t i = 0; i < 5; i++)
	{
		double TmpTemperature = 0.0;
		do
		{
			TmpTemperature = Thermometer.Read();
		} while (isnan(TmpTemperature));
		AverageTemperature += TmpTemperature;
		delay(200);
	}
	AverageTemperature *= 0.2;

	OldTemperature = AverageTemperature;
	GoalTemperature = AverageTemperature;

	PIDController.SetOutputLimits(CONTROLMIN, CONTROLMAX);
	PIDController.SetSampleTime(CONTROLINTERVAL);

	RelayController.Start();

	SetIntervalTimer(millis());
}

void Furnace::Start(FurnaceStatus StartMode)
{
	Start();
	if (StartMode == StatusKeepFirstKeepTemp)
	{
		WorkStatus = StatusToFirstKeepTemp;
	}
	else if (StartMode == StatusKeepSecondkeepTemp)
	{
		WorkStatus = StatusToSecondKeepTemp;
	}
	else if (StartMode > StatusFinish)
	{
		WorkStatus = StatusStop;
	}
	else
	{
		WorkStatus = StartMode;
	}
}

bool Furnace::Tick()
{
	if (millis() >= IntervalTimer)
	{
		GetTemperature();
		if (WorkStatus != StatusFinish)
		{
			PIDController.Compute();
			RelayController.SetOnTime((unsigned long)PIDOutput);
		}
		UpdateIntervalTImer();
		return true;
	}
	RelayController.Tick();
	return false;
}

void Furnace::Stop()
{
	RelayController.Stop();
	WorkStatus = StatusStop;
	SetTimerStop(IntervalTimer);
}

void Furnace::SetIntervalTimer(unsigned long Time)
{
	StartTime = Time;
	IntervalTimer = Time + CONTROLINTERVAL;
	RelayController.SetIntervalTimer(IntervalTimer + CONTROLINTERVAL / 2); //温度計測とPIDの計算が結構重いため、リレーを半周期ずらすことでリレー制御への影響を抑えている。
}

void Furnace::GetTemperature()
{
	//温度の測定
	NowTemperature = Thermometer.Read();

	//エラーが出たときは古い値を使う。
	if (isnan(NowTemperature))
	{
		NowTemperature = OldTemperature;
		ErrorStatus = ErrorNotGetTemperature;
	}
	else
	{
		OldTemperature = NowTemperature;
	}

	//PIDに突っ込む温度。均すために平均っぽくしてる。
	AverageTemperature = (AverageTemperature + NowTemperature) * 0.5;

	TemperatureController.InputTemperature(AverageTemperature);
}

FurnaceStatus Furnace::ShowStatus()
{
	return WorkStatus;
}

FurnaceError Furnace::CheckError()
{
	FurnaceError TempErrorStatus = ErrorStatus;
	ErrorStatus = ErrorNone;
	return TempErrorStatus;
}

void Furnace::SetGoalTemperature(double goal)
{
	if (WorkStatus == StatusManual)
	{
		GoalTemperature = goal;
	}
}

void Furnace::SetTemperatureController(ITemperatureController &temeperatureController)
{
	TemperatureController = temeperatureController;
}

void Furnace::UpdateIntervalTImer()
{
	unsigned long NowTime = millis();
	while (IntervalTimer < NowTime)
	{
		IntervalTimer += CONTROLINTERVAL;
	}

}

