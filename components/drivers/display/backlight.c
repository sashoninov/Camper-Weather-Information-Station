#include "i2c_bus.h"
#include "esp_log.h"

#define BL_ADDR 0x45

static i2c_master_dev_handle_t dev;

void backlight_init(void)
{
    i2c_bus_add_device(BL_ADDR, &dev);

    uint8_t cmd[] = {0x96, 0xFF};
    i2c_bus_write(dev, cmd, sizeof(cmd));
}