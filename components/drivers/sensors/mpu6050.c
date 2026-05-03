#include "mpu6050.h"
#include "i2c_bus1.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>
#include "calibration.h"

#define MPU6050_ADDR     0x69
#define REG_PWR_MGMT_1   0x6B
#define REG_ACCEL_XOUT   0x3B

static const char *TAG = "MPU6050";

static i2c_master_dev_handle_t dev;
extern i2c_master_bus_handle_t g_i2c1_bus;

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

    esp_err_t err = i2c_master_bus_add_device(g_i2c1_bus, &cfg, &dev);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add device: %s", esp_err_to_name(err));
        return err;
    }

    uint8_t data[2] = {REG_PWR_MGMT_1, 0x00};
    err = i2c_master_transmit(dev, data, 2, 100);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to wake MPU6050: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "MPU6050 initialized");
    return ESP_OK;
}

/* =========================================================
   SAFE I2C READ WITH RETRY + BUS RESET
   ========================================================= */
static esp_err_t mpu_i2c_safe_transmit(const uint8_t *data, size_t len)
{
    for (int i = 0; i < 3; i++) {
        esp_err_t err = i2c_master_transmit(dev, data, len, 100);
        if (err == ESP_OK) return ESP_OK;

        ESP_LOGW(TAG, "I2C transmit failed (%s), retry %d/3",
                 esp_err_to_name(err), i + 1);

        i2c_master_bus_reset(g_i2c1_bus);
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    return ESP_FAIL;
}

static esp_err_t mpu_i2c_safe_receive(uint8_t *data, size_t len)
{
    for (int i = 0; i < 3; i++) {
        esp_err_t err = i2c_master_receive(dev, data, len, 100);
        if (err == ESP_OK) return ESP_OK;

        ESP_LOGW(TAG, "I2C receive failed (%s), retry %d/3",
                 esp_err_to_name(err), i + 1);

        i2c_master_bus_reset(g_i2c1_bus);
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    return ESP_FAIL;
}

/* =========================================================
   READ (SAFE)
   ========================================================= */
esp_err_t mpu6050_read(mpu6050_data_t *out)
{
    uint8_t reg = REG_ACCEL_XOUT;
    uint8_t data[14];

    // Set register pointer
    esp_err_t err = mpu_i2c_safe_transmit(&reg, 1);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set register pointer");
        return err;
    }

    vTaskDelay(pdMS_TO_TICKS(2));

    // Read 14 bytes
    err = mpu_i2c_safe_receive(data, 14);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read sensor data");
        return err;
    }

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
        if (mpu6050_read(&d) != ESP_OK) continue;

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
    if (mpu6050_read(&d) != ESP_OK) {
        mpu_angles_t zero = {0};
        return zero;
    }

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
