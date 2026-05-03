#include "scd41.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "i2c_bus1.h"

#define SCD41_ADDR 0x62

static const char *TAG = "SCD41";

static i2c_master_dev_handle_t dev;
extern i2c_master_bus_handle_t g_i2c1_bus;

/* =========================================================
   SAFE I2C HELPERS
   ========================================================= */
static esp_err_t scd41_safe_tx(const uint8_t *data, size_t len)
{
    for (int i = 0; i < 3; i++) {
        esp_err_t err = i2c_master_transmit(dev, data, len, 100);
        if (err == ESP_OK) return ESP_OK;

        ESP_LOGW(TAG, "TX failed (%s), retry %d/3", esp_err_to_name(err), i + 1);
        i2c_master_bus_reset(g_i2c1_bus);
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    return ESP_FAIL;
}

static esp_err_t scd41_safe_rx(uint8_t *data, size_t len)
{
    for (int i = 0; i < 3; i++) {
        esp_err_t err = i2c_master_receive(dev, data, len, 100);
        if (err == ESP_OK) return ESP_OK;

        ESP_LOGW(TAG, "RX failed (%s), retry %d/3", esp_err_to_name(err), i + 1);
        i2c_master_bus_reset(g_i2c1_bus);
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    return ESP_FAIL;
}

/* =========================================================
   CRC
   ========================================================= */
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

/* =========================================================
   DATA READY
   ========================================================= */
static bool scd41_data_ready(void)
{
    uint8_t cmd[] = {0xE4, 0xB8};
    uint8_t data[3];

    if (scd41_safe_tx(cmd, 2) != ESP_OK)
        return false;

    vTaskDelay(pdMS_TO_TICKS(2));

    if (scd41_safe_rx(data, 3) != ESP_OK)
        return false;

    uint16_t ready = (data[0] << 8) | data[1];
    return (ready & 0x07FF) != 0;
}

/* =========================================================
   INIT
   ========================================================= */
esp_err_t scd41_init(void)
{
    i2c_device_config_t cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = SCD41_ADDR,
        .scl_speed_hz = 100000,
    };

    esp_err_t err = i2c_master_bus_add_device(g_i2c1_bus, &cfg, &dev);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add SCD41 device: %s", esp_err_to_name(err));
        return err;
    }

    uint8_t reset_cmd[] = {0x36, 0x46};
    scd41_safe_tx(reset_cmd, 2);
    vTaskDelay(pdMS_TO_TICKS(1000));

    uint8_t stop_cmd[] = {0x3F, 0x86};
    scd41_safe_tx(stop_cmd, 2);
    vTaskDelay(pdMS_TO_TICKS(500));

    uint8_t start_cmd[] = {0x21, 0xB1};
    for (int i = 0; i < 5; i++) {
        if (scd41_safe_tx(start_cmd, 2) == ESP_OK) {
            ESP_LOGI(TAG, "Start OK");
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    vTaskDelay(pdMS_TO_TICKS(8000));

    ESP_LOGI(TAG, "SCD41 init DONE");
    return ESP_OK;
}

/* =========================================================
   READ
   ========================================================= */
esp_err_t scd41_read(uint16_t *co2, float *temp, float *hum)
{
    if (!scd41_data_ready())
        return ESP_ERR_INVALID_STATE;

    uint8_t cmd[] = {0xEC, 0x05};
    uint8_t data[9];

    if (scd41_safe_tx(cmd, 2) != ESP_OK)
        return ESP_FAIL;

    vTaskDelay(pdMS_TO_TICKS(2));

    if (scd41_safe_rx(data, 9) != ESP_OK)
        return ESP_FAIL;

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
