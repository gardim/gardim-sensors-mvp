#ifndef SOIL_SENSOR_H
#define SOIL_SENSOR_H

#define MAX_ANALOG_SOIL_VALUE    4095
#define MIN_ANALOG_SOIL_VALUE    1500

void soil_sensor_init(void *pvParameters);

#endif /* SOIL_SENSOR_H */