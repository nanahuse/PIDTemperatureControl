#include <max6675.h>
#include <PID_v1.h> //http://playground.arduino.cc/Code/PIDLibrary
#include <SPI.h>


//ピンの設定
#define SSPIN 10
#define MISOPIN 12
#define SCKPIN 13
#define CONTROLPIN 9

//温度計測

MAX6675 thermocouple(SCKPIN, SSPIN, MISOPIN);
double OldTemp = 0.0;
double AverageTemp = 0.0;
double now_Temperature;


//PID
double goal_Temperature, PIDOutput;
double Kp = 500.0, Ki = 5.0, Kd = 0.0;
PID myPID(&AverageTemp, &PIDOutput, &goal_Temperature, Kp, Ki, Kd);

//制御系
#define PIDLength 0.5 //摂氏 ±PIDLengthの範囲でのみPIDでパラメータを調整する。
#define ControlMax 500.0
#define ControlMin 0.0
int ControlOutput;
uint16_t GoalTemp;

//時間系
#define Interval 1000
#define RelayChangeTime 20 //一様データシートだと>10msになってる。これ以下の制御は無効になる。

//その他
//INPUT_PULLUPを使ったときにdigitalReadではONのときTrueにならないため。読みやすいように
#define isPressed(pin) !digitalRead(pin)

#if defined(ARDUINO_ARCH_SAMD)
// for Zero, output on USB Serial console, remove line below if using programming port to program the Zero!
#define Serial SerialUSB
#endif


void setup()
{
#ifndef ESP8266
	while ( !Serial );     // will pause Zero, Leonardo, etc until serial console opens
#endif

	Serial.begin(115200);
	Serial.println("START");

	pinMode(CONTROLPIN, OUTPUT);
	digitalWrite(CONTROLPIN, LOW);


	AverageTemp = thermocouple.readCelsius();
	while ( isnan(AverageTemp) )
	{
		AverageTemp = thermocouple.readCelsius();
	}

	Temperature_Set_Goal(GoalTemp);

	myPID.SetOutputLimits(ControlMin, ControlMax);
	myPID.SetSampleTime(Interval);

	delay(500);
}


void loop()
{

	TemperatureGetStep();

	TempSetStep();

	CalculateStep();

	RelayControlStep();

	DataSendStep();

}

//--------------------------------------------------------各ステップの中身--------------------------------------------------------

void TemperatureGetStep()
{
	//温度の測定
	now_Temperature = thermocouple.readCelsius();

	//エラーが出たときは古い値を使う。
	if ( isnan(now_Temperature) )
	{
		now_Temperature = OldTemp;
	}
	else
	{
		OldTemp = now_Temperature;
	}

}

void CalculateStep()
{

	//PIDに突っ込む温度。均すために平均っぽくしてる。
	AverageTemp = (AverageTemp + now_Temperature) * 0.5;

	//PIDの計算
	//ComputeはPIDの値が更新されたときTrueになるためここで計算するまで待つ。
	while ( !myPID.Compute() );

	ControlOutput = (int)PIDOutput;

}

void RelayControlStep()
{
	//出力
	unsigned long StartTime = millis();

	if ( ControlOutput >= RelayChangeTime ) //制御がリレー切り替えの時間より短い時は無効化
	{
		digitalWrite(CONTROLPIN, HIGH);
		delay(RelayChangeTime);  //リレーが壊れないようにスイッチが完了するまで待つ。
		while ( ControlOutput > millis() - StartTime ); //OFFになる時間まで空ループを回して待機。
	}

	digitalWrite(CONTROLPIN, LOW);
	delay(RelayChangeTime);  //リレーが壊れないようにスイッチが完了するまで待つ。
	while ( ControlMax > millis() - StartTime ); //制御時間が終わるまで空ループを回して待機。
}

void TempSetStep()
{ }

void DataSendStep()
{

	Serial.print(millis());
	Serial.print("\t");
	Serial.print(now_Temperature);
	Serial.print("\t");
	Serial.print(AverageTemp);
	Serial.print("\t");
	Serial.print(goal_Temperature);
	Serial.print("\t");
	Serial.print(ControlOutput);
	Serial.println();

}

//--------------------------------------------------------以下関数群--------------------------------------------------------

void Temperature_Set_Goal(uint16_t goal)
{
	goal_Temperature = (double)goal;
}

/*
覚書
温度の測定および計算に必要な時間は30-40msくらい。インターバルが200msはほしい。※31855と同列低グレード品の熱電対アンプMAX6675での実験結果
このPIDライブラリーの一回の計算時間は20ms程度。
※Arduino Nanoの場合
*/