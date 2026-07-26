#pragma once

#include <stdbool.h>

#include "app_state.h"

#ifdef __cplusplus
extern "C" {
#endif

bool environment_init(void);

/* Read BME680 and update cached values */
bool environment_update(void);

/* Copy last values */
bool environment_read(sensor_data_t *data);

/* Pointer to last values */
const sensor_data_t *environment_get(void);

#ifdef __cplusplus
}
#endif