#include <TFT_eSPI.h> // Графика и шрифты для дисплея
#include <SPI.h>      // SPI-коммуникация

// Создаем объект для работы с дисплеем
TFT_eSPI tft = TFT_eSPI();

// Определение пинов для дисплея (может отличаться, проверьте документацию)
// Общие пины:
#define TFT_MOSI 3
#define TFT_SCLK 5
#define TFT_CS   4
#define TFT_DC   2
#define TFT_RST  1
#define TFT_LED  38

void setup() {
  Serial.begin(115200);
  Serial.println("Starting...");

  // Инициализируем дисплей
  tft.init();

  // Устанавливаем ориентацию дисплея (0-3, в зависимости от того, как вы хотите его видеть)
  tft.setRotation(1);  // Попробуйте разные значения от 0 до 3

  // Очищаем дисплей черным цветом
  tft.fillScreen(TFT_BLACK);

  // Устанавливаем цвет текста (белый)
  tft.setTextColor(TFT_WHITE);

  // Устанавливаем размер текста
  tft.setTextSize(2);

  // Устанавливаем позицию текста
  tft.setCursor(0, 0);

  // Выводим текст на дисплей
  tft.println("Hello, Lilygo T-Dongle S3!");
  tft.println("Display Test");
  tft.println("Running...");


  
}

void loop() {
  // Выводим текущее время
  tft.setCursor(0, 60); // Перемещаем курсор ниже
  tft.print("Millis: ");
  tft.println(millis());

  // Небольшая задержка
  delay(1000);
}