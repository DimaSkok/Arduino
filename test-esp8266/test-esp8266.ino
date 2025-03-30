#include <Arduino.h>
#include <U8g2lib.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

// U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);   // РАБОТАЕТ

// Инициализация дисплея для I2C, по умолчанию D1=SCL, D2=SDA
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* clock=*/ 5, /* data=*/ 6, /* reset=*/ U8X8_PIN_NONE);   // РАБОТАЕТ  (D1 = 5, D2 = 6)


void setup() {
  Serial.begin(115200);
  Serial.println("SH1106 OLED Test");
  u8g2.begin(); // Инициализация U8g2
  u8g2.clearBuffer();  // Очищаем буфер
  u8g2.setFont(u8g_font_ncenB08); // Выбор шрифта
  u8g2.setCursor(0, 15); // Установка позиции курсора
  u8g2.print("Hello SH1106!"); // Вывод текста
  u8g2.sendBuffer(); // Отправляем буфер на дисплей
}


void loop() {
  //Ваш код в цикле
  delay(1000);
  u8g2.clearBuffer(); //Очистка экрана
  u8g2.setFont(u8g_font_ncenB08);
  u8g2.setCursor(0, 15);
  u8g2.print("Test loop"); //Вывод текста
  u8g2.sendBuffer();
  delay(1000);
}