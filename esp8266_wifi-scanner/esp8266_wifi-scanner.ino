#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  Serial.begin(115200);
  
  // Инициализация дисплея (SDA = GPIO14/D5, SCL = GPIO12/D6)
  Wire.begin(14, 12);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 init failed");
    while(1);
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("WiFi Scanner");
  display.display();
  delay(2000);

  // Настройка WiFi
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
}

void loop() {

  // Сканирование сетей
  int n = WiFi.scanNetworks();
  display.clearDisplay();
  
  if(n == 0) {
    display.setCursor(0,0);
    display.println("No networks found");
  } else {
    display.setCursor(0,0);
    display.printf("Found %d nets\n", n);
    
    // Выводим до 7 сетей (максимум для этого дисплея)
    for(int i = 0; i < min(n, 7); i++) {
      display.printf("%s (%d)\n", 
        WiFi.SSID(i).c_str(), 
        WiFi.RSSI(i));
    }
  }
  
  display.display();
  delay(5000); // Пауза 5 сек между сканированиями
}