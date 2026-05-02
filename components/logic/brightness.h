#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void brightness_touch_event(void);
uint8_t brightness_calculate(float lux);

#ifdef __cplusplus
}
#endif