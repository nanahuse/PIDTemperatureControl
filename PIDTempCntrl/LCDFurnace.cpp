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

//時間系
const unsigned long ControlInterval = 1000; //Furnaceの制御周期。RelayThreadでも使用している。


//制御系
const double TemperatuerControllerThreadForPrepreg::AllowableTemperatureError = 1.0; //摂氏。制御の際に許す誤差。

//温度設定
const double TemperatuerControllerThreadForPrepreg::IncreaseTemperaturePerMillis = 2.0/60.0/1000.0;  //1msあたりの温度上昇量 一分当たり2℃上昇
const double TemperatuerControllerThreadForPrepreg::IncreaseTemperaturePerInterval = IncreaseTemperaturePerMillis*ControlInterval;
const double TemperatuerControllerThreadForPrepreg::MillisPerIncreaseTemperature = 1.0 / IncreaseTemperaturePerMillis;//終了時間の計算のために1周期あたりの計算
const double TemperatuerControllerThreadForPrepreg::FirstKeepTemperature = 80.0 + AllowableTemperatureError; //第一保持温度(℃)
const unsigned long TemperatuerControllerThreadForPrepreg::FirstKeepTime = 30*60*1000; //30分維持
const double TemperatuerControllerThreadForPrepreg::SecondKeepTemperature = 130.0 + AllowableTemperatureError; //第二保持温度(℃)
const unsigned long TemperatuerControllerThreadForPrepreg::SecondKeepTime = 120*60*100; //120分維持

const unsigned long FurnaceDisplay::DisplayChangeTime = 3000; //画面の自動遷移の間隔

const unsigned long SolidStateRelayThread::ChangeTime = 20;//リレーのスイッチ時間。FOTEK SSR-40DAのデータシートだと<10msになってる。これ以下の制御は無効になる。
const unsigned long SolidStateRelayThread::Interval = ControlInterval;
const double SolidStateRelayThread::OutputLimit = Interval*0.5;
const double SolidStateRelayThread::OutputLimitPerRelayOutputMax = OutputLimit / RelayOutputMax;



//--------------------------------------------------------クラスの定義--------------------------------------------------------


Thermocouple::Thermocouple(int8_t SCKPin, int8_t MISOPin, int8_t SSPin) : thermocouple(SCKPin, MISOPin, SSPin)
{ }

double Thermocouple::Read()
{
	double ReturnTemperature = thermocouple.readCelsius();

	//温度の測定
	int i = 1;
	while ( isnan(ReturnTemperature) && (i < 5) ) //失敗したときに備えて五回まではミスを認める
	{
		delay(100); //気持ち
		ReturnTemperature = thermocouple.readCelsius();
		i++;
	} 

	return thermocouple.readCelsius();
}

SolidStateRelayThread::SolidStateRelayThread(uint8_t controlPin) :MainTimer(), SwitchingTimer(), SafetyTimer()
{
	ControlPin = controlPin;
	pinMode(ControlPin, OUTPUT);
	digitalWrite(ControlPin, LOW);

	MainTimer.SetInterval(Interval);
	OnTime = 0;

	SafetyTimer.SetInterval(ChangeTime);

	MainTimer.Stop();
	SafetyTimer.Stop();
	SwitchingTimer.Stop();
}


void SolidStateRelayThread::Start()
{
	MainTimer.Start();
	SafetyTimer.Stop();
	SwitchingTimer.Stop();
}

void SolidStateRelayThread::Stop()
{
	delay(ChangeTime);
	digitalWrite(ControlPin, LOW);
	delay(ChangeTime);
	MainTimer.Stop();
	SafetyTimer.Stop();
	SwitchingTimer.Stop();
}

bool SolidStateRelayThread::Tick()
{
	unsigned long TempTimer = millis();

	if ( SafetyTimer.isRunning() )		//リレーが壊れないようにスイッチが完了するまではピンの切り替えを行わない。
	{
		if ( SafetyTimer.Tick() )
		{
			SafetyTimer.Stop();
		}
		else
		{
			return false;
		}
	}

	if ( MainTimer.Tick() )
	{
		if ( OnTime >= ChangeTime ) //リレーの開放時間がリレーの切り替えより長いときのみ開放する．
		{
			digitalWrite(ControlPin, HIGH);
			SafetyTimer.SetIntervalTimer(TempTimer); //リレーが壊れないようにスイッチ完了まで待つよ．SafetyTimer始動
		}
		if ( OnTime > Interval - ChangeTime )
		{
			SwitchingTimer.SetInterval(Interval - ChangeTime);
		}
		else
		{
			SwitchingTimer.SetInterval(OnTime);
		}
		SwitchingTimer.SetIntervalTimer(TempTimer); //
		return true;
	}

	if ( SwitchingTimer.Tick() )
	{
		digitalWrite(ControlPin, LOW);
		SafetyTimer.SetIntervalTimer(TempTimer); //リレーが壊れないようにスイッチ完了まで待つよ．SafetyTimer始動

		SwitchingTimer.Stop();
	}
	return false;
}

bool SolidStateRelayThread::isRunning()
{
	return MainTimer.isRunning();
}

void SolidStateRelayThread::SetIntervalTimer(unsigned long Time)
{
	MainTimer.SetIntervalTimer(Time);
}

void SolidStateRelayThread::SetOutput(double Output)
{
	OnTime = (unsigned long)(Output*OutputLimitPerRelayOutputMax);
}

FurnaceDisplay::FurnaceDisplay(uint8_t RelayControlPin, IThermometer &thermometer, LiquidCrystal &lcd) : FurnaceThread(RelayControlPin, thermometer), DisplayChangeTimer(), LCD(lcd)
{
	NowDisplayMode = SetupDisplayMode;
}

void FurnaceDisplay::Setup()
{
	LCD.begin(16, 2); //これここ？
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
	FurnaceThread::Start();
	DisplayChangeTimer.Start();
	NowDisplayMode = ControlInfoDispalyMode;
}

bool FurnaceDisplay::Tick()
{
	if ( FurnaceThread::Tick() )
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
	FurnaceThread::Stop();
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
	PrintTemperature(NowTemperature);
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
		case FurnaceStatus_Stop:
			break;
		case FurnaceStatus_ToFirstKeepTemp:
			if ( FirstKeepTemperature > MinimumTemperature + AllowableTemperatureError )
			{
				RisingTemperature();
			}
			else
			{ //保持温度付近に到達したら制御値を保持温度にして維持する
				CalcFinishTime();
				KeepTimer = millis();
				GoalTemperature = FirstKeepTemperature;
				WorkStatus = FurnaceStatus_KeepFirstKeepTemp;
			}
			break;

		case FurnaceStatus_KeepFirstKeepTemp:
			if ( millis() - KeepTimer >= FirstKeepTime )
			{
				WorkStatus = FurnaceStatus_ToSecondKeepTemp;
			}
			break;

		case FurnaceStatus_ToSecondKeepTemp:
			if ( SecondKeepTemperature > MinimumTemperature + AllowableTemperatureError )
			{
				RisingTemperature();
			}
			else
			{ //保持温度付近に到達したら制御値を保持温度にして維持する
				CalcFinishTime();
				KeepTimer = millis();
				GoalTemperature = SecondKeepTemperature;
				WorkStatus = FurnaceStatus_KeepSecondkeepTemp;
			}
			break;

		case FurnaceStatus_KeepSecondkeepTemp:
			if ( millis() - KeepTimer >= SecondKeepTemperature )
			{ //焼き終わり。リレーをオフにして冷やしていく。
				GoalTemperature = 0.0;
				Order = FurnaceOrder_StopRelay;
				WorkStatus = FurnaceStatus_Finish;
			}
			break;

		case FurnaceStatus_Finish:
			//END
			break;

		default:
			//ERROR ここに来てしまったらどうしようもない。
			Stop();
			ErrorStatus = ErrorUnknown;
			WorkStatus = FurnaceStatus_FatalError;
			break;
	}
}

void TemperatuerControllerThreadForPrepreg::RisingTemperature()
{ 
	GoalTemperature += IncreaseTemperaturePerInterval;

	if ( GoalTemperature > MinimumTemperature + AllowableTemperatureError )
	{
		ErrorStatus = FurnaceError_NotEnoughTemperatureUpward;
		GoalTemperature = MinimumTemperature + AllowableTemperatureError;
		CalcFinishTime();
	}
	MinimumTemperature = GoalTemperature;
}

void TemperatuerControllerThreadForPrepreg::CalcFinishTime()
{
		unsigned long TempTime = 0;
		FinishTimer = millis();

		if ( WorkStatus == FurnaceStatus_ToFirstKeepTemp )
		{
			TempTime += FirstKeepTime;
		}

		if ( WorkStatus <= FurnaceStatus_ToSecondKeepTemp )
		{
			TempTime += SecondKeepTime;
		}

		TempTime += (unsigned long)((SecondKeepTemperature - (double)MinimumTemperature) * MillisPerIncreaseTemperature);

		if ( TempTime > 359999000 )
		{
			TempTime = 359999000; //359999000は99:59:59をミリ秒に直したもの
		}
		FinishTimer += TempTime;
}
