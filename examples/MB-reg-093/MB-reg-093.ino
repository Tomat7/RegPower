// !! DEBUG version !!
//#define ETHERNET_ENC28J60
#define ETHERNET_WIZ5100
#define ETHERNET_MACID 0x01
//#define ETHERNET_DHCP

#define OWPINS { 6 }

#include "config.h"
// обязательно заглянуть и поправить при необходимости!

void setup()
//int main( void )
{
  Serial.begin(SERIALSPEED);    //активируем прием сообщения
  key.add_key(pSW);
  key.add_enccoder(pCLK, pDT);
  lcd.init();
  lcd.backlight();
  lcd.clear();

  TEH.init(MAXPOWER);

  delay(3000);
  Serial.println(F(SKETCHVERSION));
  printFreeRam();
  setupNetMB();

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
  //mb.addHreg(hrConvTimeout, DS_CONVTIME); // default timeout
  mb.addHreg(hrConvTimeout, 10);
  mb.addHreg(hrSECONDS, msReinit / 1000); // Like "alive" flag counter for future
  //dsReInit = true;
  printFreeRam();
  delay(1000);
  inst_P = 987;
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
    TEH.LagFactor = mb.Hreg(hrConvTimeout);
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

