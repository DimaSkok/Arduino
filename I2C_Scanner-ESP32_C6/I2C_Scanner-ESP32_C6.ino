#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

// Для ESP32-C6 Super Mini (проверьте даташит!)
#define I2C_SDA 19 
#define I2C_SCL 20  
#define LED_PIN 8

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_NeoPixel led(1, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(115200);
  delay(1000); // Даём время для инициализации Serial
  
  led.begin();
  led.setBrightness(50);

  // Инициализация I2C с указанием пинов
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(100000); // 100 kHz
  
  // Инициализация дисплея (пробуем оба адреса)
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  // Попробуйте 0x3D, если не работает
    Serial.println("SSD1306 не обнаружен!");
    while (1); // Зависаем, если дисплей не найден
  }
  
  Serial.println("SSD1306 найден!");
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Hello, ESP32-C6!");
  display.display();
}

void loop() {
  byte error, address;
  int nDevices = 0;

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Scanning I2C...");
  display.display();
  delay(1000);

  for(address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("I2C device found at 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);

      display.print("Found: 0x");
      if (address < 16) display.print("0");
      display.println(address, HEX);
      nDevices++;
    }
  }

  if (nDevices == 0) {
    Serial.println("No I2C devices found");
    display.println("No devices found");
  } else {
    Serial.println("Scan completed.");
  }

  display.display();
  // Плавный градиент: красный → зелёный → синий → красный
  for (int hue = 0; hue < 256; hue++) {
    led.setPixelColor(0, led.ColorHSV(hue * 256, 255, 255));
    led.show();
    delay(20);
  }
  delay(1000); // Пауза между сканированиями
}