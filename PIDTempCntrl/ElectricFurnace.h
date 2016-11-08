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
enum FurnaceThreadStatus
{
	FurnaceThreadStatus_Stop = 0, //停止中.
	FurnaceThreadStatus_Working,
	FurnaceThreadStatus_StopRelay, //リレーのみ停止している状態．
	FurnaceThreadStatus_FatalError, //ならないと良いな。
};

enum FurnaceThreadError
{
	FurnaceThreadError_None = 0,  //エラー無し　false
	FurnaceThreadError_OutputShortage, //出力が足りない 出力が最大の時に表示
};

//FurnaceThreadへ命令するときに使う．
enum FurnaceOrder
{
	FurnaceOrder_Work = 0, //通常通り動いてね　false
	FurnaceOrder_StopAll,  //停止させる．
	FurnaceOrder_StopRelay //リレーだけ止めて温度計測は続けるとき
};

const double RelayOutputMax = 1000.0;

//--------------------------------------------------------クラスの定義--------------------------------------------------------


/*
リレーを操作するクラス
*/
class IRelayController
{
public:
	virtual void SetOutput(double Output) = 0; //0.0-1000.0の間
	virtual void Start() = 0;
	virtual void Stop() = 0;
};


/*
Furnaceで温度計を使うためのwrapperのインターフェース。
Readで温度を返すようにする。
*/
class IThermometer
{
public:
	IThermometer() { }
	virtual double Read() = 0;
};


class ITemperatureController //なんか名前変かも　ControllerとかHostとかのほうがよさげ
{
public:
	virtual double ShowGoalTemperature() = 0;
	virtual void InputTemperature(double Temperature) = 0;
	virtual FurnaceOrder ShowOrderForFurnaceThread() = 0;
};


/*
炉の制御クラス。
リレーのオン時間をPID制御によって調整し、温度を制御する。SISO。
*/
class FurnaceThread :public ThreadBase
{
public:
	FurnaceThread(IRelayController &relayController, IThermometer &thermometer, ITemperatureController &temperatureController, double Kp, double Ki, double Kd, double Interval);
	void Start();
	void Start(unsigned long intervalTimer);
	bool Tick(); //温度測定及びPID計算を行ったタイミングでTrueを返す。
	void Stop();
	bool isRunning();

	FurnaceThreadStatus ShowStatus(); //炉の状態を表示する。
	FurnaceThreadError CheckError(); //炉のエラーを表示する．

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

	ITemperatureController &TemperatureController; //温度の変化を制御するコントローラー

private:
	void GetTemperature(); //温度を取得する。エラーが出た時は一つ前に取得できた値を使用する。
	void GetOrder(); //ホストからの命令を読み込む．
};



#endif

