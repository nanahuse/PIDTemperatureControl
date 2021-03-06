#include "ThreadBase.h"
SimpleTimerThread::SimpleTimerThread()
{
	SetInterval(0);
	Stop();
}

bool SimpleTimerThread::Tick()
{
	if ( GlobalTimer >= IntervalTimer )
	{
		UpdateIntervalTimer();
		return true;
	}
	return false;
}
void SimpleTimerThread::Start()
{
	IntervalTimer = GlobalTimer + Interval;
}
void SimpleTimerThread::Start(unsigned long intervalTimer) //デフォルト引数使ってまとめようとしたらStart()ないって怒られた(´・ω・`)
{
	IntervalTimer = intervalTimer;
	UpdateIntervalTimer();
}
void SimpleTimerThread::Stop()
{
	IntervalTimer = UnsignedLongMax;//タイマーを上限にすることで動きを止める
}
void SimpleTimerThread::SetInterval(unsigned long interval)
{
	Interval = (interval != 0) ? interval : 1;
}
bool SimpleTimerThread::isRunning()
{
	return IntervalTimer != UnsignedLongMax;
}

void SimpleTimerThread::GlobalTimerTick()
{
	GlobalTimer = millis();
}

void SimpleTimerThread::UpdateIntervalTimer() //現在時よりも未来にすることで動作を確実にする．
{
	while ( IntervalTimer <= GlobalTimer )		IntervalTimer += Interval;
}
const unsigned long SimpleTimerThread::UnsignedLongMax = 4294967295; //unsigned longの上限

unsigned long SimpleTimerThread::GlobalTimer = 0;