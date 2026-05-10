#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

extern bool power_sleep_active;

void power_manager_init(void);
void power_manager_task(void *arg);
void power_manager_force_sleep(void);
void power_manager_force_wake(void);

#ifdef __cplusplus
}
#endif