#ifndef SOIL_SENSOR_H
#define SOIL_SENSOR_H

#define MAX_ANALOG_VALUE    4095
#define MIN_ANALOG_VALUE    0

#define MAX_PERCENT_VALUE   100
#define MIN_PERCENT_VALUE   0

void soil_sensor_init(void *pvParameters);

#endif /* SOIL_SENSOR_H */