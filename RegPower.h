/*
* Оригинальная идея (c) Sebra
* Алгоритм регулирования (c) Chatterbox
* 
* Вольный перевод в библиотеку Tomat7
*/
#ifndef RegPower_h
#define RegPower_h

#include "Arduino.h"
//#include "config/known_16bit_timers.h"

#define ZEROOFFSET 10         // Сдвиг срабатывания детектора нуля относительно реального нуля в тиках таймера
#define C_TIMER 625-ZEROOFFSET          // Максимальное количество срабатываний таймера за полупериод синусоиды
#define ACS_RATIO 0.048828125 // Коэффициент датчика ACS712 |5А - 0.024414063 | 20А - 0.048828125 | 30A - 0.073242188 |
#define RESIST_ADDR 0                  // адрес в eeprom для хранения сопротивления нагрузки
#define ZCROSS 3
#define TRIAC PORTD5	// пока не проверялось! возможно дальше порт прямо указан в программе!
#define BOOST_LAG 300  // work with this to get small jitter 
#define AVG_FACTOR 5   // also work with this to get small jitter

class RegPower
{
public:
	RegPower();
	float angle = C_TIMER;
	float resist = 0;
	float Inow = 0;   		// переменная расчета RMS тока
	float Iset = 0;   		// установленный ток
	uint16_t Pnow;
	uint16_t Pset;
	int boost_lag = BOOST_LAG;            // Коэффициент интеграции в расчете угла
	uint16_t ZCount;
	unsigned long __Isumm;
	unsigned int __cntr;

	void init(uint16_t Pmax);
	// ============= Расчет угла открытия триака
	void control();
	int getpower();
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
	volatile static uint16_t _zcc;

};
extern RegPower TEH;

#endif
