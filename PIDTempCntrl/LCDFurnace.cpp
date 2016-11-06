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
const unsigned long ControlInterval = 1000; //Furnaceの制御周期。SolidStateRelayThreadでも使用している。


//制御系
const double TemperatuerControllerThreadForPrepreg::AllowableTemperatureError = 1.0; //摂氏。制御の際に許す誤差。
const unsigned long TemperatuerControllerThreadForPrepreg::Interval = ControlInterval; //制御周期

//温度設定
const double TemperatuerControllerThreadForPrepreg::IncreaseTemperaturePerMillis = 2.0 / 60.0 / 1000.0;  //1msあたりの温度上昇量 一分当たり2℃上昇
const double TemperatuerControllerThreadForPrepreg::IncreaseTemperaturePerInterval = IncreaseTemperaturePerMillis*((double)Interval);
const double TemperatuerControllerThreadForPrepreg::MillisPerIncreaseTemperature = 1.0 / IncreaseTemperaturePerMillis;//終了時間の計算のために1周期あたりの計算
const double TemperatuerControllerThreadForPrepreg::FirstKeepTemperature = 80.0 + AllowableTemperatureError; //第一保持温度(℃)
const unsigned long TemperatuerControllerThreadForPrepreg::FirstKeepTime = 30 * 60 * 1000; //30分維持
const double TemperatuerControllerThreadForPrepreg::SecondKeepTemperature = 130.0 + AllowableTemperatureError; //第二保持温度(℃)
const unsigned long TemperatuerControllerThreadForPrepreg::SecondKeepTime = 120 * 60 * 100; //120分維持

const unsigned long FurnaceDisplay::DisplayChangeTime = 3000; //画面の自動遷移の間隔
const unsigned long FurnaceDisplay::Interval = ControlInterval;

const unsigned long SolidStateRelayThread::ChangeTime = 20;//リレーのスイッチ時間。FOTEK SSR-40DAのデータシートだと<10msになってる。これ以下の制御は無効になる。
const unsigned long SolidStateRelayThread::Interval = ControlInterval;
const double SolidStateRelayThread::OutputLimit = ((double)Interval)*0.5;
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

FurnaceDisplay::FurnaceDisplay(uint8_t ControlPin, IThermometer &thermometer, LiquidCrystal &lcd) :SolidStateRelay(ControlPin), FurnaceThread(SolidStateRelay, thermometer, Kp, Ki, Kd, Interval), DisplayChangeTimer(), LCD(lcd)
{
	NowDisplayMode = DisplayMode_Setup;
}

void FurnaceDisplay::Setup()
{
	LCD.begin(16, 2); //これここ？
	DisplaySetupMode();
	DisplayChangeTimer.SetInterval(DisplayChangeTime);
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
	Serial.print(TemperatureControllerForPrepreg.ShowRemainTime());
	Serial.println();
}

void FurnaceDisplay::SetTemperatureController(TemperatuerControllerThreadForPrepreg & temperatureController)
{
	TemperatureControllerForPrepreg = temperatureController;
	FurnaceThread::SetTemperatureController(temperatureController);
}

void FurnaceDisplay::Start()
{
	FurnaceThread::Start();
	NowDisplayMode = DisplayMode_Temperature;
	DisplayChangeTimer.Start();
}

bool FurnaceDisplay::Tick()
{
	if ( FurnaceThread::Tick() )
	{
		switch ( TemperatureControllerForPrepreg.ShowStatus() )
		{
			case FurnaceControllerStatus_Finish:
				NowDisplayMode = DisplayMode_END;
				DisplayChangeTimer.Stop();
				break;
			case FurnaceControllerStatus_FatalError:
				NowDisplayMode = DisplayMode_Error;
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
		case DisplayMode_Temperature:
			NowDisplayMode = DisplayMode_ControlInfo;
			break;
		case DisplayMode_Time:
			NowDisplayMode = DisplayMode_Temperature;
			break;
		case DisplayMode_ControlInfo:
			NowDisplayMode = DisplayMode_Time;
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
		case DisplayMode_ControlInfo:
			NowDisplayMode = DisplayMode_Temperature;
			break;
		case DisplayMode_Time:
			NowDisplayMode = DisplayMode_ControlInfo;
			break;
		case DisplayMode_Temperature:
			NowDisplayMode = DisplayMode_Time;
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
		case DisplayMode_Setup:
			DisplaySetupMode();
			break;
		case DisplayMode_Temperature:
			DisplayTemperature();
			break;
		case DisplayMode_Time:
			DisplayTime();
			break;
		case DisplayMode_ControlInfo:
			DisplayControlInfo();
			break;
		case DisplayMode_END:
			DisplayENDMode();
			break;
		case DisplayMode_Error:
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
	LCD.print("HELLO-1234567890");
	LCD.setCursor(0, 1);
	LCD.print("------STOP------");
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
		case FurnaceControllerStatus_ToFirstKeepTemp:
			LCD.print("To 80");
			LCD.write(0xDF);
			LCD.print("C");
			break;
		case FurnaceControllerStatus_KeepFirstKeepTemp:
			LCD.print("Keep 80");
			break;
		case FurnaceControllerStatus_ToSecondKeepTemp:
			LCD.print("To130");
			LCD.write(0xDF);
			LCD.print("C");
			break;
		case FurnaceControllerStatus_KeepSecondkeepTemp:
			LCD.print("Keep130");
			break;
		case FurnaceControllerStatus_Finish:
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
	LCD.print(TemperatureControllerForPrepreg.CheckError());
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

TemperatuerControllerThreadForPrepreg::TemperatuerControllerThreadForPrepreg() :MainTimer(), KeepTimer()
{
	MainTimer.SetInterval(Interval);
	this->Stop();
}

bool TemperatuerControllerThreadForPrepreg::Tick()
{
	if ( MainTimer.Tick() )
	{
		ErrorStatus = FurnaceControllerError_None;
		switch ( WorkStatus )
		{
			case FurnaceControllerStatus_Stop:
				break;

			case FurnaceControllerStatus_Startup:
				if ( KeepTimer.Tick() )
				{
					KeepTimer.Stop();
					if ( MinimumTemperature >= SecondKeepTemperature + AllowableTemperatureError )
					{
						this->Stop();
						WorkStatus = FurnaceControllerStatus_FatalError;
						ErrorStatus = FurnaceControllerError_StartupFailure;
					}
					else
					{
						GoalTemperature = MinimumTemperature; //現在温度を目標値に設定する．
						WorkStatus = FurnaceControllerStatus_ToFirstKeepTemp;
						Order = FurnaceOrder_Work;
					}
				}
				break;

			case FurnaceControllerStatus_ToFirstKeepTemp:
				if ( FirstKeepTemperature > MinimumTemperature + AllowableTemperatureError )
				{
					RisingTemperature();
				}
				else
				{ //保持温度付近に到達したら制御値を保持温度にして維持する
					KeepTimer.SetInterval(FirstKeepTime);
					KeepTimer.Start();
					GoalTemperature = FirstKeepTemperature;
					CalcFinishTime();
					WorkStatus = FurnaceControllerStatus_KeepFirstKeepTemp;
				}
				break;

			case FurnaceControllerStatus_KeepFirstKeepTemp:
				if ( KeepTimer.Tick() )
				{
					KeepTimer.Stop();
					WorkStatus = FurnaceControllerStatus_ToSecondKeepTemp;
				}
				break;

			case FurnaceControllerStatus_ToSecondKeepTemp:
				if ( SecondKeepTemperature > MinimumTemperature + AllowableTemperatureError )
				{
					RisingTemperature();
				}
				else
				{ //保持温度付近に到達したら制御値を保持温度にして維持する
					KeepTimer.SetInterval(SecondKeepTime);
					KeepTimer.Start();
					GoalTemperature = SecondKeepTemperature;
					CalcFinishTime();
					WorkStatus = FurnaceControllerStatus_KeepSecondkeepTemp;
				}
				break;

			case FurnaceControllerStatus_KeepSecondkeepTemp:
				if ( KeepTimer.Tick() )
				{ //焼き終わり。リレーをオフにして冷やしていく。
					KeepTimer.Stop();
					GoalTemperature = 0.0;
					Order = FurnaceOrder_StopRelay;
					WorkStatus = FurnaceControllerStatus_Finish;
				}
				break;

			case FurnaceControllerStatus_Finish:
				//END
				break;

			case FurnaceControllerStatus_FatalError:
				//どうしたものか
				this->Stop();
				break;
		}
	}
	return false;
}

void TemperatuerControllerThreadForPrepreg::Start()
{
	MainTimer.Start();
	GoalTemperature = 0.0;
	MinimumTemperature = SecondKeepTemperature + AllowableTemperatureError; //入力を受け入れるために大きめにしておく
	KeepTimer.SetInterval(10000); //十秒間温度を取得してもらう．
	KeepTimer.Start();
	StartTime = millis();
	WorkStatus = FurnaceControllerStatus_Startup;
	Order = FurnaceOrder_StopRelay;
}

void TemperatuerControllerThreadForPrepreg::Stop()
{
	MainTimer.Stop();
	KeepTimer.Stop();
	GoalTemperature = 0.0;
	MinimumTemperature = 0.0;
	WorkStatus = FurnaceControllerStatus_Stop;
	Order = FurnaceOrder_StopAll;
}

bool TemperatuerControllerThreadForPrepreg::isRunning()
{
	return MainTimer.isRunning();
}

double TemperatuerControllerThreadForPrepreg::ShowGoalTemperature()
{
	return GoalTemperature;
}

double TemperatuerControllerThreadForPrepreg::ShowMinimumTemperature()
{
	return MinimumTemperature;
}

FurnaceOrder TemperatuerControllerThreadForPrepreg::ShowOrderForFurnaceThread()
{
	return Order;
}

FurnaceControllerStatus TemperatuerControllerThreadForPrepreg::ShowStatus()
{
	return WorkStatus;
}

FurnaceControllerError TemperatuerControllerThreadForPrepreg::CheckError()
{
	return ErrorStatus;
}

unsigned long TemperatuerControllerThreadForPrepreg::ShowElapseTime()
{
	return millis() - StartTime;
}

unsigned long TemperatuerControllerThreadForPrepreg::ShowRemainTime()
{
	return FinishTime - millis();
}

void TemperatuerControllerThreadForPrepreg::InputTemperature(double Temperature)
{
	if ( Temperature < MinimumTemperature )
	{
		MinimumTemperature = Temperature;
	}
}

void TemperatuerControllerThreadForPrepreg::RisingTemperature()
{
	GoalTemperature += IncreaseTemperaturePerInterval;

	if ( GoalTemperature > MinimumTemperature + AllowableTemperatureError )
	{
		ErrorStatus = FurnaceControllerError_NotEnoughTemperatureUpward;
		GoalTemperature = MinimumTemperature + AllowableTemperatureError;
		CalcFinishTime();
	}
	MinimumTemperature = GoalTemperature;
}

void TemperatuerControllerThreadForPrepreg::CalcFinishTime()
{
	unsigned long TempTime = 0;
	FinishTime = millis();

	if ( WorkStatus == FurnaceControllerStatus_ToFirstKeepTemp )
	{
		TempTime += FirstKeepTime;
	}

	if ( WorkStatus <= FurnaceControllerStatus_ToSecondKeepTemp )
	{
		TempTime += SecondKeepTime;
	}

	TempTime += (unsigned long)((SecondKeepTemperature - (double)GoalTemperature) * MillisPerIncreaseTemperature);

	if ( TempTime > 359999000 )
	{
		TempTime = 359999000; //359999000は99:59:59をミリ秒に直したもの
	}
	FinishTime += TempTime;
}
