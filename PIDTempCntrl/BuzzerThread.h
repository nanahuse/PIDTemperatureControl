// ToolThred.h

#ifndef _TOOLTHRED_h
#define _TOOLTHRED_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include "ThreadBase.h"

/*
擬似並列実行可能なブザー
一度だけ鳴らす、一定周期ごとに繰り返し鳴らすのどちらかが可能。
*/
class BuzzerThread :public ThreadBase
{
public:
	BuzzerThread(uint8_t controlPin);
	void SetSound(unsigned long interval, unsigned long soundLength); //intervalごとにsoundLengthの長さブザーを鳴らす。
	void SoundOnce(unsigned long soundLength); //実行時からSoundLengthの長さブザーを鳴らす。一度鳴るとタイマーが止まる。
	bool Tick(); //音が鳴り始める時にTrueを返す。
	void Start();
	void Stop();
	bool isRunning();

private:
	SimpleTimerThread MainTimer;
	SimpleTimerThread LengthTimer;
	uint8_t ControlPin;
};

#endif

