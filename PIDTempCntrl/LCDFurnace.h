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
	SetupDisplayMode,
	TemperatureDisplayMode,
	TimeDisplayMode,
	ControlInfoDispalyMode,
	ENDDisplayMode,
	ErrorDisplayMode
};

//Furnace(炉)の稼働状態を表す。Furnace::ShowStatusで返る値。
enum FurnaceControllerStatus
{
	FurnaceStatus_Stop = 0,
	FurnaceStatus_ToFirstKeepTemp,
	FurnaceStatus_KeepFirstKeepTemp,
	FurnaceStatus_ToSecondKeepTemp,
	FurnaceStatus_KeepSecondkeepTemp,
	FurnaceStatus_Finish,
	FurnaceStatus_FatalError //回復不可能なエラー
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

	double& ShowGoalTemperatureReference();
	double ShowGoalTemperature();
	void InputTemperature(double Temperature);
	void ErrorHandling(FurnaceError ErrorNumber);
private:
	double GoalTemperature;
	double MinimumTemperature;
	unsigned long KeepTimer, StartTime, FinishTimer; //温度維持のため、焼きの開始時間、焼きの終了予想時間を示すタイマー

	FurnaceStatus WorkStatus;

	void UpdateGoalTemperature(); //目標温度を制御状態に合わせて変化させる。ここをインターフェースを用意して別クラスにすれば汎用性が出そう。
	void RisingTemperature(); //目標温度を上昇させる。実際の温度と離れると補正がはいる。
	void CalcFinishTime(); //終了予定時間の計算をする。

	static const double IncreaseTemperaturePerMillis;  //1msあたりの温度上昇量 一分当たり2℃上昇 2/60/1000
	static  const double IncreaseTemperaturePerInterval;
	static const double MillisPerIncreaseTemperature;//終了時間の計算のために1周期あたりの計算
	static const double FirstKeepTemperature; //第一保持温度(℃)
	static const unsigned long FirstKeepTime; //30分維持
	static const double SecondKeepTemperature; //第二保持温度(℃)
	static const unsigned long SecondKeepTime; //120分維持

};

/*
リレーを操作するクラス
リレーをIntervalごとにOnTimeの長さだけ開放する。
*/

class SolidStateRelayThread : public ThreadBase,public IRelayController
{
public:
	SolidStateRelayThread(uint8_t controlPin);
	void Start();
	void Stop();
	bool Tick(); //リレーがOnになるタイミングでTrueを返す。
	bool isRunning();
	void SetIntervalTimer(unsigned long Time);
	void SetOnTime(unsigned long Time);

private:
	uint8_t ControlPin;
	SimpleTimerThread MainTimer;
	SimpleTimerThread SwitchingTimer, SafetyTimer;
	unsigned long OnTime;

	static const unsigned long ChangeTime; //リレーの切り替え時間
};


/*
今回はLCDに情報を表示しながら電気炉の制御を行う構成にしたので
Furnaceを拡張し、LCDに情報を表示できるように。
*/
class FurnaceDisplay : public FurnaceThread
{
public:
	FurnaceDisplay(uint8_t RelayControlPin, IThermometer &thermometer, LiquidCrystal &lcd);
	void Setup();
	void Start();
	bool Tick(); //FurnaceがTrueを返した時に画面表示の更新を行いTrueを返す。
	void Stop();

	void PrevDisplay(); //次の画面に遷移。Temperature,Time,ControlInfoをぐるぐるする。
	void NextDisplay(); //前の画面に遷移。Temperature,Time,ControlInfoをぐるぐるする。
	void SetDisplayChanging(); //自動画面遷移のフラグを反転させる。
	void SetDisplayChanging(bool flag); //自動で画面遷移するかどうか。
	void SetDisplayMode(DisplayMode displayMode); //任意の画面を表示する。

	void DataOutputBySerial();

private:
	DisplayMode NowDisplayMode; //表示している画面
	SimpleTimerThread DisplayChangeTimer; //画面の自動遷移のためのタイマー
	LiquidCrystal &LCD; //表示用のLCD。別の場所でインスタンスの生成が出来たほうが幸せなので。
	//	VelocityPID_Ipd pidController(AverageTemperature, PIDOutput, TemperatureController.ShowGoalTemperatureReference(), KP, KI, KD, CONTROLMAX*0.5)

	void PrintTemperature(double InputTemperature); //画面に温度を表示する
	void PrintTime(unsigned long InputTime); //画面に時間を表示する。99:59:59を超えると位置ずれする。

	void Update(); //DisplayModeに従い画面を更新する。

//表示する画面の中身
	void DisplaySetupMode();
	void DisplayTemperature();
	void DisplayTime();
	void DisplayControlInfo();
	void DisplayENDMode();
	void DisplayError();

	static const unsigned long DisplayChangeTime; //画面の自動遷移の間隔
};


#endif