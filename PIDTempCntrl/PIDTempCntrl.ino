#include <max6675.h>
#include <PID_v1.h> //http://playground.arduino.cc/Code/PIDLibrary
#include <SPI.h>


//�s���̐ݒ�
#define SSPIN 10
#define MISOPIN 12
#define SCKPIN 13
#define CONTROLPIN 9

//���x�v��

MAX6675 thermocouple(SCKPIN, SSPIN, MISOPIN);
double OldTemp = 0.0;
double AverageTemp = 0.0;
double now_Temperature;


//PID
double goal_Temperature, PIDOutput;
double Kp = 500.0, Ki = 5.0, Kd = 0.0;
PID myPID(&AverageTemp, &PIDOutput, &goal_Temperature, Kp, Ki, Kd);

//����n
#define PIDLength 0.5 //�ێ� �}PIDLength�͈̔͂ł̂�PID�Ńp�����[�^�𒲐�����B
#define ControlMax 500.0
#define ControlMin 0.0
int ControlOutput;
uint16_t GoalTemp;

//���Ԍn
#define Interval 1000
#define RelayChangeTime 20 //��l�f�[�^�V�[�g����>10ms�ɂȂ��Ă�B����ȉ��̐���͖����ɂȂ�B

//���̑�
//INPUT_PULLUP���g�����Ƃ���digitalRead�ł�ON�̂Ƃ�True�ɂȂ�Ȃ����߁B�ǂ݂₷���悤��
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

//--------------------------------------------------------�e�X�e�b�v�̒��g--------------------------------------------------------

void TemperatureGetStep()
{
	//���x�̑���
	now_Temperature = thermocouple.readCelsius();

	//�G���[���o���Ƃ��͌Â��l���g���B
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

	//PID�ɓ˂����މ��x�B�ς����߂ɕ��ς��ۂ����Ă�B
	AverageTemp = (AverageTemp + now_Temperature) * 0.5;

	//PID�̌v�Z
	//Compute��PID�̒l���X�V���ꂽ�Ƃ�True�ɂȂ邽�߂����Ōv�Z����܂ő҂B
	while ( !myPID.Compute() );

	ControlOutput = (int)PIDOutput;

}

void RelayControlStep()
{
	//�o��
	unsigned long StartTime = millis();

	if ( ControlOutput >= RelayChangeTime ) //���䂪�����[�؂�ւ��̎��Ԃ��Z�����͖�����
	{
		digitalWrite(CONTROLPIN, HIGH);
		delay(RelayChangeTime);  //�����[�����Ȃ��悤�ɃX�C�b�`����������܂ő҂B
		while ( ControlOutput > millis() - StartTime ); //OFF�ɂȂ鎞�Ԃ܂ŋ󃋁[�v���񂵂đҋ@�B
	}

	digitalWrite(CONTROLPIN, LOW);
	delay(RelayChangeTime);  //�����[�����Ȃ��悤�ɃX�C�b�`����������܂ő҂B
	while ( ControlMax > millis() - StartTime ); //���䎞�Ԃ��I���܂ŋ󃋁[�v���񂵂đҋ@�B
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

//--------------------------------------------------------�ȉ��֐��Q--------------------------------------------------------

void Temperature_Set_Goal(uint16_t goal)
{
	goal_Temperature = (double)goal;
}

/*
�o��
���x�̑��肨��ьv�Z�ɕK�v�Ȏ��Ԃ�30-40ms���炢�B�C���^�[�o����200ms�͂ق����B��31855�Ɠ����O���[�h�i�̔M�d�΃A���vMAX6675�ł̎�������
����PID���C�u�����[�̈��̌v�Z���Ԃ�20ms���x�B
��Arduino Nano�̏ꍇ
*/