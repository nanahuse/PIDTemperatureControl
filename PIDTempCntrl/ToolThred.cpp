// 
// 
// 

#include "ToolThred.h"

BuzzerThread::BuzzerThread(uint8_t controlPin)
{
	ControlPin = controlPin;
	pinMode(ControlPin, OUTPUT);
	digitalWrite(ControlPin, LOW);
	this->Stop();
}

void BuzzerThread::SetSound(unsigned long interval, unsigned long soundLength)
{
	Interval = interval;
	Length = soundLength;
}

void BuzzerThread::SoundOnce(unsigned long soundLength)
{
	SetTimerStop(IntervalTimer);
	LengthTimer = millis() + soundLength;
	SetTimerStop(Interval);
	SetTimerStop(Length);
	digitalWrite(ControlPin, HIGH);
}

bool BuzzerThread::Tick()
{
	if ( millis() >= IntervalTimer )
	{
		digitalWrite(ControlPin, HIGH);
		UpdateIntervalTImer();
		return true;
	}
	if ( millis() >= LengthTimer )
	{
		digitalWrite(ControlPin, LOW);
		LengthTimer = IntervalTimer + Length;
	}
	return false;
}

void BuzzerThread::Start()
{
	IntervalTimer = millis() + Interval;
	LengthTimer = IntervalTimer + Length;
}

void BuzzerThread::Stop()
{
	digitalWrite(ControlPin, LOW);
	SetTimerStop(IntervalTimer);
	SetTimerStop(LengthTimer);
}

void BuzzerThread::UpdateIntervalTImer()
{
	unsigned long NowTime = millis();
	while ( IntervalTimer < NowTime )
	{
		IntervalTimer += Interval;
	}
}

ButtonClass::ButtonClass(uint8_t controlPin, bool canRepeat) : Timer()
{
	ControlPin = controlPin;
	pinMode(ControlPin, INPUT_PULLUP);
	SetCanRepeat(CanRepeat);
	SetRepeatTime(100);
	OldStatus = false;
}

void ButtonClass::SetRepeatTime(unsigned long RepeatTime)
{
	if ( CanRepeat )
	{
		Timer.SetInterval(RepeatTime);
	}
}

void ButtonClass::SetCanRepeat(bool canRepeat)
{
	CanRepeat = canRepeat;
	if ( !CanRepeat )
	{
		Timer.SetInterval(100); //チャタリングの防止用
		OldStatus = false;
	}
}

bool ButtonClass::isPressed()
{
	if ( !digitalRead(ControlPin) ) //プルアップ回路なのでdigitalReadがfalseの時に押されている。
	{
		if ( Timer.Tick() )
		{
			if ( OldStatus )
			{
				if ( CanRepeat )
				{
					return true;
				}
				else
				{
					return false;
					Timer.Stop();
				}
			}
			else
			{
				Timer.Start();
				OldStatus = true;
				return true;
			}
		}
		else
		{
			return false;
		}
	}
	else
	{
		if ( OldStatus )
		{
			OldStatus = false;
			if ( !CanRepeat )
			{
				Timer.Start();
			}
		}
		return false;

	}
}