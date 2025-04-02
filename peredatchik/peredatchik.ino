#include <RH_ASK.h>
#include <SPI.h> // Включаем SPI

RH_ASK driver; // Создаем экземпляр класса для ASK модуляции

void setup() {
  Serial.begin(9600);
  if (!driver.init())
    Serial.println("init failed");
}

void loop() {
  const char *msg = "Key0!"; // Сообщение для отправки
  Serial.print("Sending: ");
  Serial.println(msg);

  driver.send((uint8_t *)msg, strlen(msg)); // Отправляем сообщение
  driver.waitPacketSent(); // Ждем окончания передачи

  delay(1000); // Отправляем сообщение каждую секунду
}