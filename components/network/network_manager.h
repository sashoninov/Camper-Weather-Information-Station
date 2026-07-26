#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    NETWORK_STATE_OFFLINE = 0,
    NETWORK_STATE_GSM
} network_state_t;

void network_manager_init(void);

network_state_t network_get_state(void);

bool network_has_internet(void);

bool network_is_wifi(void);

bool network_is_gsm(void);

#ifdef __cplusplus
}
#endif