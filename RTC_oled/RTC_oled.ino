#include <SoftWire.h>
#include <RTClib.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // Ширина OLED экрана, в пикселях
#define SCREEN_HEIGHT 32  // Высота OLED экрана, в пикселях
#define OLED_RESET    -1

#define RTC_SDA A2  
#define RTC_SCL A3

SoftWire swire(RTC_SDA, RTC_SCL);
RTC_DS1307 rtc;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

char daysOfTheWeek[7][12] = {
  "Su",
  "Mo",
  "Tu",
  "We",
  "Th",
  "Fr",
  "Sa"
};

void setup () {
  Serial.begin(9600);

  swire.begin();
  swire.setTimeout(100);
  // SETUP RTC MODULE
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1);
  }

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Адрес 0x3D для некоторых экранов
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("Initializing...");
  display.display();

  // automatically sets the RTC to the date & time on PC this sketch was compiled
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  // manually sets the RTC with an explicit date & time, for example to set
  // January 21, 2021 at 3am you would call:
  // rtc.adjust(DateTime(2021, 1, 21, 3, 0, 0));
}

void loop () {
  DateTime now = rtc.now();

  // Вывод данных на OLED экран
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);

  display.print(now.hour(), DEC);
  display.print(':');
  display.print(now.minute(), DEC);
  display.print(':');
  display.print(now.second(), DEC);
  display.print('|');
  display.println(daysOfTheWeek[now.dayOfTheWeek()][0]);
  
  display.print(now.day(), DEC);
  display.print('/');
  display.print(now.month(), DEC);
  display.print('/');
  display.print(now.year(), DEC);
  display.print('|');
  display.println(daysOfTheWeek[now.dayOfTheWeek()][1]);

  
  display.display();

  delay(100);
}
