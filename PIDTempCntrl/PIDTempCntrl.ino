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
const uint8_t  BuzzerPin = 13; //Buzzer

//--------------------------------------------------------変数の宣言--------------------------------------------------------

TestThremo thermocouple = TestThremo();
SolidStateRelayThread relay = SolidStateRelayThread(SSRControlPin);
BuzzerThread Buzzer = BuzzerThread(BuzzerPin);
TemperatuerControllerThreadForPrepreg TemperatureController = TemperatuerControllerThreadForPrepreg();
FurnaceThread Furnace = FurnaceThread(relay, thermocouple, TemperatureController, 1, 1, 1, 1000);

//ButtonClass RightButton = ButtonClass(RightSwitchPin,Pullup, false);
//ButtonClass CenterButton = ButtonClass(CenterSwitchPin,Pullup, false);
//ButtonClass LeftButton = ButtonClass(LeftSwitchPin,Pullup, false);


//--------------------------------------------------------メイン--------------------------------------------------------
void setup()
{
	/*
	LCD.begin(16, 2);
	LCD.clear();
	LCD.setCursor(0, 0);
	LCD.print("HELLO-1234567890");
	LCD.setCursor(0, 1);
	LCD.print("-----SET UP-----");
	*/
	Serial.begin(115200);
	while ( !Serial );     // will pause Zero, Leonardo, etc until serial console opens
	Serial.println("START");


	Buzzer.SetSound(2000, 1000);
	Buzzer.Start();
	TemperatureController.Start();
	Furnace.Start();
	/*
	furnaceDisplay.Start();
	*/
}


void loop()
{
	SimpleTimerThread::GlobalTimerTick();

	relay.Tick();
	Buzzer.Tick();
	if ( Furnace.Tick() )
	{
		if ( Furnace.ShowStatus() !=FurnaceThreadStatus::Working )
		{
			Serial.println("Furnace is NOT Working ");
		}
	}
	if( TemperatureController.Tick())
	{
		switch ( TemperatureController.CheckError() )
		{
			case FurnaceControllerError::None:
				break;
			case FurnaceControllerError::NotEnoughTemperatureUpward:
				Serial.println("Not Enough TemperatureUpward");
				Buzzer.SoundOnce(200);
				break;
			case FurnaceControllerError::TooHot:
				Buzzer.SoundOnce(3000);
			case FurnaceControllerError::StartupFailure:
				Buzzer.SetSound(5000, 3000);
				Serial.println("Startup Failure");
				while ( true );
				break;
		}
	}
	/*
		furnaceDisplay.Tick();
		if ( TemperatureController.Tick() )
		{
			switch ( TemperatureController.CheckError() )
			{
				case FurnaceControllerError::None:
					break;
				case FurnaceControllerError::NotEnoughTemperatureUpward:
					Buzzer.SoundOnce(200);
					break;
				case FurnaceControllerError::TooHot:
					Buzzer.SoundOnce(3000);
				case FurnaceControllerError::StartupFailure:
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

			if ( TemperatureController.ShowStatus() == FurnaceControllerStatus::Finish )
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
		Serial.println("Press!");
		}

		*/
}

/*
覚書

温度の測定および計算に必要な時間は30-40msくらい。インターバルが200msはほしい。※31855と同列低グレード品の熱電対アンプMAX6675での実験結果
このPIDライブラリーの一回の計算時間は20ms程度。
※Arduino Nanoの場合
*/

