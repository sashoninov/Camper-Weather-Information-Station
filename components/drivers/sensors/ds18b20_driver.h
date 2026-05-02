#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t ds18b20_init(void);
esp_err_t ds18b20_read_all(float *temps, int max_count);

// 🔥 ново
float ds18b20_get_by_name(const char *name, float *temps);

#ifdef __cplusplus
}
#endif