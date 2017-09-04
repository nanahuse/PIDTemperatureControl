// 
// 
// 

#include "FurnaceControllerHostThreadForPrepreg.h"

//--------------------------------------------------------設定とか--------------------------------------------------------
//時間系はミリ秒だよ

//制御系
const double FurnaceControllerHostThreadForPrepreg::AllowableTemperatureError = 1.0; //摂氏。制御の際に許す誤差。
const double FurnaceControllerHostThreadForPrepreg::ErrorTemperature = 140; //これ超えたらエラーはくよ
const unsigned long FurnaceControllerHostThreadForPrepreg::Interval = ControlInterval; //制御周期

																					   //温度設定
const double FurnaceControllerHostThreadForPrepreg::IncreaseTemperaturePerMillis = 2.0 / 60.0 / 1000.0;  //1msあたりの温度上昇量 一分当たり2℃上昇
const double FurnaceControllerHostThreadForPrepreg::IncreaseTemperaturePerInterval = IncreaseTemperaturePerMillis*(( double ) Interval);
const double FurnaceControllerHostThreadForPrepreg::MillisPerIncreaseTemperature = 1.0 / IncreaseTemperaturePerMillis;//終了時間の計算のために1周期あたりの計算
const double FurnaceControllerHostThreadForPrepreg::FirstKeepTemperature = 80.0 + AllowableTemperatureError; //第一保持温度(℃)
const unsigned long FurnaceControllerHostThreadForPrepreg::FirstKeepTime = 30 * 60 * 1000; //30分維持
const double FurnaceControllerHostThreadForPrepreg::SecondKeepTemperature = 130.0 + AllowableTemperatureError; //第二保持温度(℃)
const unsigned long FurnaceControllerHostThreadForPrepreg::SecondKeepTime = 120 * 60 * 100; //120分維持

//--------------------------------------------------------クラスの定義--------------------------------------------------------

FurnaceControllerHostThreadForPrepreg::FurnaceControllerHostThreadForPrepreg( ) :MainTimer( ) , KeepTimer( ) {
	MainTimer.SetInterval(Interval);
	this->Stop( );
}

bool FurnaceControllerHostThreadForPrepreg::Tick( ) {
	if (MainTimer.Tick( ))
	{
		ErrorStatus = FurnaceControllerError::None;
		switch (WorkStatus)
		{
		case FurnaceControllerStatus::Stop:
			break;

		case FurnaceControllerStatus::Startup:
			if (KeepTimer.Tick( ))
			{
				KeepTimer.Stop( );
				if (MinimumTemperature >= ErrorTemperature) //始めの値から更新されてないときはエラーを吐く
				{
					this->Stop( );
					WorkStatus = FurnaceControllerStatus::FatalError;
					ErrorStatus = FurnaceControllerError::StartupFailure;
				} else
				{
					GoalTemperature = MinimumTemperature; //現在温度を目標値に設定する．
					WorkStatus = FurnaceControllerStatus::ToFirstKeepTemp;
					Order = FurnaceOrder::Work;
				}
			}
			break;

		case FurnaceControllerStatus::ToFirstKeepTemp:
			if (FirstKeepTemperature > MinimumTemperature + AllowableTemperatureError)
			{
				RisingTemperature( );
			} else
			{ //保持温度付近に到達したら制御値を保持温度にして維持する
				KeepTimer.SetInterval(FirstKeepTime);
				KeepTimer.Start( );
				GoalTemperature = FirstKeepTemperature;
				CalcFinishTime( );
				WorkStatus = FurnaceControllerStatus::KeepFirstKeepTemp;
			}
			break;

		case FurnaceControllerStatus::KeepFirstKeepTemp:
			if (KeepTimer.Tick( ))
			{
				KeepTimer.Stop( );
				WorkStatus = FurnaceControllerStatus::ToSecondKeepTemp;
			}
			break;

		case FurnaceControllerStatus::ToSecondKeepTemp:
			if (SecondKeepTemperature > MinimumTemperature + AllowableTemperatureError)
			{
				RisingTemperature( );
			} else
			{ //保持温度付近に到達したら制御値を保持温度にして維持する
				KeepTimer.SetInterval(SecondKeepTime);
				KeepTimer.Start( );
				GoalTemperature = SecondKeepTemperature;
				CalcFinishTime( );
				WorkStatus = FurnaceControllerStatus::KeepSecondkeepTemp;
			}
			break;

		case FurnaceControllerStatus::KeepSecondkeepTemp:
			if (KeepTimer.Tick( ))
			{ //焼き終わり。リレーをオフにして冷やしていく。
				KeepTimer.Stop( );
				GoalTemperature = 0.0;
				Order = FurnaceOrder::StopRelay;
				WorkStatus = FurnaceControllerStatus::Finish;
			}
			break;

		case FurnaceControllerStatus::Finish:
			//END
			break;

		case FurnaceControllerStatus::FatalError:
			//どうしたものか
			this->Stop( );
			break;
		}
		return true;
	}
	return false;
}

void FurnaceControllerHostThreadForPrepreg::Start( ) {
	MainTimer.Start( );
	GoalTemperature = 0.0;
	MinimumTemperature = ErrorTemperature + AllowableTemperatureError; //入力を受け入れるために大きめにしておく
	KeepTimer.SetInterval(10000); //十秒間温度を取得してもらう．
	KeepTimer.Start( );
	StartTime = millis( );
	WorkStatus = FurnaceControllerStatus::Startup;
	Order = FurnaceOrder::StopRelay;
}

void FurnaceControllerHostThreadForPrepreg::Stop( ) {
	MainTimer.Stop( );
	KeepTimer.Stop( );
	GoalTemperature = 0.0;
	MinimumTemperature = 0.0;
	WorkStatus = FurnaceControllerStatus::Stop;
	Order = FurnaceOrder::StopAll;
}

bool FurnaceControllerHostThreadForPrepreg::isRunning( ) {
	return MainTimer.isRunning( );
}

double FurnaceControllerHostThreadForPrepreg::ShowGoalTemperature( ) {
	return GoalTemperature;
}

double FurnaceControllerHostThreadForPrepreg::ShowMinimumTemperature( ) {
	return MinimumTemperature;
}

FurnaceOrder FurnaceControllerHostThreadForPrepreg::ShowOrderForFurnaceThread( ) {
	return Order;
}

FurnaceControllerStatus FurnaceControllerHostThreadForPrepreg::ShowStatus( ) {
	return WorkStatus;
}

FurnaceControllerError FurnaceControllerHostThreadForPrepreg::CheckError( ) {
	return ErrorStatus;
}

unsigned long FurnaceControllerHostThreadForPrepreg::ShowElapseTime( ) {
	return millis( ) - StartTime;
}

unsigned long FurnaceControllerHostThreadForPrepreg::ShowRemainTime( ) {
	return FinishTime - millis( );
}

void FurnaceControllerHostThreadForPrepreg::InputTemperature(double Temperature) {
	if (Temperature < MinimumTemperature)
	{
		MinimumTemperature = Temperature;
	}
	if (Temperature > ErrorTemperature)
	{
		ErrorStatus = FurnaceControllerError::TooHot;
	}
}

void FurnaceControllerHostThreadForPrepreg::RisingTemperature( ) {
	GoalTemperature += IncreaseTemperaturePerInterval;

	if (GoalTemperature > MinimumTemperature + AllowableTemperatureError)
	{
		ErrorStatus = FurnaceControllerError::NotEnoughTemperatureUpward;
		GoalTemperature = MinimumTemperature + AllowableTemperatureError;
		CalcFinishTime( );
	}
	MinimumTemperature = GoalTemperature;
}

void FurnaceControllerHostThreadForPrepreg::CalcFinishTime( ) {
	unsigned long TempTime = 0;
	FinishTime = millis( );

	if (WorkStatus == FurnaceControllerStatus::ToFirstKeepTemp)
	{
		TempTime += FirstKeepTime;
	}

	if (WorkStatus <= FurnaceControllerStatus::ToSecondKeepTemp)
	{
		TempTime += SecondKeepTime;
	}

	TempTime += ( unsigned long ) ((SecondKeepTemperature - ( double ) GoalTemperature) * MillisPerIncreaseTemperature);

	if (TempTime > 359999000)
	{
		TempTime = 359999000; //359999000は99:59:59をミリ秒に直したもの
	}
	FinishTime += TempTime;
}