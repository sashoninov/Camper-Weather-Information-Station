#include "ina3221.h"
#include "i2c_bus1.h"   // или i2c_bus.h ако е на I2C0

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define INA3221_ADDR 0x40

static i2c_master_dev_handle_t dev;
extern i2c_master_bus_handle_t g_i2c1_bus;   // смени на g_i2c_bus ако е на I2C0

static esp_err_t read_reg(uint8_t reg, uint16_t *out)
{
    uint8_t data[2];

    // WRITE register address
    ESP_ERROR_CHECK(i2c_master_transmit(dev, &reg, 1, 100));
    vTaskDelay(pdMS_TO_TICKS(2));

    // READ 2 bytes
    ESP_ERROR_CHECK(i2c_master_receive(dev, data, 2, 100));

    *out = (data[0] << 8) | data[1];
    return ESP_OK;
}

esp_err_t ina3221_init(void)
{
    i2c_device_config_t cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = INA3221_ADDR,
        .scl_speed_hz = 100000,
    };

    // Добавяме устройството към I2C1
    ESP_ERROR_CHECK(i2c_master_bus_add_device(g_i2c1_bus, &cfg, &dev));

    // CONFIG register
    uint8_t config[] = {0x00, 0x71, 0x27};
    ESP_ERROR_CHECK(i2c_master_transmit(dev, config, 3, 100));

    return ESP_OK;
}

esp_err_t ina3221_read(ina3221_data_t *out)
{
    uint16_t raw;

    read_reg(0x02, &raw);
    out->voltage[0] = (raw >> 3) * 0.008f;

    read_reg(0x04, &raw);
    out->voltage[1] = (raw >> 3) * 0.008f;

    read_reg(0x06, &raw);
    out->voltage[2] = (raw >> 3) * 0.008f;

    return ESP_OK;
}
