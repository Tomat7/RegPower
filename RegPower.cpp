/*
* Оригинальная идея (c) Sebra
* Алгоритм регулирования (c) Chatterbox
* 
* Вольный перевод в библиотеку мелкие доработки алгоритма - Tomat7
*/

#include "Arduino.h"
#include "RegPower.h"

RegPower TEH;              // preinstatiate

volatile bool RegPower::_getI;
volatile int RegPower::_cntr;
volatile unsigned long RegPower::_Isumm;
volatile float RegPower::_angle;

//=== Обработка прерывания по совпадению OCR1A (угла открытия) и счетчика TCNT1 
// (который сбрасывается в "0" по zero_crosss_int) 

ISR(TIMER1_COMPA_vect) {
	RegPower::SetTriac_int();
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
	// настойка АЦП
	ADMUX = (0 << REFS1) | (1 << REFS0) | (0 << MUX2) | (0 << MUX1) | (1 << MUX0); //
	ADCSRA = B11101111; //Включение АЦП
	ACSR = (1 << ACD);
	//- Timer1 - Таймер задержки времени открытия триака после детектирования нуля (0 триак не откроется)
	TCCR1A = 0x00;  //
	TCCR1B = 0x00;    //
	TCCR1B = (0 << CS12) | (1 << CS11) | (1 << CS10); // Тактирование от CLK.
	OCR1A = 0;                   // Верхняя граница счета. Диапазон от 0 до 65535.
	TIMSK1 |= (1 << OCIE1A);     // Разрешить прерывание по совпадению
	attachInterrupt(1, ZeroCross_int, RISING);//вызов прерывания при детектировании нуля
	resist = ( (220*220.01) / Pmax );
	LagFactor = LAG_FACTOR;
	Serial.print(F(LIBVERSION));
	Serial.println(resist);
	_getI = true;
}

void RegPower::control()
{
	if ( !_getI && (_cntr == 1024))
	{
		ADCperiod = millis() - _ADCmillis;
		_ADCmillis = millis();
		_Isumm >>= 10;
		//Inow = (Inow * (AVG_FACTOR - 1) + ((_Isumm > 2) ? sqrt(_Isumm) * ACS_RATIO : 0)) / AVG_FACTOR;
		Inow = (_Isumm > 2) ? sqrt(_Isumm) * ACS_RATIO : 0;
		_Isumm = 0;                 // обнуляем суммы токов и счетчики
		_getI = true;
		Pnow = (uint16_t)(pow(Inow, 2) * resist);
		
		if (Iset)
		{ // Расчет угла открытия триака
			_angle += (Inow - Iset)  * LagFactor;
			_angle = constrain(_angle, ZEROOFFSET, C_TIMER);
		} else _angle = C_TIMER;
		//OCR1A = int(angle);
	}
	return;
}

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
	if (_getI == true) 
	{	
		_getI = false;
		_cntr = 0;
	}
	OCR1A = int(_angle);
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
	//PORTD |= (1 << TRIAC);  - установит "1" и откроет триак
	//PORTD &= ~(1 << TRIAC); - установит "0" и закроет триак
}

/*
//===========================================================Настройка АЦП

ADMUX = (0 << REFS1) | (1 << REFS0) | (0 << MUX2) | (0 << MUX1) | (1 << MUX0); //

/*     Биты 7:6 – REFS1:REFS0. Биты выбора опорного напряжения. Если мы будем менять эти биты во время преобразования,
		то изменения вступят в силу только после текущего преобразования. В качестве опорного напряжения может быть выбран AVcc
		(ток источника питания), AREF или внутренний 2.56В источник опорного напряжения.
		Рассмотрим какие же значения можно записать в эти биты:
		REFS1:REFS0
		00    AREF
		01    AVcc, с внешним конденсатором на AREF
		10    Резерв
		11    Внутренний 2.56В  источник, с внешним конденсатором на AREF
		Бит 5 – ADLAR. Определяет как результат преобразования запишется в регистры ADCL и ADCH.
		ADLAR = 0
		Биты 3:0 – MUX3:MUX0 – Биты выбора аналогового канала.
		MUX3:0
		0000      ADC0
		0001      ADC1
		0010      ADC2
		0011      ADC3
		0100      ADC4
		0101      ADC5
		0110      ADC6
		0111      ADC7
		

//--------------------------Включение АЦП

ADCSRA = B11101111;
/*    Бит 7 – ADEN. Разрешение АЦП.
	0 – АЦП выключен
	1 – АЦП включен
	Бит 6 – ADSC. Запуск преобразования (в режиме однократного
	преобразования)
	0 – преобразование завершено
	1 – начать преобразование
	Бит 5 – ADFR. Выбор режима работы АЦП
	0 – режим однократного преобразования
	1 – режим непрерывного преобразования
	Бит 4 – ADIF. Флаг прерывания от АЦП. Бит устанавливается, когда преобразование закончено.
	Бит 3 – ADIE. Разрешение прерывания от АЦП
	0 – прерывание запрещено
	1 – прерывание разрешено
	Прерывание от АЦП генерируется (если разрешено) по завершении преобразования.
	Биты 2:1 – ADPS2:ADPS0. Тактовая частота АЦП
	ADPS2:ADPS0
	000         СК/2
	001         СК/2
	010         СК/4
	011         СК/8
	100         СК/16
	101         СК/32
	110         СК/64
	111         СК/128

//------ Timer1 ---------- Таймер задержки времени открытия триака после детектирования нуля (0 триак не откроется)

TCCR1A = 0x00;  //
TCCR1B = 0x00;    //
TCCR1B = (0 << CS12) | (1 << CS11) | (1 << CS10); // Тактирование от CLK.

// Если нужен предделитель :
// TCCR1B |= (1<<CS10);           // CLK/1 = 0,0000000625 * 1 = 0,0000000625, 0,01с / 0,0000000625 = 160000 отсчетов 1 полупериод
// TCCR1B |= (1<<CS11);           // CLK/8 = 0,0000000625 * 8 = 0,0000005, 0,01с / 0,0000005 = 20000 отсчетов 1 полупериод
// TCCR1B |= (1<<CS10)|(1<<CS11); // CLK/64 = 0,0000000625 * 64 = 0,000004, 0,01с / 0,000004 = 2500 отсчетов 1 полупериод
// TCCR1B |= (1<<CS12);           // CLK/256  = 0,0000000625 * 256 = 0,000016, 0,01с / 0,000016 = 625 отсчетов 1 полупериод
// TCCR1B |= (1<<CS10)|(1<<CS12); // CLK/1024
// Верхняя граница счета. Диапазон от 0 до 255.
OCR1A = 0;           // Верхняя граница счета. Диапазон от 0 до 65535.

// Частота прерываний будет = Fclk/(N*(1+OCR1A))
// где N - коэф. предделителя (1, 8, 64, 256 или 1024)

TIMSK1 |= (1 << OCIE1A) | (1 << TOIE1); // Разрешить прерывание по совпадению и переполнению
*/
