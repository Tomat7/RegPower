/*
* 
*/

#include "Arduino.h"
#include "RegPower.h"

RegPower TEH;              // preinstatiate

//unsigned short TimerOne::pwmPeriod = 0;
//unsigned char TimerOne::clockSelectBits = 0;
//void (*TimerOne::isrCallback)() = NULL;
//unsigned int RegPower::cntr = 0;
// interrupt service routine that wraps a user defined function supplied by attachInterrupt

volatile bool RegPower::zero;
volatile int RegPower::cntr;
volatile unsigned long RegPower::Isumm;
volatile uint16_t RegPower::ZCrossCount;

//=== Обработка прерывания по совпадению OCR1A (угла открытия) и счетчика TCNT1 
// (который сбрасывается в "0" по zero_crosss_int) 

ISR(TIMER1_COMPA_vect) {
	RegPower::SetTriac_int();
	//if (TCNT1 < C_TIMER) PORTD |= (1 << PORTD5);
	//PORTD &= ~(1 << PORTD5);	
	//Serial.print("+");
	// установит "1" на выводе D5 - триак откроется
}

//================= Обработка прерывания АЦП для расчета среднеквадратичного тока
ISR(ADC_vect) {
	RegPower::GetI_int();
}

RegPower::RegPower()
{  
}

void RegPower::init(uint16_t maxP) //__attribute__((always_inline))
{  
	pinMode(ZCROSS, INPUT);          //детектор нуля
	pinMode(PORTD5, OUTPUT);          //тиристор
	PORTD &= ~(1 << PORTD5);
	ADMUX = (0 << REFS1) | (1 << REFS0) | (0 << MUX2) | (0 << MUX1) | (1 << MUX0); //
	ADCSRA = B11101111; //Включение АЦП
	ACSR = (1 << ACD);
	//- Timer1 - Таймер задержки времени открытия триака после детектирования нуля (0 триак не откроется)
	TCCR1A = 0x00;  //
	TCCR1B = 0x00;    //
	TCCR1B = (1 << CS12) | (0 << CS11) | (0 << CS10); // Тактирование от CLK.
	OCR1A = 0;                   // Верхняя граница счета. Диапазон от 0 до 65535.
	TIMSK1 |= (1 << OCIE1A);     // Разрешить прерывание по совпадению
	attachInterrupt(1, ZeroCross_int, RISING);//вызов прерывания при детектировании нуля
	resist = ( (220*220.01) / maxP );
	Serial.println(resist);
	zero = false;
}

void RegPower::control()
{
	__cntr = cntr;
	__Isumm = Isumm;
	if (zero && cntr == 1024)
	{
		Isumm >>= 10;
		real_I = (real_I * (AVERAGE_FACTOR - 1) + ((Isumm > 2) ? sqrt(Isumm) * ACS_COEFF : 0)) / AVERAGE_FACTOR;
		zero = false;
		Isumm = cntr = 0;                 // обнуляем суммы токов и счетчики
	}
	if (inst_I)
	{ // Расчет угла открытия триака
		angle += (real_I - inst_I)  / lag_integr;
		angle = constrain(angle, ZEROOFFSET, C_TIMER);
	} else angle = C_TIMER;
	OCR1A = int(angle);
	Power = (uint16_t)(pow(real_I, 2) * resist);
	ZCount = ZCrossCount;
	return;
}

int RegPower::getpower()
{	
	//Power = (uint16_t)(pow(real_I, 2) * resist);
	return Power;
}

int RegPower::setpower()
{	
	return inst_P;
}

void RegPower::setpower(int setP)
{	
	inst_P = setP;
	inst_I = sqrt(inst_P / resist);
	return;
}

void RegPower::ZeroCross_int() //__attribute__((always_inline))
{
	TCNT1 = 0;
	PORTD &= ~(1 << PORTD5); // установит "0" на выводе D5 - триак закроется
	zero = true;
	ZCrossCount++;
	//Serial.println("*");
}

void RegPower::GetI_int() //__attribute__((always_inline))
{
	unsigned long Iism = 0; //мгновенные значения тока
	byte An_pin = ADCL;
	byte An = ADCH;
	if (cntr < 1024) {
		Iism = ((An << 8) + An_pin) - 512;
		Iism *= Iism;                    // возводим значение в квадрат
		Isumm += Iism;                   // складываем квадраты измерений
		cntr++;
		//Serial.print(cntr);
	}
}

void RegPower::SetTriac_int() //__attribute__((always_inline))
{
	if (TCNT1 < C_TIMER) PORTD |= (1 << PORTD5);
	//PORTD &= ~(1 << PORTD5);
}