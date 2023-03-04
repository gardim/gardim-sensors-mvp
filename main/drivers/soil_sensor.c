#include <soil_sensor.h>
#include <mqtt_esp32.h>
#include <utils.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_log.h"

#define SOIL_TAG "Soil Sensor"
#define MQTT_SUBSCRIBED_BIT BIT2

extern QueueHandle_t mqtt_queue;

void soil_sensor_init(void *pvParameters)
{
    uint32_t uNotificationValue;
    message_payload_t payload;

    const char *topic = CONFIG_SOIL_TOPIC;

    memcpy(payload.topic, topic, sizeof(payload.topic));
    payload.data = 0;

    ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT_12));
    ESP_ERROR_CHECK(adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11));

    xTaskNotifyWaitIndexed(0, 0x00, ULONG_MAX, &uNotificationValue, portMAX_DELAY);
    if(uNotificationValue != MQTT_SUBSCRIBED_BIT) {
        esp_restart();
    }

    while(1) {
        int soil_adc = adc1_get_raw(ADC1_CHANNEL_0);
        if (soil_adc < MIN_ANALOG_SOIL_VALUE) {
            soil_adc = MIN_ANALOG_SOIL_VALUE;
        }
        long soil_percent = map_function(soil_adc, MAX_ANALOG_SOIL_VALUE, MIN_ANALOG_SOIL_VALUE, MIN_PERCENT_VALUE, MAX_PERCENT_VALUE);
        payload.data = soil_percent;
        xQueueSend(mqtt_queue, (void *) &payload, (TickType_t) 10); 
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
}