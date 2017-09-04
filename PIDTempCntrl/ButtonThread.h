// ButtonThread.h

#ifndef _BUTTONTHREAD_h
#define _BUTTONTHREAD_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

#include "ThreadBase.h"

enum struct ButtonCircuitPattern {
	InputPullup ,
	Pullup ,
	Pulldown
};

/*
CanRepeatをFalseにすると一度だけ
CanRepeatをTrueにすると押しっぱなしの時はRepeatTimeの間隔でTrueを返す。
ループしながらボタン入力をするときを想定。
*/
class ButtonClass {
public:
	ButtonClass(uint8_t controlPin , ButtonCircuitPattern pattern , bool canRepeat);
	void SetRepeatTime(unsigned long RepeatTime);
	void SetCanRepeat(bool canRepeat);
	bool isPressed( );
private:
	SimpleTimerThread Timer;
	uint8_t ControlPin;
	bool OldStatus;
	bool CanRepeat;
	bool isPulldown;
};
#endif

