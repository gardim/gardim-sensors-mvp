#ifndef MQTT_ESP32_H
#define MQTT_ESP32_H

typedef struct message_payload {
    long data;
    char topic[30];
} message_payload_t;

void mqtt_init(void *p1);

#endif /* MQTT_ESP32_H */ 