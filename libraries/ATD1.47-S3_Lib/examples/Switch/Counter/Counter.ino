#include <Arduino.h>
#include <ATD1.47-S3.h>
#include <demos/lv_demos.h>
#include <lvgl.h>

int count = 0;

void setup() {
  Serial.begin(115200);
  
  Switch.begin();
  Switch.onPressed(A, []() {
    count++;
    Serial.print("count = ");
    Serial.print(count);
    Serial.println();
    
  });

  /*
  Switch.onPressed(B, []() {
    // Your code do when pressed button B
  });
  Switch.onPressed(C, []() {
    // Your code do when pressed button C
  });
  */
}

void loop() {
  Switch.loop(); // keep switch still work
}
