#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <MPU6050.h>
#include "RTClib.h"
#include <math.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include "VL53L1X.h"
#include <esp_task_wdt.h>

#define CHAOS_STAR_X      32
#define CHAOS_STAR_Y      40
#define CHAOS_STAR_SIZE   20

// ========== ПИНЫ ==========
const uint8_t PIN_SDA = 5;
const uint8_t PIN_SCL = 6;
const uint8_t PIN_LASER = 2;
const uint8_t PIN_BUTTON = 21;
const uint8_t PIN_BUTTON2 = 20;
const uint8_t ACTIVATE = 10;
const uint8_t BATTERY_PIN = 0;

// ========== ДИСПЛЕИ ==========
#define SCREEN1_WIDTH 128
#define SCREEN1_HEIGHT 64
#define SCREEN1_ADDR 0x3D
Adafruit_SSD1306 display1(SCREEN1_WIDTH, SCREEN1_HEIGHT, &Wire);

#define SCREEN2_WIDTH 128
#define SCREEN2_HEIGHT 32
#define SCREEN2_ADDR 0x3C
Adafruit_SSD1306 display2(SCREEN2_WIDTH, SCREEN2_HEIGHT, &Wire);

// ========== NTP ==========
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 10800);

// ========== ДАТЧИКИ ==========
MPU6050 imu(0x69);
RTC_DS1307 rtc;
VL53L1X tofSensor;

// ========== ДАННЫЕ ==========
struct {
  uint16_t distance;
  float angle;
} measurements[2];

struct {
  float x, y, z;
} acceleration;

struct {
  bool useNTP = false;
  bool wifiConnected = false;
} wifiState;

const struct {
  const char* ssid;
  const char* password;
} wifiNetworks[3] = {
  {"TP-Link_C552", "57174198"},
  {"ssid", "passwd"},
  {"CH_net.su", "vybp2748"} 
};

// ========== СОСТОЯНИЕ ==========
volatile bool buttonPressed = false;
volatile unsigned long lastButtonPress = 0;
uint8_t current_point = 0;
uint8_t operation_mode = 0;
bool laser_active = false;
uint8_t chaosPulsePhase = 0;
uint32_t lastSync = 0;
uint32_t last_update = 0;
volatile bool button2Pressed = false;
volatile unsigned long lastButton2Press = 0;

// ========== КОНСТАНТЫ ==========
const uint8_t DISPLAY_ORIGIN_X = 128/2 + 30;
const uint8_t DISPLAY_ORIGIN_Y = 40;
const uint8_t AXIS_LENGTH = 20;
const uint16_t MEASURE_INTERVAL = 50;
const char daysOfTheWeek[7][12] = {"Su","Mo","Tu","We","Th","Fr","Sa"};
const int wifiNetworksCount = sizeof(wifiNetworks) / sizeof(wifiNetworks[0]);
const float VOLTAGE_FULL = 4.2;
const float VOLTAGE_EMPTY = 3.0;

// ========== ПРОТОТИПЫ ФУНКЦИЙ ==========
void IRAM_ATTR buttonISR();
void IRAM_ATTR button2ISR();
void drawChaosStar(Adafruit_SSD1306 &display, uint8_t x, uint8_t y, uint8_t size, uint8_t pulsePhase);
void draw3DAxis(Adafruit_SSD1306 &display, float pitch, float roll, float yaw);
void handleButtonPress();
void handleButton2Press();
void showInitMessage(const String& message, bool clear = true);

void setup() {
  Serial.begin(115200);

  // Инициализация основного дисплея в первую очередь
  Wire.begin(PIN_SDA, PIN_SCL);
  if(!display1.begin(SSD1306_SWITCHCAPVCC, SCREEN1_ADDR)) {
    Serial.println("Display1 не найден!");
    while(1);
  }
  
  showInitMessage("Starting system...");
  delay(500);

  setupPins();
  setupI2C();
  
  showInitMessage("Initializing components:");
  delay(200);
  
  setupDisplays();
  setupSensors();
  
  showInitMessage("Connecting to WiFi...");
  wifiState.wifiConnected = connectToWiFi();
  wifiState.useNTP = wifiState.wifiConnected;

  if (wifiState.wifiConnected) {
    showInitMessage("Syncing time...");
    syncTimeWithNTP();
  }
  
  // Финальное сообщение
  display1.clearDisplay();
  display1.setTextSize(1);
  display1.setCursor(0,0);
  display1.println("System initialized");
  display1.setCursor(0,16);
  display1.print("WiFi: ");
  display1.println(wifiState.wifiConnected ? "OK" : "FAIL");
  display1.setCursor(0,32);
  display1.println("Ready to work!");
  display1.display();
  delay(1000);
  
  Serial.println("Система инициализирована");
}

void loop() {
  if (wifiState.wifiConnected && millis() - lastSync >= 3600000) {
    syncTimeWithNTP();
    lastSync = millis();
  } 
  
  uint32_t current_time = millis();
  
  if(current_time - last_update < MEASURE_INTERVAL) return;
  last_update = current_time;
  
  if (buttonPressed) {
    buttonPressed = false;
    handleButtonPress();
  }
  
  if (button2Pressed) { 
    button2Pressed = false;
    handleButton2Press();
  }
  
  readSensors();
  updateDisplays();
}

// ========== ФУНКЦИЯ ДЛЯ ВЫВОДА СООБЩЕНИЙ ИНИЦИАЛИЗАЦИИ ==========
void showInitMessage(const String& message, bool clear) {
  if(clear) display1.clearDisplay();
  display1.setTextSize(1);
  display1.setTextColor(WHITE);
  display1.setCursor(0,0);
  display1.println(message);
  display1.display();
  Serial.println(message);
}

// ========== ОБРАБОТЧИКИ ПРЕРЫВАНИЙ ==========
void IRAM_ATTR buttonISR() {
  unsigned long currentTime = millis();
  if (currentTime - lastButtonPress > 250) {
    buttonPressed = true;
    lastButtonPress = currentTime;
  }
}

void IRAM_ATTR button2ISR() {
  unsigned long currentTime = millis();
  if (currentTime - lastButton2Press > 250) {
    button2Pressed = true;
    lastButton2Press = currentTime;
  }
}

// ========== ОБРАБОТКА НАЖАТИЙ ==========
void handleButtonPress() {
  operation_mode = (operation_mode + 1) % 3;
  
  switch(operation_mode) {
    case 1:
      laser_active = true;
      digitalWrite(PIN_LASER, HIGH);
      takeMeasurement();
      break;
      
    case 2:
      laser_active = false;
      digitalWrite(PIN_LASER, LOW);
      current_point = (current_point + 1) % 2;
      break;
  }
}

void handleButton2Press() {
  // Здесь можно добавить функционал для второй кнопки
  Serial.println("Button 2 pressed");
}

// ========== ОСНОВНЫЕ ФУНКЦИИ ==========
DateTime getCurrentTime() {
  if (wifiState.useNTP && wifiState.wifiConnected) {
    timeClient.update();
    return DateTime(timeClient.getEpochTime());
  }
  return rtc.now();
}

bool connectToWiFi() {
  if (wifiNetworksCount == 0) {
    showInitMessage("No WiFi networks", false);
    return false;
  }
  
  for (int i = 0; i < wifiNetworksCount; i++) {
    if (strlen(wifiNetworks[i].ssid) == 0) continue;
    
    display1.print("Trying: ");
    display1.println(wifiNetworks[i].ssid);
    display1.display();
    
    WiFi.begin(wifiNetworks[i].ssid, wifiNetworks[i].password);
    int attempts = 0;
    while (attempts < 20) {
      if (WiFi.status() == WL_CONNECTED) {
        showInitMessage("WiFi connected!", false);
        timeClient.begin();
        return true;
      }
      delay(500);
      display1.print(".");
      display1.display();
      attempts++;
    }
    showInitMessage("Failed to connect", false);
    WiFi.disconnect();
    delay(1000);
  }
  
  showInitMessage("All attempts failed", false);
  return false;
}

void syncTimeWithNTP() {
  if (wifiState.wifiConnected && timeClient.update()) {
    DateTime ntpTime(timeClient.getEpochTime());
    rtc.adjust(ntpTime);
    showInitMessage("Time synced!", false);
    Serial.println("Time synchronized with NTP");
  }
}

void setupPins() {
  pinMode(PIN_LASER, OUTPUT);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  pinMode(PIN_BUTTON2, INPUT_PULLUP);
  digitalWrite(PIN_LASER, LOW);
  analogReadResolution(12);       // Устанавливаем разрешение АЦП (9-12 бит)
  analogSetAttenuation(ADC_11db); // Установка аттенюатора: 11 dB (макс. ~3.1 В)
  
  attachInterrupt(digitalPinToInterrupt(PIN_BUTTON), buttonISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_BUTTON2), button2ISR, FALLING);
  
  showInitMessage("Pins initialized", false);
}

void setupI2C() {
  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(100000);
  pinMode(PIN_SDA, INPUT_PULLUP);
  pinMode(PIN_SCL, INPUT_PULLUP);
  delay(100);
  
  showInitMessage("I2C initialized", false);
}

void setupDisplays() {
  showInitMessage("Init displays...", false);
  
  if(!display2.begin(SSD1306_SWITCHCAPVCC, SCREEN2_ADDR)) {
    showInitMessage("Display2: FAIL", false);
    Serial.println("Display2 не найден!");
    while(1);
  }
  showInitMessage("Display2: OK", false);
  
  display2.clearDisplay();
  display2.setTextSize(1);
  display2.setTextColor(WHITE);
  display2.setCursor(32,16);
  display2.println("Made in Dimasik");
  display2.setCursor(72,24);
  display2.println("from FAKI");
  display2.display();
}

void setupSensors() {
  showInitMessage("Init sensors:", false);
  
  // MPU6050
  imu.initialize();
  bool mpuStatus = imu.testConnection();
  display1.print("MPU6050: ");
  display1.println(mpuStatus ? "OK" : "FAIL");
  display1.display();
  Serial.println(mpuStatus ? "MPU6050 OK" : "MPU6050 fail");
  delay(100);
  
  // RTC
  if(!rtc.begin()) {
    showInitMessage("RTC: FAIL", false);
    Serial.println("RTC не инициализирован");
    while(1);
  }
  showInitMessage("RTC: OK", false);
  delay(100);
  
  // VL53L1X
  Wire.beginTransmission(0x29);
  if (Wire.endTransmission() == 0) {
    if (!tofSensor.init()) {
      showInitMessage("VL53L1X: FAIL", false);
      Serial.println("Ошибка инициализации VL53L1X!");
      while (1);
    }
    tofSensor.setAddress(0x29);
    tofSensor.setDistanceMode(VL53L1X::Long);
    tofSensor.setMeasurementTimingBudget(50000);
    tofSensor.startContinuous(50);
    showInitMessage("VL53L1X: OK", false);
    Serial.println("VL53L1X инициализирован");
  } else {
    showInitMessage("VL53L1X: Not found", false);
  }
  delay(100);
  
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}

uint8_t readBatteryPercent() {
  int adcValue = analogRead(BATTERY_PIN);
  float voltage = adcValue * (3.3 / 4095.0) * 2.0; // делитель 1:1

  voltage = constrain(voltage, VOLTAGE_EMPTY, VOLTAGE_FULL);
  float percentFloat = (voltage - VOLTAGE_EMPTY) / (VOLTAGE_FULL - VOLTAGE_EMPTY) * 100.0;

  percentFloat = constrain(percentFloat, 0.0, 100.0);
  uint8_t percent = (uint8_t)round(percentFloat);

  return percent;
}

void drawBatteryIndicator(uint8_t percent, Adafruit_SSD1306 &display) {
  if (percent > 100) percent = 100;

  // Координаты и размеры индикатора
  int16_t x = SCREEN1_WIDTH - 25;
  int16_t y = 2;
  int16_t width = 20;
  int16_t height = 10;
  int16_t capWidth = 3;
  int16_t capHeight = 4;

  // Очищаем область
  display.fillRect(x - 1, y - 1, width + capWidth + 2, height + 2, SSD1306_BLACK);
  // Рисуем контур батареи
  display.drawRect(x, y, width, height, SSD1306_WHITE);
  // Рисуем "колпачок" батареи
  display.fillRect(x + width, y + (height - capHeight) / 2, capWidth, capHeight, SSD1306_WHITE);

  // Рассчитываем ширину заполненной части
  int16_t fillWidth = (width - 2) * percent / 100;

  // Ограничиваем ширину заполнения, чтобы не выходить за границы
  fillWidth = constrain(fillWidth, 0, width - 2);

  // Определяем стиль отображения в зависимости от уровня заряда
  bool invert = false;

  if (percent < 20) {
    // Низкий заряд - мигаем, инвертируя изображение
    if (millis() % 1000 < 500) {
      invert = true; // Мигание
    }
  }

  // Рисуем уровень заряда (или инвертированное изображение)
  if (fillWidth > 0) {
    if (invert) {
      //Инвертируем только ту область, которую хотим "показать"
      display.fillRect(x + 1, y + 1, fillWidth, height - 2, SSD1306_BLACK); // Черный прямоугольник (как бы "стираем")
      display.fillRect(x + 1, y + 1, fillWidth, height - 2, SSD1306_WHITE); //Рисуем рамку "батарейки"
    } else {
      display.fillRect(x + 1, y + 1, fillWidth, height - 2, SSD1306_WHITE); //Белый прямоугольник (заполняем батарею)
    }
  } else if (percent < 20 && !invert) {
    //Если аккумулятор почти разряжен и не мигает - показываем хотя бы рамку
       display.drawRect(x + 1, y + 1, 1, height - 2, SSD1306_WHITE); // рисуем одну полоску, чтобы показать что он вообще есть
  }
}

void readSensors() {
  static int16_t ax, ay, az;
  static int16_t ax_prev, ay_prev, az_prev;
  
  imu.getAcceleration(&ax, &ay, &az);
  acceleration.x = (ax_prev * 0.7) + (ax * 0.3) / 16384.0;
  acceleration.y = (ay_prev * 0.7) + (ay * 0.3) / 16384.0;
  acceleration.z = (az_prev * 0.7) + (az * 0.3) / 16384.0;
  ax_prev = ax; ay_prev = ay; az_prev = az;
}

void takeMeasurement() {
  tofSensor.read();
  measurements[current_point].distance = tofSensor.ranging_data.range_mm / 10;
  measurements[current_point].angle = atan2(acceleration.y, acceleration.x);
}

void updateDisplays() {
  DateTime now = rtc.now();
  
  // Основной дисплей
  display1.clearDisplay();

  uint8_t batPercent = readBatteryPercent();
  drawBatteryIndicator(batPercent, display1);

  display1.setTextSize(1);
  display1.setTextColor(SSD1306_WHITE);
  display1.setCursor(0, 0);
  display1.print("P1:");
  display1.print(measurements[0].distance);
  display1.print("cm P2:");
  display1.print(measurements[1].distance);
  display1.println("cm");
  
  uint16_t dist = sqrt(measurements[1].distance*measurements[1].distance + measurements[0].distance*measurements[0].distance - 2*measurements[1].distance*measurements[0].distance*cos(measurements[1].angle - measurements[0].angle));
  display1.print("Distance: ");
  display1.print(dist);
  display1.println("cm");

  draw3DAxis(display1, 
    atan2(acceleration.y, acceleration.z),
    atan2(-acceleration.x, sqrt(acceleration.y*acceleration.y + acceleration.z*acceleration.z)),
    0);

  chaosPulsePhase++;
  drawChaosStar(display1, CHAOS_STAR_X, CHAOS_STAR_Y, CHAOS_STAR_SIZE, chaosPulsePhase, 0.5);
  display1.display();

  // Второй дисплей
  display2.clearDisplay();
  display2.setTextSize(2);
  display2.setTextColor(WHITE);
  display2.setCursor(0,0);
  display2.print(now.hour(), DEC);
  display2.print(':');
  display2.print(now.minute(), DEC);
  display2.print(':');
  display2.print(now.second(), DEC);
  display2.setCursor(104,0);
  display2.print('|');
  display2.println(daysOfTheWeek[now.dayOfTheWeek()][0]);
  
  display2.print(now.day(), DEC);
  display2.print('/');
  display2.print(now.month(), DEC);
  display2.print('/');
  display2.print(now.year() % 100, DEC);
  display2.setCursor(104,16);
  display2.print('|');
  display2.println(daysOfTheWeek[now.dayOfTheWeek()][1]);
  display2.display();
}

void draw3DAxis(Adafruit_SSD1306 &display, float pitch, float roll, float yaw) {
  const uint8_t ORIGIN_X = DISPLAY_ORIGIN_X;
  const uint8_t ORIGIN_Y = DISPLAY_ORIGIN_Y;
  const uint8_t AXIS_LENGTH = 20;

  const float cosX = cosf(roll), sinX = sinf(roll);
  const float cosY = cosf(pitch), sinY = sinf(pitch);
  const float cosZ = cosf(yaw), sinZ = sinf(yaw);

  const float axisEnds[3][3] = {{AXIS_LENGTH,0,0},{0,AXIS_LENGTH,0},{0,0,AXIS_LENGTH}};
  int16_t x2d[3], y2d[3];

  for (uint8_t i = 0; i < 3; i++) {
    float x = axisEnds[i][0]*cosZ - axisEnds[i][1]*sinZ;
    float y = axisEnds[i][0]*sinZ + axisEnds[i][1]*cosZ;
    float z = axisEnds[i][2];
    float x2 = x*cosY + z*sinY;
    z = -x*sinY + z*cosY;
    y = y*cosX - z*sinX;
    x2d[i] = ORIGIN_X + (int16_t)x2;
    y2d[i] = ORIGIN_Y - (int16_t)y;
  }

  const char axisLabels[3] = {'X','Y','Z'};
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  for (uint8_t i = 0; i < 3; i++) {
    display.drawLine(ORIGIN_X, ORIGIN_Y, x2d[i], y2d[i], SSD1306_WHITE);
    display.setCursor(x2d[i], y2d[i]);
    display.print(axisLabels[i]);
  }
}

void drawChaosStar(Adafruit_SSD1306 &display, uint8_t x, uint8_t y, uint8_t size, uint8_t pulsePhase, float dark) {
  uint8_t pulseOffset = (sin(pulsePhase * 0.1) * 2);
  size += pulseOffset;
  
  for (int i = 0; i < 8; i++) {
    float angle = i * PI / 4;
    for (int j = -1; j <= 1; j++) {
      float offsetAngle = angle + j * 0.05;
      int16_t x1 = x + cos(offsetAngle)*size;
      int16_t y1 = y + sin(offsetAngle)*size;
      int16_t x2 = x + cos(offsetAngle)*(size*0.4);
      int16_t y2 = y + sin(offsetAngle)*(size*0.4);
      display.drawLine(x, y, x1, y1, SSD1306_WHITE);
      display.drawLine(x, y, x2, y2, SSD1306_WHITE);
    }
  }
  display.fillCircle(x, y, size*dark, SSD1306_BLACK);
  for (int r = size-1; r <= size+1; r++) {
    display.drawCircle(x, y, r*0.8, SSD1306_WHITE);
  }
}