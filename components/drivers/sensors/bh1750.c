#include "bh1750.h"
#include "i2c_bus1.h"   // или i2c_bus.h ако е на I2C0
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BH1750_ADDR 0x5C

static i2c_master_dev_handle_t dev;
extern i2c_master_bus_handle_t g_i2c1_bus;   // смени на g_i2c_bus ако е на I2C0

void bh1750_init(void)
{
    i2c_device_config_t cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = BH1750_ADDR,
        .scl_speed_hz = 100000,
    };

    // Добавяме устройството към I2C1
    i2c_master_bus_add_device(g_i2c1_bus, &cfg, &dev);

    // Power ON + Continuously H-Resolution Mode
    uint8_t cmd = 0x10;
    i2c_master_transmit(dev, &cmd, 1, 100);
}

float bh1750_read_lux(void)
{
    uint8_t data[2];

    if (i2c_master_receive(dev, data, 2, 100) != ESP_OK)
        return -1;

    uint16_t raw = (data[0] << 8) | data[1];
    return raw / 1.2f;
}
