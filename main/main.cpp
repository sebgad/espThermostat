/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "espThermostat.h"
#include "espWifi.h"


espWifi objWifi;
espThermostat objThermo;

// necessary for cpp
extern "C" {
  void app_main();
}

void app_main(void)
{

  ESP_LOGI("MainTask", "[APP] Startup..");
  ESP_LOGI("MainTask", "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
  ESP_LOGI("MainTask", "[APP] IDF version: %s", esp_get_idf_version());

  ESP_ERROR_CHECK(nvs_flash_init());
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  objWifi.Init();
  objThermo.Init();

  while(1)
  {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

}