#ifndef _IThread_h
#define _IThread_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

//時間系はミリ秒だよ

//--------------------------------------------------------マクロ--------------------------------------------------------

#define ULMAX 4294967295 //unsigned longの最大値
#define SetTimerStop(Timer) Timer = ULMAX //タイマーを取りうる最大値にすることによって実質的に止める。動く可能性があるのは約50日後の一瞬

//--------------------------------------------------------クラスの定義--------------------------------------------------------


/*
擬似並列実行が出来るようにするクラスベース
使用の際はループ内でTickを実行する。
*/
class ThreadBase
{
public:
	virtual bool Tick() = 0; //処理の実行。ループ内で使用する。タイマーによって処理が起きたときにTrueを返すようにする。
	virtual void Start() = 0; //タイマーの開始処理他。
	virtual void Stop() = 0; //タイマーの停止処理他。
	virtual bool isRunning()=0; //タイマーを見ることで動いているかを確認する。

protected:
};


/*
ThreadBaseによる簡易タイマー機能。
Tickをループ内で使用することでIntervalごとにTickがTrueを返し、周期動作させることができる。
*/
class SimpleTimerThread : public ThreadBase
{
public:
	SimpleTimerThread()
	{
		SetInterval(0);
		Stop();
	}

	virtual bool Tick()
	{
		if ( millis()> IntervalTimer )
		{
			UpdateIntervalTimer();
			return true;
		}
		return false;
	}
	virtual void Start()
	{
		IntervalTimer = millis() + Interval;
	}
	virtual void Stop()
	{
		SetTimerStop(IntervalTimer);
	}
	virtual void SetInterval(unsigned long interval)
	{
		if ( interval = 0 )
		{
			Interval = 1; //無限ループの防止
		}
		else
		{
			Interval = interval;
		}
	}

	virtual	void SetIntervalTimer(unsigned long Time) //IntervalTimerを任意の時間に設定する。これによって処理のタイミングを調整出来る。
	{
		IntervalTimer = Time;
		UpdateIntervalTimer();
	}

	virtual bool isRunning() //タイマーを見ることで動いているかを確認する。
	{
		return IntervalTimer != ULMAX; //SetTimerStopがタイマーを最大値にすることによってタイマーを止めているため。
	}

protected:
	unsigned long IntervalTimer,Interval;
	void UpdateIntervalTimer() //タイマーが確実に現在時間より未来になるようにすることで誤動作を防ぐ
	{
		unsigned long NowTime = millis();
		while ( IntervalTimer < NowTime )
		{
			IntervalTimer += Interval;
		}
	}
};

#endif