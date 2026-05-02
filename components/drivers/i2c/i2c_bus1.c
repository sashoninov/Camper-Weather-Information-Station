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
