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

//--------------------------------------------------------マクロ--------------------------------------------------------
//時間系は全てミリ秒


//FurnaceThread(炉)のエラー状態を表す。Furnace::CheckStatusで返る値。
enum FurnaceThreadStatus
{
	FurnaceThreadStatus_OK = 0, //エラー無し　false
	FurnaceThreadStatus_StopAll, //停止中．
	FurnaceThreadStatus_StopReray, //リレーのみ停止している状態．
	FurnaceThreadStatus_FatalError, //ならないと良いな。
	FurnaceThreadStatus_OutputShortage, //出力が足りない 出力が最大の時に表示
	FurnaceThreadStatus_NotGetTemperature //温度計がうまく動かない
};

//FurnaceThreadへ命令するときに使う．
enum FurnaceOrder
{
	FurnaceOrder_None = 0, //通常通り動いてね　false
	FurnaceOrder_StopAll,  //停止させる．
	FurnaceOrder_StopRelay //リレーだけ止めて温度計測は続けるとき
};

//--------------------------------------------------------クラスの定義--------------------------------------------------------


/*
リレーを操作するクラス
*/
class IRelayController
{
public:
	virtual void SetOutput(unsigned long Time) = 0;
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
	virtual double& ShowGoalTemperatureReference() = 0;
	virtual double ShowGoalTemeperature() = 0;
	virtual void InputTemperature(double Temperature) = 0;
	virtual void ShowFurnaceStatus() = 0;
	virtual FurnaceOrder ShowOrderForFurnaceThread() = 0;
};


/*
炉の制御クラス。
リレーのオン時間をPID制御によって調整し、温度を制御する。SISO。
*/
class FurnaceThread :public ThreadBase
{
public:
	FurnaceThread(IRelayController &relayController, IThermometer &thermometer, IPID &pidController);
	void Start();
	bool Tick(); //温度測定及びPID計算を行ったタイミングでTrueを返す。
	void Stop();
	bool isRunning();
	void SetIntervalTimer(unsigned long Time);

	FurnaceThreadStatus ShowStatus(); //炉の状態を表示する。一度実行するとエラー無しが返るようになる。

	static void SetTemperatureController(ITemperatureController &temeperatureController);

protected:
	SimpleTimerThread MainTimer;
	IRelayController &RelayController;

	//PID
	IPID &PIDController; //目標値が変化していく影響を受けないように比例微分先行型のPIDを使っている。
	double PIDOutput;

	//Temperature
	IThermometer &Thermometer; //炉の温度を取得するための温度計
	double OldTemperature, AverageTemperature, NowTemperature; //ひとつ前の、平均化した、現在の温度

	FurnaceThreadStatus WorkStatus; //炉の状態を表す

	static ITemperatureController &TemperatureController; //温度の変化を制御するコントローラー 全部の並列で走らせても共通されるようにstatic

private:
	void GetTemperature(); //温度を取得する。エラーが出た時は一つ前に取得できた値を使用する。
	void GetOrder(); //ホストからの命令を読み込む．
};



#endif

