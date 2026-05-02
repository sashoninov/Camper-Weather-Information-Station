#include "mpu6050.h"
#include "i2c_bus1.h"   // или i2c_bus.h ако е на I2C0
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>
#include "calibration.h"

#define MPU6050_ADDR     0x69
#define REG_PWR_MGMT_1   0x6B
#define REG_ACCEL_XOUT   0x3B

static i2c_master_dev_handle_t dev;
extern i2c_master_bus_handle_t g_i2c1_bus;   // смени на g_i2c_bus ако е на I2C0

/* =========================================================
   INIT
   ========================================================= */
esp_err_t mpu6050_init(void)
{
    i2c_device_config_t cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = MPU6050_ADDR,
        .scl_speed_hz = 400000,
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(g_i2c1_bus, &cfg, &dev));

    uint8_t data[2] = {REG_PWR_MGMT_1, 0x00};
    ESP_ERROR_CHECK(i2c_master_transmit(dev, data, 2, 100));

    return ESP_OK;
}

/* =========================================================
   READ
   ========================================================= */
esp_err_t mpu6050_read(mpu6050_data_t *out)
{
    uint8_t reg = REG_ACCEL_XOUT;
    uint8_t data[14];

    // Set register pointer
    ESP_ERROR_CHECK(i2c_master_transmit(dev, &reg, 1, 100));
    vTaskDelay(pdMS_TO_TICKS(2));

    // Read 14 bytes
    ESP_ERROR_CHECK(i2c_master_receive(dev, data, 14, 100));

    int16_t ax = (data[0] << 8) | data[1];
    int16_t ay = (data[2] << 8) | data[3];
    int16_t az = (data[4] << 8) | data[5];

    int16_t gx = (data[8] << 8) | data[9];
    int16_t gy = (data[10] << 8) | data[11];
    int16_t gz = (data[12] << 8) | data[13];

    out->ax = ax / 16384.0f;
    out->ay = ay / 16384.0f;
    out->az = az / 16384.0f;

    out->gx = gx / 131.0f;
    out->gy = gy / 131.0f;
    out->gz = gz / 131.0f;

    return ESP_OK;
}

/* =========================================================
   CALIBRATION
   ========================================================= */
void mpu6050_calibrate(void)
{
    float pitch_sum = 0;
    float roll_sum  = 0;

    vTaskDelay(pdMS_TO_TICKS(500));

    const int samples = 200;

    for (int i = 0; i < samples; i++)
    {
        mpu6050_data_t d;
        mpu6050_read(&d);

        float pitch = atan2f(d.ay, d.az) * 57.2958f;
        float roll  = atan2f(-d.ax, sqrtf(d.ay * d.ay + d.az * d.az)) * 57.2958f;

        pitch_sum += pitch;
        roll_sum  += roll;

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    calibration_set(pitch_sum / samples, roll_sum / samples);
}

/* =========================================================
   ANGLES
   ========================================================= */
mpu_angles_t mpu6050_get_angles(void)
{
    mpu6050_data_t d;
    mpu6050_read(&d);

    return mpu6050_get_angles_from_data(&d);
}

mpu_angles_t mpu6050_get_angles_from_data(mpu6050_data_t *d)
{
    mpu_angles_t a;

    a.pitch = atan2f(d->ay, d->az) * 57.2958f;
    a.roll  = atan2f(-d->ax, sqrtf(d->ay * d->ay + d->az * d->az)) * 57.2958f;

    calibration_data_t calib;
    calibration_get(&calib);

    a.pitch -= calib.pitch_offset;
    a.roll  -= calib.roll_offset;

    return a;
}
