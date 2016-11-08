// LCDFurnace.h

#ifndef _LCDFURNACE_h
#define _LCDFURNACE_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif
#include <LiquidCrystal.h>
#include <max6675.h>
#include "ElectricFurnace.h"

//cppのほうに温度の設定とか移動

//--------------------------------------------------------定義とか--------------------------------------------------------
//画面の表示状態を決める値。名前が変な気がする。
enum DisplayMode
{
	DisplayMode_Temperature,
	DisplayMode_Time,
	DisplayMode_ControlInfo,
	DisplayMode_END,
	DisplayMode_Error
};

//Furnace(炉)の稼働状態を表す。Furnace::ShowStatusで返る値。
enum FurnaceControllerStatus
{
	FurnaceControllerStatus_Stop = 0,
	FurnaceControllerStatus_Startup,
	FurnaceControllerStatus_ToFirstKeepTemp,
	FurnaceControllerStatus_KeepFirstKeepTemp,
	FurnaceControllerStatus_ToSecondKeepTemp,
	FurnaceControllerStatus_KeepSecondkeepTemp,
	FurnaceControllerStatus_Finish,
	FurnaceControllerStatus_FatalError //回復不可能なエラー
};

enum FurnaceControllerError
{
	FurnaceControllerError_None = 0,
	FurnaceControllerError_NotEnoughTemperatureUpward,
	FurnaceControllerError_TooHot,
	FurnaceControllerError_StartupFailure //回復不能
};

//--------------------------------------------------------クラスの定義--------------------------------------------------------


//今回使用する熱電対をIThermometerの形にラップ
class Thermocouple : public IThermometer
{
public:
	Thermocouple(int8_t SCKPin, int8_t SSPin, int8_t MISOPin);
	double Read();
private:
	MAX6675 thermocouple;
};

class TemperatuerControllerThreadForPrepreg : public ITemperatureController, public ThreadBase
{
public:
	TemperatuerControllerThreadForPrepreg();

	bool Tick();
	void Start();
	void Stop();
	bool isRunning();

	double ShowGoalTemperature();
	double ShowMinimumTemperature();
	FurnaceOrder ShowOrderForFurnaceThread();
	FurnaceControllerStatus ShowStatus();
	FurnaceControllerError CheckError();
	unsigned long ShowElapseTime(); //経過時間
	unsigned long ShowRemainTime(); //残り時間
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

	void RisingTemperature(); //目標温度を上昇させる。実際の温度と離れると補正がはいる。
	void CalcFinishTime(); //終了予定時間の計算をする。

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

/*
リレーを操作するクラス
リレーをIntervalごとにOnTimeの長さだけ開放する。
*/

class SolidStateRelayThread : public ThreadBase, public IRelayController
{
public:
	SolidStateRelayThread(uint8_t controlPin);
	void Start();
	void Start(unsigned long intervalTimer);
	void Stop();
	bool Tick(); //リレーがOnになるタイミングでTrueを返す。
	bool isRunning();
	void SetOutput(double Output);

private:
	uint8_t ControlPin;
	SimpleTimerThread MainTimer;
	SimpleTimerThread SwitchingTimer, SafetyTimer;
	unsigned long OnTime;

	static const unsigned long ChangeTime; //リレーの切り替え時間
	static const unsigned long Interval;
	static const double OutputLimit;
	static const double OutputLimitPerRelayOutputMax;
};


/*
今回はLCDに情報を表示しながら電気炉の制御を行う構成にしたので
Furnaceを拡張し、LCDに情報を表示できるように。
*/
class FurnaceDisplay : public FurnaceThread
{
public:
	FurnaceDisplay(uint8_t ControlPin, IThermometer &thermometer, LiquidCrystal &lcd, TemperatuerControllerThreadForPrepreg &temperatureController);
	void Start();
	void Start(unsigned long intervalTimer);
	bool Tick(); //FurnaceがTrueを返した時に画面表示の更新を行いTrueを返す。
	void Stop();

	void PrevDisplay(); //次の画面に遷移。Temperature,Time,ControlInfoをぐるぐるする。
	void NextDisplay(); //前の画面に遷移。Temperature,Time,ControlInfoをぐるぐるする。
	void SetDisplayChanging(bool flag); //自動で画面遷移するかどうか。
	void SetDisplayChanging(); //自動で画面遷移するかどうかのフラグを反転する。

	void DataOutputBySerial();

private:
	DisplayMode NowDisplayMode; //表示している画面
	SimpleTimerThread DisplayChangeTimer; //画面の自動遷移のためのタイマー
	LiquidCrystal &LCD; //表示用のLCD。別の場所でインスタンスの生成が出来たほうが幸せなので。
	SolidStateRelayThread SolidStateRelay;

	void PrintTemperature(double InputTemperature); //画面に温度を表示する
	void PrintTime(unsigned long InputTime); //画面に時間を表示する。99:59:59を超えると位置ずれする。

	void Update(); //DisplayModeに従い画面を更新する。

//表示する画面の中身
	void DisplayTemperature();
	void DisplayTime();
	void DisplayControlInfo();
	void DisplayENDMode();
	void DisplayError();

	TemperatuerControllerThreadForPrepreg& TemperatureControllerForPrepreg;

//定数
	static const unsigned long DisplayChangeTime; //画面の自動遷移の間隔
	static const double Kp; //PIDパラメータ
	static const double Ki; //PIDパラメータ
	static const double Kd; //PIDパラメータ
	static const unsigned long Interval;
};


#endif