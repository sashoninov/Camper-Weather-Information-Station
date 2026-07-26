#include "network_manager.h"
#include "sim_a7670.h"

void network_manager_init(void)
{
}

network_state_t network_get_state(void)
{
    return gsm_status.ready ?
        NETWORK_STATE_GSM :
        NETWORK_STATE_OFFLINE;
}

bool network_has_internet(void)
{
    return gsm_status.ready;
}

bool network_is_wifi(void)
{
    return false;
}

bool network_is_gsm(void)
{
    return true;
}