#include <Arduino.h>
#include <ATD1.47-S3.h>

void setup() {
  Serial.begin(115200);
  Display.begin();
}

void loop() {
  for (int i=0;i<20;i++) {
    uint16_t color = random(0x0000, 0xFFFF);
    Display.fillScreen(color);
  }

  Display.fillScreen(0x0000);
  for (int i=0;i<20;i++) {
    int size = random(10, 50);
    int x = random(size, 320 - size);
    int y = random(size, 172 - size);
    uint16_t color = random(0x0000, 0xFFFF);
    Display.fillCircle(x, y, size, color);
  }

  Display.fillScreen(0x0000);
  for (int i=0;i<20;i++) {
    int x1 = random(0, 320);
    int y1 = random(0, 172);
    int x2 = random(x1, 320 - x1);
    int y2 = random(y1, 172 - y1);
    uint16_t color = random(0x0000, 0xFFFF);
    Display.fillRect(x1, y1, x2, y2, color);
  }

  Display.fillScreen(0x0000);
  for (int i=0;i<20;i++) {
    int w = random(10, 50);
    int h = random(10, 50);
    int xc = random(0, 320 - w);
    int yc = random(0, 172 - h);
    int angle = random(0, 359);
    uint16_t color = random(0x0000, 0xFFFF);
    Display.drawTriangle(xc, yc, w, h, angle, color);
  }
}
