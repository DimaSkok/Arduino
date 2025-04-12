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
const uint8_t PIN_BUTTON = 21; // Измените если используете другой пин

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

// ========== КОНСТАНТЫ ==========
const uint8_t DISPLAY_ORIGIN_X = 128/2 + 30;
const uint8_t DISPLAY_ORIGIN_Y = 40;
const uint8_t AXIS_LENGTH = 20;
const uint16_t MEASURE_INTERVAL = 50;
const char daysOfTheWeek[7][12] = {"Su","Mo","Tu","We","Th","Fr","Sa"};
const int wifiNetworksCount = sizeof(wifiNetworks) / sizeof(wifiNetworks[0]);

// ========== ПРОТОТИПЫ ФУНКЦИЙ ==========
void IRAM_ATTR buttonISR();
void drawChaosStar(Adafruit_SSD1306 &display, uint8_t x, uint8_t y, uint8_t size, uint8_t pulsePhase);
void draw3DAxis(Adafruit_SSD1306 &display, float pitch, float roll, float yaw);
void handleButtonPress();

void setup() {
  Serial.begin(115200);

  
  setupPins();
  setupI2C();
  setupDisplays();
  setupSensors();
  
  wifiState.wifiConnected = connectToWiFi();
  wifiState.useNTP = wifiState.wifiConnected;

  if (wifiState.wifiConnected) {
    syncTimeWithNTP();
  }
  
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
  
  readSensors();
  updateDisplays();
}

// ========== ОБРАБОТЧИК ПРЕРЫВАНИЯ ==========
void IRAM_ATTR buttonISR() {
  unsigned long currentTime = millis();
  if (currentTime - lastButtonPress > 250) { // Антидребезг 250мс
    buttonPressed = true;
    lastButtonPress = currentTime;
  }
}

// ========== ОБРАБОТКА НАЖАТИЯ ==========
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

// ========== ОСТАЛЬНЫЕ ФУНКЦИИ ==========
DateTime getCurrentTime() {
  if (wifiState.useNTP && wifiState.wifiConnected) {
    timeClient.update();
    return DateTime(timeClient.getEpochTime());
  }
  return rtc.now();
}

bool connectToWiFi() {
  if (wifiNetworksCount == 0) return false;
  for (int i = 0; i < wifiNetworksCount; i++) {
    if (strlen(wifiNetworks[i].ssid) == 0) continue;
    Serial.printf("Попытка подключения к %s\n", wifiNetworks[i].ssid);
    WiFi.begin(wifiNetworks[i].ssid, wifiNetworks[i].password);
    int attempts = 0;
    while (attempts < 20) {
      if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("Успешно подключено к %s\n", wifiNetworks[i].ssid);
        timeClient.begin();
        return true;
      }
      delay(500);
      Serial.print(".");
      attempts++;
    }
    Serial.println("\nНе удалось подключиться");
    WiFi.disconnect();
    delay(1000);
  }
  Serial.println("Не удалось подключиться ни к одной сети");
  return false;
}

void syncTimeWithNTP() {
  if (wifiState.wifiConnected && timeClient.update()) {
    DateTime ntpTime(timeClient.getEpochTime());
    rtc.adjust(ntpTime);
    Serial.println("Time synchronized with NTP");
  }
}

void setupPins() {
  pinMode(PIN_LASER, OUTPUT);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  digitalWrite(PIN_LASER, HIGH);
}

void setupI2C() {
  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(100000);
  pinMode(PIN_SDA, INPUT_PULLUP);
  pinMode(PIN_SCL, INPUT_PULLUP);
  delay(100);
}

void setupDisplays() {
  if(!display2.begin(SSD1306_SWITCHCAPVCC, SCREEN2_ADDR)) {
    Serial.println("Display2 не найден!");
    while(1);
  }
  display2.clearDisplay();
  display2.setTextSize(1);
  display2.setTextColor(WHITE);
  display2.setCursor(32,16);
  display2.println("Made in Dimasik");
  display2.setCursor(72,24);
  display2.println("from FAKI");
  display2.display();

  if(!display1.begin(SSD1306_SWITCHCAPVCC, SCREEN1_ADDR)) {
    Serial.println("Display1 не найден!");
    while(1);
  }
  display1.clearDisplay();
  display1.setTextSize(2);
  display1.setTextColor(WHITE);
  display1.setCursor(8,16);
  display1.println("CH Watch");
  display1.setCursor(8,32);
  display1.println("WEll Cum");
  display1.display();
}

void setupSensors() {
  imu.initialize(); 
  Serial.println(imu.testConnection() ? "MPU6050 OK" : "MPU6050 fail");
  
  if(!rtc.begin()) {
    Serial.println("RTC не инициализирован");
    for(;;);
  }
  
  Wire.beginTransmission(0x29);
  if (Wire.endTransmission() == 0) {
    if (!tofSensor.init()) {
      Serial.println("Ошибка инициализации VL53L1X!");
      while (1);
    }
    tofSensor.setAddress(0x29);
    tofSensor.setDistanceMode(VL53L1X::Long);
    tofSensor.setMeasurementTimingBudget(50000);
    tofSensor.startContinuous(50);
    Serial.println("VL53L1X инициализирован");
  }
  
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  attachInterrupt(digitalPinToInterrupt(PIN_BUTTON), buttonISR, FALLING);
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