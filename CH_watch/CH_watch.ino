#include <Wire.h>
#include <U8g2lib.h>
#include <MPU6050.h>
#include "RTClib.h"
#include "Math.h"

// Конфигурация пинов
#define TRIGGER_PIN 9
#define ECHO_PIN 10
#define LAZER_PIN 2
#define BUTTON_PIN 3

// Облегченные дисплеи (page buffer) с указанием адресов
U8G2_SSD1306_128X64_NONAME_1_HW_I2C display1(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ SCL, /* data=*/ SDA); // Адрес по умолчанию 0x3C
U8G2_SSD1306_128X32_UNIVISION_1_HW_I2C display2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ SCL, /* data=*/ SDA); // Адрес по умолчанию 0x3C

MPU6050 mpu(0x69);
RTC_DS1307 rtc;

int16_t ax, ay, az;
// Минимальная структура данных
struct {
  uint8_t flag : 2;
  uint8_t step : 1;
  struct {
    uint8_t dist;
    float grad;
  } points[2];
} data;

// Дни недели в PROGMEM
const char days[7][3] PROGMEM = {"Su","Mo","Tu","We","Th","Fr","Sa"};

const uint8_t ORIGIN_X = 128 / 2 + 30;
const uint8_t ORIGIN_Y = 40;

// Параметры осей
const uint8_t AXIS_LENGTH = 20;    // Длина оси в пикселях

// Структура для хранения углов
struct Angles {
  float roll;
  float pitch;
  float yaw;
};

// Оптимизированная функция преобразования
inline float gyroToRadians(int16_t gyroRaw) {
  return gyroRaw * (PI / (180.0f * 131.0f));  // Комбинированные константы
}

// Оптимизированная функция рисования 3D крестовины
void draw3DCross(U8G2 &display, const Angles &angles) {
  // Предварительно вычисляем синусы и косинусы
  const float cosX = cosf(angles.roll);
  const float sinX = sinf(angles.roll);
  const float cosY = cosf(angles.pitch);
  const float sinY = sinf(angles.pitch);
  const float cosZ = cosf(angles.yaw);
  const float sinZ = sinf(angles.yaw);

  // Координаты концов осей (X, Y, Z)
  const float axisEnds[3][3] = {
    {AXIS_LENGTH, 0, 0},  // X
    {0, AXIS_LENGTH, 0},  // Y
    {0, 0, AXIS_LENGTH}   // Z
  };

  // Результирующие 2D координаты
  int16_t x2d[3], y2d[3];

  // Применяем повороты для каждой оси
  for (uint8_t i = 0; i < 3; i++) {
    // Поворот Z
    float x = axisEnds[i][0] * cosZ - axisEnds[i][1] * sinZ;
    float y = axisEnds[i][0] * sinZ + axisEnds[i][1] * cosZ;
    float z = axisEnds[i][2];

    // Поворот Y
    float x2 = x * cosY + z * sinY;
    z = -x * sinY + z * cosY;

    // Поворот X
    y = y * cosX - z * sinX;

    // Проекция на 2D (с инверсией Y)
    x2d[i] = ORIGIN_X + (int16_t)x2;
    y2d[i] = ORIGIN_Y - (int16_t)y;
  }

  // Рисуем оси
  const char axisLabels[3] = {'X', 'Y', 'Z'};
  
  display.setDrawColor(1); // Белый цвет
  for (uint8_t i = 0; i < 3; i++) {
    display.drawLine(ORIGIN_X, ORIGIN_Y, x2d[i], y2d[i]);
    
    // Подписи осей
    display.setFont(u8g2_font_6x10_tf);
    display.drawGlyph(x2d[i], y2d[i], axisLabels[i]);
  }
}

void setup() {
  Serial.begin(9600);
  
  // 1. Безопасная инициализация пинов
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LAZER_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  digitalWrite(LAZER_PIN, LOW);

  // 2. Инициализация I2C
  Wire.begin();
  Wire.setClock(100000);

  // 3. Минимальная инициализация дисплеев с разными адресами
  display1.setI2CAddress(0x3D * 2); // Устанавливаем адрес 0x3D для первого дисплея
  display1.begin();
  
  display2.setI2CAddress(0x3C * 2); // Устанавливаем адрес 0x3C для второго дисплея
  display2.begin();
  
  showStartupScreen();

  // 4. Инициализация MPU6050
  mpu.initialize(); 
  Serial.println(mpu.testConnection() ? "MPU6050 OK" : "MPU6050 fail");

  // 5. Инициализация RTC
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1);
  }
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  data.flag = 0;
  digitalWrite(LAZER_PIN, HIGH);
  Serial.println("All started");
}

void loop() {
  static uint32_t timer = 0;
  if(millis() - timer < 100) return; // Цикл 10 Гц
  timer = millis();

  // 1. Обновление данных
  mpu.getAcceleration(&ax, &ay, &az);
  handleButton();
  
  // 2. Постраничный вывод на дисплеи
  updateDisplays();
}

void handleButton() {
  if(digitalRead(BUTTON_PIN)) {
    data.flag += 1;
    if (data.flag >= 3) {data.flag = 0;};

    if(data.flag == 1) { // Измерение
      digitalWrite(LAZER_PIN, HIGH);
      triggerMeasurement();
      data.points[data.step].dist = pulseIn(ECHO_PIN, HIGH) / 58.2;
      data.points[data.step].grad = ax * 3.14f / 32768.0f;
    }
    else if(data.flag == 2) { // Смена точки
      digitalWrite(LAZER_PIN, LOW);
      data.step += 1;
      if (data.step >= 2) {data.step = 0;};
    }
    

  }
}

void triggerMeasurement() {
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);
}

void updateDisplays() {
  DateTime now = rtc.now();
  
  // Дисплей 1 (акселерометр + расстояние)
  display1.firstPage();
  do {
    display1.setFont(u8g2_font_8bitclassic_tr);
    
 /*
    // Акселерометр (только X)
    display1.setCursor(0, 10);
    display1.print(F("X:"));
    display1.print(ax / 32768.0f);
    display1.print(" ");
    display1.print(ay / 32768.0f);
*/
    // Расстояние
    display1.setCursor(0, 10);
    display1.print(F("Point:"));
    display1.print(data.points[0].dist);
    display1.print(" ");
    display1.print(data.points[1].dist);
    display1.print(F("cm"));
/*
    display1.setCursor(0, 40);
    display1.print(F("Pionts:"));
    display1.print(data.points[0].grad);
    display1.print(" ");
    display1.print(data.points[1].grad);
    display1.print(F("cm"));
*/
    display1.setCursor(0, 25);
    display1.print(F("Dist:"));
    int dist = sqrt(data.points[1].dist*data.points[1].dist + data.points[0].dist*data.points[0].dist - 2*data.points[1].dist*data.points[0].dist*cos(data.points[0].grad - data.points[1].grad));
    display1.print(dist); 
    display1.print(F("cm"));

    Angles currentAngles = {
    gyroToRadians(ax),
    gyroToRadians(ay),
    gyroToRadians(az)
    };

    draw3DCross(display1, currentAngles);

  } while(display1.nextPage());

  // Дисплей 2 (дата + день недели)
  display2.firstPage();
  do {
    display2.setFont(u8g2_font_8bitclassic_tr);
    
    // Дата
    char buf[12];
    snprintf_P(buf, sizeof(buf), PSTR("%02d/%02d/%04d"), now.day(), now.month(), now.year());
    display2.drawStr(0, 25, buf);
    
    // День недели из PROGMEM
    char dayBuf[3];
    strncpy_P(dayBuf, days[now.dayOfTheWeek()], 3);
    display2.drawStr(90, 10, dayBuf);

    snprintf_P(buf, sizeof(buf), PSTR("%02d:%02d:%02d"), now.hour(), now.minute(), now.second());;
    display2.drawStr(0, 10, buf);
  } while(display2.nextPage());
}

void showStartupScreen() {
  display1.firstPage();
  do {
    display1.setFont(u8g2_font_8bitclassic_tr);
    display1.drawStr(0, 20, "Presents...");
  } while(display1.nextPage());

  display2.firstPage();
  do {
    display2.setFont(u8g2_font_8bitclassic_tr);
    display2.drawStr(0, 10, "Dimasik");
    display2.drawStr(0, 25, "Entertaiment");
  } while(display2.nextPage());
  
  delay(1500);
}