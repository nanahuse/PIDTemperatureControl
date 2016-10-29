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


//時間系はミリ秒だよ

//--------------------------------------------------------マクロ--------------------------------------------------------

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
enum FurnaceStatus
{
	StatusStop = 0,
	StatusManual, //マニュアルで温度調整出来るモード
	StatusToFirstKeepTemp,
	StatusKeepFirstKeepTemp,
	StatusToSecondKeepTemp,
	StatusKeepSecondkeepTemp,
	StatusFinish,
	StatusError //回復不可能なエラー
};

//温度設定
const double TemperatuerControllerThreadForPrepreg::IncreaseTemperaturePerMillis = 0.00003333333333333333;  //1msあたりの温度上昇量 一分当たり2℃上昇 2/60/1000
const double TemperatuerControllerThreadForPrepreg::IncreaseTemperaturePerInterval = IncreaseTemperaturePerMillis*CONTROLINTERVAL;
const double TemperatuerControllerThreadForPrepreg::MillisPerIncreaseTemperature = 1.0 / IncreaseTemperaturePerMillis;//終了時間の計算のために1周期あたりの計算
const double TemperatuerControllerThreadForPrepreg::FirstKeepTemperature = 80.0 + TEMPCONTROLLENGTH; //第一保持温度(℃)
const unsigned long TemperatuerControllerThreadForPrepreg::FirstKeepTime = 1800000; //30分維持
const double TemperatuerControllerThreadForPrepreg::SecondKeepTemperature = 130.0 + TEMPCONTROLLENGTH; //第二保持温度(℃)
const unsigned long TemperatuerControllerThreadForPrepreg::SecondKeepTime = 7200000; //120分維持


const unsigned long FurnaceDisplay::DisplayChangeTime = 3000; //画面の自動遷移の間隔


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

class TemperatuerControllerThreadForPrepreg : ITemperatureController, SimpleTimerThread
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
今回はLCDに情報を表示しながら電気炉の制御を行う構成にしたので
Furnaceを拡張し、LCDに情報を表示できるように。
*/
class FurnaceDisplay : public Furnace
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


/*
擬似並列実行可能なブザー
一度だけ鳴らす、一定周期ごとに繰り返し鳴らすのどちらかが可能。
*/
class BuzzerThread :private SimpleTimerThread
{
public:
	BuzzerThread(uint8_t controlPin);
	void SetSound(unsigned long interval, unsigned long soundLength); //intervalごとにsoundLengthの長さブザーを鳴らす。
	void SoundOnce(unsigned long soundLength); //実行時からSoundLengthの長さブザーを鳴らす。一度鳴るとタイマーが止まる。
	bool Tick(); //音が鳴り始める時にTrueを返す。
	void Start();
	void Stop();

private:
	unsigned long LengthTimer;
	unsigned long Interval, Length;
	uint8_t ControlPin;
	void UpdateIntervalTImer();
};


/*
INPUT_PULLUPを使ったボタンクラス。タクトスイッチの接続に注意。
CanRepeatをFalseにすると一度だけ
CanRepeatをTrueにすると押しっぱなしの時はRepeatTimeの間隔でTrueを返す。
ループしながらボタン入力をするときを想定。
*/
class ButtonClass
{
public:
	ButtonClass(uint8_t controlPin, bool canRepeat);
	void SetRepeatTime(unsigned long RepeatTime);
	void SetCanRepeat(bool canRepeat);
	bool isPressed();
private:
	SimpleTimerThread Timer;
	uint8_t ControlPin;
	bool OldStatus;
	bool CanRepeat;
};

#endif