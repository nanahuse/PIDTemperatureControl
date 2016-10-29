// 
// 
// 

#include "LCDFurnace.h"

Thermocouple::Thermocouple(int8_t SCKPin, int8_t MISOPin, int8_t SSPin) : thermocouple(SCKPin, MISOPin, SSPin)
{ }

double Thermocouple::Read()
{
	return thermocouple.readCelsius();
}

FurnaceDisplay::FurnaceDisplay(uint8_t RelayControlPin, IThermometer &thermometer, LiquidCrystal &lcd) : Furnace(RelayControlPin, thermometer), DisplayChangeTimer(), LCD(lcd)
{
	NowDisplayMode = SetupDisplayMode;
}

void FurnaceDisplay::Setup()
{
	LCD.begin(16, 2);
	DisplaySetupMode();
	DisplayChangeTimer.SetInterval(DisplayChangeTime);
}

void FurnaceDisplay::SetDisplayMode(DisplayMode displayMode)
{
	NowDisplayMode = displayMode;
}

void FurnaceDisplay::DataOutputBySerial()
{
	Serial.print(millis() / 1000);
	Serial.print("\t");
	Serial.print(NowTemperature);
	Serial.print("\t");
	Serial.print(AverageTemperature);
	Serial.print("\t");
	Serial.print(GoalTemperature);
	Serial.print("\t");
	Serial.print(WorkStatus);
	Serial.print("\t");
	Serial.print(PIDOutput);
	Serial.print("\t");
	Serial.print(FinishTimer);
	Serial.println();
}

void FurnaceDisplay::Start()
{
	Furnace::Start();
	DisplayChangeTimer.Start();
	NowDisplayMode = ControlInfoDispalyMode;
}

bool FurnaceDisplay::Tick()
{
	if ( Furnace::Tick() )
	{
		switch ( WorkStatus )
		{
			case StatusFinish:
				NowDisplayMode = ENDDisplayMode;
				DisplayChangeTimer.Stop();
				break;
			case StatusError:
				NowDisplayMode = ErrorDisplayMode;
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
	Furnace::Stop();
	DisplayChangeTimer.Stop();
}

void FurnaceDisplay::PrevDisplay()
{
	switch ( NowDisplayMode )
	{
		case TemperatureDisplayMode:
			NowDisplayMode = ControlInfoDispalyMode;
			break;
		case TimeDisplayMode:
			NowDisplayMode = TemperatureDisplayMode;
			break;
		case ControlInfoDispalyMode:
			NowDisplayMode = TimeDisplayMode;
			break;
		default:
			break;
	}
	Update();
}

void FurnaceDisplay::NextDisplay()
{
	switch ( NowDisplayMode )
	{
		case ControlInfoDispalyMode:
			NowDisplayMode = TemperatureDisplayMode;
			break;
		case TimeDisplayMode:
			NowDisplayMode = ControlInfoDispalyMode;
			break;
		case TemperatureDisplayMode:
			NowDisplayMode = TimeDisplayMode;
			break;
		default:
			break;
	}
	Update();
}

void FurnaceDisplay::SetDisplayChanging()
{
	SetDisplayChanging(!DisplayChangeTimer.isRunning());
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


void FurnaceDisplay::Update()
{
	switch ( NowDisplayMode )
	{
		case SetupDisplayMode:
			DisplaySetupMode();
			break;
		case TemperatureDisplayMode:
			DisplayTemperature();
			break;
		case TimeDisplayMode:
			DisplayTime();
			break;
		case ControlInfoDispalyMode:
			DisplayControlInfo();
			break;
		case ENDDisplayMode:
			DisplayENDMode();
			break;
		case ErrorDisplayMode:
			DisplayError();
			break;
		default:
			break;
	}
}

void FurnaceDisplay::DisplaySetupMode()
{
	LCD.clear();
	LCD.setCursor(0, 0);
	LCD.print("HELLO 1234567890");
	LCD.setCursor(0, 1);
	LCD.print("SETUP START");
}

void FurnaceDisplay::DisplayTemperature()
{
	LCD.clear();
	LCD.setCursor(0, 0);
	LCD.print("NOW : ");
	PrintTemperature(AverageTemperature);
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
	PrintTime(millis() - StartTime);

	LCD.setCursor(0, 1);
	LCD.print("REMAIN : ");
	PrintTime(FinishTimer - millis());
}

void FurnaceDisplay::DisplayControlInfo()
{
	LCD.clear();
	LCD.setCursor(0, 0);
	LCD.print("STATUS : ");
	switch ( WorkStatus )
	{
		case StatusToFirstKeepTemp:
			LCD.print("To 80");
			LCD.write(0xDF);
			LCD.print("C");
			break;
		case StatusKeepFirstKeepTemp:
			LCD.print("Keep 80");
			break;
		case StatusToSecondKeepTemp:
			LCD.print("To130");
			LCD.write(0xDF);
			LCD.print("C");
			break;
		case StatusKeepSecondkeepTemp:
			LCD.print("Keep130");
			break;
		case StatusFinish:
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
	PrintTemperature(AverageTemperature);
	LCD.write(0xDF);
	LCD.print("C");

	LCD.setCursor(0, 1);
	LCD.print("RESULT : ");
	PrintTime(FinishTimer - StartTime);
}

void FurnaceDisplay::DisplayError()
{
	LCD.clear();
	LCD.setCursor(0, 0);
	LCD.print("ERROR");
	LCD.setCursor(0, 1);
	LCD.print("ERRORCODE : ");
	LCD.print(ErrorStatus);
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

BuzzerThread::BuzzerThread(uint8_t controlPin)
{
	ControlPin = controlPin;
	pinMode(ControlPin, OUTPUT);
	digitalWrite(ControlPin, LOW);
	this->Stop();
}

void BuzzerThread::SetSound(unsigned long interval, unsigned long soundLength)
{
	Interval = interval;
	Length = soundLength;
}

void BuzzerThread::SoundOnce(unsigned long soundLength)
{
	SetTimerStop(IntervalTimer);
	LengthTimer = millis() + soundLength;
	SetTimerStop(Interval);
	SetTimerStop(Length);
	digitalWrite(ControlPin, HIGH);
}

bool BuzzerThread::Tick()
{
	if ( millis() >= IntervalTimer )
	{
		digitalWrite(ControlPin, HIGH);
		UpdateIntervalTImer();
		return true;
	}
	if ( millis() >= LengthTimer )
	{
		digitalWrite(ControlPin, LOW);
		LengthTimer = IntervalTimer + Length;
	}
	return false;
}

void BuzzerThread::Start()
{
	IntervalTimer = millis() + Interval;
	LengthTimer = IntervalTimer + Length;
}

void BuzzerThread::Stop()
{
	digitalWrite(ControlPin, LOW);
	SetTimerStop(IntervalTimer);
	SetTimerStop(LengthTimer);
}

void BuzzerThread::UpdateIntervalTImer()
{
	unsigned long NowTime = millis();
	while ( IntervalTimer < NowTime )
	{
		IntervalTimer += Interval;
	}
}

ButtonClass::ButtonClass(uint8_t controlPin, bool canRepeat) : Timer()
{
	ControlPin = controlPin;
	pinMode(ControlPin, INPUT_PULLUP);
	SetCanRepeat(CanRepeat);
	SetRepeatTime(100);
	OldStatus = false;
}

void ButtonClass::SetRepeatTime(unsigned long RepeatTime)
{
	if ( CanRepeat )
	{
		Timer.SetInterval(RepeatTime);
	}
}

void ButtonClass::SetCanRepeat(bool canRepeat)
{
	CanRepeat = canRepeat;
	if ( !CanRepeat )
	{
		Timer.SetInterval(100); //チャタリングの防止用
		OldStatus = false;
	}
}

bool ButtonClass::isPressed()
{
	if ( !digitalRead(ControlPin) ) //プルアップ回路なのでdigitalReadがfalseの時に押されている。
	{
		if ( Timer.Tick() )
		{
			if ( OldStatus )
			{
				if ( CanRepeat )
				{
					return true;
				}
				else
				{
					return false;
					Timer.Stop();
				}
			}
			else
			{
				Timer.Start();
				OldStatus = true;
				return true;
			}
		}
		else
		{
			return false;
		}
	}
	else
	{
		if ( OldStatus )
		{
			OldStatus = false;
			if ( !CanRepeat )
			{
				Timer.Start();
			}
		}
		return false;

	}
}

double& TemperatuerControllerThreadForPrepreg::ShowGoalTemperatureReference()
{
	return GoalTemperature;
}

double TemperatuerControllerThreadForPrepreg::ShowGoalTemperature()
{
	return GoalTemperature;
}

void TemperatuerControllerThreadForPrepreg::InputTemperature(double Temperature)
{
	if ( Temperature < MinimumTemperature )
	{
		MinimumTemperature = Temperature;
	}
}

void TemperatuerControllerThreadForPrepreg::UpdateGoalTemperature()
{
	switch ( WorkStatus )
	{
		case StatusStop:
			break;
		case StatusManual:
			break;
		case StatusToFirstKeepTemp:
			if ( FirstKeepTemperature > MinimumTemperature + TEMPCONTROLLENGTH )
			{
				RisingTemperature();
			}
			else
			{ //保持温度付近に到達したら制御値を保持温度にして維持する
				CalcFinishTime();
				KeepTimer = millis();
				GoalTemperature = FirstKeepTemperature;
				WorkStatus = StatusKeepFirstKeepTemp;
			}
			break;

		case StatusKeepFirstKeepTemp:
			if ( millis() - KeepTimer >= FirstKeepTime )
			{
				WorkStatus = StatusToSecondKeepTemp;
			}
			break;

		case StatusToSecondKeepTemp:
			if ( SecondKeepTemperature > MinimumTemperature + TEMPCONTROLLENGTH )
			{
				RisingTemperature();
			}
			else
			{ //保持温度付近に到達したら制御値を保持温度にして維持する
				CalcFinishTime();
				KeepTimer = millis();
				GoalTemperature = SecondKeepTemperature;
				WorkStatus = StatusKeepSecondkeepTemp;
			}
			break;

		case StatusKeepSecondkeepTemp:
			if ( millis() - KeepTimer >= SecondKeepTemperature )
			{ //焼き終わり。リレーをオフにして冷やしていく。
				GoalTemperature = 0.0;
				RelayController.Stop();
				WorkStatus = StatusFinish;
			}
			break;

		case StatusFinish:
			//END
			break;

		default:
			//ERROR ここに来てしまったらどうしようもない。
			Stop();
			ErrorStatus = ErrorUnknown;
			WorkStatus = StatusError;
			break;
	}
}

void TemperatuerControllerThreadForPrepreg::RisingTemperature()
{ 
	GoalTemperature += IncreaseTemperaturePerInterval;

	if ( GoalTemperature > MinimumTemperature + TEMPCONTROLLENGTH )
	{
		ErrorStatus = ErrorNotEnoughTemperatureUpward;
		GoalTemperature = MinimumTemperature + TEMPCONTROLLENGTH;
		CalcFinishTime();
	}
}

void TemperatuerControllerThreadForPrepreg::CalcFinishTime()
{
		unsigned long TempTime = 0;
		FinishTimer = millis();

		if ( WorkStatus == StatusToFirstKeepTemp )
		{
			TempTime += FirstKeepTime;
		}

		if ( WorkStatus <= StatusToSecondKeepTemp )
		{
			TempTime += SecondKeepTime;
		}

		TempTime += (unsigned long)((SecondKeepTemperature - (float)MinimumTemperature) * MillisPerIncreaseTemperature);

		if ( TempTime > 359999000 )
		{
			TempTime = 359999000; //359999000は99:59:59をミリ秒に直したもの
		}
		FinishTimer += TempTime;
}
