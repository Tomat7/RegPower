#define VERSION "98b"           // версия скетча (чтобы самому не запутаться :)
#define MAXPOWER 3000           // номинальная мощность ТЭНа ** важно ** от этого считаются другие параметры
#define OWPINS { 6 }            // pin на котором ds18b20 (не забывайте про подтягивающий резистор 4.7 кОм!)

//#define ETHERNET_ENC28J60     // *** с регулятором мощности шилд enc28j60 использовать не рекомендуется ***
#define ETHERNET_WIZ5100        // если используется w5100 шилд, с регулятором только он
#define ETHERNET_ID 0x01        // младший байт MAC адреса ** для каждого устройства в сети MAC должен быть уникальным **
// также смотрите и правьте MACADDRESS, MAC0-2 в config.h
#define ETHERNET_DHCP           // запрашивать IP адрес от DHCP сервера при старте (скетч занимает 99% памяти!)
// если закоментировать то адрес будет назначен принудительно
// *** посмотреть IPADDRESS в config.h ***

#include "config.h"
// *** обязательно заглянуть и поправить при необходимости! ***

void setup()
//int main( void )
{
  Serial.begin(SERIALSPEED);    //активируем прием сообщения
  key.add_key(pSW);
  key.add_enccoder(pCLK, pDT);
  lcd.init();
  lcd.backlight();
  lcd.clear();
  const char* SketchInfo = SHOWVERSION;
  lcd.setCursor(0, 0);
  lcd.print(SketchInfo);
  lcd.setCursor(0, 1);
  lcd.print("CONFIG ");
  lcd.print(MAXPOWER);
  lcd.print(" watt");
  TEH.init(MAXPOWER);

  delay(300);
  Serial.println(F(SKETCHVERSION));
  //printFreeRam();
  setupNetMB();
  delay(msSHOWCONFIG);
  lcd.clear();
  lcdNetInfo();

  for (int i = 0; i < sensCount; i++)
  {
    sensor[i].init(DS_CONVTIME);
    Serial.print(i);
    Serial.println(sensor[i].Connected);
    mb.addHreg(i);
    mb.addHreg(hrSECONDS + i + 1);
  }

  msReinit = millis();
  Serial.println(msReinit);
  mb.addHreg(hrPset);
  mb.addHreg(hrPnow);
  mb.addHreg(hrLAGFACTOR, 10);
  mb.addHreg(hrSECONDS);          // Like "alive" flag counter for future
  printFreeRam();
  delay(msSHOWCONFIG);
  lcd.clear();
}
void loop()
{
  TEH.control();
  mb.task();
  key.readkey();
  if (set_menu) key.readencoder();     // можно не так часто

  if ((millis() - msReinit) > msConvTimeout)
  {
    msReinit = millis();
    for (int i = 0; i < sensCount; i++)
    {
      sensor[i].check();
      float t = sensor[i].Temp;
      mb.Hreg(i, round( t * 100));  // заносим в регистр Modbus (температура * 100)
      mb.Hreg((hrSECONDS + i + 1), sensor[i].TimeConv);  //сохраним время потраченное на преобразование
      printDS(t, i);
    }
    // ***********************************
    //updInfo(sensor[0].Temp, sensor[1].Temp);
    //msConvTimeout = mb.Hreg(hrConvTimeout);
    // ***********************************
    updInfo(sensor[0].Temp, TEH.ADCperiod);
    msConvTimeout = 750;
    TEH.LagFactor = mb.Hreg(hrLAGFACTOR);
    //, TEH.ADCperiod);
    // ***********************************
    //remote_buf = TEH.Pset;
    mb.Hreg(hrPnow, show_P);
    if (ModbusON)
    {
      mbMasterOK = mbHeartBeat();
      if (mbMasterOK)
      {
        inst_P = mb.Hreg(hrPset);
      }
      else {
        inst_P = 0;
        Serial.print("Master OFFline ");
        Serial.println(mb.Hreg(hrSECONDS));
        //lcd.setCursor(9, 0);
        //lcd.cursor();
        //lcd.blink();
      }
    }
    TEH.setpower(inst_P);
    mb.Hreg(hrPset, TEH.Pset);
    updLcd();
    chkSerial();
  }

  if ((millis() - msLcd) > IO_REFRESH)
  {
    chkKeys();
    msLcd = millis();
    if (set_menu) updLcd();
  }
}


