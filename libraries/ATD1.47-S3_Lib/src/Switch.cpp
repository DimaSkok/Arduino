#include "Switch.h"
#include <Arduino.h>

static uint8_t switch_pin[] = { 4, 5, 45 };

SwitchClass::SwitchClass() {
}

void SwitchClass::begin() {
  for (uint8_t i=0;i<sizeof(switch_pin);i++) {
    pinMode(switch_pin[i], INPUT_PULLUP);
  }
}

void SwitchClass::onPressed(SwitchNum sw, SwitchCallback cb) {
  this->cb[(int)sw].pressed_cb = cb;
}
void SwitchClass::onRelease(SwitchNum sw, SwitchCallback cb) {
  this->cb[(int)sw].release_cb = cb;
}

bool SwitchClass::isPressed(SwitchNum sw) {
  return state[(int)sw] == 2;
}

bool SwitchClass::isRelease(SwitchNum sw) {
  return state[(int)sw] == 0;
}

void SwitchClass::loop() {
  static unsigned long timer = 0;
  if ((millis() < timer) || (timer == 0) || ((millis() - timer) >= 10)) {
    for (uint8_t i=0;i<sizeof(switch_pin);i++) {
      if (state[i] == 0) { // Check first press
        if (digitalRead(switch_pin[i]) == 0) { // if press
          state[i] = 1;
        }
      } else if (state[i] == 1) { // press debounce
        if (digitalRead(switch_pin[i]) == 0) { // if press
          if (this->cb[i].pressed_cb) {
            this->cb[i].pressed_cb();
          }
          state[i] = 2;
        } else {
          state[i] = 0;
        }
      } else if (state[i] == 2) { // Check first release
        if (digitalRead(switch_pin[i]) == 1) { // if release
          state[i] = 3;
        }
      } else if (state[i] == 3) { // release debounce
        if (digitalRead(switch_pin[i]) == 1) { // if release
          if (this->cb[i].release_cb) {
            this->cb[i].release_cb();
          }
          state[i] = 0;
        } else {
          state[i] = 2;
        }
      }
    }
  }
}

SwitchClass Switch;
