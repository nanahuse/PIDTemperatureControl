// 
// 
// 

#include "BuzzerThread.h"

BuzzerThread::BuzzerThread(uint8_t controlPin) : MainTimer(), LengthTimer()
{
	ControlPin = controlPin;
	pinMode(ControlPin, OUTPUT);
	digitalWrite(ControlPin, LOW);
	this->Stop();
}

void BuzzerThread::SetSound(unsigned long interval, unsigned long soundLength)
{
	MainTimer.SetInterval(interval);
	LengthTimer.SetInterval(soundLength);
}

void BuzzerThread::SoundOnce(unsigned long soundLength)
{
	MainTimer.Stop();
	LengthTimer.SetInterval(soundLength);
	digitalWrite(ControlPin, HIGH);
	LengthTimer.Start();
}

bool BuzzerThread::Tick()
{
	if ( MainTimer.Tick() )
	{
		digitalWrite(ControlPin, HIGH);
		LengthTimer.Start();
		return true;
	}

	if ( LengthTimer.Tick() )
	{
			digitalWrite(ControlPin, LOW);
			LengthTimer.Stop();
	}
	return false;
}

void BuzzerThread::Start()
{
	MainTimer.Start();
	digitalWrite(ControlPin, HIGH);
	LengthTimer.Start();
}

void BuzzerThread::Stop()
{
	digitalWrite(ControlPin, LOW);
	MainTimer.Stop();
	LengthTimer.Stop();
}

bool BuzzerThread::isRunning()
{
	return MainTimer.isRunning();
}


