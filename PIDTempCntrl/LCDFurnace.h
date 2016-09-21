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


//���Ԍn�̓~���b����

//--------------------------------------------------------�}�N��--------------------------------------------------------

//��ʂ̕\����Ԃ����߂�l�B���O���ςȋC������B
#define SetupDisplayMode 0
#define TemperatureDisplayMode 1
#define TimeDisplayMode 2
#define ControlInfoDispalyMode 3
#define ENDDisplayMode 4
#define ErrorDisplayMode 5

#define DISPLAYCHANGETIME 3000 //��ʂ̎����J�ڂ̊Ԋu


//--------------------------------------------------------�N���X�̒�`--------------------------------------------------------


//����g�p����M�d�΂�IThermometer�̌`�Ƀ��b�v
class Thermocouple : public IThermometer
{
public:
	Thermocouple(int8_t SCKPin, int8_t SSPin, int8_t MISOPin);
	double Read();
private:
	MAX6675 thermocouple;
};


/*
�����LCD�ɏ���\�����Ȃ���d�C�F�̐�����s���\���ɂ����̂�
Furnace���g�����ALCD�ɏ���\���ł���ɁB
*/
class FurnaceDisplay : public Furnace
{
public:
	FurnaceDisplay(uint8_t RelayControlPin, IThermometer* thermometer, LiquidCrystal* lcd);
	void Setup();
	void Start();
	bool Tick(); //Furnace��True��Ԃ������ɉ�ʕ\���̍X�V���s��True��Ԃ��B
	void Stop();

	void PrevDisplay(); //���̉�ʂɑJ�ځBTemperature,Time,ControlInfo�����邮�邷��B
	void NextDisplay(); //�O�̉�ʂɑJ�ځBTemperature,Time,ControlInfo�����邮�邷��B
	void SetDisplayChanging(); //������ʑJ�ڂ̃t���O�𔽓]������B
	void SetDisplayChanging(bool flag); //�����ŉ�ʑJ�ڂ��邩�ǂ����B
	void SetDisplayMode(uint8_t displayMode); //�C�ӂ̉�ʂ�\������B

	void DataOutputBySerial();

private:
	uint8_t DisplayMode; //�\�����Ă�����
	SimpleTimerThread DisplayChangeTimer; //��ʂ̎����J�ڂ̂��߂̃^�C�}�[
	LiquidCrystal* LCD; //�\���p��LCD�B�ʂ̏ꏊ�ŃC���X�^���X�̐������o�����ق����K���Ȃ̂ŁB

	void PrintTemperature(double InputTemperature); //��ʂɉ��x��\������
	void PrintTime(unsigned long InputTime); //��ʂɎ��Ԃ�\������B99:59:59�𒴂���ƈʒu���ꂷ��B

	void Update(); //DisplayMode�ɏ]����ʂ��X�V����B

//�\�������ʂ̒��g
	void DisplaySetupMode();
	void DisplayTemperature();
	void DisplayTime();
	void DisplayControlInfo();
	void DisplayENDMode();
	void DisplayError();
};


/*
�[��������s�\�ȃu�U�[
��x�����炷�A���������ƂɌJ��Ԃ��炷�̂ǂ��炩���\�B
*/
class BuzzerThread :public ThreadBase
{
public:
	BuzzerThread(uint8_t controlPin);
	void SetSound(unsigned long interval, unsigned long soundLength); //interval���Ƃ�soundLength�̒����u�U�[��炷�B
	void SoundOnce(unsigned long soundLength); //���s������SoundLength�̒����u�U�[��炷�B��x��ƃ^�C�}�[���~�܂�B
	bool Tick(); //������n�߂鎞��True��Ԃ��B
	void Start();
	void Stop();

private:
	unsigned long LengthTimer;
	unsigned long Interval, Length;
	uint8_t ControlPin;
	void UpdateIntervalTImer();
};


/*
INPUT_PULLUP���g�����{�^���N���X�B�^�N�g�X�C�b�`�̐ڑ��ɒ��ӁB
CanRepeat��False�ɂ���ƈ�x����
CanRepeat��True�ɂ���Ɖ������ςȂ��̎���RepeatTime�̊Ԋu��True��Ԃ��B
���[�v���Ȃ���{�^�����͂�����Ƃ���z��B
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