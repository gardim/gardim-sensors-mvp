#include <stdio.h>
#include <wifi_esp32.h>
#include <soil_sensor.h>
#include <mqtt_esp32.h>
#include <lux_sensor.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "nvs_flash.h"

#define STACK_SOIL_SENSOR 2000
TaskHandle_t soil_sensor_handle;

#define STACK_LUX_SENSOR 2000
TaskHandle_t lux_sensor_handle;

#define STACK_WIFI_NETWORK 4000
TaskHandle_t wifi_handle;

#define STACK_MQTT_NETWORK 4000
TaskHandle_t mqtt_handle;
QueueHandle_t mqtt_queue;

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    xTaskCreatePinnedToCore(wifi_init_sta, "init wifi", STACK_WIFI_NETWORK, NULL, 1, &wifi_handle, 0);
    xTaskCreatePinnedToCore(mqtt_init, "init mqtt", STACK_MQTT_NETWORK, NULL, 2, &mqtt_handle, 0);
    xTaskCreatePinnedToCore(soil_sensor_init, "init soil sensor", STACK_SOIL_SENSOR, NULL, 3, &soil_sensor_handle, 1);
    xTaskCreatePinnedToCore(lux_sensor_init, "init lux sensor", STACK_LUX_SENSOR, NULL, 3, &lux_sensor_handle, 1);
    while(1) {
        vTaskDelay(portMAX_DELAY);
    }
}
