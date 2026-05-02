#pragma once

#include "esp_err.h"
#include "driver/i2c_master.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Init
esp_err_t i2c1_bus_init(void);

// Get bus handle
i2c_master_bus_handle_t i2c1_bus_get_handle(void);

// Add device
esp_err_t i2c1_bus_add_device(uint8_t address, i2c_master_dev_handle_t *dev);

// Basic read/write
esp_err_t i2c1_bus_write(i2c_master_dev_handle_t dev, uint8_t *data, size_t len);
esp_err_t i2c1_bus_read(i2c_master_dev_handle_t dev, uint8_t *data, size_t len);

// Thread safety
void i2c1_bus_lock(void);
void i2c1_bus_unlock(void);

#ifdef __cplusplus
}
#endif
