// ToolThred.h

#ifndef _TOOLTHRED_h
#define _TOOLTHRED_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include "ThreadBase.h"

enum struct ButtonCircuitPattern
{
	InputPullup,
	Pullup,
	Pulldown
};


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


/*
INPUT_PULLUPを使ったボタンクラス。タクトスイッチの接続に注意。
CanRepeatをFalseにすると一度だけ
CanRepeatをTrueにすると押しっぱなしの時はRepeatTimeの間隔でTrueを返す。
ループしながらボタン入力をするときを想定。
*/
class ButtonClass
{
public:
	ButtonClass(uint8_t controlPin,ButtonCircuitPattern pattern, bool canRepeat);
	void SetRepeatTime(unsigned long RepeatTime);
	void SetCanRepeat(bool canRepeat);
	bool isPressed();
private:
	SimpleTimerThread Timer;
	uint8_t ControlPin;
	bool OldStatus;
	bool CanRepeat;
};

#endif

