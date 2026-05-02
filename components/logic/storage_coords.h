#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool storage_save_coords(float lat, float lon);
bool storage_load_coords(float *lat, float *lon);

#ifdef __cplusplus
}
#endif
