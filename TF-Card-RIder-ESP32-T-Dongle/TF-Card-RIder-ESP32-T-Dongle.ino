#include <Arduino.h>
#include <SdFat.h>
#include <USB.h>
#include <USBMSC.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

// SD_MMC pins (4-bit mode)
#define SDMMC_CLK  12
#define SDMMC_CMD  16
#define SDMMC_D0   14
#define SDMMC_D1   17
#define SDMMC_D2   21
#define SDMMC_D3   18

// TFT Display settings
#define TFT_CS   4
#define TFT_RST  1
#define TFT_DC   2
#define TFT_SCLK 5
#define TFT_MOSI 3

#define EJECT_BUTTON_PIN 0
const char* VENDOR_NAME = "CH-Eng";
const char* PRODUCT_NAME = "micr0-read3r";

SdFat SD;
USBMSC msc;
bool usbMounted = true;
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

// USB MSC callbacks
static int32_t onWrite(uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize) {
    return SD.card()->writeBlock(lba, buffer) ? bufsize : -1;
}

static int32_t onRead(uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize) {
    return SD.card()->readBlock(lba, (uint8_t*)buffer) ? bufsize : -1; // Явное приведение типа
}

static bool onStartStop(uint8_t power_condition, bool start, bool load_eject) {
    if (!start) {
        tft.fillScreen(ST77XX_WHITE);
        tft.setTextColor(ST77XX_BLACK);
        tft.setCursor(8, 40);
        tft.print("Eject Requested");
        usbMounted = false;
    }
    return true;
}

void safelyEject() {
    if (!usbMounted) return;
    
    tft.fillScreen(ST77XX_WHITE);
    tft.setTextColor(ST77XX_BLACK);
    tft.setCursor(8, 40);
    tft.print("Ejecting...");
    
    msc.mediaPresent(false);
    delay(1000);
    
    msc.end();
    usbMounted = false;
    tft.setCursor(8, 50);
    tft.print("Safe to remove");
}

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);
    pinMode(EJECT_BUTTON_PIN, INPUT_PULLUP);

    // Initialize display
    tft.initR(INITR_BLACKTAB);
    tft.setRotation(1);
    tft.fillScreen(ST77XX_WHITE);
    tft.setTextSize(1);
    tft.setTextColor(ST77XX_BLACK);
    tft.setCursor(8, 30);
    tft.print("Initializing...");

    // Initialize SD card with reduced speed if needed
    while (!SD.begin(SdioConfig(FIFO_SDIO))) {
        tft.setCursor(8, 40);
        tft.print("SD Init Failed!");
        delay(1000);
        tft.setCursor(8, 50);
        tft.print("Retrying...");
        delay(1000);
    }

    // Get card info
    uint64_t cardSize = SD.card()->cardSize() / 2048; // Convert to MB (sectors * 512 / 1048576)
    tft.fillScreen(ST77XX_WHITE);
    tft.setCursor(8, 20);
    tft.print("Card Detected");
    
    tft.setCursor(8, 30);
    if (cardSize <= 2000) {
        tft.print("SDSC (<2GB)");
    } else if (cardSize <= 32000) {
        tft.print("SDHC (2-32GB)");
    } else {
        tft.print("SDXC (>32GB)");
    }

    tft.setCursor(8, 40);
    tft.printf("Size: %llu MB", cardSize);
    tft.setCursor(8, 50);
    tft.printf("FS: %s", (cardSize > 32000) ? "exFAT" : "FAT32");

    // Configure USB MSC
    msc.vendorID(VENDOR_NAME);
    msc.productID(PRODUCT_NAME);
    msc.productRevision("1.0");
    msc.onRead(onRead);
    msc.onWrite(onWrite);
    msc.onStartStop(onStartStop);
    msc.mediaPresent(true);
    msc.begin(SD.card()->cardSize(), 512);

    USB.begin();
    tft.setCursor(8, 60);
    tft.print("USB MSC Ready");
}

void loop() {
    static unsigned long lastPress = 0;
    if (digitalRead(EJECT_BUTTON_PIN) == LOW && millis() - lastPress > 50) {
        lastPress = millis();
        safelyEject();
        while (digitalRead(EJECT_BUTTON_PIN) == LOW) delay(10);
    }
    delay(10);
}