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



//Furnace(炉)のエラー状態を表す。Furnace::CheckErrorで返る値。
enum FurnaceError
{
	ErrorNone = 0, //エラー無し　false
	ErrorUnknown, //ならないと良いな。
	ErrorNotEnoughTemperatureUpward, //温度上昇が足りない
	ErrorNotGetTemperature //温度計がうまく動かない
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
class RelayThread : public SimpleTimerThread
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


class ITemperatureController
{
public:
	virtual double& ShowGoalTemperatureReference() = 0;
	virtual double ShowGoalTemeperature() = 0;
	virtual void InputTemperature(double Temperature) = 0;
	virtual void ErrorHandling(FurnaceError ErrorNumber) = 0;
};


/*
炉の制御クラス。
リレーのオン時間をPID制御によって調整し、温度を制御する。SISO。
*/
class Furnace :protected  SimpleTimerThread
{
public:
	Furnace(uint8_t RelayControlPin, IThermometer &thermometer);
	void Start();
	void Start(FurnaceStatus StartMode);
	bool Tick(); //温度測定及びPID計算を行ったタイミングでTrueを返す。
	void Stop();
	void SetIntervalTimer(unsigned long Time);

	FurnaceStatus ShowStatus(); //制御状態を表示。
	FurnaceError CheckError(); //エラーが起きたかを表示する。一度実行するとエラー無しが返るようになる。

	void SetGoalTemperature(double goal); //目標温度の設定。マニュアルモードのみで有効。

	static void SetTemperatureController(ITemperatureController &temeperatureController);

protected:

	RelayThread RelayController;

	//PID
	VelocityPID_Ipd PIDController; //目標値が変化していく影響を受けないように比例微分先行型のPIDを使っている。
	double GoalTemperature, PIDOutput;

	//Temperature
	IThermometer &Thermometer; //炉の温度を取得するための温度計
	double OldTemperature, AverageTemperature, NowTemperature; //ひとつ前の、平均化した、現在の温度
	FurnaceStatus WorkStatus; //炉の制御状態。
    FurnaceError ErrorStatus; //エラーが起きているときのエラーの情報。

	static ITemperatureController &TemperatureController; //温度の変化を制御するコントローラー 全部の並列で走らせても共通されるようにstatic

private:
	void GetTemperature(); //温度を取得する。エラーが出た時は一つ前に取得できた値を使用する。
	void UpdateIntervalTImer();
};



#endif

