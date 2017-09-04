// Display.h

#ifndef _ElectricFurnace_h
#define _ElectricFurnace_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

#include "SomePIDs.h"
#include "ThreadBase.h"

//----------------------------------------------------------------------------------------------------------------
//時間系は全てミリ秒


//FurnaceThread(炉)のエラー状態を表す。Furnace::CheckStatusで返る値。
enum struct FurnaceThreadStatus {
	Stop = 0 , //停止中.
	Working ,
	StopRelay , //リレーのみ停止している状態．
	FatalError , //ならないと良いな。
};

enum struct FurnaceThreadError {
	None = 0 ,  //エラー無し　false
	OutputShortage , //出力が足りない 出力が最大の時に表示
};

//FurnaceThreadへ命令するときに使う．
enum struct FurnaceOrder {
	Work = 0 , //通常通り動いてね　false
	StopAll ,  //停止させる．
	StopRelay //リレーだけ止めて温度計測は続けるとき
};

//リレーに入力される値の上限．出力の制限をしたい場合はここをいじるのではなくリレー側で
const double RelayInputMax = 1000.0;

//--------------------------------------------------------クラスの定義--------------------------------------------------------


/*
リレーを操作するクラス
*/
class IRelayController {
public:
	virtual void SetOutput(double Output) = 0; //0.0-1000.0の間
	virtual void Start( ) = 0;
	virtual void Stop( ) = 0;
};


/*
Furnaceで温度計を使うためのwrapperのインターフェース。
Readで温度を返すようにする。
*/
class IThermometer {
public:
	IThermometer( ) {
	}
	virtual double Read( ) = 0;
};

/*
FurnanceThreadを管理するクラス
目標温度を設定したり，炉のリレーを停止させたりとかの命令をだせる．
*/
class IFurnaceControllerHost {
public:
	virtual double ShowGoalTemperature( ) = 0;
	virtual void InputTemperature(double Temperature) = 0; //炉の温度状況を受ける
	virtual FurnaceOrder ShowOrderForFurnaceThread( ) = 0;
};


/*
炉の制御クラス。
リレーのオン時間をPID制御によって調整し、温度を制御する。SISO。
*/
class FurnaceThread :public ThreadBase {
public:
	FurnaceThread(IRelayController &relayController , IThermometer &thermometer , IFurnaceControllerHost &temperatureController , double Kp , double Ki , double Kd , double Interval);
	void Start( );
	void Start(unsigned long intervalTimer);
	bool Tick( ); //温度測定及びPID計算を行ったタイミングでTrueを返す。
	void Stop( );
	bool isRunning( );

	FurnaceThreadStatus ShowStatus( ); //炉の状態を表示する。
	FurnaceThreadError CheckError( ); //炉のエラーを表示する．
	double ShowTemperature( ); //炉の温度を表示する

protected:
	SimpleTimerThread MainTimer;
	IRelayController &RelayController;

	//PID
	VelocityPID_Ipd PIDController; //目標値の変動による影響を受けないように比例微分先行型のPIDを使う
	double PIDOutput;

	//Temperature
	IThermometer &Thermometer; //炉の温度を取得するための温度計
	double NowTemperature; //現在の温度
	double GoalTemperature; //目標温度

	FurnaceThreadStatus WorkStatus; //炉の稼働状態を表す
	FurnaceThreadError ErrorStatus; //炉に関するエラー

	IFurnaceControllerHost &TemperatureController; //温度の変化を制御するコントローラー

private:
	void GetTemperature( ); //温度を取得する。エラーが出た時は一つ前に取得できた値を使用する。
	void GetOrder( ); //ホストからの命令を読み込む．
};



#endif

