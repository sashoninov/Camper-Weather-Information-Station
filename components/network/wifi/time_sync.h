#pragma once

#include "gps.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void time_sync_init(void);
void time_sync_task(void *arg);

// нужно за weather_task и app_main
bool time_sync_is_valid(void);
bool time_sync_is_gps(void);
bool time_sync_is_rtc(void);

#ifdef __cplusplus
}
#endif
