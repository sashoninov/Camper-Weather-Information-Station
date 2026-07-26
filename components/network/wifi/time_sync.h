#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void time_sync_init(void);
void time_sync_task(void *arg);

bool time_sync_is_valid(void);
bool time_sync_is_gps(void);
bool time_sync_is_rtc(void);

#ifdef __cplusplus
}
#endif
