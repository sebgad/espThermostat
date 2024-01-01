/*
*/
#ifndef ESPTHERMOSTAT_H
#define ESPTHERMOSTAT_H

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "sdkconfig.h"
#include "esp_netif_sntp.h"
#include "esp_sntp.h"
#include <time.h>
#include <sys/time.h>
#include "cJSON.h"

#define ESP_EQUIVA_GPIO_ENC_PIN_1 CONFIG_ESP_EQUIVA_N_GPIO_ENC_1
#define ESP_EQUIVA_GPIO_ENC_PIN_2 CONFIG_ESP_EQUIVA_N_GPIO_ENC_2
#define ESP_EQUIVA_GPIO_SLEEP_PIN CONFIG_ESP_EQUIVA_N_GPIO_SLEEP
#define ESP_EQUIVA_MQTT_URI CONFIG_ESP_EQUIVA_MQTT_BROKER_ADDR
#define ESP_EQUIVA_MQTT_CLIENT_USERNAME CONFIG_ESP_EQUIVA_MQTT_USERNAME
#define ESP_EQUIVA_MQTT_CLIENT_PASSWORD CONFIG_ESP_EQUIVA_MQTT_PASSWORD
#define ESP_EQUIVA_MQTT_SENSOR_NODE "tele/" CONFIG_ESP_EQUIVA_DEV_NAME "/SENSOR"
#define ESP_EQUIVA_MQTT_STATE_NODE "tele/" CONFIG_ESP_EQUIVA_DEV_NAME "/STATE"
#define ESP_EQUIVA_MQTT_LWT_NODE "tele/" CONFIG_ESP_EQUIVA_DEV_NAME "/LWT"
#define ESP_EQUIVA_MQTT_CMD_TEMPSET "cmnd/" CONFIG_ESP_EQUIVA_DEV_NAME "/TEMPSET"
#define ESP_EQUIVA_MQTT_CMD_HEATING "cmnd/" CONFIG_ESP_EQUIVA_DEV_NAME "/POWER1"
#define ESP_EQUIVA_MQTT_CMD_HEAT_OFF "OFF"
#define ESP_EQUIVA_MQTT_CMD_HEAT_ON  "ON"
#define ESP_EQUIVA_ENC_SIGNAL_HOLD_MS 1000
#define ESP_EQUIVA_TEMP_STEP_ROT_MULT 1
#define ESP_EQUIVA_TEMP_STEP_ROT_DIV 1
#define ESP_EQUIVA_TEMP_MIN_VALUE 18
#define ESP_EQUIVA_LOG_TAG "Thermostat"

class espThermostat
{
   private:
      static esp_mqtt_client_handle_t _mqtt_client_hdl;
      static void _init(void);
      esp_err_t _init_thermostat(void);
      static esp_err_t _init_mqtt_connection(void);
      static esp_err_t _init_time(void);
      static void _get_time_string(char [], size_t);
      static void _mqtt_event_handler(void *, esp_event_base_t , int32_t , void *);
      esp_err_t _reset_temperature(void);
      esp_err_t _simulate_enc_rot(uint16_t, bool);
      static char * _dump_heat_state();
      static char * _dump_measurement();
   public:
      static int TempSet;
      static int Temp;
      static bool PowerOn;
      espThermostat(void);
      void Init(void);
      esp_err_t set_temperature(float);
};

#endif
