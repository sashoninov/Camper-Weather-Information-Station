#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void power_manager_init(void);
void power_manager_task(void *arg);
bool power_manager_is_sleeping(void);

#ifdef __cplusplus
}
#endif