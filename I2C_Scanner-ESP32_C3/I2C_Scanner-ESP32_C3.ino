#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels
#define OLED_RESET -1     // Reset pin # (or -1 if sharing Arduino reset pin)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  Serial.begin(115200);
  while (!Serial); // Wait for serial monitor for debugging
  Wire.begin(6, 5);
  Wire.setClock(100000);
  pinMode(5, INPUT_PULLUP);
  pinMode(6, INPUT_PULLUP);
  delay(100);
  // Initialize OLED display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3D)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("I2C Scanner");
  //Serial.println("I2C Scanner");
  display.display();
  delay(2000);
  
  
  
  Serial.println("\nI2C Scanner");
}

void loop() {
  byte error, address;
  int nDevices = 0;
  
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Scanning I2C...");
  //Serial.println("Scanning I2C...");
  display.display();
  
  delay(1000);
  
  for(address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16) Serial.print("0");
      Serial.print(address, HEX);
      Serial.println(" !");
      
      display.print("Found: 0x");
      if (address < 16) display.print("0");
      display.println(address, HEX);
      nDevices++;
    }
    else if (error == 4) {
      Serial.print("Unknown error at address 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
    }    
  }
  
  if (nDevices == 0) {
    Serial.println("No I2C devices found\n");
    display.println("No devices found");
  }
  else {
    Serial.println("done\n");
  }
  
  display.display();
  delay(5000); // Wait 5 seconds before next scan
}