#ifndef LUX_SENSOR_H
#define LUX_SENSOR_H

#define BH1750_POWER_DOWN           0x00
#define BH1750_POWER_ON             0x01
#define BH1750_RESET                0x07

#define BH1750_DEFAULT_MTREG        69
#define BH1750_MTREG_MIN            31
#define BH1750_MTREG_MAX            254

#define BH1750_I2C_ADDRESS          0x23
#define BH1750_CONV_FACTOR          1.2

#define HIGH_BIT_MEASUREMENT_TIME   0x08 << 3
#define LOW_BIT_MEASUREMENT_TIME    0x03 << 5

#define MAX_LUX_VALUE    9000
#define MIN_LUX_VALUE    0

typedef enum LuxMode {
    UNCONFIGURED =                  0,
    CONTINUOUS_HIGH_RES_MODE =      0x10,
    CONTINUOUS_HIGH_RES_MODE_2 =    0x11,
    CONTINUOUS_LOW_RES_MODE =       0x13,
    ONE_TIME_HIGH_RES_MODE =        0x20,
    ONE_TIME_HIGH_RES_MODE_2 =      0x21,
    ONE_TIME_LOW_RES_MODE =         0x23
} Mode;

void lux_sensor_init(void *pvParameters);

#endif /* LUX_SENSOR_H */