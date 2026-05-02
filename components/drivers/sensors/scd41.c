#include "scd41.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "i2c_bus1.h"   // <-- важно

#define SCD41_ADDR 0x62

static const char *TAG = "SCD41";

static i2c_master_dev_handle_t dev;
extern i2c_master_bus_handle_t g_i2c1_bus;   // <-- важно

// =========================
// CRC
// =========================
static uint8_t scd41_crc(uint8_t *data)
{
    uint8_t crc = 0xFF;

    for (int i = 0; i < 2; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++)
            crc = (crc & 0x80) ? (crc << 1) ^ 0x31 : (crc << 1);
    }
    return crc;
}

// =========================
// DATA READY
// =========================
static bool scd41_data_ready(void)
{
    uint8_t cmd[] = {0xE4, 0xB8};
    uint8_t data[3];

    if (i2c_master_transmit(dev, cmd, 2, 100) != ESP_OK)
        return false;

    vTaskDelay(pdMS_TO_TICKS(2));

    if (i2c_master_receive(dev, data, 3, 100) != ESP_OK)
        return false;

    uint16_t ready = (data[0] << 8) | data[1];
    return (ready & 0x07FF) != 0;
}

// =========================
// INIT
// =========================
esp_err_t scd41_init(void)
{
    i2c_device_config_t cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = SCD41_ADDR,
        .scl_speed_hz = 100000,
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(g_i2c1_bus, &cfg, &dev));

    // SOFT RESET
    uint8_t reset_cmd[] = {0x36, 0x46};
    i2c_master_transmit(dev, reset_cmd, 2, 100);
    vTaskDelay(pdMS_TO_TICKS(1000));

    // STOP measurement
    uint8_t stop_cmd[] = {0x3F, 0x86};
    i2c_master_transmit(dev, stop_cmd, 2, 100);
    vTaskDelay(pdMS_TO_TICKS(500));

    // START measurement
    uint8_t start_cmd[] = {0x21, 0xB1};
    for (int i = 0; i < 5; i++)
    {
        if (i2c_master_transmit(dev, start_cmd, 2, 100) == ESP_OK)
        {
            ESP_LOGI(TAG, "Start OK");
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    // WAIT FOR FIRST DATA
    vTaskDelay(pdMS_TO_TICKS(8000));

    ESP_LOGI(TAG, "SCD41 init DONE");
    return ESP_OK;
}

// =========================
// READ
// =========================
esp_err_t scd41_read(uint16_t *co2, float *temp, float *hum)
{
    if (!scd41_data_ready())
        return ESP_ERR_INVALID_STATE;

    uint8_t cmd[] = {0xEC, 0x05};
    uint8_t data[9];

    if (i2c_master_transmit(dev, cmd, 2, 100) != ESP_OK)
        return ESP_FAIL;

    vTaskDelay(pdMS_TO_TICKS(2));

    if (i2c_master_receive(dev, data, 9, 100) != ESP_OK)
        return ESP_FAIL;

    // CRC
    if (scd41_crc(&data[0]) != data[2]) return ESP_FAIL;
    if (scd41_crc(&data[3]) != data[5]) return ESP_FAIL;
    if (scd41_crc(&data[6]) != data[8]) return ESP_FAIL;

    *co2 = (data[0] << 8) | data[1];

    uint16_t raw_temp = (data[3] << 8) | data[4];
    uint16_t raw_hum  = (data[6] << 8) | data[7];

    float t = -45 + 175 * ((float)raw_temp / 65535.0f);
    float h = 100 * ((float)raw_hum / 65535.0f);

    if (t < -20 || t > 60) return ESP_FAIL;
    if (h < 1 || h > 100) return ESP_FAIL;
    if (*co2 < 300 || *co2 > 5000) return ESP_FAIL;

    *temp = t;
    *hum  = h;

    return ESP_OK;
}
