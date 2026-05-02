#pragma once
#include "calibration.h"
#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif

bool storage_save_calibration(const calibration_data_t *calib);
bool storage_load_calibration(calibration_data_t *calib);

#ifdef __cplusplus
}
#endif