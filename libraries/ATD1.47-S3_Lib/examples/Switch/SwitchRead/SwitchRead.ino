#include <Arduino.h>
#include <ATD1.47-S3.h>

void setup() {
  Serial.begin(115200);
  
  Switch.begin();
}

void loop() {
  Switch.loop(); // keep switch still work
  bool A_value = Switch.isPressed(A);
  bool B_value = Switch.isPressed(B);
  bool C_value = Switch.isPressed(C);
  Serial.printf("Switch A: %d, B: %d, C: %d\n", A_value, B_value, C_value);
  delay(10);
}
