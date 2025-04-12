#include <Wire.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <MPU6050.h> // Библиотека для GY-521
#include <math.h>
#include <RTClib.h>

#define SCREEN_WIDTH1 128 // Ширина OLED экрана, в пикселях
#define SCREEN_HEIGHT1 64  // Высота OLED экрана, в пикселях
#define SCREEN_WIDTH2 128 // Ширина OLED экрана, в пикселях
#define SCREEN_HEIGHT2 32
#define OLED_RESET    -1  // Пин сброса (используется -1, если не подключен)
#define TRIGGER_PIN   9  // Пин TRIGGER для HC-SR04
#define ECHO_PIN      10 // Пин ECHO для HC-SR04
#define LAZER 2
#define MPU6050_ADDR 0x69

Adafruit_SSD1306 display2(SCREEN_WIDTH2, SCREEN_HEIGHT2, &Wire, OLED_RESET);
Adafruit_SSD1306 display1(SCREEN_WIDTH1, SCREEN_HEIGHT1, &Wire, OLED_RESET);
MPU6050 mpu(MPU6050_ADDR);
RTC_DS1307 rtc;

char daysOfTheWeek[7][12] = {
  "Su",
  "Mo",
  "Tu",
  "We",
  "Th",
  "Fr",
  "Sa"
};

int16_t ax, ay, az;
int16_t gx, gy, gz;

long duration;
int distance;

// Центр экрана
const int ORIGIN_X = 128 / 2 + 30;
const int ORIGIN_Y = 64 / 2 + 15;

// Размер осей
const int AXIS_LENGTH = 20; // Длина оси (в пикселях)
const int AXIS_THICKNESS = 1; // Толщина линий осей

// Переменные для хранения углов (в радианах)
float angleX = 0;
float angleY = 0;
float angleZ = 0;

// Функция для перевода "сырых" значений в радианы (или в любые другие единицы)
// Используем значения, которые вы использовали, для преобразования углов
float gyroToRadians(int16_t gyroRaw) {
  // Замените 131.0 на коэффициент масштабирования вашего гироскопа.
  // Это может быть другое значение в зависимости от настроек датчика.
  // (Например, 250, 500, 1000, или 2000 градусов/с)
  // Если у вас есть проблемы, попробуйте посмотреть примеры калибровки в библиотеке.
  return (float)gyroRaw / 131.0 * (PI / 180.0);  // Конвертируем в радианы
}

// Функция для рисования 3D-крестовины
void draw3DCross(float roll, float pitch, float yaw) {
  // Roll, Pitch, Yaw (углы Эйлера)

  // Матрица поворота (комбинация поворотов вокруг осей X, Y, Z)
  float cosX = cos(roll);
  float sinX = sin(roll);
  float cosY = cos(pitch);
  float sinY = sin(pitch);
  float cosZ = cos(yaw);
  float sinZ = sin(yaw);

  // Координаты концов осей
  // X-axis
  float x1 = AXIS_LENGTH;
  float y1 = 0;
  float z1 = 0;

  // Y-axis
  float x2 = 0;
  float y2 = AXIS_LENGTH;
  float z2 = 0;

  // Z-axis
  float x3 = 0;
  float y3 = 0;
  float z3 = AXIS_LENGTH;

  // Применяем матрицу поворота (порядок ZYX)
  // Поворот вокруг оси Z
  float x1r = x1 * cosZ - y1 * sinZ;
  float y1r = x1 * sinZ + y1 * cosZ;
  float x2r = x2 * cosZ - y2 * sinZ;
  float y2r = x2 * sinZ + y2 * cosZ;
  float x3r = x3 * cosZ - y3 * sinZ;
  float y3r = x3 * sinZ + y3 * cosZ;

  // Поворот вокруг оси Y
  float x1r2 = x1r * cosY + z1 * sinY;
  float z1r2 = -x1r * sinY + z1 * cosY;
  float x2r2 = x2r;
  float z2r2 = -x2r * sinY + z2 * cosY;
  float x3r2 = x3r * cosY + z3 * sinY;
  float z3r2 = -x3r * sinY + z3 * cosY;

  // Поворот вокруг оси X
  float y1r3 = y1r * cosX - z1r2 * sinX;
  float z1r3 = y1r * sinX + z1r2 * cosX;
  float y2r3 = y2r * cosX - z2r2 * sinX;
  float z2r3 = y2r * sinX + z2r2 * cosX;
  float y3r3 = y3r * cosX - z3r2 * sinX;
  float z3r3 = y3r * sinX + z3r2 * cosX;

  // Проекция на 2D-экран (простая ортогональная проекция)
  int x1_2d = ORIGIN_X + x1r2;
  int y1_2d = ORIGIN_Y - y1r3; // Инвертируем Y, т.к. ось Y на экране направлена вниз
  int x2_2d = ORIGIN_X + x2r2;
  int y2_2d = ORIGIN_Y - y2r3;
  int x3_2d = ORIGIN_X + x3r2;
  int y3_2d = ORIGIN_Y - y3r3;

  // Рисуем оси
  display1.drawLine(ORIGIN_X, ORIGIN_Y, x1_2d, y1_2d, WHITE); // X
  display1.drawLine(ORIGIN_X, ORIGIN_Y, x2_2d, y2_2d, WHITE); // Y
  display1.drawLine(ORIGIN_X, ORIGIN_Y, x3_2d, y3_2d, WHITE); // Z

    // Optional: Draw Labels
    display1.setTextSize(1);
    display1.setCursor(x1_2d, y1_2d);
    display1.print("X");
    display1.setCursor(x2_2d, y2_2d);
    display1.print("Y");
    display1.setCursor(x3_2d, y3_2d);
    display1.print("Z");
}

const char* Key_rad = "2EP&H&JVJE";

void setup() {
  Serial.begin(115200); 

  // Инициализация MPU6050
  Wire.begin();
  mpu.initialize(); 
  Serial.println(mpu.testConnection() ? "MPU6050 OK" : "MPU6050 fail");


  if(!display2.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Адрес 0x3D для некоторых экранов
    Serial.println(F("SSD1306 2 allocation failed"));
    for(;;);
  }
  delay(1000);

  display2.clearDisplay();
  display2.setTextSize(1);
  display2.setTextColor(WHITE);
  display2.setCursor(0,0);
  display2.println("Initializing...");
  display2.display();

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1);
  }

  if(!display1.begin(SSD1306_SWITCHCAPVCC, 0x3D)) { 
    Serial.println(F("SSD1306 1 allocation failed"));
    for(;;);
  }

  delay(1000);

  display1.clearDisplay();
  display1.setTextSize(1);
  display1.setTextColor(WHITE);
  display1.setCursor(0,0);
  display1.println("Initializing...");
  display1.display();
  
  // automatically sets the RTC to the date & time on PC this sketch was compiled
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  // Настройка пинов HC-SR04
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(LAZER, OUTPUT);

  

  delay(2000); // Подождать после инициализации
}

void loop() {
  // Считывание данных с HC-SR04
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(5);
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);

  duration = pulseIn(ECHO_PIN, HIGH);
  distance = (duration / 2) / 29.1;

  // Считывание данных с GY-521
  mpu.getAcceleration(&ax, &ay, &az);
  mpu.getRotation(&gx, &gy, &gz);

  // Расчет углов (радианы)
  // Использование библиотечной функции для преобразования "сырых" значений в угловую скорость
  // Замените gx, gy, gz на результат расчётов из библиотеки.
  // Используйте библиотеку MPU6050 или похожую, для корректного преобразования "сырых" данных
  angleX = float(sin(double(ay)/32768*3.14)) * float(cos(double(ax)/32768*3.14));  // Roll - поворот вокруг оси X
  angleY = float(sin(double(ay)/32768*3.14)) * float(sin(double(ax)/32768*3.14));  // Pitch - поворот вокруг оси Y
  angleZ = float(cos(double(ay)/32768*3.14));  // Yaw - поворот вокруг оси Z

  // Вывод данных на OLED экраны

  DateTime now = rtc.now();

  // Вывод данных на OLED экран
  display2.clearDisplay();
  display2.setTextSize(2);
  display2.setTextColor(WHITE);
  display2.setCursor(0,0);

  display2.print(now.hour(), DEC);
  display2.print(':');
  display2.print(now.minute(), DEC);
  display2.print(':');
  display2.print(now.second(), DEC);
  display2.print('|');
  display2.println(daysOfTheWeek[now.dayOfTheWeek()][0]);
  
  display2.print(now.day(), DEC);
  display2.print('/');
  display2.print(now.month(), DEC);
  display2.print('/');
  display2.print(now.year(), DEC);
  display2.print('|');
  display2.println(daysOfTheWeek[now.dayOfTheWeek()][1]);

  
  display2.display();


  display1.clearDisplay();
  display1.setTextSize(1);
  display1.setTextColor(WHITE);
  display1.setCursor(0,0);

  display1.print("Distance: ");
  display1.print(distance);
  display1.println(" cm");
  draw3DCross(angleX, angleY, angleZ);

  display1.display();

/*
  display.print("X=");
  display.print(float(ax)/32768);
  display.print(" ");
  display.println(float(gx)/32768);
  display.print("Y=");
  display.print(float(ay)/32768);
  display.print(" ");
  display.println(float(gy)/32768);
*/
  // Рисуем 3D-крестовину
  

  digitalWrite(LAZER, LOW);
  delay(250); // Задержка между измерениями
  digitalWrite(LAZER, HIGH);
}