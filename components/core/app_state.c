#include "app_state.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <string.h>

app_state_t app_state;
static SemaphoreHandle_t state_mutex;

void app_state_init(void)
{
    state_mutex = xSemaphoreCreateMutex();
    memset(&app_state, 0, sizeof(app_state));

    // ===================== CUSTOM DEFAULTS =====================
    app_state.hourly_chime_enabled = false;   // по подразбиране изключено
    app_state.last_hour_chime = -1;           // още не е пускан звук
    app_state.dimming_active = false;         // екранът не е димиран
}

void app_state_lock(void)
{
    xSemaphoreTake(state_mutex, portMAX_DELAY);
}

void app_state_unlock(void)
{
    xSemaphoreGive(state_mutex);
}

app_state_t* app_state_get(void)
{
    return &app_state;
}
