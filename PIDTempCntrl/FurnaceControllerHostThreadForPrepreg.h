// FurnaceControllerHostThreadForPrepreg.h

#ifndef _FURNACECONTROLLERHOSTTHREADFORPREPREG_h
#define _FURNACECONTROLLERHOSTTHREADFORPREPREG_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include "Construct.h"
#include "ElectricFurnace.h"
#include "ThreadBase.h"

//--------------------------------------------------------定義とか--------------------------------------------------------
//Furnace(炉)の稼働状態を表す。Furnace::ShowStatusで返る値。
enum struct FurnaceControllerStatus {
	Stop = 0 ,
	Startup ,
	ToFirstKeepTemp ,
	KeepFirstKeepTemp ,
	ToSecondKeepTemp ,
	KeepSecondkeepTemp ,
	Finish ,
	FatalError //回復不可能なエラー
};

enum struct FurnaceControllerError {
	None = 0 ,
	NotEnoughTemperatureUpward ,
	TooHot ,
	StartupFailure //回復不能
};

//--------------------------------------------------------クラスの定義--------------------------------------------------------

class FurnaceControllerHostThreadForPrepreg : public IFurnaceControllerHost , public ThreadBase {
public:
	FurnaceControllerHostThreadForPrepreg( );

	bool Tick( );
	void Start( );
	void Stop( );
	bool isRunning( );

	double ShowGoalTemperature( );
	double ShowMinimumTemperature( );
	FurnaceOrder ShowOrderForFurnaceThread( );
	FurnaceControllerStatus ShowStatus( );
	FurnaceControllerError CheckError( );
	unsigned long ShowElapseTime( ); //経過時間
	unsigned long ShowRemainTime( ); //残り時間
	void InputTemperature(double Temperature);
private:
	double GoalTemperature;
	double MinimumTemperature;
	unsigned long StartTime; //焼きの開始時間
	unsigned long FinishTime; //焼きの終了予想時間を示す
	SimpleTimerThread MainTimer;
	SimpleTimerThread KeepTimer;//温度維持のためのタイマー

	FurnaceControllerStatus WorkStatus;
	FurnaceControllerError ErrorStatus;
	FurnaceOrder Order;

	void RisingTemperature( ); //目標温度を上昇させる。実際の温度と離れると補正がはいる。
	void CalcFinishTime( ); //終了予定時間の計算をする

	//定数
	static const unsigned long Interval; //制御周期
	static const double IncreaseTemperaturePerMillis;  //1msあたりの温度上昇量 一分当たり2℃上昇 2/60/1000
	static  const double IncreaseTemperaturePerInterval;
	static const double MillisPerIncreaseTemperature;//終了時間の計算のために1周期あたりの計算
	static const double FirstKeepTemperature; //第一保持温度(℃)
	static const unsigned long FirstKeepTime; //30分維持
	static const double SecondKeepTemperature; //第二保持温度(℃)
	static const unsigned long SecondKeepTime; //120分維持
	static const double AllowableTemperatureError; //温度制御の許容誤差
	static const double ErrorTemperature; //許容最大温度．
};

#endif

