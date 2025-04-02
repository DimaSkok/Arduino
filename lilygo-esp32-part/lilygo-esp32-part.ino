#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

// ----- WiFi Settings -----
const char* ssid = "TP-Link_C552";       // Replace with your WiFi SSID
const char* password = "57174198";   // Replace with your WiFi password

// ----- Web Server Settings -----
WebServer server(80); // Web server on port 80

// ----- TFT Display -----
#define TFT_CS   4
#define TFT_RST  1
#define TFT_DC   2
#define TFT_SCLK 5
#define TFT_MOSI 3

#define TFT_WIDTH  80
#define TFT_HEIGHT 160

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

// ----- System Data Structure -----
struct SystemData {
  int cpu;
  int ram;
  int disk;
  int gpu;  // Added GPU usage
};

SystemData systemData;

// ----- Function Prototypes -----
void handleRoot();
void handleData();
void updateDisplay(int cpu, int ram, int disk, int gpu);  // Added GPU argument

void setup() {
  Serial.begin(115200);

  // ----- TFT Initialization -----
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);
  tft.fillScreen(ST77XX_WHITE);
  Serial.println("TFT initialized");

  // ----- WiFi Connection -----
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected to WiFi, IP address: ");
  Serial.println(WiFi.localIP());
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_BLACK);
  tft.setCursor(0, 40);
  tft.print("Dongle IP:");
  tft.println(WiFi.localIP());

  // ----- Web Server Routes -----
  server.on("/", handleRoot);    // Handle root page
  server.on("/data", handleData); // Handle data updates
  server.begin();                // Start the server
  Serial.println("Web server started");
}

void loop() {
  server.handleClient(); // Listen for HTTP requests
}

// ----- HTTP Handlers -----

// Handle the root page (just for testing)
void handleRoot() {
  server.send(200, "text/plain", "Hello from ESP32!");
}

// Handle the /data endpoint - expects POST request with data
void handleData() {
  if (server.method() == HTTP_POST) {
    String cpuStr = server.arg("cpu");
    String ramStr = server.arg("ram");
    String diskStr = server.arg("disk");
    String gpuStr = server.arg("gpu");  // Added GPU

    systemData.cpu = cpuStr.toInt();
    systemData.ram = ramStr.toInt();
    systemData.disk = diskStr.toInt();
    systemData.gpu = gpuStr.toInt();  // Assign GPU value

    Serial.printf("Received: CPU=%d, RAM=%d, DISK=%d, GPU=%d\n", systemData.cpu, systemData.ram, systemData.disk, systemData.gpu);
    updateDisplay(systemData.cpu, systemData.ram, systemData.disk, systemData.gpu);  // Update the TFT display with the data
    server.send(200, "text/plain", "Data received");
  } else {
    server.send(405, "text/plain", "Method Not Allowed");
  }
}

// ----- Function to Update the TFT Display -----
void updateDisplay(int cpu, int ram, int disk, int gpu) { // Pass the GPU data
  tft.fillScreen(ST77XX_WHITE);

  tft.setTextSize(2);
  tft.setTextColor(ST77XX_BLACK);

  tft.setCursor(5, 30);
  tft.print("CPU: ");
  tft.print(cpu);
  tft.println("%");

  tft.setCursor(5, 90);
  tft.print("RAM: ");
  tft.print(ram);
  tft.println("%");

  tft.setCursor(5, 70);
  tft.print("DISK:");
  tft.print(disk);
  tft.println("%");

  tft.setCursor(5, 50); // Add display for GPU
  tft.print("GPU: ");
  tft.print(gpu);
  tft.println("%");
}