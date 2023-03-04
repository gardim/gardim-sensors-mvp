#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <mqtt_esp32.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "esp_ota_ops.h"

#define MQTT_TAG "MQTT Server"

#define MQTT_SUBSCRIBED_BIT BIT2
extern TaskHandle_t soil_sensor_handle;
extern TaskHandle_t lux_sensor_handle;
extern QueueHandle_t mqtt_queue;

static uint8_t retries = 0;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    /*
     * esp_mqtt_client_handle_t client = event->client;
     * int msg_id;
     */ 
    esp_mqtt_client_handle_t client = event->client;
    if(retries > 10) {
        esp_restart();
    }
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_CONNECTED");
        /*
         * msg_id = esp_mqtt_client_subscribe(client, "/gardim/esp32/000000/soil", 0);
         * ESP_LOGI(MQTT_TAG, "sent subscribe successful, msg_id=%d", msg_id);
         * msg_id = esp_mqtt_client_subscribe(client, "/gardim/esp32/000000/lux", 0);
         * ESP_LOGI(MQTT_TAG, "sent subscribe successful, msg_id=%d", msg_id);
        */
        xTaskNotifyIndexedFromISR(soil_sensor_handle, 0, MQTT_SUBSCRIBED_BIT, eSetBits, pdFALSE);
        xTaskNotifyIndexedFromISR(lux_sensor_handle, 0 , MQTT_SUBSCRIBED_BIT, eSetBits, pdFALSE);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_DISCONNECTED");
        retries++;
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(MQTT_TAG,"TOPIC=%.*s\r\n", event->topic_len, event->topic);
        ESP_LOGI(MQTT_TAG, "DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(MQTT_TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            ESP_LOGI(MQTT_TAG, "Last error code reported from esp-tls: 0x%x", event->error_handle->esp_tls_last_esp_err);
            ESP_LOGI(MQTT_TAG, "Last tls stack error number: 0x%x", event->error_handle->esp_tls_stack_err);
            ESP_LOGI(MQTT_TAG, "Last captured errno : %d (%s)",  event->error_handle->esp_transport_sock_errno,
                        strerror(event->error_handle->esp_transport_sock_errno));
        } else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
            ESP_LOGI(MQTT_TAG, "Connection refused error: 0x%x", event->error_handle->connect_return_code);
        } else {
            ESP_LOGW(MQTT_TAG, "Unknown error type: 0x%x", event->error_handle->error_type);
        }
        retries++;
        break;
    default:
        ESP_LOGI(MQTT_TAG, "Other event id:%d", event->event_id);
        retries++;
        break;
    }
}

void mqtt_init(void *p1)
{
    uint32_t uNotificationValue;
    message_payload_t payload;
    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address = {
                .hostname = CONFIG_HOST_ADDRESS,
                .port = 8080,
                .transport = MQTT_TRANSPORT_OVER_WS,
            },
        },
    };

    xTaskNotifyWaitIndexed(0, 0x00, ULONG_MAX, &uNotificationValue, portMAX_DELAY);

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    ESP_ERROR_CHECK(esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL));
    ESP_ERROR_CHECK(esp_mqtt_client_start(client));

    mqtt_queue = xQueueCreate(20, sizeof(message_payload_t));

    while(1) {
        xQueueReceive(mqtt_queue, (void *)&payload, portMAX_DELAY);
        char *message = malloc(sizeof(payload.data));
        sprintf(message, "%ld", payload.data);
        esp_mqtt_client_publish(client, payload.topic, (const char *)message, sizeof(payload.data), 0, 0);
        free(message);
    }
}
