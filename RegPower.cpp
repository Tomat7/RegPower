/*
* Оригинальная идея (c) Sebra
* Алгоритм регулирования (c) Chatterbox
* 
* Вольный перевод в библиотеку Tomat7
*/

#include "Arduino.h"
#include "RegPower.h"

RegPower TEH;              // preinstatiate

//unsigned short TimerOne::pwmPeriod = 0;
//unsigned char TimerOne::clockSelectBits = 0;
//void (*TimerOne::isrCallback)() = NULL;
//unsigned int RegPower::cntr = 0;
// interrupt service routine that wraps a user defined function supplied by attachInterrupt

volatile bool RegPower::_zero;
volatile int RegPower::_cntr;
volatile unsigned long RegPower::_Isumm;
//volatile uint16_t RegPower::_zcc;

//=== Обработка прерывания по совпадению OCR1A (угла открытия) и счетчика TCNT1 
// (который сбрасывается в "0" по zero_crosss_int) 

ISR(TIMER1_COMPA_vect) {
	RegPower::SetTriac_int();
	//if (TCNT1 < C_TIMER) PORTD |= (1 << TRIAC);
	//PORTD &= ~(1 << TRIAC) - установит "1" на выводе D5 - триак откроется
}

//================= Обработка прерывания АЦП для расчета среднеквадратичного тока
ISR(ADC_vect) {
	RegPower::GetI_int();
}

RegPower::RegPower()
{  
}

void RegPower::init(uint16_t Pmax) //__attribute__((always_inline))
{  
	pinMode(ZCROSS, INPUT);          //детектор нуля
	pinMode(TRIAC, OUTPUT);          //тиристор
	PORTD &= ~(1 << TRIAC);
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
	resist = ( (220*220.01) / Pmax );
	Serial.println(resist);
	_zero = false;
}

void RegPower::control()
{
	//__cntr = _cntr;
	//__Isumm = _Isumm;
	if (_zero && _cntr == 1024)
	{
		_Isumm >>= 10;
		Inow = (Inow * (AVG_FACTOR - 1) + ((_Isumm > 2) ? sqrt(_Isumm) * ACS_RATIO : 0)) / AVG_FACTOR;
		_zero = false;
		_Isumm = _cntr = 0;                 // обнуляем суммы токов и счетчики
	}
	if (Iset)
	{ // Расчет угла открытия триака
		angle += (Inow - Iset)  / BOOST_LAG;
		angle = constrain(angle, ZEROOFFSET, C_TIMER);
	} else angle = C_TIMER;
	OCR1A = int(angle);
	Pnow = (uint16_t)(pow(Inow, 2) * resist);
	//ZCount = _zcc;
	return;
}

int RegPower::getpower()
{	
	//Power = (uint16_t)(pow(Inow, 2) * resist);
	return Pnow;
}
/*
int RegPower::setpower()
{	
	return Pset;
}
*/
void RegPower::setpower(int setPower)
{	
	Pset = setPower;
	Iset = sqrt(Pset / resist);
	return;
}

void RegPower::ZeroCross_int() //__attribute__((always_inline))
{
	TCNT1 = 0;
	PORTD &= ~(1 << TRIAC); // установит "0" на выводе D5 - триак закроется
	_zero = true;
	//_zcc++;
	//Serial.println("*");
}

void RegPower::GetI_int() //__attribute__((always_inline))
{
	unsigned long Iism = 0; //мгновенные значения тока
	byte An_pin = ADCL;
	byte An = ADCH;
	if (_cntr < 1024) {
		Iism = ((An << 8) + An_pin) - 512;
		Iism *= Iism;                    // возводим значение в квадрат
		_Isumm += Iism;                   // складываем квадраты измерений
		_cntr++;
		//Serial.print(cntr);
	}
}

void RegPower::SetTriac_int() //__attribute__((always_inline))
{
	if (TCNT1 < C_TIMER) PORTD |= (1 << TRIAC);
	//PORTD &= ~(1 << TRIAC);
}
