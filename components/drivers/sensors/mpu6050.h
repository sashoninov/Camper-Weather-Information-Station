#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float ax, ay, az;
    float gx, gy, gz;
} mpu6050_data_t;

typedef struct {
    float pitch;
    float roll;
} mpu_angles_t;

esp_err_t mpu6050_init(void);
esp_err_t mpu6050_read(mpu6050_data_t *out);

void mpu6050_calibrate(void);

mpu_angles_t mpu6050_get_angles(void);
mpu_angles_t mpu6050_get_angles_from_data(mpu6050_data_t *d);

#ifdef __cplusplus
}
#endif