#include <Wire.h>
#include <LiquidCrystal_I2C.h>      // https://github.com/marcoschwartz/LiquidCrystal_I2C
LiquidCrystal_I2C lcd(0x3F, 16, 2);

#define pSW 7     // энкодер SW кнопка
#define pCLK 8    // Pin для энкодера CLK
#define pDT 9     // Pin для энкодера DT
#include <ReadDigKey.h>
ReadDigKey key;

//uint16_t count_P = 0;
uint16_t inst_P = 0;          // установливаемая мощность
uint16_t show_P;
//int mb_P = 0;
uint16_t local_buf = 0;
uint16_t remote_buf = 0;
uint16_t max_P = MAXPOWER;
char show_Kub[7], show_[7];
char step_P = 50;          // Шаг установки мощности нагрузки,
char degC = 223;
String T1, Var;
volatile bool set_menu = false;
volatile bool ModbusON = true;
volatile bool mbMasterOK = true;

//================== Информация в порт и дисплей
void updInfo(float temp_Kub, float temp_)
{
  dtostrf(temp_, 5, 1, show_);
  dtostrf(temp_Kub, 5, 1, show_Kub);
  //Serial.print("Angle: ");
  //Serial.println(TEH.angle);
  Serial.print(F("Pnow="));
  Serial.println(show_P);
  Serial.print(F("Pset="));
  Serial.println(inst_P);
  Serial.print(F("Pteh="));
  Serial.println(TEH.Pset);
}

void updLcd()
{
  static char buff0[17] = { "                " };
  static char buff1[17] = { "                " };
  show_P = TEH.Pnow;
  //count_P++;
  if (set_menu)
    sprintf(buff1, "Set %4d << %4d", inst_P, local_buf);
  else {
    sprintf(buff0, "Now %4dw %s%c", show_P, show_Kub, degC);
    //sprintf(buff1, "Set %4dw %s%c", buf_inst, show_Kub, degC);
    sprintf(buff1, "Set %4dw %s", TEH.Pset, show_);
  }
  lcd.setCursor(0, 0); lcd.print(buff0);
  lcd.setCursor(0, 1); lcd.print(buff1);
  if (!ModbusON)
  {
    lcd.setCursor(9, 1);
    lcd.cursor();
    lcd.blink();
  } else if (!mbMasterOK)
  {
    lcd.setCursor(9, 0);
    lcd.cursor();
    lcd.blink();
  }
  //lcd.print("test");
}

void chkKeys()
{
  //key.readkey();
  if (key.shot_press()) {           // Короткое нажатие кнопки
    //Serial.println("SHORT_PRESS===");
    set_menu = !set_menu;
    if (set_menu) {
      local_buf = inst_P;
    } else {
      inst_P = local_buf;  //мощность будет меняться только после короткого нажатия
    }
  }
  // Долгое нажатие кнопки
  // ==================== !! здесь переход с МБ на локальное управление !!
  if (key.long_press()) {
    set_menu = false;
    local_buf = 0;
    inst_P = 0;
    ModbusON = !ModbusON;
  }
  //key.readencoder();
  if (key.encoder)                   // вращение
  {
    if (set_menu) {
      local_buf += int(step_P * key.encoder * abs(key.encoder));
      local_buf = (local_buf < 0) ? 0 : (local_buf > max_P) ? max_P : local_buf;
      //inst_P = local_buf;  // мощность будет меняться во время кручения энкодера, удобно с лампочкой
    }
    key.encoder = 0;
  }
  if (key.previous_millis_down + 10000 < millis()) set_menu = false;
}

void parseParams(String inputString) { //Функция разбора принятой в Serial port строки
  String bb = inputString;
  //String T1;
  if (bb.substring(0, 2) == "SP") {
    T1 = bb.substring(bb.indexOf("SP", 2) + 3); //команда
    remote_buf = T1.toFloat();          //Выставленная мощность с Serial
    inst_P = remote_buf;
  }
}

void chkSerial() {
  while (Serial.available()) //Serial port, пока не конец сообщения, читаем данные и формируем строку
  {
    char ch = Serial.read();
    Var += ch;
    if (ch == '\n')
    {
      parseParams(Var); //вызываем функцию разбора принятой строки
      Var = "";
    }
  }
}

