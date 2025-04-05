#include <Wire.h>
#include <RTClib.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // Ширина OLED экрана, в пикселях
#define SCREEN_HEIGHT 32  // Высота OLED экрана, в пикселях
#define OLED_RESET    -1

RTC_DS1307 rtc;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

char daysOfTheWeek[7][12] = {
  "Bc",
  "Pn",
  "Bt",
  "Cp",
  "Ch",
  "Pt",
  "Cb"
};

void setup () {
  Serial.begin(9600);

  Wire.begin();

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1);
  }

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
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
  display.setCursor(104,0);
  display.print('|');
  display.println(daysOfTheWeek[now.dayOfTheWeek()][0]);
  
  display.print(now.day(), DEC);
  display.print('/');
  display.print(now.month(), DEC);
  display.print('/');
  display.print(now.year() % 100, DEC);
  display.setCursor(104,16);
  display.print('|');
  display.println(daysOfTheWeek[now.dayOfTheWeek()][1]);

  
  display.display();

  delay(100);
}
