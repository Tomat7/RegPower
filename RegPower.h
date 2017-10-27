
#ifndef RegPower_h
#define RegPower_h

#include "Arduino.h"
//#include "config/known_16bit_timers.h"

#define ZEROOFFSET 10         // Сдвиг срабатывания детектора нуля относительно реального нуля в тиках таймера
#define C_TIMER 625-ZEROOFFSET          // Максимальное количество срабатываний таймера за полупериод синусоиды
#define ACS_COEFF 0.048828125 // Коэффициент датчика ACS712 |5А - 0.024414063 | 20А - 0.048828125 | 30A - 0.073242188 |
#define RESIST_ADDR 0                  // адрес в eeprom для хранения сопротивления нагрузки
#define AVERAGE_FACTOR 5
#define ZCROSS 3
#define TRIACPORT PORTD5	// это пока не работает! дальше прошито в программе!!

class RegPower
{
public:
	RegPower();
	float angle = C_TIMER;
	float real_I = 0;   		// переменная расчета RMS тока
	float inst_I = 0;   		// установленный ток
	float resist = 0;
	uint16_t inst_P = 0;          // установливаемая мощность
	int lag_integr = 300;            // Коэффициент интеграции в расчете угла
	uint16_t ZCount;
	uint16_t Power;
	unsigned long __Isumm;
	unsigned int __cntr;

	void init(uint16_t max_P);
	// ============= Расчет угла открытия триака
	void control();
	int getpower();
	int setpower();
	void setpower(int setP);
	//================= Прерывание по нулю
	static void ZeroCross_int() __attribute__((always_inline));
	static void GetI_int() __attribute__((always_inline));
	static void SetTriac_int() __attribute__((always_inline));
	//static void get_I_int();

protected:
	volatile static bool zero;
	volatile static int cntr;
	volatile static unsigned long Isumm;
	volatile static uint16_t ZCrossCount;

};
extern RegPower TEH;

#endif