#include "bh1750.h"
#include "i2c_bus1.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BH1750_ADDR 0x5C

static const char *TAG = "BH1750";

static i2c_master_dev_handle_t dev;
extern i2c_master_bus_handle_t g_i2c1_bus;

/* =========================================================
   SAFE I2C HELPERS
   ========================================================= */
static esp_err_t bh_safe_tx(const uint8_t *data, size_t len)
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

static esp_err_t bh_safe_rx(uint8_t *data, size_t len)
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
   INIT
   ========================================================= */
void bh1750_init(void)
{
    i2c_device_config_t cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = BH1750_ADDR,
        .scl_speed_hz = 100000,
    };

    esp_err_t err = i2c_master_bus_add_device(g_i2c1_bus, &cfg, &dev);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add BH1750: %s", esp_err_to_name(err));
        return;
    }

    uint8_t cmd = 0x10; // Continuously H-Resolution Mode
    if (bh_safe_tx(&cmd, 1) != ESP_OK) {
        ESP_LOGE(TAG, "BH1750 init failed");
    } else {
        ESP_LOGI(TAG, "BH1750 init OK");
    }
}

/* =========================================================
   READ
   ========================================================= */
float bh1750_read_lux(void)
{
    uint8_t data[2];

    if (bh_safe_rx(data, 2) != ESP_OK)
        return -1;

    uint16_t raw = (data[0] << 8) | data[1];
    return raw / 1.2f;
}
