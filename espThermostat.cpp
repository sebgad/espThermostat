/*
*/

#include "espThermostat.h"

int espThermostat::TempSet = 0;
int espThermostat::Temp = 0;
bool espThermostat::PowerOn = false;
esp_mqtt_client_handle_t espThermostat::_mqtt_client_hdl;

espThermostat::espThermostat()
{
}

void espThermostat::Init(void)
{
    /* Initialize NTP Server */
    ESP_ERROR_CHECK_WITHOUT_ABORT(_init_time());
    char strftime_buf[64];
    _get_time_string(strftime_buf, 64);
    ESP_LOGI(ESP_EQUIVA_LOG_TAG, "The current date/time is: %s", strftime_buf);

    /* Initialize MQTT connection */
    ESP_ERROR_CHECK_WITHOUT_ABORT(_init_mqtt_connection());

    /* Initialize Thermostat */
    ESP_ERROR_CHECK_WITHOUT_ABORT(_init_thermostat());
}

char * espThermostat::_dump_heat_state()
{
    char * json_out_buffer = NULL;
    cJSON * root_object = cJSON_CreateObject();
    if (PowerOn == true)
    {
        cJSON_AddStringToObject(root_object, "POWER1", "ON");
    } else {
        cJSON_AddStringToObject(root_object, "POWER1", "OFF");
    }
    json_out_buffer = cJSON_Print(root_object);
    cJSON_Delete(root_object);
    return json_out_buffer;
}

char * espThermostat::_dump_measurement(void)
{
    char * json_out_buffer = NULL;
    cJSON * root_object = cJSON_CreateObject();

    /* Create Temperature entries in Thermostat array */
    cJSON * thermostat = cJSON_CreateObject();
    cJSON_AddNumberToObject(thermostat, "Temperature", Temp);
    cJSON_AddNumberToObject(thermostat, "TempSet", TempSet);

    /* Create Entries to array */
    char timebuf[64];
    _get_time_string(timebuf, 64);
    cJSON_AddStringToObject(root_object, "Time", timebuf);
    cJSON_AddItemToObject(root_object, "Thermostat", thermostat);
    cJSON_AddStringToObject(root_object, "TempUnit", "C");

    json_out_buffer = cJSON_Print(root_object);

    cJSON_Delete(root_object);

    return json_out_buffer;
}

esp_err_t espThermostat::_init_time(void)
{
    esp_err_t return_value = ESP_OK;

    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();

    int retry = 0;
    const int retry_count = 15;

    while (esp_sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
        ESP_LOGI(ESP_EQUIVA_LOG_TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    if (retry == retry_count)
    {
        return_value = ESP_FAIL;
    }

    /* Set timezone to west european time */
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
    tzset();
    return return_value;
}

void espThermostat::_get_time_string(char buffer[], size_t buf_size)
{
    time_t now = 0;
    struct tm timeinfo = {};

    time(&now);
    localtime_r(&now, &timeinfo);
    strftime(buffer, buf_size, "%Y-%m-%dT%H:%M:%S", &timeinfo);
}

esp_err_t espThermostat::_init_thermostat(void)
{
    esp_err_t return_value = ESP_OK;

    gpio_num_t enc_pin_1, enc_pin_2;
    enc_pin_1 = (gpio_num_t)ESP_EQUIVA_GPIO_ENC_PIN_1;
    enc_pin_2 = (gpio_num_t)ESP_EQUIVA_GPIO_ENC_PIN_2;

    /* Configure GPIOs for Encoder signal */
    gpio_config_t gpio_enc_config = {};
    gpio_enc_config.pin_bit_mask = ((1ULL << ESP_EQUIVA_GPIO_ENC_PIN_1) | (1ULL << ESP_EQUIVA_GPIO_ENC_PIN_2));
    gpio_enc_config.mode = GPIO_MODE_OUTPUT;
    gpio_enc_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_enc_config.pull_up_en = GPIO_PULLUP_DISABLE;
    return_value = ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_config(&gpio_enc_config));

    if (return_value == ESP_OK) {
        return_value = ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_set_level(enc_pin_1, 0));
    }

    if (return_value == ESP_OK) {
        return_value = ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_set_level(enc_pin_2, 0));
    }
    return return_value;
}

esp_err_t espThermostat::_simulate_enc_rot(uint16_t rotations, bool inverted)
{
    esp_err_t return_value = ESP_OK;
    gpio_num_t pin_enc_1, pin_enc_2;

    if (!inverted)
    {
        pin_enc_1 = (gpio_num_t)ESP_EQUIVA_GPIO_ENC_PIN_1;
        pin_enc_2 = (gpio_num_t)ESP_EQUIVA_GPIO_ENC_PIN_2;
    } else {
        pin_enc_1 = (gpio_num_t)ESP_EQUIVA_GPIO_ENC_PIN_2;
        pin_enc_2 = (gpio_num_t)ESP_EQUIVA_GPIO_ENC_PIN_1;
    }

    /* Reset temperature via simulating encoder signal */
    for(uint16_t i=0; i<rotations; i++)
    {
        /* Connect Encoder signal 1 to ground and wait */
        ESP_ERROR_CHECK(gpio_set_level(pin_enc_1, 1));
        vTaskDelay(ESP_EQUIVA_ENC_SIGNAL_HOLD_MS / portTICK_PERIOD_MS);

        /* Encoder Signal 1 and 2 to ground and wait */
        ESP_ERROR_CHECK(gpio_set_level(pin_enc_2, 1));
        vTaskDelay(ESP_EQUIVA_ENC_SIGNAL_HOLD_MS / portTICK_PERIOD_MS);

        /* Encoder Signal 2 only to ground and wait */
        ESP_ERROR_CHECK(gpio_set_level(pin_enc_1, 0));
        vTaskDelay(ESP_EQUIVA_ENC_SIGNAL_HOLD_MS / portTICK_PERIOD_MS);

        /* No signal to ground and wait */
        ESP_ERROR_CHECK(gpio_set_level(pin_enc_2, 0));
        vTaskDelay(ESP_EQUIVA_ENC_SIGNAL_HOLD_MS / portTICK_PERIOD_MS);
    }
    return return_value;
}

esp_err_t espThermostat::_reset_temperature()
{
    esp_err_t return_value = ESP_OK;
    ESP_LOGD(ESP_EQUIVA_LOG_TAG, "Reset Thermostat");
    return_value = _simulate_enc_rot(10, true);

    return return_value;
}

esp_err_t espThermostat::set_temperature(float target_temperature)
{
    esp_err_t return_value = ESP_OK;
    int rotations;

    if (target_temperature > ESP_EQUIVA_TEMP_MIN_VALUE)
    {
        ESP_LOGI(ESP_EQUIVA_LOG_TAG, "Set Thermostat temperature to %.2f C.", target_temperature);
        /* Reset temperature to minimum temperature */
        return_value = _reset_temperature();

        if (return_value == ESP_OK)
        {
            /* Calculate number of rotations to archive target temperature */
            rotations = round((float)(target_temperature - ESP_EQUIVA_TEMP_MIN_VALUE) / ESP_EQUIVA_TEMP_STEP_ROT_MULT * ESP_EQUIVA_TEMP_STEP_ROT_DIV);
            return_value = _simulate_enc_rot(rotations, false);
        }
    }
    return return_value;
}

void espThermostat::_mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(ESP_EQUIVA_LOG_TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    char * buffer = NULL;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(ESP_EQUIVA_LOG_TAG, "MQTT_EVENT_CONNECTED");
        /* Publish MQTT client alive signal */
        TempSet = 25;
        Temp = 20;
        buffer = _dump_measurement();
        msg_id = esp_mqtt_client_publish(client, ESP_EQUIVA_MQTT_SENSOR_NODE, buffer, 0, 0, 0);
        ESP_LOGI(ESP_EQUIVA_LOG_TAG, "MQTT Sensor Info sent, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_publish(client, ESP_EQUIVA_MQTT_LWT_NODE, "Online", 0, 0, 0);
        ESP_LOGI(ESP_EQUIVA_LOG_TAG, "MQTT status set to online, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, ESP_EQUIVA_MQTT_CMD_TEMPSET, 0);
        ESP_LOGI(ESP_EQUIVA_LOG_TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, ESP_EQUIVA_MQTT_CMD_HEATING, 0);
        ESP_LOGI(ESP_EQUIVA_LOG_TAG, "sent subscribe successful, msg_id=%d", msg_id);
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(ESP_EQUIVA_LOG_TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(ESP_EQUIVA_LOG_TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(ESP_EQUIVA_LOG_TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(ESP_EQUIVA_LOG_TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_DATA:
        ESP_LOGI(ESP_EQUIVA_LOG_TAG, "MQTT_EVENT_DATA");
        if (strncmp(ESP_EQUIVA_MQTT_CMD_TEMPSET, event->topic, event->topic_len) == 0)
        {
            /* Temperature Setting changed */
            TempSet = atoi(event->data);
            buffer = _dump_measurement();
            msg_id = esp_mqtt_client_publish(client, ESP_EQUIVA_MQTT_SENSOR_NODE, buffer, 0, 0, 0);
        }
        else if (strncmp(ESP_EQUIVA_MQTT_CMD_HEATING, event->topic, event->topic_len) == 0)
        {
            if(strncmp(ESP_EQUIVA_MQTT_CMD_HEAT_ON, event->data, event->data_len) == 0)
            {
                PowerOn = true;
                buffer = _dump_heat_state();
                msg_id = esp_mqtt_client_publish(client, ESP_EQUIVA_MQTT_STATE_NODE, buffer, 0, 0, 0);
            } else if (strncmp(ESP_EQUIVA_MQTT_CMD_HEAT_OFF, event->data, event->data_len) == 0) {
                PowerOn = false;
                buffer = _dump_heat_state();
                msg_id = esp_mqtt_client_publish(client, ESP_EQUIVA_MQTT_STATE_NODE, buffer, 0, 0, 0);
            } else {
                ESP_LOGE(ESP_EQUIVA_LOG_TAG, "INVALID Heating command: %s", event->data);
            }
        } else {
            ESP_LOGI(ESP_EQUIVA_LOG_TAG, "unknown Topic: %s", event->topic);
        }
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGI(ESP_EQUIVA_LOG_TAG, "MQTT_EVENT_ERROR");
        break;

    default:
        ESP_LOGI(ESP_EQUIVA_LOG_TAG, "Other event id:%d", event->event_id);
        break;
    }
    free(buffer);
}

esp_err_t espThermostat::_init_mqtt_connection()
{
    esp_err_t return_value = ESP_OK;
    esp_mqtt_client_config_t mqtt_config = {};

    mqtt_config.broker.address.uri = ESP_EQUIVA_MQTT_URI;
    mqtt_config.credentials.username = ESP_EQUIVA_MQTT_CLIENT_USERNAME;
    mqtt_config.credentials.authentication.password = ESP_EQUIVA_MQTT_CLIENT_PASSWORD;
    _mqtt_client_hdl = esp_mqtt_client_init(&mqtt_config);
    esp_mqtt_client_register_event(_mqtt_client_hdl, (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID, _mqtt_event_handler, NULL);
    esp_mqtt_client_start(_mqtt_client_hdl);

    return return_value;
}