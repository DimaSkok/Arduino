#include <RH_ASK.h>
#include <SPI.h> // Включаем SPI

RH_ASK driver; // Создаем экземпляр класса для ASK модуляции

void setup() {
  Serial.begin(9600);
  if (!driver.init())
    Serial.println("init failed");
}

void loop() {
  uint8_t buf[RH_ASK_MAX_MESSAGE_LEN]; // Буфер для принятого сообщения
  uint8_t len = sizeof(buf);

  if (driver.recv(buf, &len)) { // Если пришло сообщение
    Serial.print("Received: ");
    Serial.println((char*)buf); // Выводим сообщение в Serial Monitor
  }
}