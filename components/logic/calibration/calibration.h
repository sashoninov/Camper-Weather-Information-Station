#pragma once

#include <stdbool.h>   // 🔥 МНОГО ВАЖНО

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float pitch_offset;
    float roll_offset;
} calibration_data_t;

void calibration_set(float pitch, float roll);
void calibration_get(calibration_data_t *out);

bool calibration_save(void);
bool calibration_load(void);

#ifdef __cplusplus
}
#endif