#include <WiFi.h>
#include <USB.h>
#include <USBCDC.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

// Настройки дисплея
#define TFT_CS   4
#define TFT_RST  1
#define TFT_DC   2
#define TFT_SCLK 5
#define TFT_MOSI 3

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

// Настройки Wi-Fi
const char* wifi_ssid = "TP-Link_C552";
const char* wifi_password = "57174198";

USBCDC USBSerial;
bool usbConnected = false;
bool wifiConnected = false;

void displayPrintln(const String &text) {
  static uint8_t y = 8;
  static uint8_t lineHeight = 10;
  
  if (y + lineHeight > tft.height() - 8) {
    tft.fillScreen(ST7735_BLACK);
    y = 8;
  }
  tft.setCursor(4, y);
  tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
  tft.println(text);
  y += lineHeight;
  Serial.println(text);
}

void displayStatus() {
  tft.fillScreen(ST7735_BLACK);
  uint8_t y = 8;
  
  tft.setCursor(4, y);
  tft.setTextColor(ST7735_GREEN, ST7735_BLACK);
  tft.println("USB WiFi Adapter");
  y += 12;

  tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
  
  tft.setCursor(4, y);
  tft.print("USB: ");
  tft.setTextColor(usbConnected ? ST7735_GREEN : ST7735_RED, ST7735_BLACK);
  tft.println(usbConnected ? "Connected" : "Disconnected");
  y += 10;
  
  tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
  
  tft.setCursor(4, y);
  tft.print("Wi-Fi: ");
  tft.setTextColor(wifiConnected ? ST7735_GREEN : ST7735_RED, ST7735_BLACK);
  tft.println(wifiConnected ? "Connected" : "Disconnected");
  y += 10;
  
  if (wifiConnected) {
    tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
    tft.setCursor(4, y);
    tft.print("IP: ");
    tft.println(WiFi.localIP());
    y += 10;
  }
}

#if CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3
void usbEventCallback(void* arg, esp_event_base_t event_base, 
                     int32_t event_id, void* event_data) {
  if (event_base == ARDUINO_USB_CDC_EVENTS) {
    switch (event_id) {
      case ARDUINO_USB_CDC_CONNECTED_EVENT:
        usbConnected = true;
        displayPrintln("USB Connected");
        displayStatus();
        break;
      case ARDUINO_USB_CDC_DISCONNECTED_EVENT:
        usbConnected = false;
        displayPrintln("USB Disconnected");
        displayStatus();
        break;
    }
  }
}
#endif

void connectToWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  
  displayPrintln("Connecting to WiFi...");
  WiFi.begin(wifi_ssid, wifi_password);
  
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
    delay(500);
    displayPrintln(".");
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    displayPrintln("WiFi Connected");
    displayPrintln("IP: " + WiFi.localIP().toString());
  } else {
    wifiConnected = false;
    displayPrintln("WiFi Connection Failed");
  }
  displayStatus();
}

void setup() {
  // Инициализация дисплея
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);
  tft.fillScreen(ST7735_BLACK);
  tft.setTextSize(1);
  tft.setTextWrap(true);
  
  Serial.begin(115200);
  displayPrintln("Initializing...");

  // Инициализация USB CDC
  #if CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3
  USB.begin();
  USBSerial.begin();
  USBSerial.onEvent(usbEventCallback);
  USBSerial.enableReboot(true);
  #endif

  // Подключение к Wi-Fi
  WiFi.mode(WIFI_STA);
  connectToWiFi();

  // Настройка сети
  if (!WiFi.config(
    IPAddress(192, 168, 4, 100),  // Локальный IP
    IPAddress(192, 168, 4, 1),    // Шлюз
    IPAddress(255, 255, 255, 0),  // Маска
    IPAddress(8, 8, 8, 8),        // DNS1
    IPAddress(8, 8, 4, 4)         // DNS2
  )) {
    displayPrintln("Network config failed");
  }

  displayStatus();
}

void loop() {
  // Проверка состояния Wi-Fi
  if (WiFi.status() != WL_CONNECTED && wifiConnected) {
    wifiConnected = false;
    displayStatus();
    connectToWiFi();
  } else if (WiFi.status() == WL_CONNECTED && !wifiConnected) {
    wifiConnected = true;
    displayStatus();
  }

  // Обработка USB данных
  #if CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3
  if (usbConnected && USBSerial.available()) {
    String cmd = USBSerial.readStringUntil('\n');
    cmd.trim();
    if (cmd == "reboot") {
      ESP.restart();
    }
  }
  #endif

  delay(1000);
}