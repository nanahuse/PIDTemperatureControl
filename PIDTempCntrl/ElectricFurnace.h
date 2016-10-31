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

//時間系
#define CONTROLINTERVAL 1000 //Furnaceの制御周期。RelayThreadでも使用している。
#define RELAYCHANGETIME 20 //リレーのスイッチ時間。FOTEK SSR-40DAのデータシートだと<10msになってる。これ以下の制御は無効になる。

//制御系
#define TEMPCONTROLLENGTH 1.0 //摂氏。制御の際に許す誤差。
#define CONTROLMAX 500.0 //PID出力上限。リレーが開きぱなしにならないように周期の半分にしてる
#define CONTROLMIN 0.0 //PID出力下限


//PIDparameter
#define KP 500.0
#define KI 5.0
#define KD 0.0



//--------------------------------------------------------クラスの定義--------------------------------------------------------


/*
リレーを操作するクラス
リレーをIntervalごとにOnTimeの長さだけ開放する。
*/
class RelayThread : protected SimpleTimerThread
{
public:
	RelayThread(uint8_t controlPin);
	void Start();
	void Stop();
	bool Tick(); //リレーがOnになるタイミングでTrueを返す。
	void SetIntervalTimer(unsigned long Time);
	void SetOnTime(unsigned long Time);


private:
	uint8_t ControlPin;
	unsigned long SwitchingTimer, SafetyTimer;
	unsigned long OnTime;
	void UpdateIntervalTimer();

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
//	virtual void ErrorHandling(FurnaceError ErrorNumber) = 0;
	virtual void ShowFurnaceStatus() = 0;
	virtual FurnaceOrder ShowOrderForFurnaceThread() =0;
};


/*
炉の制御クラス。
リレーのオン時間をPID制御によって調整し、温度を制御する。SISO。
*/
class FurnaceThread :protected  SimpleTimerThread
{
public:
	FurnaceThread(uint8_t RelayControlPin, IThermometer &thermometer);
	void Start();
	bool Tick(); //温度測定及びPID計算を行ったタイミングでTrueを返す。
	void Stop();
	void SetIntervalTimer(unsigned long Time);
	bool isRunning();

	FurnaceThreadStatus ShowStatus(); //炉の状態を表示する。一度実行するとエラー無しが返るようになる。

	static void SetTemperatureController(ITemperatureController &temeperatureController);

protected:

	RelayThread RelayController;

	//PID
	VelocityPID_Ipd PIDController; //目標値が変化していく影響を受けないように比例微分先行型のPIDを使っている。
	double PIDOutput;

	//Temperature
	IThermometer &Thermometer; //炉の温度を取得するための温度計
	double OldTemperature, AverageTemperature, NowTemperature; //ひとつ前の、平均化した、現在の温度
    FurnaceThreadStatus WorkStatus; //エラーが起きているときのエラーの情報。

	static ITemperatureController &TemperatureController; //温度の変化を制御するコントローラー 全部の並列で走らせても共通されるようにstatic

private:
	void GetTemperature(); //温度を取得する。エラーが出た時は一つ前に取得できた値を使用する。
	void GetOrder(); //ホストからの命令を読み込む．
	void UpdateIntervalTimer();
};



#endif

