// 
// 
// 

#include "LCDFurnace.h"


//--------------------------------------------------------設定とか--------------------------------------------------------
//時間系はミリ秒だよ

//PIDparameter
const double FurnaceDisplay::Kp = 500.0;
const double FurnaceDisplay::Ki = 5.0;
const double FurnaceDisplay::Kd = 0.0;

const unsigned long FurnaceDisplay::DisplayChangeTime = 3000; //画面の自動遷移の間隔
const unsigned long FurnaceDisplay::Interval = ControlInterval;


//--------------------------------------------------------クラスの定義--------------------------------------------------------
FurnaceDisplay::FurnaceDisplay(uint8_t ControlPin, IThermometer &thermometer, LiquidCrystal &lcd, FurnaceControllerHostThreadForPrepreg &temperatureController) :SolidStateRelay(ControlPin), FurnaceThread(SolidStateRelay, thermometer, temperatureController, Kp, Ki, Kd, Interval), DisplayChangeTimer(), LCD(lcd), TemperatureControllerForPrepreg(temperatureController)
{
	NowDisplayMode = DisplayMode::Temperature;
}

void FurnaceDisplay::DataOutputBySerial()
{
	Serial.print(millis() / 1000);
	Serial.print("\t");
	Serial.print(NowTemperature);
	Serial.print("\t");
	Serial.print(GoalTemperature);
	Serial.print("\t");
	Serial.print(static_cast<int>(WorkStatus));
	Serial.print("\t");
	Serial.print(PIDOutput);
	Serial.print("\t");
	Serial.print(TemperatureControllerForPrepreg.ShowRemainTime());
	Serial.println();
}



void FurnaceDisplay::Start()
{
	Start(millis()); //デフォルト引数使うとStart()居ないって怒られるため．
}

void FurnaceDisplay::Start(unsigned long intervalTimer)
{
	FurnaceThread::Start(intervalTimer);
	NowDisplayMode = DisplayMode::Temperature;
	DisplayChangeTimer.Start(intervalTimer);
}

bool FurnaceDisplay::Tick()
{
	if ( FurnaceThread::Tick() )
	{
		switch ( TemperatureControllerForPrepreg.ShowStatus() )
		{
			case FurnaceControllerStatus::Finish:
				NowDisplayMode = DisplayMode::END;
				DisplayChangeTimer.Stop();
				break;
			case FurnaceControllerStatus::FatalError:
				NowDisplayMode = DisplayMode::Error;
				break;
			default:

				break;
		}
		Update();
		return true;
	}
	if ( DisplayChangeTimer.Tick() )
	{
		NextDisplay();
	}
	return false;
}

void FurnaceDisplay::Stop()
{
	FurnaceThread::Stop();
	DisplayChangeTimer.Stop();
}

void FurnaceDisplay::PrevDisplay()
{
	switch ( NowDisplayMode )
	{
		case DisplayMode::Temperature:
			NowDisplayMode = DisplayMode::ControlInfo;
			break;
		case DisplayMode::Time:
			NowDisplayMode = DisplayMode::Temperature;
			break;
		case DisplayMode::ControlInfo:
			NowDisplayMode = DisplayMode::Time;
			break;
		default:
			break;
	}
	Update();
	DisplayChangeTimer.Start();
}

void FurnaceDisplay::NextDisplay()
{
	switch ( NowDisplayMode )
	{
		case DisplayMode::ControlInfo:
			NowDisplayMode = DisplayMode::Temperature;
			break;
		case DisplayMode::Time:
			NowDisplayMode = DisplayMode::ControlInfo;
			break;
		case DisplayMode::Temperature:
			NowDisplayMode = DisplayMode::Time;
			break;
		default:
			break;
	}
	Update();
	DisplayChangeTimer.Start();
}

void FurnaceDisplay::SetDisplayChanging(bool flag)
{
	if ( flag )
	{
		DisplayChangeTimer.Start();
	}
	else
	{
		DisplayChangeTimer.Stop();
	}
}

void FurnaceDisplay::SetDisplayChanging()
{
	if ( DisplayChangeTimer.isRunning() )
	{
		DisplayChangeTimer.Stop();
	}
	else
	{
		DisplayChangeTimer.Start();
	}
}


void FurnaceDisplay::Update()
{
	switch ( NowDisplayMode )
	{
		case DisplayMode::Temperature:
			DisplayTemperature();
			break;
		case DisplayMode::Time:
			DisplayTime();
			break;
		case DisplayMode::ControlInfo:
			DisplayControlInfo();
			break;
		case DisplayMode::END:
			DisplayENDMode();
			break;
		case DisplayMode::Error:
			DisplayError();
			break;
		default:
			break;
	}
}

void FurnaceDisplay::DisplayTemperature()
{
	LCD.clear();
	LCD.setCursor(0, 0);
	LCD.print("NOW : ");
	PrintTemperature(NowTemperature);
	LCD.write(0xDF);
	LCD.print("C");
	LCD.setCursor(0, 1);
	LCD.print("TO  : ");
	PrintTemperature(GoalTemperature);
	LCD.write(0xDF);
	LCD.print("C");
}

void FurnaceDisplay::DisplayTime()
{
	LCD.clear();
	LCD.setCursor(0, 0);
	LCD.print("ELAPSE : ");
	PrintTime(TemperatureControllerForPrepreg.ShowElapseTime());

	LCD.setCursor(0, 1);
	LCD.print("REMAIN : ");
	PrintTime(TemperatureControllerForPrepreg.ShowRemainTime());
}

void FurnaceDisplay::DisplayControlInfo()
{
	LCD.clear();
	LCD.setCursor(0, 0);
	LCD.print("STATUS : ");
	switch ( TemperatureControllerForPrepreg.ShowStatus() )
	{
		case FurnaceControllerStatus::ToFirstKeepTemp:
			LCD.print("To 80");
			LCD.write(0xDF);
			LCD.print("C");
			break;
		case FurnaceControllerStatus::KeepFirstKeepTemp:
			LCD.print("Keep 80");
			break;
		case FurnaceControllerStatus::ToSecondKeepTemp:
			LCD.print("To130");
			LCD.write(0xDF);
			LCD.print("C");
			break;
		case FurnaceControllerStatus::KeepSecondkeepTemp:
			LCD.print("Keep130");
			break;
		case FurnaceControllerStatus::Finish:
			LCD.print("COOLING");
			break;
		default:
			break;
	}

	LCD.setCursor(0, 1);
	LCD.print("OUTPUT : ");
	LCD.print(PIDOutput);
}

void FurnaceDisplay::DisplayENDMode()
{
	LCD.clear();
	LCD.setCursor(0, 0);
	LCD.print("COOLING  ");
	PrintTemperature(NowTemperature);
	LCD.write(0xDF);
	LCD.print("C");

	LCD.setCursor(0, 1);
	LCD.print("RESULT : ");
	PrintTime(TemperatureControllerForPrepreg.ShowElapseTime() + TemperatureControllerForPrepreg.ShowRemainTime());
}

void FurnaceDisplay::DisplayError()
{
	LCD.clear();
	LCD.setCursor(0, 0);
	LCD.print("ERROR");
	LCD.setCursor(0, 1);
	LCD.print("ERRORCODE : ");
	LCD.print(static_cast<int>(TemperatureControllerForPrepreg.CheckError()));
}

void FurnaceDisplay::PrintTemperature(double InputTemperature)
{
	float Var = (float)InputTemperature;
	uint16_t IntPart = (uint16_t)Var;
	uint16_t DecPart = (uint16_t)(Var * 10) - IntPart * 10;
	if ( IntPart >= 100 )
	{
		LCD.print(IntPart);
	}
	else if ( IntPart >= 10 )
	{
		LCD.print('0');
		LCD.print(IntPart);
	}
	else
	{
		LCD.print("00");
		LCD.print(IntPart);
	}
	LCD.print('.');
	LCD.print(DecPart);
}

void FurnaceDisplay::PrintTime(unsigned long InputTime)
{
	unsigned long Time = InputTime / 1000;
	unsigned long H = Time / 3600;
	unsigned long M = (Time / 60 - H * 60);
	unsigned long S = Time - H * 3600 - M * 60;

	LCD.print(H);
	LCD.print(':');
	if ( M >= 10 )
	{
		LCD.print(M);
	}
	else
	{
		LCD.print('0');
		LCD.print(M);
	}
	LCD.print(':');
	if ( S >= 10 )
	{
		LCD.print(S);
	}
	else
	{
		LCD.print('0');
		LCD.print(S);
	}
}



TestThremo::TestThremo()
{
	Value = 0.0;
}

double TestThremo::Read()
{
	Value += 0.02;
	return Value;
}

void TestRelay::Start()
{ }

void TestRelay::Stop()
{ }

void TestRelay::SetOutput(double Output)
{ }
