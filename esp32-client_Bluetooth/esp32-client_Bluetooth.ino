#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

const uint8_t PIN_LED = 2;

// UUID сервиса и характеристики, к которым нужно подключиться
static BLEUUID serviceUUID("f048d655-5081-4115-9396-2530964dceae");
static BLEUUID    charUUID("fe276fbf-dbc8-4d1a-8ec3-083fb4a9e217");

// Глобальные переменные
static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLERemoteService* pRemoteService;
static BLEAdvertisedDevice *myDevice;

// Колбэк для получения информации об найденных устройствах
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      Serial.print("Advertised Device: ");
      Serial.println(advertisedDevice.toString().c_str());

      if (advertisedDevice.haveServiceUUID() && advertisedDevice.getServiceUUID().equals(serviceUUID)) {
        Serial.print("Found Our Service!  address: ");
        advertisedDevice.getScan()->stop();
        myDevice = new BLEAdvertisedDevice(advertisedDevice);
        doConnect = true;
        doScan = false;
      }
    }
};

// Функция для подключения к серверу
bool connectToServer() {
    Serial.print("Forming a connection to: ");
    Serial.println(myDevice->getAddress().toString().c_str());

    BLEClient* pClient = BLEDevice::createClient();
    Serial.println(" - Connecting to server...");
    
    // Подключаемся к серверу
    if (!pClient->connect(myDevice)) {
        Serial.println(" - Failed to connect to server");
        return false;
    }
    Serial.println(" - Connected to server");

    // Получаем сервис
    pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
        Serial.print("Failed to find our service UUID: ");
        Serial.println(serviceUUID.toString().c_str());
        pClient->disconnect();
        return false;
    }
    Serial.println(" - Found our service");

    // Получаем характеристику
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
        Serial.print("Failed to find our characteristic UUID: ");
        Serial.println(charUUID.toString().c_str());
        pClient->disconnect();
        return false;
    }
    Serial.println(" - Found our characteristic");

    // Читаем значение, если возможно
    if(pRemoteCharacteristic->canRead()) {
        String value = pRemoteCharacteristic->readValue();
        Serial.print("The initial value was: ");
        Serial.println(value.c_str());
    }

    connected = true;
    return true;
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);

  BLEDevice::init("ESP32 Client"); // Инициализация BLE
  Serial.println("BLE Client initialized");

  // Создаем сканер
  BLEScan* pScan = BLEDevice::getScan();
  pScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pScan->setActiveScan(true); // Set active scanning for more detail
  pScan->setInterval(100);
  pScan->setWindow(99);

  doScan = true; // Начинаем сканирование
  Serial.println("Starting scan...");
}

void loop() {

  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
    } else {
      Serial.println("Failed to connect to the server; Restart scanning");
      doScan = true;
    }
    doConnect = false;
  }

  if (connected) {
    String newValue = "Time since boot: " + String(millis()/1000);
    Serial.println("Setting new characteristic value to \"" + newValue + "\"");
    pRemoteCharacteristic->writeValue(newValue.c_str(), newValue.length());
    digitalWrite(PIN_LED, HIGH);
  } else if (doScan) {
    BLEDevice::getScan()->start(0);  // 0 = don't stop scanning after ...
  }
  delay(1000); // Добавлена задержка, чтобы не перегружать loop
}
