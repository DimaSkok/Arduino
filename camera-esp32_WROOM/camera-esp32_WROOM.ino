#include <Wire.h>
#include <driver/ledc.h>
#include <WiFi.h>
#include <WebServer.h>
#include <JPEGENC.h>

WebServer server(80);

// Разрешение кадра (QQVGA)
#define FRAME_WIDTH 240         // max 320
#define FRAME_HEIGHT 180        // max 240
uint8_t frameBuffer[FRAME_WIDTH * FRAME_HEIGHT];

// Конфигурация пинов
#define XCLK_PIN  4
#define PCLK_PIN  13
#define VSYNC_PIN 15
#define HREF_PIN  12
#define DATA_PINS {16, 17, 18, 19, 21, 22, 23, 25}
#define SIOC_PIN 26
#define SIOD_PIN 27

JPEGENC jpeg;

bool writeReg(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(0x21);
  Wire.write(reg);
  Wire.write(val);
  return Wire.endTransmission() == 0;
}

uint8_t readDataBus() {
  const uint8_t pins[] = DATA_PINS;
  return (digitalRead(pins[7]) << 7) | (digitalRead(pins[6]) << 6) | 
         (digitalRead(pins[5]) << 5) | (digitalRead(pins[4]) << 4) |
         (digitalRead(pins[3]) << 3) | (digitalRead(pins[2]) << 2) | 
         (digitalRead(pins[1]) << 1) | digitalRead(pins[0]);
}

void captureFrame() {
  while(digitalRead(VSYNC_PIN) == HIGH);
  while(digitalRead(VSYNC_PIN) == LOW);
  
  for(int y = 0; y < FRAME_HEIGHT; y++) {
    while(digitalRead(HREF_PIN) == LOW);
    
    for(int x = 0; x < FRAME_WIDTH; x++) {
      while(digitalRead(PCLK_PIN) == HIGH);
      uint8_t highByte = readDataBus();
      while(digitalRead(PCLK_PIN) == LOW);
      while(digitalRead(PCLK_PIN) == HIGH);
      uint8_t lowByte = readDataBus();
      
      frameBuffer[y * FRAME_WIDTH + x] = (highByte << 8) | lowByte;
    }
    
    while(digitalRead(HREF_PIN) == HIGH);
  }
}

void handleRoot() {
  // HTML страница с автоматическим обновлением изображения
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta http-equiv='refresh' content='1'>";
  html += "<style>body{text-align:center;}</style>";
  html += "</head><body>";
  html += "<h1>OV7670 Camera Stream</h1>";
  html += "<img src='/jpg' width='320' height='240'>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

void handleJPEG() {
  captureFrame();
  
  uint8_t *jpegBuf = (uint8_t *)malloc(FRAME_WIDTH * FRAME_HEIGHT * 2);
  if (!jpegBuf) {
    server.send(500, "text/plain", "Memory error");
    return;
  }

  int rc = jpeg.open(jpegBuf, FRAME_WIDTH * FRAME_HEIGHT * 2);
  if (rc != 0) {
    server.send(500, "text/plain", "JPEG init failed");
    free(jpegBuf);
    return;
  }

  JPEGENCODE jpe;
  rc = jpeg.encodeBegin(&jpe, FRAME_WIDTH, FRAME_HEIGHT, JPEGE_PIXEL_RGB565, JPEGE_SUBSAMPLE_420, JPEGE_Q_HIGH);
  if (rc != 0) {
    server.send(500, "text/plain", "Encode begin failed");
    jpeg.close();
    free(jpegBuf);
    return;
  }

  rc = jpeg.addFrame(&jpe, (uint8_t *)frameBuffer, FRAME_WIDTH * 2);
  if (rc != 0) {
    server.send(500, "text/plain", "Encode failed");
    jpeg.close();
    free(jpegBuf);
    return;
  }

  int jpegSize = jpeg.close();
  if (jpegSize <= 0) {
    server.send(500, "text/plain", "JPEG close failed");
    free(jpegBuf);
    return;
  }

  // Правильные HTTP заголовки для отображения в браузере
  server.send_P(200, "image/jpeg", (const char*)jpegBuf, jpegSize);
  free(jpegBuf);
}

bool configureOV7670() {
  if(!writeReg(0x12, 0x80)) return false;
  delay(100);
  
  const uint8_t config[][2] = {
    {0x12, 0x0C}, {0x11, 0xC0}, {0x3A, 0x04}, {0x40, 0xD0},
    {0x17, 0x16}, {0x18, 0x04}, {0x32, 0xA4}, {0x19, 0x03},
    {0x1A, 0x7B}, {0x03, 0x0A}, {0x0C, 0x00}, {0x3E, 0x00},
    {0x70, 0x00}, {0x71, 0x00}, {0x72, 0x11}, {0x73, 0xF1}
  };

  for(auto &reg : config) {
    if(!writeReg(reg[0], reg[1])) return false;
    delay(10);
  }
  return true;
}

void setup() {
  Serial.begin(115200);
  
  // Настройка GPIO
  const uint8_t pins[] = DATA_PINS;
  for(uint8_t i = 0; i < 8; i++) pinMode(pins[i], INPUT);
  pinMode(PCLK_PIN, INPUT);
  pinMode(VSYNC_PIN, INPUT);
  pinMode(HREF_PIN, INPUT);

  // Настройка XCLK (8MHz)
  ledc_timer_config_t timer_conf = {
    .speed_mode = LEDC_HIGH_SPEED_MODE,
    .duty_resolution = LEDC_TIMER_1_BIT,
    .timer_num = LEDC_TIMER_0,
    .freq_hz = 8000000,
    .clk_cfg = LEDC_AUTO_CLK
  };
  ledc_timer_config(&timer_conf);

  ledc_channel_config_t channel_conf = {
    .gpio_num = XCLK_PIN,
    .speed_mode = LEDC_HIGH_SPEED_MODE,
    .channel = LEDC_CHANNEL_0,
    .timer_sel = LEDC_TIMER_0,
    .duty = 1,
    .hpoint = 0
  };
  ledc_channel_config(&channel_conf);

  // Инициализация I2C
  Wire.begin(SIOD_PIN, SIOC_PIN);
  
  if(!configureOV7670()) {
    Serial.println("Camera init failed!");
    while(1);
  }

  // Подключение WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin("TP-Link_C552", "57174198");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected! IP: " + WiFi.localIP().toString());

  // Настройка сервера
  server.on("/", handleRoot);
  server.on("/jpg", handleJPEG);
  server.begin();
  Serial.println("HTTP server ready");
}

void loop() {
  server.handleClient();
  delay(10);
}