#include <lux_sensor.h>
#include <mqtt_esp32.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "driver/i2c.h"
#include "esp_log.h"

#define LUX_TAG "Lux Sensor"
#define I2C_MASTER_NUM              0
#define I2C_MASTER_FREQ_HZ          400000
#define I2C_MASTER_TX_BUF_DISABLE   0
#define I2C_MASTER_RX_BUF_DISABLE   0
#define I2C_MASTER_TIMEOUT_MS       1000
#define MQTT_SUBSCRIBED_BIT BIT2

extern QueueHandle_t mqtt_queue;

static esp_err_t i2c_master_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = 21,
        .scl_io_num = 22,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    i2c_param_config(i2c_master_port, &conf);

    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

static esp_err_t bh1750_register_write_bit(uint8_t data)
{
    int ret;
    uint8_t write_buf = data;

    ret = i2c_master_write_to_device(I2C_MASTER_NUM, BH1750_I2C_ADDRESS, &write_buf, sizeof(write_buf), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    if(ret) {
        ESP_LOGI(LUX_TAG, "error write %d", ret);
    }
    return ret;
}

void static bh1750_begin(Mode mode, uint8_t MTreg)
{
    ESP_ERROR_CHECK(bh1750_register_write_bit(mode));
    ESP_ERROR_CHECK(bh1750_register_write_bit(HIGH_BIT_MEASUREMENT_TIME | (MTreg >> 5)));
    ESP_ERROR_CHECK(bh1750_register_write_bit(LOW_BIT_MEASUREMENT_TIME  | (MTreg & 0x1F)));
    ESP_ERROR_CHECK(bh1750_register_write_bit(mode));
}

void lux_sensor_init(void *pvParameters) 
{
    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(LUX_TAG, "I2C initialized successfully");
    bh1750_begin(CONTINUOUS_HIGH_RES_MODE, BH1750_DEFAULT_MTREG);

    uint8_t data[2];
    uint16_t luxInBytes = 0;
    uint32_t uNotificationValue;
    message_payload_t payload;
    float luxConverted = 0.0;
    const char *topic = CONFIG_LUX_TOPIC;

    memcpy(payload.topic, topic, sizeof(payload.topic));
    payload.data = 0;

    xTaskNotifyWaitIndexed(0, 0x00, ULONG_MAX, &uNotificationValue, portMAX_DELAY);
    if(uNotificationValue != MQTT_SUBSCRIBED_BIT) {
        esp_restart();
    }

    while(1) {
        if(!ESP_ERROR_CHECK_WITHOUT_ABORT(i2c_master_read_from_device(I2C_MASTER_NUM, BH1750_I2C_ADDRESS, data, sizeof(data), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS))){
            luxInBytes = data[0] << 8 | data[1];
            luxConverted = luxInBytes / BH1750_CONV_FACTOR;
            payload.data = (long)luxConverted;
            xQueueSend(mqtt_queue, (void *) &payload, (TickType_t) 10); 
        } else{
            /*
             * placeholder
             */
        }
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
}