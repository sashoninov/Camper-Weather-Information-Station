#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "driver/i2c_master.h"

#include "app_state.h"
#include "bme68x.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize BME680 driver.
 *
 * Initializes:
 *  - I2C device
 *  - Bosch driver
 *  - oversampling
 *  - IIR filter
 *  - heater profile
 *
 * @return true on success
 */
bool bme680_init(void);

/**
 * @brief Returns initialization state.
 */
bool bme680_is_initialized(void);

/**
 * @brief Read one measurement.
 *
 * Reads:
 *  - temperature
 *  - humidity
 *  - pressure
 *  - gas resistance
 *
 * and fills sensor_data_t.
 *
 * @param[out] data
 *
 * @return true if new data is available.
 */
bool bme680_read(sensor_data_t *data);

/**
 * @brief Deinitialize driver.
 */
void bme680_deinit(void);

#ifdef __cplusplus
}
#endif