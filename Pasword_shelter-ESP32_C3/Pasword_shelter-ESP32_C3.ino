#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SD.h>
#include <SPI.h>

// Пины для SPI (подключение SD-карты)
#define SCK  6
#define MISO 2
#define MOSI 7
#define CS   5

// Пины для I2C (OLED-дисплей)
#define SDA_PIN 9
#define SCL_PIN 8

// Пины для кнопок
#define BUTTON_UP   10
#define BUTTON_OK   20
#define BUTTON_DOWN 21

// Параметры OLED-дисплея
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Глобальные переменные
String ram_passwords[2] = {"0", "0"};
const String menu_items[5] = {"Check passwords", "Add passwords", "Delete passwords", "Check RAM", "Exit"};
int8_t current_menu = 0;
bool exit_program = false;

// Таймеры и антидребезг
const uint32_t debounce_delay = 50;
const float display_update_interval = 100; // мс
uint32_t last_debounce_time = 0;
uint32_t last_display_update = 0;

// Состояние кнопок
bool button_state[3] = {false, false, false}; // UP, OK, DOWN

bool initSD() {
  if (!SD.begin(CS, SPI, 4000000)) {
    Serial.println("SD card initialization failed!");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("SD card failed!");
    display.display();
    return false;
  }
  Serial.println("SD card initialized.");
  return true;
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 5000); // Ждём Serial, но не бесконечно

  // Инициализация SPI
  SPI.begin(SCK, MISO, MOSI, CS);

  // Инициализация I2C
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000); // 400 кГц для ESP32

  // Настройка пинов кнопок
  pinMode(BUTTON_UP, INPUT_PULLUP);
  pinMode(BUTTON_OK, INPUT_PULLUP);
  pinMode(BUTTON_DOWN, INPUT_PULLUP);

  // Инициализация OLED-дисплея
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println(F("SSD1306 initialization failed"));
    for(;;); // Остановка
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Passwd shelter");
  display.display();
  delay(2000);

  // Инициализация SD-карты
  if (!initSD()) {
    for(;;); // Остановка при ошибке SD
  }

  showMenu(menu_items[current_menu]);
}

void showMenu(const String& message) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(message);
  display.display();
  Serial.println("Menu: " + message);
}

String generatePassword(uint8_t length) {
  String password = "";
  char last_char = 0;

  for (uint8_t i = 0; i < length; i++) {
    char new_char;
    do {
      uint8_t type = random(0, 3);
      if (type == 0) new_char = random(35, 38);   // #, $, %
      else if (type == 1) new_char = random(48, 58); // 0-9
      else new_char = random(65, 91);            // A-Z
    } while (new_char == last_char);

    password += new_char;
    last_char = new_char;
  }
  return password;
}

void writePassword(const String& name, const String& password) {
  File file = SD.open("/Shelter.txt", FILE_APPEND);
  if (file) {
    file.println(name);
    file.println(password);
    file.close();
    Serial.println("Data written: " + name + ", " + password);
  } else {
    Serial.println("Error opening Shelter.txt for write");
  }
}

void readPassword(int index, String& name, String& password) {
  name = "";
  password = "";
  File file = SD.open("/Shelter.txt", FILE_READ);
  if (file) {
    int line = 0;
    while (file.available() && line < index * 2) {
      file.readStringUntil('\n');
      line++;
    }
    if (file.available()) {
      name = file.readStringUntil('\n');
      name.trim();
    }
    if (file.available()) {
      password = file.readStringUntil('\n');
      password.trim();
    }
    file.close();
    Serial.println("Read: " + name + ", " + password);
  } else {
    Serial.println("Error opening Shelter.txt for read");
  }
}

void deletePassword(int index) {
  File temp = SD.open("/Temp.txt", FILE_WRITE);
  File file = SD.open("/Shelter.txt", FILE_READ);
  if (file && temp) {
    int line = 0;
    while (file.available()) {
      String data = file.readStringUntil('\n');
      if (line != index * 2 && line != index * 2 + 1) {
        temp.println(data);
      }
      line++;
    }
    file.close();
    temp.close();
    SD.remove("/Shelter.txt");
    SD.rename("/Temp.txt", "/Shelter.txt");
    Serial.println("Password deleted at index: " + String(index));
  } else {
    Serial.println("Error processing files for delete");
  }
}

bool readButtons() {
  bool changed = false;
  bool new_state[3] = {
    digitalRead(BUTTON_UP) == LOW,
    digitalRead(BUTTON_OK) == LOW,
    digitalRead(BUTTON_DOWN) == LOW
  };

  if (millis() - last_debounce_time > debounce_delay) {
    for (int i = 0; i < 3; i++) {
      if (new_state[i] != button_state[i]) {
        button_state[i] = new_state[i];
        changed = true;
      }
    }
    last_debounce_time = millis();
  }

  if (button_state[0]) current_menu--; // Вверх
  if (button_state[2]) current_menu++; // Вниз
  if (current_menu < 0) current_menu = 4;
  if (current_menu > 4) current_menu = 0;

  return changed || button_state[0] || button_state[1] || button_state[2];
}

void checkPasswords() {
  int index = 0;
  int total_passwords = 0;
  String name, password;

  // Подсчёт паролей
  while (true) {
    readPassword(total_passwords, name, password);
    if (name == "" && password == "") break;
    total_passwords++;
  }

  while (true) {
    readPassword(index, name, password);
    showMenu(name);
    delay(100);

    if (readButtons()) {
      if (button_state[1]) { // OK - показать пароль
        showMenu(password);
        delay(1000);
        bool in_submenu = true;
        while (in_submenu) {
          showMenu("1.Change 2.Delete 3.Back");
          delay(100);
          if (readButtons()) {
            if (button_state[1]) { // Изменить
              int length = password.length();
              bool change_length = true;
              while (change_length) {
                showMenu("Length: " + String(length));
                delay(100);
                if (readButtons()) {
                  if (button_state[0]) length--;
                  if (button_state[2]) length++;
                  if (button_state[1]) {
                    ram_passwords[0] = name;
                    ram_passwords[1] = password;
                    String new_pass = generatePassword(length);
                    writePassword(name, new_pass);
                    deletePassword(index);
                    showMenu("Changed!");
                    delay(1000);
                    change_length = false;
                    in_submenu = false;
                  }
                }
              }
            } else if (button_state[2]) { // Удалить
              deletePassword(index);
              showMenu("Deleted!");
              delay(1000);
              in_submenu = false;
            } else if (button_state[0]) { // Назад
              in_submenu = false;
            }
          }
        }
      }
      if (button_state[0]) index--;
      if (button_state[2]) index++;
      if (index < 0) index = total_passwords - 1;
      if (index >= total_passwords) index = 0;
    }
  }
}

void addPassword() {
  String name = "NewPass" + String(millis());
  String password = generatePassword(8);
  writePassword(name, password);
  showMenu("Added: " + name);
  delay(1000);
}

void deletePasswords() {
  SD.remove("/Shelter.txt");
  showMenu("All passwords deleted!");
  delay(1000);
}

void checkRAM() {
  showMenu("RAM: " + ram_passwords[0] + ", " + ram_passwords[1]);
  delay(2000);
}

void exitProgram() {
  exit_program = true;
  showMenu("App ended");
  delay(1000);
}

void loop() {
  if (exit_program) {
    while (true); // Остановка
  }

  if (readButtons() && button_state[1]) { // OK - выбор пункта меню
    switch (current_menu) {
      case 0: checkPasswords(); break;
      case 1: addPassword(); break;
      case 2: deletePasswords(); break;
      case 3: checkRAM(); break;
      case 4: exitProgram(); break;
    }
    delay(300); // Защита от многократного нажатия
  }

  if (millis() - last_display_update > display_update_interval) {
    showMenu(menu_items[current_menu]);
    last_display_update = millis();
  }
}