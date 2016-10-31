// 
// 
// 

#include "ToolThred.h"

BuzzerThread::BuzzerThread(uint8_t controlPin): SimpleTimerThread()
{
	ControlPin = controlPin;
	pinMode(ControlPin, OUTPUT);
	digitalWrite(ControlPin, LOW);
	this->Stop();
}

void BuzzerThread::SetSound(unsigned long interval, unsigned long soundLength)
{
	SimpleTimerThread::SetInterval(interval);
	LengthTimer.SetInterval(soundLength);
}

void BuzzerThread::SoundOnce(unsigned long soundLength)
{
	SimpleTimerThread::Stop();
	LengthTimer.SetInterval(soundLength);
	digitalWrite(ControlPin, HIGH);
	LengthTimer.Start();
}

bool BuzzerThread::Tick()
{
	if ( SimpleTimerThread::Tick() )
	{
		digitalWrite(ControlPin, HIGH);
		LengthTimer.Start();
		return true;
	}

	if ( LengthTimer.isRunning() )
	{
		if ( LengthTimer.Tick() )
		{
			digitalWrite(ControlPin, LOW);
			LengthTimer.Stop();
		}
	}
	return false;
}

void BuzzerThread::Start()
{
	SimpleTimerThread::Start();
	digitalWrite(ControlPin, HIGH);
	LengthTimer.Start();
}

void BuzzerThread::Stop()
{
	digitalWrite(ControlPin, LOW);
	SimpleTimerThread::Stop();
	LengthTimer.Stop();
}

bool BuzzerThread::isRunnning()
{
	return SimpleTimerThread::isRunning();
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