#include <Arduino.h>
#include <lvgl.h>
#include <ATD1.47-S3.h>

void setup() {
  Serial.begin(115200);

  Display.begin();
  Display.useLVGL();
  
  // Start User UI code
  lv_obj_t *label = lv_label_create(lv_scr_act());
  lv_label_set_text(label, "Hello, LVGL work !");
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
  // END of User UI code
}

void loop() {
  Display.loop(); // keep LVGL still work
}
