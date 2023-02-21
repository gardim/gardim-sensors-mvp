#include "../../include/wifi_esp32.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

#include "lwip/err.h"
#include "lwip/sys.h"

static EventGroupHandle_t s_wifi_event_group;
static esp_event_handler_instance_t instance_any_id;
static esp_event_handler_instance_t instance_got_ip;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define LED_FLAG_PORT      2
#define LED_FLAG_HIGH      0x01
#define LED_FLAG_LOW       0x00

static const char *TAG = "WiFi-Network";

static int s_retry_num = 0;
extern TaskHandle_t mqtt_handle;

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
   if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
      esp_wifi_connect();
   } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
      if (s_retry_num < CONFIG_ESP_MAXIMUM_RETRY) {
         esp_wifi_connect();
         s_retry_num++;
         ESP_LOGI(TAG, "retry to connect to the AP");
      } else {
         xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
      }
      ESP_LOGI(TAG,"connect to the AP fail");
   } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
      ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
      ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
      s_retry_num = 0;
      xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
      xTaskNotifyIndexedFromISR(mqtt_handle, 0, WIFI_CONNECTED_BIT, eSetBits, pdFALSE);
      ESP_ERROR_CHECK(gpio_set_level(LED_FLAG_PORT, LED_FLAG_LOW));
   }
}

void init_stack_tcp_ip_and_wifi()
{
   ESP_ERROR_CHECK(esp_netif_init());
   ESP_ERROR_CHECK(esp_event_loop_create_default());
   esp_netif_create_default_wifi_sta();

   wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
   ESP_ERROR_CHECK(esp_wifi_init(&cfg));
}

void wifi_register_events()
{
   s_wifi_event_group = xEventGroupCreate();
   ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                         ESP_EVENT_ANY_ID,
                                                         &event_handler,
                                                         NULL,
                                                         &instance_any_id));

   ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                         IP_EVENT_STA_GOT_IP,
                                                         &event_handler,
                                                         NULL,
                                                         &instance_got_ip));
}

static void wait_connection()
{
   EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

   if (bits & WIFI_CONNECTED_BIT) {
      ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
               CONFIG_ESP_WIFI_SSID, CONFIG_ESP_WIFI_PASSWORD);
   } else if (bits & WIFI_FAIL_BIT) {
      ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
               CONFIG_ESP_WIFI_SSID, CONFIG_ESP_WIFI_PASSWORD);
   } else {
      ESP_LOGE(TAG, "UNEXPECTED EVENT");
   }
}

static void connect_to_wifi()
{
   wifi_config_t wifi_config = {
      .sta = {
         .ssid = CONFIG_ESP_WIFI_SSID,
         .password = CONFIG_ESP_WIFI_PASSWORD,
	      .threshold.authmode = WIFI_AUTH_WPA2_PSK,

         .pmf_cfg = {
               .capable = true,
               .required = false
         },
      },
   };

   ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
   ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
   ESP_ERROR_CHECK(esp_wifi_start() );

   ESP_LOGI(TAG, "wifi_init_sta finished.");
   wait_connection();
}

static void wifi_unregister_events()
{
   ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
   ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
   vEventGroupDelete(s_wifi_event_group);
}

void wifi_init_sta(void *pvParameters)
{
   esp_rom_gpio_pad_select_gpio(LED_FLAG_PORT);
   ESP_ERROR_CHECK(gpio_set_direction(LED_FLAG_PORT, GPIO_MODE_OUTPUT));
   ESP_ERROR_CHECK(gpio_set_level(LED_FLAG_PORT, LED_FLAG_HIGH));
   init_stack_tcp_ip_and_wifi();
   wifi_register_events();
   connect_to_wifi();
   wifi_unregister_events();
   vTaskDelay(portMAX_DELAY);
}
