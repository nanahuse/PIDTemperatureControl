// ThermocoupleMAX6675.h

#ifndef _THERMOCOUPLEMAX6675_h
#define _THERMOCOUPLEMAX6675_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include <max6675.h>
#include "ElectricFurnace.h"

//今回使用する熱電対をIThermometerの形にラップ
class ThermocoupleMax6675 : public IThermometer {
public:
	ThermocoupleMax6675(int8_t SCKPin , int8_t SSPin , int8_t MISOPin);
	double Read( );
private:
	MAX6675 thermocouple;
};

#endif

