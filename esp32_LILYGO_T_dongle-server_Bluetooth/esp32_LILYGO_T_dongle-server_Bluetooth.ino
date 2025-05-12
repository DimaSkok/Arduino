#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

const uint8_t BOOT = 0;

int lastButtonState = HIGH;
int buttonState = HIGH;
bool flag = false;


// ----- TFT Display -----
#define TFT_CS   4
#define TFT_RST  1
#define TFT_DC   2
#define TFT_SCLK 5
#define TFT_MOSI 3

#define SERVICE_UUID        "f048d655-5081-4115-9396-2530964dceae"
#define CHARACTERISTIC_UUID "fe276fbf-dbc8-4d1a-8ec3-083fb4a9e217"

BLECharacteristic *pCharacteristic;
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
bool deviceConnected = false;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("Device connected");
      tft.fillScreen(ST77XX_GREEN);
      tft.setTextColor(ST77XX_BLACK);
      tft.setCursor(8, 80);
      tft.print("Device connected");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("Device disconnected");
      tft.fillScreen(ST77XX_WHITE);
      tft.setTextColor(ST77XX_BLACK);
      tft.setCursor(8, 80);
      tft.print("Device disconnected");
      pServer->startAdvertising();
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      String rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        Serial.println("*********");
        Serial.print("Received Value: ");
        Serial.println(rxValue.c_str());

        tft.fillScreen(ST77XX_WHITE);
        tft.setTextColor(ST77XX_BLACK);
        tft.setCursor(8, 40);
        tft.print(rxValue.c_str());
      }
    }
};

void setup() {
  Serial.begin(115200);

  pinMode(BOOT, INPUT_PULLUP);

  // TFT Initialization
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);
  tft.fillScreen(ST77XX_WHITE);
  Serial.println("TFT initialized");

  tft.setTextSize(1);
  tft.setTextColor(ST77XX_BLACK);
  tft.setCursor(8, 40);
  tft.print("TFT initialized");

  // BLE Initialization
  BLEDevice::init("esp32 Test");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  
  // Создаем характеристику с нужными свойствами
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_WRITE |
                      BLECharacteristic::PROPERTY_NOTIFY  // Добавляем поддержку уведомлений
                    );
  
  pCharacteristic->setCallbacks(new MyCallbacks());
  pService->start();
  
  // Настройка рекламы
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->start();
  
  Serial.println("BLE Server started!");
  tft.setCursor(8, 60);
  tft.print("Server start!");
}

String message = "1";

void loop() {
  static unsigned long lastDebounceTime = 0;
  const unsigned long debounceDelay = 50;
  
  if (deviceConnected) {
    int reading = digitalRead(BOOT);
    
    // Антидребезг
    if (reading != lastButtonState) {
      lastDebounceTime = millis();
    }
    
    if ((millis() - lastDebounceTime) > debounceDelay) {
      if (reading != buttonState) {
        buttonState = reading;
        
        // Только при нажатии кнопки (LOW, если подключена с подтяжкой к +)
        if (buttonState == LOW) {
          String message = flag ? "1" : "0";
          flag = !flag; // Инвертируем флаг
          
          message.trim();
          if (message.length() > 0) {
            pCharacteristic->setValue(message.c_str());
            pCharacteristic->notify();
            
            // Выводим на экран отправленное сообщение
            tft.fillScreen(ST77XX_WHITE);
            tft.setTextColor(ST77XX_BLACK);
            tft.setCursor(8, 20);
            tft.print("Sent:");
            tft.setCursor(8, 40);
            tft.print(message.c_str());
          }
        }
      }
    }
    lastButtonState = reading;
  }
  delay(10);
}