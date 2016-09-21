#include "LCDFurnace.h"
#include <LiquidCrystal.h>

//--------------------------------------------------------�s���̐ݒ�--------------------------------------------------------
//�M�d��
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
//�{�^��
#define LEFTSW  15
#define CENTERSW  16
#define RIGHTSW 17
//���̑�
#define CONTROLPIN 2 //SSR
#define BUZZERPIN 14 //Buzzer

//--------------------------------------------------------�ϐ��̐錾--------------------------------------------------------
Thermocouple thermocouple = Thermocouple(TM_SCKPIN, TM_SSPIN, TM_MISOPIN);
LiquidCrystal LCD = LiquidCrystal(LCD_RS, LCD_RW, LCD_ENAB, LCD_DB4, LCD_DB5, LCD_DB6, LCD_DB7);
FurnaceDisplay furnaceDisplay = FurnaceDisplay(CONTROLPIN, &thermocouple, &LCD);
BuzzerThread Buzzer = BuzzerThread(BUZZERPIN);

ButtonClass RightButton = ButtonClass(RIGHTSW, false);
ButtonClass CenterButton = ButtonClass(CENTERSW, false);
ButtonClass LeftButton = ButtonClass(LEFTSW, false);


//--------------------------------------------------------���C��--------------------------------------------------------
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
			//�Ă����I��������u�U�[��炷�B�^�񒆂̃{�^������������J���B
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
�o��

���x�̑��肨��ьv�Z�ɕK�v�Ȏ��Ԃ�30-40ms���炢�B�C���^�[�o����200ms�͂ق����B��31855�Ɠ����O���[�h�i�̔M�d�΃A���vMAX6675�ł̎�������
����PID���C�u�����[�̈��̌v�Z���Ԃ�20ms���x�B
��Arduino Nano�̏ꍇ
*/

