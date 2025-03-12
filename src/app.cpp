#include <Arduino.h>
#include <ESP_Panel_Library.h>
#include <lvgl.h>
#include "lvgl_port_v8.h"
#include "display.h"

#include "esp_heap_caps.h"
#include "esp_system.h"
#include "init_functions.h"

void my_print(const char *buf) {
  Serial.print(buf);
}

void lv_task(void *pvParameters) {
  while (1) {
      lv_timer_handler();
      vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void setup()
{
  String title = "LVGL porting example";

  Serial.begin(115200);
  Serial.println(title + " start");

  Serial.println("Initialize panel device");
  //xTaskCreate(lv_task, "LVGL Task", 8192, NULL, 1, NULL);
  //lv_log_register_print_cb(my_print);
  init_functions();
  init_display();

  Serial.println(title + " end");
}

void printMemoryInfo() {
    size_t freeHeap = esp_get_free_heap_size();
    size_t minFreeHeap = esp_get_minimum_free_heap_size();

    Serial.printf("Free heap: %d bytes\n", freeHeap);
    Serial.printf("Minimum free heap: %d bytes\n", minFreeHeap);
}




void loop()
{
  //printMemoryInfo();

  lv_timer_handler();
  //Serial.println("IDLE loop");
  delay(10);
}


