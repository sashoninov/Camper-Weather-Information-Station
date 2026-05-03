#include "ina3221.h"
#include "i2c_bus1.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define INA3221_ADDR 0x40

static const char *TAG = "INA3221";

static i2c_master_dev_handle_t dev;
extern i2c_master_bus_handle_t g_i2c1_bus;

/* =========================================================
   SAFE I2C HELPERS
   ========================================================= */
static esp_err_t ina_safe_tx(const uint8_t *data, size_t len)
{
    for (int i = 0; i < 3; i++) {
        esp_err_t err = i2c_master_transmit(dev, data, len, 100);
        if (err == ESP_OK) return ESP_OK;

        ESP_LOGW(TAG, "TX failed (%s), retry %d/3",
                 esp_err_to_name(err), i + 1);

        i2c_master_bus_reset(g_i2c1_bus);
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    return ESP_FAIL;
}

static esp_err_t ina_safe_rx(uint8_t *data, size_t len)
{
    for (int i = 0; i < 3; i++) {
        esp_err_t err = i2c_master_receive(dev, data, len, 100);
        if (err == ESP_OK) return ESP_OK;

        ESP_LOGW(TAG, "RX failed (%s), retry %d/3",
                 esp_err_to_name(err), i + 1);

        i2c_master_bus_reset(g_i2c1_bus);
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    return ESP_FAIL;
}

/* =========================================================
   READ REGISTER (SAFE)
   ========================================================= */
static esp_err_t read_reg(uint8_t reg, uint16_t *out)
{
    uint8_t data[2];

    if (ina_safe_tx(&reg, 1) != ESP_OK)
        return ESP_FAIL;

    vTaskDelay(pdMS_TO_TICKS(2));

    if (ina_safe_rx(data, 2) != ESP_OK)
        return ESP_FAIL;

    *out = (data[0] << 8) | data[1];
    return ESP_OK;
}

/* =========================================================
   INIT
   ========================================================= */
esp_err_t ina3221_init(void)
{
    i2c_device_config_t cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = INA3221_ADDR,
        .scl_speed_hz = 100000,
    };

    esp_err_t err = i2c_master_bus_add_device(g_i2c1_bus, &cfg, &dev);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add INA3221: %s", esp_err_to_name(err));
        return err;
    }

    uint8_t config[] = {0x00, 0x71, 0x27};
    if (ina_safe_tx(config, 3) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write config");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "INA3221 init OK");
    return ESP_OK;
}

/* =========================================================
   READ CHANNELS
   ========================================================= */
esp_err_t ina3221_read(ina3221_data_t *out)
{
    uint16_t raw;

    if (read_reg(0x02, &raw) != ESP_OK) return ESP_FAIL;
    out->voltage[0] = (raw >> 3) * 0.008f;

    if (read_reg(0x04, &raw) != ESP_OK) return ESP_FAIL;
    out->voltage[1] = (raw >> 3) * 0.008f;

    if (read_reg(0x06, &raw) != ESP_OK) return ESP_FAIL;
    out->voltage[2] = (raw >> 3) * 0.008f;

    return ESP_OK;
}
