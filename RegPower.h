/*
* Оригинальная идея (c) Sebra
* Алгоритм регулирования (c) Chatterbox
* 
* Вольный перевод в библиотеку Tomat7
* Version 0.4
*/
#ifndef RegPower_h
#define RegPower_h

#include "Arduino.h"
//#include "config/known_16bit_timers.h"

#define LIBVERSION "RegPower_v20171122 R="
#define ZEROOFFSET 10         // Сдвиг срабатывания детектора нуля относительно реального нуля в тиках таймера
#define C_TIMER 625-ZEROOFFSET          // Максимальное количество срабатываний таймера за полупериод синусоиды
#define ACS_RATIO 0.048828125 // Коэффициент датчика ACS712 |5А - 0.024414063 | 20А - 0.048828125 | 30A - 0.073242188 |
#define ZCROSS 3
#define TRIAC PORTD5	// пока не проверялось! возможно дальше порт прямо указан в программе!

#define LAG_FACTOR 30	// Коэффициент интеграции в расчете угла (defaul 300)
#define AVG_FACTOR 5	// Фактор усреднения измеренного тока (defaul 5)

class RegPower
{
public:
	RegPower();
	//volatile float angle = C_TIMER;
	float resist = 0;
	float Inow = 0;   		// переменная расчета RMS тока
	float Iset = 0;   		// установленный ток
	uint16_t Pnow;
	uint16_t Pset;
	//int boost_lag = BOOST_LAG;            
	//uint16_t ZCount;
	//unsigned long __Isumm;
	//unsigned int __cntr;

	void init(uint16_t Pmax);
	// ============= Расчет угла открытия триака
	void control();
	//int getpower();
	//int setpower();
	void setpower(int setP);
	//================= Прерывания
	static void ZeroCross_int() __attribute__((always_inline));
	static void GetI_int() __attribute__((always_inline));
	static void SetTriac_int() __attribute__((always_inline));
	//static void get_I_int();

protected:
	volatile static bool _zero;
	volatile static int _cntr;
	volatile static unsigned long _Isumm;
	volatile static float _angle = C_TIMER;
	//volatile static uint16_t _zcc;

};
extern RegPower TEH;

#endif