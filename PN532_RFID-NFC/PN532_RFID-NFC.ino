#include <SPI.h>
#include <PN532_SPI.h>
#include <PN532.h>

PN532_SPI pn532spi(SPI, 10);  // CS подключен к пину 10
PN532 nfc(pn532spi);

void setup(void) {
  Serial.begin(115200);
  Serial.println("Starting NFC Emulation");

  nfc.begin();

  // Настройка PN532
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println("PN532 not found");
    while (1);
  }
  
  Serial.print("Found chip PN5");
  Serial.println((versiondata >> 24) & 0xFF, HEX);
  Serial.print("Firmware ver. ");
  Serial.print((versiondata >> 16) & 0xFF, DEC);
  Serial.print('.');
  Serial.println((versiondata >> 8) & 0xFF, DEC);

  // Настраиваем режим эмуляции
  nfc.SAMConfig();
  nfc.setPassiveActivationRetries(0xFF);

  Serial.println("Waiting for RFID reader...");
}

void loop() {
  uint8_t command[] = {0xD4, 0x8C, 0x00};  // Команда для инициализации цели
  uint8_t response[32];
  uint8_t responseLength = sizeof(response);

  // Инициализация в режиме цели
  if (nfc.tgInitAsTarget(command, sizeof(command), 2000) {
    Serial.println("Emulating NFC tag");

    while (1) {
      // Ожидание команды от ридера
      responseLength = sizeof(response);
      if (nfc.tgGetData(response, &responseLength)) {
        Serial.print("Received command: ");
        for (uint8_t i = 0; i < responseLength; i++) {
          Serial.print(response[i], HEX);
          Serial.print(" ");
        }
        Serial.println();
      }
      
      // Здесь можно добавить обработку конкретных команд
      delay(100);
    }
  }
  
  delay(1000);
}