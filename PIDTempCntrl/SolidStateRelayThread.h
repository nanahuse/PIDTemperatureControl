// SolidStateRelayThread.h

#ifndef _SOLIDSTATERELAYTHREAD_h
#define _SOLIDSTATERELAYTHREAD_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

#include "Construct.h"
#include "ElectricFurnace.h"

/*
リレーを操作するクラス
リレーをIntervalごとにOnTimeの長さだけ開放する。
*/

class SolidStateRelayThread : public ThreadBase , public IRelayController {
public:
	SolidStateRelayThread(uint8_t controlPin);
	void Start( );
	void Start(unsigned long intervalTimer);
	void Stop( );
	bool Tick( ); //リレーがOnになるタイミングでTrueを返す。
	bool isRunning( );
	void SetOutput(double Output);

private:
	uint8_t ControlPin;
	SimpleTimerThread MainTimer;
	SimpleTimerThread SwitchingTimer , SafetyTimer;
	unsigned long OnTime;

	//定数
	static const unsigned long ChangeTime; //リレーの切り替え時間
	static const unsigned long Interval;
	static const double OutputLimit;
	static const double OutputLimitPerRelayOutputMax;
};


#endif

