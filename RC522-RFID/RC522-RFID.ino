#include <SPI.h>
#include <MFRC522.h>

// Define the pins used by the SPI interface
#define SS_PIN  10  // Slave Select pin
#define RST_PIN 9  // Reset pin

// Create an instance of the MFRC522 class
MFRC522 mfrc522(SS_PIN, RST_PIN);  // MFRC522 instance

void setup() {
  Serial.begin(115200);  // Initialize serial communications with the PC
  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522 card

  Serial.println("Scan PICC to see UID, SAK, type, and data blocks...");
}

void loop() {
  // Look for new cards
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Select one of the cards
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  // Show some details of the PICC (that is, the RFID card)
  Serial.print(F("Card UID:"));
  dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
  Serial.println();

  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  Serial.println(mfrc522.PICC_GetTypeName(piccType));

  // Authenticate using key A for all sectors
  MFRC522::MIFARE_Key key;
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;  // Default key (0xFF 0xFF 0xFF 0xFF 0xFF 0xFF)

  Serial.println(F("Reading data from block:"));
  byte buffer[18];
  byte block;
  byte sector = 0;
  byte trailerBlock = 3;

  // Loop through each sector and block
  for (sector = 0; sector < 16; sector++) {
    trailerBlock = sector * 4 + 3; // Calculate the trailer block

    //Authenticate sector (Попробуем использовать PCD_MFAuthent)
    MFRC522::StatusCode status = (MFRC522::StatusCode)mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));

    if (status != MFRC522::STATUS_OK) {
    //Если не найдена, попробуем команду 0x60
      Serial.print(F("Authentication failed for sector "));
      Serial.print(sector);
      Serial.print(F(": "));
      Serial.println(mfrc522.GetStatusCodeName(status));
      continue;
    }

    Serial.print(F("Sector "));
    Serial.println(sector);

    // Read each block within the sector
    for (block = sector * 4; block < trailerBlock; block++) {
      byte bufferSize = sizeof(buffer);

      // Read the block
      status = (MFRC522::StatusCode)mfrc522.MIFARE_Read(block, buffer, &bufferSize);
      if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Read() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        continue;
      }

      Serial.print(F("  Block "));
      if (block < 10) Serial.print(F(" ")); // Add zero if needed
      Serial.print(block);
      Serial.print(F(":"));

      // Dump the data
      dump_byte_array(buffer, 16); // MIFARE Classic 1K has 16 bytes per block
      Serial.println();
    }
  }

  // Halt PICC
  mfrc522.PICC_HaltA();

  // Stop encryption on PCD
  mfrc522.PCD_StopCrypto1();

  delay(1000); // Short delay before next scan
}

/**
 * Helper routine to dump a byte array as hex values to Serial
 */
void dump_byte_array(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}