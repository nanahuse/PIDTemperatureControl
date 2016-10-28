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

Furnace::Furnace(uint8_t RelayControlPin, IThermometer* thermometer) : PIDController(&AverageTemperature, &PIDOutput, &GoalTemperature, KP, KI, KD), RelayController(RelayControlPin)
{
	OldTemperature = 0.0;
	AverageTemperature = 0.0;
	NowTemperature = 0.0;

	StartTime = 0;
	FinishTimer = 0;
	KeepTimer = 0;
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
			TmpTemperature = Thermometer->Read();
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
	WorkStatus = StatusToFirstKeepTemp;
	CalcFinishTime();

}

void Furnace::Start(uint8_t StartMode)
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
		UpdateGoalTemperature();
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
	NowTemperature = Thermometer->Read();

	//エラーが出たときは古い値を使う。
	if (isnan(NowTemperature))
	{
		NowTemperature = OldTemperature;
	}
	else
	{
		OldTemperature = NowTemperature;
	}

	//PIDに突っ込む温度。均すために平均っぽくしてる。
	AverageTemperature = (AverageTemperature + NowTemperature) * 0.5;
}

void Furnace::UpdateGoalTemperature()
{
	switch (WorkStatus)
	{
	case StatusStop:
		break;
	case StatusManual:
		break;
	case StatusToFirstKeepTemp:
		if (FIRSTKEEPTEMP > AverageTemperature + TEMPCONTROLLENGTH) 
		{
			RisingTemperature();
		}
		else
		{ //保持温度付近に到達したら制御値を保持温度にして維持する
			CalcFinishTime();
			KeepTimer = millis();
			GoalTemperature = FIRSTKEEPTEMP;
			WorkStatus = StatusKeepFirstKeepTemp;
		}
		break;

	case StatusKeepFirstKeepTemp:
		if (millis() - KeepTimer >= FIRSTKEEPMILLIS)
		{
			WorkStatus = StatusToSecondKeepTemp;
		}
		break;

	case StatusToSecondKeepTemp:
		if (SECONDKEEPTEMP > AverageTemperature + TEMPCONTROLLENGTH)
		{
			RisingTemperature();
		}
		else
		{ //保持温度付近に到達したら制御値を保持温度にして維持する
			CalcFinishTime();
			KeepTimer = millis();
			GoalTemperature = SECONDKEEPTEMP;
			WorkStatus = StatusKeepSecondkeepTemp;
		}
		break;

	case StatusKeepSecondkeepTemp:
		if (millis() - KeepTimer >= SECONDKEEPMILLIS)
		{ //焼き終わり。リレーをオフにして冷やしていく。
			GoalTemperature = 0.0;
			RelayController.Stop();
			WorkStatus = StatusFinish;
		}
		break;

	case StatusFinish:
		//END
		break;

	default:
		//ERROR ここに来てしまったらどうしようもない。
		Stop();
		ErrorStatus = ErrorUnknown;
		WorkStatus = StatusError;
		break;
	}
}


uint8_t Furnace::ShowStatus()
{
	return WorkStatus;
}

uint8_t Furnace::CheckError()
{
	uint8_t TempErrorStatus = ErrorStatus;
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

void Furnace::RisingTemperature()
{
	GoalTemperature += INCTEMPperINTERVAL;

	if (GoalTemperature > AverageTemperature + TEMPCONTROLLENGTH)
	{
		ErrorStatus = ErrorNotEnoughTemperatureUpward;
		GoalTemperature = AverageTemperature + TEMPCONTROLLENGTH;
		CalcFinishTime();
	}
}

void  Furnace::CalcFinishTime()
{
	FinishTimer = 0;
	if (WorkStatus >= StatusToFirstKeepTemp);
	{
		FinishTimer = millis();

		if (WorkStatus == StatusToFirstKeepTemp)
		{
			FinishTimer += FIRSTKEEPMILLIS;
		}

		if (WorkStatus <= StatusToSecondKeepTemp)
		{
			FinishTimer += SECONDKEEPMILLIS;
		}

		FinishTimer += (unsigned long)((SECONDKEEPTEMP - (float)AverageTemperature) * MILLISperINCTEMP);
	}
}

void Furnace::UpdateIntervalTImer()
{
	unsigned long NowTime = millis();
	while (IntervalTimer < NowTime)
	{
		IntervalTimer += CONTROLINTERVAL;
	}

}

