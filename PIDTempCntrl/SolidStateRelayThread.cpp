// 
// 
// 

#include "SolidStateRelayThread.h"

//--------------------------------------------------------設定とか--------------------------------------------------------
//時間系はミリ秒だよ

const unsigned long SolidStateRelayThread::ChangeTime = 20;//リレーのスイッチ時間。FOTEK SSR-40DAのデータシートだと<10msになってる。これ以下の制御は無効になる。
const unsigned long SolidStateRelayThread::Interval = ControlInterval;
const double SolidStateRelayThread::OutputLimit = (( double ) Interval)*0.5;
const double SolidStateRelayThread::OutputLimitPerRelayOutputMax = OutputLimit / RelayInputMax;

//--------------------------------------------------------クラスの定義--------------------------------------------------------


SolidStateRelayThread::SolidStateRelayThread(uint8_t controlPin) :MainTimer( ) , SwitchingTimer( ) , SafetyTimer( ) {
	ControlPin = controlPin;
	pinMode(ControlPin , OUTPUT);
	digitalWrite(ControlPin , LOW);

	MainTimer.SetInterval(Interval);
	OnTime = 0;

	SafetyTimer.SetInterval(ChangeTime);

	MainTimer.Stop( );
	SafetyTimer.Stop( );
	SwitchingTimer.Stop( );
}


void SolidStateRelayThread::Start( ) {
	Start(millis( )); //デフォルト引数使うとStart()居ないって怒られるため．
}
void SolidStateRelayThread::Start(unsigned long intervalTimer) {
	MainTimer.Start(intervalTimer);
	SafetyTimer.Stop( );
	SwitchingTimer.Stop( );
}

void SolidStateRelayThread::Stop( ) {
	delay(ChangeTime);
	digitalWrite(ControlPin , LOW);
	delay(ChangeTime);
	MainTimer.Stop( );
	SafetyTimer.Stop( );
	SwitchingTimer.Stop( );
}

bool SolidStateRelayThread::Tick( ) {
	if (SafetyTimer.isRunning( ))		//リレーが壊れないようにスイッチが完了するまではピンの切り替えを行わない。
	{
		if (SafetyTimer.Tick( ))
		{
			SafetyTimer.Stop( );
		} else
		{
			return false;
		}
	}

	if (MainTimer.Tick( ))
	{
		if (OnTime >= ChangeTime) //リレーの開放時間がリレーの切り替えより長いときのみ開放する．
		{
			digitalWrite(ControlPin , HIGH);
			SafetyTimer.Start( ); //リレーが壊れないようにスイッチ完了まで待つよ．SafetyTimer始動
		}
		if (OnTime > Interval - ChangeTime)
		{
			SwitchingTimer.SetInterval(Interval - ChangeTime);
		} else
		{
			SwitchingTimer.SetInterval(OnTime);
		}
		SwitchingTimer.Start( ); //
		return true;
	}

	if (SwitchingTimer.Tick( ))
	{
		digitalWrite(ControlPin , LOW);
		SafetyTimer.Start( ); //リレーが壊れないようにスイッチ完了まで待つよ．SafetyTimer始動

		SwitchingTimer.Stop( );
	}
	return false;
}

bool SolidStateRelayThread::isRunning( ) {
	return MainTimer.isRunning( );
}

void SolidStateRelayThread::SetOutput(double Output) {
	OnTime = ( unsigned long ) (Output*OutputLimitPerRelayOutputMax);
}
