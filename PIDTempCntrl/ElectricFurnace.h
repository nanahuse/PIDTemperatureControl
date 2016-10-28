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

//Furnace(炉)の稼働状態を表す。Furnace::ShowStatusで返る値。
#define StatusStop 0
#define StatusManual 1 //マニュアルで温度調整出来るモード
#define StatusToFirstKeepTemp 2
#define StatusKeepFirstKeepTemp 3
#define StatusToSecondKeepTemp 4
#define StatusKeepSecondkeepTemp 5
#define StatusFinish 6
#define StatusError 10 //回復不可能なエラー

//Furnace(炉)のエラー状態を表す。Furnace::CheckErrorで返る値。
#define ErrorNone 0
#define ErrorUnknown 1 //ならないと良いな。
#define ErrorNotEnoughTemperatureUpward 2 //温度上昇が足りない

//時間系
#define CONTROLINTERVAL 1000 //Furnaceの制御周期。RelayThreadでも使用している。
#define RELAYCHANGETIME 20 //リレーのスイッチ時間。FOTEK SSR-40DAのデータシートだと<10msになってる。これ以下の制御は無効になる。

//制御系
#define TEMPCONTROLLENGTH 1.0 //摂氏。制御の際に許す誤差。
#define CONTROLMAX 500.0 //PID出力上限。リレーが開きぱなしにならないように周期の半分にしてる
#define CONTROLMIN 0.0 //PID出力下限

//温度設定
#define INCTEMPperMILLIS 0.00003333333333333333  //1msあたりの温度上昇量 一分当たり2℃上昇 2/60/1000
#define INCTEMPperINTERVAL INCTEMPperMILLIS*CONTROLINTERVAL
#define MILLISperINCTEMP 1.0/INCTEMPperMILLIS //終了時間の計算のために1周期あたりの計算
#define FIRSTKEEPTEMP (80.0+TEMPCONTROLLENGTH) //第一保持温度(℃)
#define FIRSTKEEPMILLIS 1800000 //30分維持
#define SECONDKEEPTEMP (130.0+TEMPCONTROLLENGTH) //第二保持温度(℃)
#define SECONDKEEPMILLIS 7200000 //120分維持

//PIDparameter
#define KP 500.0
#define KI 5.0
#define KD 0.0



//--------------------------------------------------------クラスの定義--------------------------------------------------------


/*
リレーを操作するクラス
リレーをIntervalごとにOnTimeの長さだけ開放する。
*/
class RelayThread : public ThreadBase
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


/*
炉の制御クラス。
リレーのオン時間をPID制御によって調整し、温度を制御する。SISO。
*/
class Furnace :public  ThreadBase
{
public:
	Furnace(uint8_t RelayControlPin, IThermometer* thermometer);
	void Start();
	void Start(uint8_t StartMode);
	bool Tick(); //温度測定及びPID計算を行ったタイミングでTrueを返す。
	void Stop();
	void SetIntervalTimer(unsigned long Time);

	uint8_t ShowStatus(); //制御状態を表示。
	uint8_t CheckError(); //エラーが起きたかを表示する。一度実行するとエラー無しが返るようになる。

	void SetGoalTemperature(double goal); //目標温度の設定。マニュアルモードのみで有効。

protected:

	RelayThread RelayController;

	//PID
	VelocityPID_Ipd PIDController; //目標値が変化していく影響を受けないように比例微分先行型のPIDを使っている。
	double GoalTemperature, PIDOutput;

	//Temperature
	IThermometer* Thermometer; //炉の温度を取得するための温度計
	unsigned long KeepTimer, StartTime, FinishTimer; //温度維持のため、焼きの開始時間、焼きの終了予想時間を示すタイマー
	double OldTemperature, AverageTemperature, NowTemperature; //ひとつ前の、平均化した、現在の温度
	uint8_t WorkStatus; //炉の制御状態。
	uint8_t ErrorStatus; //エラーが起きているときのエラーの情報。


private:
	void GetTemperature(); //温度を取得する。エラーが出た時は一つ前に取得できた値を使用する。
	void UpdateGoalTemperature(); //目標温度を制御状態に合わせて変化させる。ここをインターフェースを用意して別クラスにすれば汎用性が出そう。
	void RisingTemperature(); //目標温度を上昇させる。実際の温度と離れると補正がはいる。
	void CalcFinishTime(); //終了予定時間の計算をする。
	void UpdateIntervalTImer();
};



#endif

