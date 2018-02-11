#define SKETCHVERSION __FILE__ " " __DATE__ " " __TIME__
#define SKETCHFILE __FILE__
#define SKETCHTIME " " __DATE__ " " __TIME__
#define SHOWVERSION "v" VERSION " " __DATE__
#define ZCROSS  3
#define TRIAC   5

#include "DStemp.h"
DSThermometer sensor[] = OWPINS;

#define MAXPOWER 3000
#define SERIALSPEED 115200
#define DS_CONVTIME 750
#define IO_REFRESH DS_CONVTIME/3
#define MB_TIMEOUT 50 // сколько можно работать без мастера Модбаса

#include "RegPower.h"
//#include <avr/pgmspace.h>
#include "power.h"

//===================================
#define hrConvTimeout sensCount
#define hrPset sensCount + 1
#define hrPnow sensCount + 2
#define hrSECONDS sensCount + 3

//===================================
#ifdef ETHERNET_ENC28J60
#include <EtherCard.h>
#include <ModbusIP_ENC28J60.h>
#endif

#ifdef ETHERNET_WIZ5100
#include <Ethernet.h>
#include <ModbusIP.h>
#endif

#include <Modbus.h>
ModbusIP mb;
//===============================

//#include "HDvalve.h"
//#include <OneWire.h>
//#include <ASOLED.h>

#ifndef ETHERNET_MACID
#include <EEPROM.h>
#endif

//OneWire ow[] = { DS2, DS6 };
//DSThermometer sensor[] = { &ow[0], &ow[1] };
const int sensCount = sizeof(sensor) / sizeof(DSThermometer);

unsigned long msReinit, msLcd;
//uint16_t msGet, msLcd;
uint16_t msConvTimeout = DS_CONVTIME;
byte macID;

void setupNetMB()
{
  //Config Modbus IP
#ifdef ETHERNET_MACID
  macID = ETHERNET_MACID;
#else
  macID = EEPROM.read(EEPROM.length() - 1);
#endif

  const byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0x30, macID };
  Serial.print("MAC: ");
  for (byte i = 0; i < 6; ++i)
  {
    Serial.print(mac[i], HEX);
    if (i < 5) Serial.print(':');
  }
  Serial.println();

#ifdef ETHERNET_DHCP
  mb.config(mac);
#else
  const byte ip[] = { 192, 168, 1, 30 + macID };
  mb.config(mac, ip);
#endif

#ifdef ETHERNET_ENC28J60
  ether.printIp("NC28J60 IP: ", ether.myip);
  ether.printIp("MASK: ", ether.netmask);
  ether.printIp("GW: ", ether.gwip);
  //ether.printIp("DNS: ", ether.dnsip);
#endif

#ifdef ETHERNET_WIZ5100
  Serial.print(F("WIZ5100 IP: "));
  Serial.println(Ethernet.localIP());
#endif
}

bool mbHeartBeat()
{
  // если мастер онлайн - он должен записывать 0 в регистр SECONDS
  // это будет признаком "живости" Мастера Modbus'а для модуля
  // и наоборот: не 0 в SECONDS - признак "живости" модуля для Мастера
  // хотя Мастеру логичнее отслеживать "живость" по GetQuality
  //
  if (mb.Hreg(hrSECONDS) == 0) mb.Hreg(hrSECONDS, msReinit / 1000);
  if (((uint16_t)(msReinit / 1000) - mb.Hreg(hrSECONDS)) < MB_TIMEOUT)
  {
    return true;
  }
  else
  {
    return false; // если мастера нет больше 10 секунд - выключаем
  }
}

void printDS(float t, int i)
{
  //Serial.print(F("DS "));
  Serial.print(i);
  Serial.print(F(": "));
  Serial.print(t);
  Serial.print(F(" | "));
  Serial.println(sensor[i].Parasite);
  //Serial.print(F(" | "));
  //Serial.println(millis());
}

void printFreeRam ()
{
  Serial.print(F("Free RAM: "));
  extern int __heap_start, *__brkval;
  int v, r;
  r = (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
  Serial.println(r);
}

