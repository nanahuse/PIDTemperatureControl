#include "ToolThred.h"
#include "LCDFurnace.h"
#include <LiquidCrystal.h>

//--------------------------------------------------------ピンの設定--------------------------------------------------------
//熱電対
const uint8_t TM_SSPin = 11;
const uint8_t  TM_MISOPin = 12;
const uint8_t  TM_SCKPin = 13;
//LCD
const uint8_t  LCD_RSPin = 3;
const uint8_t  LCD_RWPin = 9;
const uint8_t  LCD_ENABPin = 4;
const uint8_t  LCD_DB4Pin = 8;
const uint8_t  LCD_DB5Pin = 5;
const uint8_t  LCD_DB6Pin = 7;
const uint8_t  LCD_DB7Pin = 6;
//ボタン
const uint8_t  LeftSwitchPin = 15;
const uint8_t  CenterSwitchPin = 16;
const uint8_t  RightSwitchPin = 17;
//その他
const uint8_t  SSRControlPin = 2; //SSR
const uint8_t  BuzzerPin = 14; //Buzzer

//--------------------------------------------------------変数の宣言--------------------------------------------------------
Thermocouple thermocouple = Thermocouple(TM_SCKPin, TM_SSPin, TM_MISOPin);
LiquidCrystal LCD = LiquidCrystal(LCD_RSPin, LCD_RWPin, LCD_ENABPin, LCD_DB4Pin, LCD_DB5Pin, LCD_DB6Pin, LCD_DB7Pin);
TemperatuerControllerThreadForPrepreg TemperatureController = TemperatuerControllerThreadForPrepreg();
FurnaceDisplay furnaceDisplay = FurnaceDisplay(SSRControlPin, thermocouple, LCD,TemperatureController);
BuzzerThread Buzzer = BuzzerThread(BuzzerPin);

ButtonClass RightButton = ButtonClass(RightSwitchPin, false);
ButtonClass CenterButton = ButtonClass(CenterSwitchPin, false);
ButtonClass LeftButton = ButtonClass(LeftSwitchPin, false);


//--------------------------------------------------------メイン--------------------------------------------------------
void setup()
{
	LCD.begin(16, 2); 
	LCD.clear();
	LCD.setCursor(0, 0);
	LCD.print("HELLO-1234567890");
	LCD.setCursor(0, 1);
	LCD.print("-----SET UP-----");

#ifndef ESP8266
	while ( !Serial );     // will pause Zero, Leonardo, etc until serial console opens
#endif
	Serial.begin(115200);
	Serial.println("START");


	Buzzer.SoundOnce(1000);
	furnaceDisplay.Start();
	TemperatureController.Start();
}


void loop()
{
	Buzzer.Tick();
	furnaceDisplay.Tick();
	if ( TemperatureController.Tick() )
	{
		switch ( TemperatureController.CheckError() )
		{
			case FurnaceControllerError_None:
				break;
			case FurnaceControllerError_NotEnoughTemperatureUpward:
				Buzzer.SoundOnce(200);
				break;
			case FurnaceControllerError_TooHot:
				Buzzer.SoundOnce(3000);
			case FurnaceControllerError_StartupFailure:
				Buzzer.SetSound(10000, 3000);
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
				break;
		}

		if ( TemperatureController.ShowStatus() == FurnaceControllerStatus_Finish )
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

