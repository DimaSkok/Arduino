
void setup() {
  Serial.begin(115200);
}

void loop() {
  float temp_celsius = temperatureRead();

  Serial.print("Temp onBoard ");
  Serial.print(temp_celsius);
  Serial.println("°C");

  delay(1000);
}