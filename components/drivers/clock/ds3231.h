#pragma once

#include <stdbool.h>
#include <time.h>
#include "i2c_bus.h"

#ifdef __cplusplus
extern "C" {
#endif

bool ds3231_init(void);
bool ds3231_get_time(struct tm *out);
bool ds3231_set_time(const struct tm *in);

void ds3231_force_set(int year, int month, int day, int hour, int min, int sec);

#ifdef __cplusplus
}
#endif