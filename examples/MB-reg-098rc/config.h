#define SKETCHVERSION __FILE__ " " __DATE__ " " __TIME__
#define SKETCHFILE __FILE__
#define SKETCHTIME " " __DATE__ " " __TIME__
#define SHOWVERSION "v" VERSION " " __DATE__
#define I2C_LCDADDRESS 0x3F       // адрес LCD дисплея - необходимо найти с помощью i2c_scanner.ino
#define SERIALSPEED 115200        // скорость в последовательном порту

#define ETHERNET_MAC MAC0, MAC1, MAC2, 0xff, 0x30, ETHERNET_ID  // MAC адрес Ethernet шилда 

#ifndef ETHERNET_DHCP                               // если не используется DHCP
#define ETHERNET_IP 192, 168, 1, 30 + ETHERNET_ID   // задаём IP адрес Ethernet модуля 
#endif
/*
   к младшему байту прибавляется ETHERNET_ID, то есть при ETHERNET_ID==1, IP будет 192.168.1.31
   адрес должен быть из диапазона сети в которой планируется использовать модуль, и не должен совпадать
   с адресом любого другого устройства в сети. посмотреть IP адреса можно командой ipconfig /all (Windows)
   или ifconfig (Linux) с ПК подключенного и корректно работающего в сети - смотреть на строки
   "IPv4 Address" или "inet addr". но лучше всего залить тестовый скетч для соответствующего модуля
   и посмотреть монитор порта - DHCP должен выдать адрес. это еще и подтвердит работоспособность модуля.
   File->Examples->Ethernet-> DhcpAddressPrinter для w5100
   File->Examples->EtherCard-> testDHCP для enc28j60

   https://ru.wikipedia.org/wiki/%D0%A7%D0%B0%D1%81%D1%82%D0%BD%D1%8B%D0%B9_IP-%D0%B0%D0%B4%D1%80%D0%B5%D1%81
   обычно в домашних сетях используются IP адреса вида:
    10.0.0.1 - 10.255.255.254
    172.16.0.1 - 172.31.255.254
    192.168.0.1 - 192.168.255.254
   вот "что-то подобное" и нужно тут прописать :) тема бесконечная, пишите - отвечу.

   если не используется DHCP, то кроме адреса можно (иногда и нужно) прописать адрес шлюза по умолчанию
   (default gateway) и адрес(а) DNS сервера (в данном скетче смысла не имеет). шлюз по умолчанию необходим
   если Modbus master (SCADA) и slave (наше устройство) находятся в разных подсетях и между ними маршрутизатор.
   такая конфигурация может возникнуть если модули дома, а SCADA на работе. если понадобится, пишите - расскажу,
   хотя самый простой совет в этом случае использовать DHCP. тема действительно бесконечная, пишите - отвечу.
*/

#define DS_CONVTIME 750           // как часто опрашивать DS18B20 (миллисекунды)
#define IO_REFRESH DS_CONVTIME/3  // как часто обновлять информацию на дисплее
#define MB_TIMEOUT 50             // как долго можно работать без мастера Модбаса (секунды)
#define msSHOWCONFIG 3000

#include "DStemp.h"
DSThermometer sensor[] = OWPINS;

#include "RegPower.h"
#include "power.h"

// *** задаём нумерацию регистров Modbus
#define hrConvTimeout sensCount
#define hrLAGFACTOR sensCount
#define hrPset sensCount + 1
#define hrPnow sensCount + 2
#define hrSECONDS sensCount + 3

//===================================
#ifndef ETHERNET_DHCP // если компилим _не_ для использования DHCP
#define MAC0 0x00         // на всякий случай, чтобы по старшему байту MAC адреса можно было определить
#else
#define MAC0 0x0A         // с каким параметром скомпилирован скетч - "0A" DHCP или "00" статика
#endif

#ifdef ETHERNET_ENC28J60
#include <EtherCard.h>
#include <ModbusIP_ENC28J60.h>
#define MAC1 0x28               // если второй байт адреса равен 28 - скомпилировано под enc28j60 
#define MAC2 0x60               // на всякий случай, чтобы по второму (после старшего) байту MAC адреса
#endif                          // можно было определить под какой шилд скомпилирован скетч

#ifdef ETHERNET_WIZ5100
#include <Ethernet.h>
#include <ModbusIP.h>
#define MAC1 0x51               // если второй байт адреса равен 51 - скомпилировано под wiz5100
#define MAC2 0x51
#endif

#include <Modbus.h>
ModbusIP mb;
//===============================

const int sensCount = sizeof(sensor) / sizeof(DSThermometer);
unsigned long msReinit, msLcd;
uint16_t msConvTimeout = DS_CONVTIME;
//byte macID;
const byte mac[] = { ETHERNET_MAC };
//const byte ip[] = { ETHERNET_IP };

void setupNetMB()     //Config Modbus IP
{
  //const byte mac[] = { ETHERNET_MAC };
  Serial.print("MAC: ");
  for (byte i = 0; i < 6; ++i)
  {
    Serial.print(mac[i], HEX);
    if (i < 5) Serial.print(':');
  }
  Serial.println();

#ifndef ETHERNET_DHCP
  const byte ip[] = { ETHERNET_IP };
  mb.config(mac, ip);
#else
  mb.config(mac);
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

void lcdNetInfo()
{
  lcd.setCursor(0, 0);
  //lcd.print("MAC: ");
  for (byte i = 0; i < 6; ++i)
  {
    char mh[2];
    String macStr = String(mac[i], HEX);
    //macStr.toUpperCase();
    macStr.toCharArray(mh, 3);
    lcd.print(mh);
    if (i < 5) lcd.print(':');
  }

  lcd.setCursor(0, 1);
  //lcd.print("IP: ");
  for (byte i = 0; i < 4; ++i)
  {
    lcd.print(Ethernet.localIP()[i]);
    if (i < 3) lcd.print(".");
  }
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

