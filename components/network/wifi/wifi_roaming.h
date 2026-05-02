#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void wifi_roaming_start(void);

void wifi_roaming_pause(void);
void wifi_roaming_resume(void);

extern volatile bool roaming_enabled;

#ifdef __cplusplus
}
#endif

