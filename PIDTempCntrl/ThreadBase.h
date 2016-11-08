#ifndef _IThread_h
#define _IThread_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

//時間系はミリ秒だよ
//--------------------------------------------------------クラスの定義--------------------------------------------------------


/*
擬似並列実行が出来るようにするクラスベース
使用の際はループ内でTickを実行する。
*/
class ThreadBase
{
public:
	virtual bool Tick() = 0; //処理の実行。ループ内で使用する。処理が起きたときにTrueを返すようにする。
	virtual void Start() = 0;
	virtual void Stop() = 0;
	virtual bool isRunning() = 0;
};


/*
ThreadBaseによる簡易タイマー機能。
Tickをループ内で使用することでIntervalごとにTickがTrueを返し、周期動作させることができる。

UpdateIntervalTimer()において
・millis()がオーバーフローする前後で怪しい挙動を示すバグ
・Interval >= ULMAX - IntervalTimerのとき無限ループしてしまうバグ
が予測されますがmillis()は50日はオーバーフローしないため長期間の利用またはIntervalを上限ぎりぎりにすることがない限りは問題なくまた，対応が難しいためパス．
*/
class SimpleTimerThread : public ThreadBase
{
public:
	SimpleTimerThread();

	virtual bool Tick();
	virtual void Start();
	virtual void Start(unsigned long intervalTimer); //動作を開始する．IntervalTimerを任意の時間に設定しすることで処理のタイミングを調整出来る．過去の時間を設定した場合でも未来の時間までずらしていきなり動作することを防ぐ．
	virtual void Stop();
	virtual void SetInterval(unsigned long interval);
	virtual bool isRunning(); //タイマーを見ることで動いているかを確認する。

protected:
	unsigned long IntervalTimer, Interval;
	virtual void UpdateIntervalTimer(); //タイマーが確実に現在時間より未来になるようにすることで誤動作を防ぐ．もし(Interval >= ULMAX - IntervalTimer)な状態で実行されると無限ループするよ！
	static const unsigned long UnsignedLongMax; //unsigned longの最大値
};

#endif