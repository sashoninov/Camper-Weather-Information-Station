#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float voltage[3];
} ina3221_data_t;

esp_err_t ina3221_init(void);
esp_err_t ina3221_read(ina3221_data_t *out);

#ifdef __cplusplus
}
#endif