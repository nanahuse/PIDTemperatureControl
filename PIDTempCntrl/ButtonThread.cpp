// 
// 
// 

#include "ButtonThread.h"

ButtonClass::ButtonClass(uint8_t controlPin , ButtonCircuitPattern pattern , bool canRepeat) : Timer( ) {
	ControlPin = controlPin;
	switch (pattern)
	{
	case ButtonCircuitPattern::InputPullup:
		pinMode(ControlPin , INPUT_PULLUP); //Arduino DueにはINPUT_PULLUPがないみたい
		break;
	case ButtonCircuitPattern::Pullup:
		pinMode(ControlPin , INPUT);
		isPulldown = false;
		break;
	case ButtonCircuitPattern::Pulldown:
		pinMode(ControlPin , INPUT);
		isPulldown = true;
		break;

	}
	SetCanRepeat(canRepeat);
	SetRepeatTime(100);
	OldStatus = false;
}

void ButtonClass::SetRepeatTime(unsigned long RepeatTime) {
	if (CanRepeat)
	{
		Timer.SetInterval(RepeatTime);
	}
}

void ButtonClass::SetCanRepeat(bool canRepeat) {
	CanRepeat = canRepeat;
	if (!CanRepeat)
	{
		Timer.SetInterval(100); //チャタリングの防止用
		OldStatus = false;
	}
	Timer.Start( );
}

bool ButtonClass::isPressed( ) {
	if (digitalRead(ControlPin) == isPulldown) //プルダウンの時は押されているとtrue，プルアップ回路は押されているとfalseなのでpulldownかどうかと同値であれば押されているとき．
	{
		if (Timer.Tick( ))
		{
			if (OldStatus)
			{
				if (CanRepeat)
				{
					return true;
				} else
				{
					return false;
					Timer.Stop( );
				}
			} else
			{
				Timer.Start( );
				OldStatus = true;
				return true;
			}
		} else
		{
			return false;
		}
	} else
	{
		if (OldStatus)
		{
			OldStatus = false;
			if (!CanRepeat)
			{
				Timer.Start( );
			}
		}
		return false;

	}
}
