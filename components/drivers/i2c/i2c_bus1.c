#include "i2c_bus1.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#define I2C1_PORT     1
#define I2C1_SDA_PIN  5
#define I2C1_SCL_PIN  6

static const char *TAG = "I2C1_BUS";

i2c_master_bus_handle_t g_i2c1_bus = NULL;
static SemaphoreHandle_t i2c1_mutex = NULL;

esp_err_t i2c1_bus_init(void)
{
    if (g_i2c1_bus != NULL) {
        ESP_LOGW(TAG, "I2C1 already initialized");
        return ESP_OK;
    }

    i2c_master_bus_config_t cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C1_PORT,
        .sda_io_num = I2C1_SDA_PIN,
        .scl_io_num = I2C1_SCL_PIN,
        .glitch_ignore_cnt = 7,
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&cfg, &g_i2c1_bus));

    i2c1_mutex = xSemaphoreCreateMutex();

    ESP_LOGI(TAG, "I2C1 bus initialized");
    return ESP_OK;
}

i2c_master_bus_handle_t i2c1_bus_get_handle(void)
{
    return g_i2c1_bus;
}

esp_err_t i2c1_bus_add_device(uint8_t address, i2c_master_dev_handle_t *dev)
{
    if (!g_i2c1_bus) {
        ESP_LOGE(TAG, "I2C1 not initialized");
        return ESP_FAIL;
    }

    i2c_device_config_t cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = address,
        .scl_speed_hz = 400000,
    };

    return i2c_master_bus_add_device(g_i2c1_bus, &cfg, dev);
}

esp_err_t i2c1_bus_write(i2c_master_dev_handle_t dev, uint8_t *data, size_t len)
{
    if (!dev) return ESP_ERR_INVALID_ARG;

    i2c1_bus_lock();

    esp_err_t ret = i2c_master_transmit(dev, data, len, -1);

    i2c1_bus_unlock();

    return ret;
}

esp_err_t i2c1_bus_read(i2c_master_dev_handle_t dev, uint8_t *data, size_t len)
{
    if (!dev) return ESP_ERR_INVALID_ARG;

    i2c1_bus_lock();

    esp_err_t ret = i2c_master_receive(dev, data, len, -1);

    i2c1_bus_unlock();

    return ret;
}

/* --------------------------------------------------------------------------
 * Combined Write + Read transaction
 * -------------------------------------------------------------------------- */

esp_err_t i2c1_bus_write_read(
    i2c_master_dev_handle_t dev,
    const uint8_t *tx,
    size_t tx_len,
    uint8_t *rx,
    size_t rx_len)
{
    if (!dev || !tx || !rx) {
        return ESP_ERR_INVALID_ARG;
    }

    i2c1_bus_lock();

    esp_err_t ret = i2c_master_transmit_receive(
        dev,
        tx,
        tx_len,
        rx,
        rx_len,
        -1);

    i2c1_bus_unlock();

    return ret;
}

void i2c1_bus_lock(void)
{
    if (i2c1_mutex) {
        xSemaphoreTake(i2c1_mutex, portMAX_DELAY);
    }
}

void i2c1_bus_unlock(void)
{
    if (i2c1_mutex) {
        xSemaphoreGive(i2c1_mutex);
    }
}

/* --------------------------------------------------------------------------
 * I2C Scanner
 * -------------------------------------------------------------------------- */

void i2c1_scan(void)
{
    ESP_LOGI("I2C_SCAN", "==============================");
    ESP_LOGI("I2C_SCAN", "Scanning I2C bus...");
    ESP_LOGI("I2C_SCAN", "==============================");

    for (uint8_t addr = 0x08; addr < 0x78; addr++)
    {
        i2c_master_dev_handle_t dev = NULL;

        i2c_device_config_t cfg = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = addr,
            .scl_speed_hz = 100000,
        };

        if (i2c_master_bus_add_device(g_i2c1_bus, &cfg, &dev) != ESP_OK)
            continue;

        uint8_t dummy = 0;
        esp_err_t err = i2c_master_transmit(dev, &dummy, 1, 20);

        if (err == ESP_OK)
        {
            ESP_LOGI("I2C_SCAN", "Found device at 0x%02X", addr);

            uint8_t reg = 0xD0;
            uint8_t id = 0;

            err = i2c_master_transmit_receive(
                    dev,
                    &reg,
                    1,
                    &id,
                    1,
                    20);

            if (err == ESP_OK)
            {
                ESP_LOGI("I2C_SCAN", "  CHIP ID = 0x%02X", id);

                switch (id)
                {
                    case 0x61:
                        ESP_LOGI("I2C_SCAN", "  -> BME680");
                        break;

                    case 0x60:
                        ESP_LOGI("I2C_SCAN", "  -> BME280");
                        break;

                    case 0x58:
                        ESP_LOGI("I2C_SCAN", "  -> BMP280");
                        break;

                    default:
                        ESP_LOGI("I2C_SCAN", "  -> Unknown");
                        break;
                }
            }
        }

        i2c_master_bus_rm_device(dev);
    }

    ESP_LOGI("I2C_SCAN", "==============================");
    ESP_LOGI("I2C_SCAN", "Scan finished.");
    ESP_LOGI("I2C_SCAN", "==============================");
}