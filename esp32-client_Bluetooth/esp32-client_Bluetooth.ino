#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Пины
const uint8_t PIN_LED = 2;
const uint8_t BOOT = 0;
const uint8_t PIN_SDA = 6;
const uint8_t PIN_SCL = 5;

// Настройки дисплея
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_ADDR 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);

int flag = true;

// UUID сервиса и характеристики BLE
static BLEUUID serviceUUID("f048d655-5081-4115-9396-2530964dceae");
static BLEUUID charUUID("fe276fbf-dbc8-4d1a-8ec3-083fb4a9e217");

// Флаги состояния
static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLERemoteService* pRemoteService;
static BLEAdvertisedDevice* myDevice;

// Функция для вывода сообщений на дисплей
void showInitMessage(const String& message, bool clear) {
  if (clear) {
    display.clearDisplay();
    display.setCursor(0, 0);
  }
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.println(message);
  display.display();
  Serial.println(message);
}

// Колбэк для обнаружения BLE устройств
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("Найдено устройство: ");
    Serial.println(advertisedDevice.toString().c_str());

    if (advertisedDevice.haveServiceUUID() && advertisedDevice.getServiceUUID().equals(serviceUUID)) {
      Serial.println("Найден наш сервис!");
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = false;
    }
  }
};

// Функция подключения к BLE серверу
bool connectToServer() {
  Serial.print("Подключаемся к ");
  Serial.println(myDevice->getAddress().toString().c_str());

  BLEClient* pClient = BLEDevice::createClient();
  Serial.println(" - Соединяемся с сервером...");

  // Подключение без таймаута (исправленная версия)
  if (!pClient->connect(myDevice)) {
    Serial.println(" - Не удалось подключиться");
    return false;
  }
  Serial.println(" - Подключено");

  // Получаем сервис
  pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.print("Не найден сервис с UUID: ");
    Serial.println(serviceUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Сервис найден");

  // Получаем характеристику
  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
  if (pRemoteCharacteristic == nullptr) {
    Serial.print("Не найдена характеристика с UUID: ");
    Serial.println(charUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Характеристика найдена");

  connected = true;
  return true;
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_LED, OUTPUT);
  pinMode(BOOT, INPUT_PULLUP);
  digitalWrite(PIN_LED, HIGH);

  // Инициализация дисплея
  Wire.begin(PIN_SDA, PIN_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDR)) {
    Serial.println("Дисплей не найден!");
    while (1);
  }
  showInitMessage("Дисплей инициализирован", true);

  // Инициализация BLE
  BLEDevice::init("ESP32 Client");
  showInitMessage("BLE клиент запущен", true);

  // Настройка сканирования BLE
  BLEScan* pScan = BLEDevice::getScan();
  pScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pScan->setActiveScan(true);
  pScan->setInterval(100);
  pScan->setWindow(99);

  doScan = true;
  showInitMessage("Сканирование...", true);
}

void loop() {
  if (doConnect) {
    if (connectToServer()) {
      showInitMessage("Подключено к BLE серверу", true);
    } else {
      showInitMessage("Ошибка подключения", true);
      doScan = true;
    }
    doConnect = false;
  }

  if (connected) {
    // Управляем светодиодом и отправляем значение
    digitalWrite(PIN_LED, flag);
    if (flag) {
      pRemoteCharacteristic->writeValue("YES");
    } else {
      pRemoteCharacteristic->writeValue("NO");
    }

    // Читаем значение
    String rxValue = pRemoteCharacteristic->readValue();
    if (rxValue.length() > 0) {
      Serial.print("Получено: ");
      Serial.println(rxValue.c_str());
      
      if (rxValue == "1") {
        flag = 0;
      } else if (rxValue == "0") {
        flag = 1;
      }
    }
    
    delay(100);  // Пауза для предотвращения срабатывания watchdog
  } else if (doScan) {
    // Сканируем с ограничением по времени (исправленная версия)
    BLEDevice::getScan()->start(2, false);
    Serial.println("Сканируем...");
    delay(100);  // Небольшая пауза между сканированиями
  }

  delay(10);  // Основная задержка в цикле
}