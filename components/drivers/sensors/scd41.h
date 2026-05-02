#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t scd41_init(void);
esp_err_t scd41_read(uint16_t *co2, float *temp, float *hum);

#ifdef __cplusplus
}
#endif