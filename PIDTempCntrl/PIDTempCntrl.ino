#include "ToolThred.h"
#include "LCDFurnace.h"
#include <LiquidCrystal.h>

//--------------------------------------------------------ピンの設定--------------------------------------------------------
//熱電対
#define TM_SSPIN 11
#define TM_MISOPIN 12
#define TM_SCKPIN 13
//LCD
#define LCD_RS 3
#define LCD_RW 9
#define LCD_ENAB 4
#define LCD_DB4 8
#define LCD_DB5 5
#define LCD_DB6 7
#define LCD_DB7 6
//ボタン
#define LEFTSW  15
#define CENTERSW  16
#define RIGHTSW 17
//その他
#define CONTROLPIN 2 //SSR
#define BUZZERPIN 14 //Buzzer

//--------------------------------------------------------変数の宣言--------------------------------------------------------
Thermocouple thermocouple = Thermocouple(TM_SCKPIN, TM_SSPIN, TM_MISOPIN);
LiquidCrystal LCD = LiquidCrystal(LCD_RS, LCD_RW, LCD_ENAB, LCD_DB4, LCD_DB5, LCD_DB6, LCD_DB7);
FurnaceDisplay furnaceDisplay = FurnaceDisplay(CONTROLPIN, &thermocouple, &LCD);
BuzzerThread Buzzer = BuzzerThread(BUZZERPIN);

ButtonClass RightButton = ButtonClass(RIGHTSW, false);
ButtonClass CenterButton = ButtonClass(CENTERSW, false);
ButtonClass LeftButton = ButtonClass(LEFTSW, false);


//--------------------------------------------------------メイン--------------------------------------------------------
void setup()
{
	furnaceDisplay.Setup();

#ifndef ESP8266
	while ( !Serial );     // will pause Zero, Leonardo, etc until serial console opens
#endif
	Serial.begin(115200);
	Serial.println("START");


	Buzzer.SoundOnce(1000);
	furnaceDisplay.Start();
}


void loop()
{
	Buzzer.Tick();
	if ( furnaceDisplay.Tick() )
	{
		furnaceDisplay.DataOutputBySerial();
		if ( furnaceDisplay.CheckError() == ErrorNotEnoughTemperatureUpward )
		{
			Buzzer.SoundOnce(200);
		}
		if ( furnaceDisplay.ShowStatus() == StatusFinish )
		{
			//焼きが終了したらブザーを鳴らす。真ん中のボタンを押したら開放。
			Buzzer.SetSound(3000, 500);
			Buzzer.Start();
			while ( !CenterButton.isPressed() )
			{
				Buzzer.Tick();
				furnaceDisplay.Tick();
			}
			Buzzer.Stop();
			while ( true )
			{
				furnaceDisplay.Tick();
			}
		}
	}

	if ( RightButton.isPressed() )
	{
		furnaceDisplay.NextDisplay();
	}
	if ( LeftButton.isPressed() )
	{
		furnaceDisplay.PrevDisplay();
	}

	if ( CenterButton.isPressed() )
	{
		furnaceDisplay.SetDisplayChanging();
	}
}

/*
覚書

温度の測定および計算に必要な時間は30-40msくらい。インターバルが200msはほしい。※31855と同列低グレード品の熱電対アンプMAX6675での実験結果
このPIDライブラリーの一回の計算時間は20ms程度。
※Arduino Nanoの場合
*/

