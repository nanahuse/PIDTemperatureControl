// 
// 
// 

#include "ThermocoupleMAX6675.h"


ThermocoupleMax6675::ThermocoupleMax6675(int8_t SCKPin , int8_t MISOPin , int8_t SSPin) : thermocouple(SCKPin , MISOPin , SSPin) {
}

double ThermocoupleMax6675::Read( ) {
	double ReturnTemperature = thermocouple.readCelsius( );

	//温度の測定
	int i = 1;
	while (isnan(ReturnTemperature) && (i < 5)) //失敗したときに備えて五回まではミスを認める
	{
		delay(100); //連続で取得するとエラーがでるので気持ち待つ
		ReturnTemperature = thermocouple.readCelsius( );
		i++;
	}

	return ReturnTemperature;
}