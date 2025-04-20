#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// ----- TFT Display -----
#define TFT_CS   4
#define TFT_RST  1
#define TFT_DC   2
#define TFT_SCLK 5
#define TFT_MOSI 3

#define TFT_WIDTH  80
#define TFT_HEIGHT 160

#define SERVICE_UUID        "f048d655-5081-4115-9396-2530964dceae"
#define CHARACTERISTIC_UUID "fe276fbf-dbc8-4d1a-8ec3-083fb4a9e217"

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

        for (int i = 0; i < rxValue.length(); i++) {
          Serial.print(rxValue[i]);
        }
        Serial.println();
        Serial.println("*********");

        tft.fillScreen(ST77XX_WHITE);
        tft.setTextColor(ST77XX_BLACK);
        tft.setCursor(8, 40);
        tft.print(rxValue.c_str());
      }
    }
};

void setup() {
  Serial.begin(115200);

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
  
  // Исправленная строка - правильно создаем характеристику
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
  
  pCharacteristic->setCallbacks(new MyCallbacks());
  pService->start();
  
  // Настройка и запуск рекламы
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->start();
  
  Serial.println("BLE Server started!");
  tft.setCursor(8, 60);
  tft.print("Server start!");
}

void loop() {
  delay(1000);
}