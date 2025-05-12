#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128  // Ширина дисплея в пикселях
#define SCREEN_HEIGHT 64  // Высота дисплея в пикселях
#define OLED_RESET     -1 // Reset pin (или -1 если используется общий reset)

// Инициализация дисплея с I2C адресом 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  Serial.begin(115200);
  
  // Инициализация I2C с указанием пинов SDA (GPIO14/D5) и SCL (GPIO12/D6)
  Wire.begin(14, 12);
  
  // Попытка инициализации дисплея
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Бесконечный цикл, если инициализация не удалась
  }
  
  Serial.println("SSD1306 initialized successfully");
  
  // Очистка буфера
  display.clearDisplay();
  
  // Установка размера текста и цвета
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  // Установка курсора и вывод текста
  display.setCursor(0, 0);
  display.println("Hello, NodeMCU!");
  display.println("SSD1306 test");
  display.println("SDA: GPIO14 (D5)");
  display.println("SCL: GPIO12 (D6)");
  
  // Вывод на дисплей
  display.display();
  
  delay(2000);
}

void loop() {
  // Демонстрация работы дисплея
  testDrawLines();
  delay(1000);
  testDrawRect();
  delay(1000);
  testFillRect();
  delay(1000);
  testDrawCircle();
  delay(1000);
  testDrawText();
  delay(1000);
}

void testDrawLines() {
  display.clearDisplay();
  
  for(int16_t i=0; i<display.width(); i+=4) {
    display.drawLine(0, 0, i, display.height()-1, SSD1306_WHITE);
    display.display();
    delay(1);
  }
  for(int16_t i=0; i<display.height(); i+=4) {
    display.drawLine(0, 0, display.width()-1, i, SSD1306_WHITE);
    display.display();
    delay(1);
  }
}

void testDrawRect() {
  display.clearDisplay();
  
  for(int16_t i=0; i<display.height()/2; i+=2) {
    display.drawRect(i, i, display.width()-2*i, display.height()-2*i, SSD1306_WHITE);
    display.display();
    delay(1);
  }
}

void testFillRect() {
  display.clearDisplay();
  
  for(int16_t i=0; i<display.height()/2; i+=3) {
    display.fillRect(i, i, display.width()-2*i, display.height()-2*i, SSD1306_WHITE);
    display.display();
    delay(1);
  }
}

void testDrawCircle() {
  display.clearDisplay();
  
  for(int16_t i=0; i<max(display.width(),display.height())/2; i+=2) {
    display.drawCircle(display.width()/2, display.height()/2, i, SSD1306_WHITE);
    display.display();
    delay(1);
  }
}

void testDrawText() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("SSD1306 Test");
  display.println("NodeMCU");
  display.println("SCL: GPIO12 (D6)");
  display.println("SDA: GPIO14 (D5)");
  display.println("Working!");
  display.display();
}