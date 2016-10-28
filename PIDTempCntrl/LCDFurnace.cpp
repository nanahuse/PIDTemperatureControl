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

FurnaceDisplay::FurnaceDisplay(uint8_t RelayControlPin, IThermometer* thermometer, LiquidCrystal* lcd) : Furnace(RelayControlPin, thermometer),DisplayChangeTimer()
{
	DisplayMode = SetupDisplayMode;
	LCD = lcd;
}

void FurnaceDisplay::Setup()
{
	LCD->begin(16, 2);
	DisplaySetupMode();
	DisplayChangeTimer.SetInterval(DISPLAYCHANGETIME);
}

void FurnaceDisplay::SetDisplayMode(uint8_t displayMode)
{
	DisplayMode = displayMode;
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
	DisplayMode = ControlInfoDispalyMode;
}

bool FurnaceDisplay::Tick()
{
	if ( Furnace::Tick() )
	{
		switch ( WorkStatus )
		{
			case StatusFinish:
				DisplayMode = ENDDisplayMode;
				DisplayChangeTimer.Stop();
				break;
			case StatusError:
				DisplayMode = ErrorDisplayMode;
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
	if ( (DisplayMode <= ControlInfoDispalyMode) && (DisplayMode != SetupDisplayMode) )
	{
		DisplayChangeTimer.Start();
		if ( DisplayMode == TemperatureDisplayMode )
		{
			DisplayMode = ControlInfoDispalyMode;
		}
		else
		{
			DisplayMode--;
		}
		Update();
	}
}

void FurnaceDisplay::NextDisplay()
{
	if ( (DisplayMode <= ControlInfoDispalyMode) &&(DisplayMode != SetupDisplayMode) )
	{
		DisplayChangeTimer.Start();
		if ( DisplayMode == ControlInfoDispalyMode )
		{
			DisplayMode = TemperatureDisplayMode;
		}
		else
		{
			DisplayMode++;
		}
		Update();
	}
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
	switch ( DisplayMode )
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
	LCD->clear();
	LCD->setCursor(0, 0);
	LCD->print("HELLO 1234567890");
	LCD->setCursor(0, 1);
	LCD->print("SETUP START");
}

void FurnaceDisplay::DisplayTemperature()
{
	LCD->clear();
	LCD->setCursor(0, 0);
	LCD->print("NOW : ");
	PrintTemperature(AverageTemperature);
	LCD->write(0xDF);
	LCD->print("C");
	LCD->setCursor(0, 1);
	LCD->print("TO  : ");
	PrintTemperature(GoalTemperature);
	LCD->write(0xDF);
	LCD->print("C");
}

void FurnaceDisplay::DisplayTime()
{
	LCD->clear();
	LCD->setCursor(0, 0);
	LCD->print("ELAPSE : ");
	PrintTime(millis() - StartTime);

	LCD->setCursor(0, 1);
	LCD->print("REMAIN : ");
	PrintTime(FinishTimer - millis());
}

void FurnaceDisplay::DisplayControlInfo()
{
	LCD->clear();
	LCD->setCursor(0, 0);
	LCD->print("STATUS : ");
	switch ( WorkStatus )
	{
		case StatusToFirstKeepTemp:
			LCD->print("To 80");
			LCD->write(0xDF);
			LCD->print("C");
			break;
		case StatusKeepFirstKeepTemp:
			LCD->print("Keep 80");
			break;
		case StatusToSecondKeepTemp:
			LCD->print("To130");
			LCD->write(0xDF);
			LCD->print("C");
			break;
		case StatusKeepSecondkeepTemp:
			LCD->print("Keep130");
			break;
		case StatusFinish:
			LCD->print("COOLING");
			break;
		default:
			break;
	}

	LCD->setCursor(0, 1);
	LCD->print("OUTPUT : ");
	LCD->print(PIDOutput);
}

void FurnaceDisplay::DisplayENDMode()
{
	LCD->clear();
	LCD->setCursor(0, 0);
	LCD->print("COOLING  ");
	PrintTemperature(AverageTemperature);
	LCD->write(0xDF);
	LCD->print("C");

	LCD->setCursor(0, 1);
	LCD->print("RESULT : ");
	PrintTime(FinishTimer - StartTime);
}

void FurnaceDisplay::DisplayError()
{
	LCD->clear();
	LCD->setCursor(0, 0);
	LCD->print("ERROR");
	LCD->setCursor(0, 1);
	LCD->print("ERRORCODE : ");
	LCD->print(ErrorStatus);
}

void FurnaceDisplay::PrintTemperature(double InputTemperature)
{
	float Var = (float)InputTemperature;
	uint16_t IntPart = (uint16_t)Var;
	uint16_t DecPart = (uint16_t)(Var * 10) - IntPart * 10;
	if ( IntPart >= 100 )
	{
		LCD->print(IntPart);
	}
	else if ( IntPart >= 10 )
	{
		LCD->print('0');
		LCD->print(IntPart);
	}
	else
	{
		LCD->print("00");
		LCD->print(IntPart);
	}
	LCD->print('.');
	LCD->print(DecPart);
}

void FurnaceDisplay::PrintTime(unsigned long InputTime)
{
	unsigned long Time = InputTime / 1000;
	unsigned long H = Time / 3600;
	unsigned long M = (Time / 60 - H * 60);
	unsigned long S = Time - H * 3600 - M * 60;

	LCD->print(H);
	LCD->print(':');
	if ( M >= 10 )
	{
		LCD->print(M);
	}
	else
	{
		LCD->print('0');
		LCD->print(M);
	}
	LCD->print(':');
	if ( S >= 10 )
	{
		LCD->print(S);
	}
	else
	{
		LCD->print('0');
		LCD->print(S);
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
	Length = 0;
	Interval = 0;
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
		Timer.Stop();
		OldStatus = false;
	}
}

bool ButtonClass::isPressed()
{
	if ( !digitalRead(ControlPin) ) //プルアップ回路なのでdigitalReadがfalseの時に押されている。
	{
		if ( CanRepeat )
		{
			if ( OldStatus )
			{
				if ( Timer.Tick() )
				{
					return true;
				}
				else
				{
					return false;
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
			if ( OldStatus )
			{
				return false;
			}
			else
			{
				OldStatus = true;
				return true;
			}
		}
	}
	else
	{
		OldStatus = false;
		return false;

	}
}