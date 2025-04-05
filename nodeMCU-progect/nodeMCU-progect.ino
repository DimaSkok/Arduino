#include <Wire.h>
#include <U8g2lib.h>
#include <MPU6050_light.h>
#include <RTClib.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

// Конфигурация WiFi
const char* ssid = "";
const char* password = "";

// Безопасные пины для NodeMCU
#define TRIGGER_PIN D5    // GPIO14
#define ECHO_PIN D6       // GPIO12
#define LAZER D7          // GPIO13
#define BUTTON_PIN D3     // GPIO0 (нужен INPUT_PULLUP)

U8G2_SSD1306_128X64_NONAME_F_HW_I2C display1(U8G2_R0);
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C display2(U8G2_R0);
MPU6050 mpu(Wire);
RTC_DS1307 rtc;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 10800);

struct {
  bool useNTP = false;
  bool wifiConnected = false;
  uint8_t flag_len = 0;
  struct {
    int distance = 0;
    float ax = 0;
  } points[2];
} state;

void safePinSetup() {
  // Настройка пинов в безопасном режиме перед инициализацией
  pinMode(TRIGGER_PIN, INPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LAZER, INPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP); // Важно для GPIO0
  
  // Дополнительная задержка для стабилизации
  delay(100);
}

void initializeDisplays() {
  // Инициализация с проверкой ошибок
  for (int i = 0; i < 3; i++) {
    if (display1.begin() && display2.begin()) {
      display1.clearDisplay();
      display2.clearDisplay();
      return;
    }
    delay(100);
  }
  Serial.println("Display init failed");
}

void initializeRTC() {
  if (rtc.begin()) {
    if (!rtc.isrunning()) {
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
    Serial.println("RTC OK");
  } else {
    Serial.println("RTC not found");
  }
}

bool tryConnectWiFi() {
  if (strlen(ssid) == 0) return false;
  
  WiFi.begin(ssid, password);
  for (int i = 0; i < 10; i++) {
    if (WiFi.status() == WL_CONNECTED) {
      timeClient.begin();
      timeClient.update();
      return true;
    }
    delay(500);
  }
  return false;
}

void setup() {
  // 1. Безопасная настройка пинов
  safePinSetup();
  
  // 2. Инициализация Serial с задержкой
  Serial.begin(115200);
  delay(1000);
  
  // 3. Инициализация I2C с пониженной скоростью
  Wire.begin();
  Wire.setClock(100000);
  
  // 4. Инициализация компонентов
  initializeDisplays();
  initializeRTC();
  
  // 5. Подключение к WiFi
  state.wifiConnected = tryConnectWiFi();
  state.useNTP = state.wifiConnected;
  
  // 6. Инициализация MPU6050
  mpu.begin();
  delay(500); // Важная задержка для стабилизации
  mpu.calcOffsets(true, true);
  
  // 7. Финальная настройка пинов
  pinMode(TRIGGER_PIN, OUTPUT);
  digitalWrite(TRIGGER_PIN, LOW);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LAZER, OUTPUT);
  digitalWrite(LAZER, LOW);
  
  Serial.println("System ready");
  updateDisplays();
}

void updateDisplays() {
  // Объединенная функция обновления для обоих дисплеев
  DateTime now = getCurrentTime();
  
  // Display 1
  display1.clearBuffer();
  char buf[20];
  snprintf(buf, sizeof(buf), "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
  display1.setFont(u8g2_font_6x10_tr);
  display1.drawStr(0, 10, buf);
  snprintf(buf, sizeof(buf), "X:%.2f", mpu.getAccX());
  display1.drawStr(0, 25, buf);
  snprintf(buf, sizeof(buf), "Dist:%dcm", state.points[0].distance);
  display1.drawStr(0, 40, buf);
  display1.drawStr(90, 10, state.useNTP ? "NTP" : "RTC");
  display1.sendBuffer();
  
  // Display 2
  display2.clearBuffer();
  snprintf(buf, sizeof(buf), "%02d/%02d/%04d", now.day(), now.month(), now.year());
  display2.drawStr(0, 10, buf);
  const char* days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  display2.drawStr(0, 25, days[now.dayOfTheWeek()]);
  if (state.wifiConnected) display2.drawStr(90, 10, "WiFi");
  display2.sendBuffer();
}

void loop() {
  static uint32_t lastUpdate = 0;
  if (millis() - lastUpdate < 100) return;
  lastUpdate = millis();
  
  mpu.update();
  handleButton();
  
  if (state.wifiConnected && millis() % 3600000 == 0) {
    syncTime();
  }
  
  updateDisplays();
}

void handleButton() {
  if (digitalRead(BUTTON_PIN) == LOW) {
    state.flag_len = (state.flag_len + 1) % 3;
    
    if (state.flag_len == 1) {
      digitalWrite(LAZER, HIGH);
      triggerUltrasonic();
      state.points[0].distance = pulseIn(ECHO_PIN, HIGH) / 58;
      state.points[0].ax = mpu.getAccX();
    } 
    else if (state.flag_len == 2) {
      digitalWrite(LAZER, LOW);
    }
    delay(50); // Дебаунс кнопки
  }
}

void triggerUltrasonic() {
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);
}

DateTime getCurrentTime() {
  if (state.useNTP && state.wifiConnected) {
    timeClient.update();
    return DateTime(timeClient.getEpochTime());
  }
  return rtc.now();
}

void syncTime() {
  if (state.wifiConnected) {
    timeClient.forceUpdate();
    if (state.useNTP) {
      rtc.adjust(DateTime(timeClient.getEpochTime()));
    }
  }
}
