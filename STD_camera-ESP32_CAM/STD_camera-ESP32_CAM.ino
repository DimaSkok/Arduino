#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "TP-Link_C552";
const char* password = "57174198";

WebServer server(80);

// Конфигурация камеры для OV2640
#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

void setupCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 10000000;
  
  // Используем RGB565, так как JPEG не поддерживается
  config.pixel_format = PIXFORMAT_RGB565;
  
  // Уменьшенное разрешение для RGB565
  config.frame_size = FRAMESIZE_240X240; // 320x240
  config.jpeg_quality = 0;            // Не используется для RGB565
  config.fb_count = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x\n", err);
    while(1);
  }
}

void sendRGB565AsJPEG(WiFiClient &client, camera_fb_t *fb) {
  // В реальном проекте здесь должна быть конвертация RGB565 в JPEG
  // Для примера просто отправим RAW данные с заголовком BMP
  
  // Создаем простой BMP заголовок для RGB565
  uint32_t image_size = fb->width * fb->height * 2;
  uint8_t bmp_header[54] = {
    'B', 'M',                         // Signature
    0, 0, 0, 0,                       // File size (заполнится позже)
    0, 0, 0, 0,                       // Reserved
    54, 0, 0, 0,                      // Offset to pixel array
    40, 0, 0, 0,                      // DIB header size
    fb->width & 0xFF, (fb->width >> 8) & 0xFF, 0, 0, // Width
    fb->height & 0xFF, (fb->height >> 8) & 0xFF, 0, 0, // Height
    1, 0,                             // Planes
    16, 0,                            // Bits per pixel (RGB565)
    3, 0, 0, 0,                       // Compression (BI_BITFIELDS)
    0, 0, 0, 0,                       // Image size
    0, 0, 0, 0,                       // X pixels per meter
    0, 0, 0, 0,                       // Y pixels per meter
    0, 0, 0, 0,                       // Colors in palette
    0, 0, 0, 0                        // Important colors
  };
  
  // Заполняем размер файла в заголовке
  uint32_t file_size = image_size + sizeof(bmp_header);
  bmp_header[2] = file_size & 0xFF;
  bmp_header[3] = (file_size >> 8) & 0xFF;
  bmp_header[4] = (file_size >> 16) & 0xFF;
  bmp_header[5] = (file_size >> 24) & 0xFF;

  // Отправляем заголовок
  client.write(bmp_header, sizeof(bmp_header));
  
  // Отправляем данные изображения
  client.write(fb->buf, fb->len);
}

void handleJPG() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    server.send(500, "text/plain", "Failed to capture image");
    return;
  }

  WiFiClient client = server.client();
  
  server.sendHeader("Content-Type", "image/bmp");
  server.sendHeader("Content-Length", String(fb->len + 54));
  server.send(200, "image/bmp", "");
  
  sendRGB565AsJPEG(client, fb);
  esp_camera_fb_return(fb);
}

void handleStream() {
  WiFiClient client = server.client();
  
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
  client.println("Connection: close");
  client.println();
  
  while (client.connected()) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      break;
    }
    
    client.println("--frame");
    client.println("Content-Type: image/bmp");
    client.println("Content-Length: " + String(fb->len + 54));
    client.println();
    
    sendRGB565AsJPEG(client, fb);
    client.println();
    
    esp_camera_fb_return(fb);
    delay(100); // Увеличиваем задержку для стабильности
  }
}

void handleRoot() {
  String html = "<html><head><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<style>body{margin:0;background:#000;}</style>";
  html += "</head><body>";
  html += "<img src='/stream' style='display:block;width:100%;height:auto;'>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);
  
  // Инициализация камеры
  setupCamera();
  
  // Подключение к WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  
  Serial.print("Stream URL: http://");
  Serial.println(WiFi.localIP());
  
  // Настройка маршрутов
  server.on("/", handleRoot);
  server.on("/jpg", handleJPG);
  server.on("/stream", handleStream);
  
  server.begin();
}

void loop() {
  server.handleClient();
}