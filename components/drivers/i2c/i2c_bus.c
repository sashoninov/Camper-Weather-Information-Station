#include "i2c_bus.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"


#define I2C_PORT       0
#define I2C_SDA_PIN    7
#define I2C_SCL_PIN    8

i2c_master_bus_handle_t g_i2c_bus = NULL;

static const char *TAG = "I2C_BUS";

static i2c_master_bus_handle_t bus_handle = NULL;
static SemaphoreHandle_t i2c_mutex = NULL;

esp_err_t i2c_bus_init(void)
{
    if (bus_handle != NULL) {
        ESP_LOGW(TAG, "I2C already initialized");
        return ESP_OK;
    }

    i2c_master_bus_config_t cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_PORT,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .glitch_ignore_cnt = 7,
    };

    // 🔥 FIX
    ESP_ERROR_CHECK(i2c_new_master_bus(&cfg, &g_i2c_bus));
    bus_handle = g_i2c_bus;

    i2c_mutex = xSemaphoreCreateMutex();

    ESP_LOGI(TAG, "I2C bus initialized");

    return ESP_OK;
}

i2c_master_bus_handle_t i2c_bus_get_handle(void)
{
    return bus_handle;
}

esp_err_t i2c_bus_add_device(uint8_t address, i2c_master_dev_handle_t *dev)
{
    if (!bus_handle) {
        ESP_LOGE(TAG, "I2C not initialized");
        return ESP_FAIL;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = address,
        .scl_speed_hz = 400000,
    };

    return i2c_master_bus_add_device(bus_handle, &dev_cfg, dev);
}

esp_err_t i2c_bus_write(i2c_master_dev_handle_t dev, uint8_t *data, size_t len)
{
    if (!dev) return ESP_ERR_INVALID_ARG;

    i2c_bus_lock();
    esp_err_t ret = i2c_master_transmit(dev, data, len, -1);
    i2c_bus_unlock();

    return ret;
}

esp_err_t i2c_bus_read(i2c_master_dev_handle_t dev, uint8_t *data, size_t len)
{
    if (!dev) return ESP_ERR_INVALID_ARG;

    i2c_bus_lock();
    esp_err_t ret = i2c_master_receive(dev, data, len, -1);
    i2c_bus_unlock();

    return ret;
}

void i2c_bus_lock(void)
{
    if (i2c_mutex) {
        xSemaphoreTake(i2c_mutex, portMAX_DELAY);
    }
}

void i2c_bus_unlock(void)
{
    if (i2c_mutex) {
        xSemaphoreGive(i2c_mutex);
    }
}