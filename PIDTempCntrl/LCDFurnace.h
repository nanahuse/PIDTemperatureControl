// LCDFurnace.h

#ifndef _LCDFURNACE_h
#define _LCDFURNACE_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif
#include <LiquidCrystal.h>
#include "ElectricFurnace.h"
#include "FurnaceControllerHostThreadForPrepreg.h"
#include "SolidStateRelayThread.h"
#include "Construct.h"

//cppのほうに温度の設定とか移動

//--------------------------------------------------------定義とか--------------------------------------------------------
//画面の表示状態を決める値。名前が変な気がする。
enum struct DisplayMode {
	Temperature ,
	Time ,
	ControlInfo ,
	END ,
	Error
};

//--------------------------------------------------------クラスの定義--------------------------------------------------------

/*
今回はLCDに情報を表示しながら電気炉の制御を行う構成にしたので
Furnaceを拡張し、LCDに情報を表示できるように。
*/
class FurnaceDisplay : public FurnaceThread {
public:
	FurnaceDisplay(uint8_t ControlPin , IThermometer &thermometer , LiquidCrystal &lcd , FurnaceControllerHostThreadForPrepreg &temperatureController);
	void Start( );
	void Start(unsigned long intervalTimer);
	bool Tick( ); //FurnaceがTrueを返した時に画面表示の更新を行いTrueを返す。
	void Stop( );

	void PrevDisplay( ); //次の画面に遷移。Temperature,Time,ControlInfoをぐるぐるする。
	void NextDisplay( ); //前の画面に遷移。Temperature,Time,ControlInfoをぐるぐるする。
	void SetDisplayChanging(bool flag); //自動で画面遷移するかどうか。
	void SetDisplayChanging( ); //自動で画面遷移するかどうかのフラグを反転する。

	void DataOutputBySerial( );

private:
	DisplayMode NowDisplayMode; //表示している画面
	SimpleTimerThread DisplayChangeTimer; //画面の自動遷移のためのタイマー
	LiquidCrystal &LCD; //表示用のLCD。別の場所でインスタンスの生成が出来たほうが幸せなので。
	SolidStateRelayThread SolidStateRelay;

	void PrintTemperature(double InputTemperature); //画面に温度を表示する
	void PrintTime(unsigned long InputTime); //画面に時間を表示する。99:59:59を超えると位置ずれする。

	void Update( ); //DisplayModeに従い画面を更新する。

//表示する画面の中身
	void DisplayTemperature( );
	void DisplayTime( );
	void DisplayControlInfo( );
	void DisplayENDMode( );
	void DisplayError( );

	FurnaceControllerHostThreadForPrepreg& TemperatureControllerForPrepreg;

	//定数
	static const unsigned long DisplayChangeTime; //画面の自動遷移の間隔
	static const double Kp; //PIDパラメータ
	static const double Ki; //PIDパラメータ
	static const double Kd; //PIDパラメータ
	static const unsigned long Interval;
};


class TestThremo : public IThermometer {
public:
	TestThremo( );
	double Read( );
private:
	double Value;
};

class  TestRelay : public IRelayController {
public:
	void Start( );
	void Stop( );
	void SetOutput(double Output);

private:

};

#endif