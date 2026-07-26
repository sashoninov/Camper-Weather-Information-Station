#include "gps.h"
#include "gps_task.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "esp_timer.h"

#include <string.h>

static const char *TAG = "GPS";

/*--------------------------------------------------------------------
 * Private data
 *------------------------------------------------------------------*/

static gps_data_t gps_data;
static SemaphoreHandle_t gps_mutex = NULL;

/*--------------------------------------------------------------------
 * Internal functions
 *------------------------------------------------------------------*/

bool gps_update_data(const gps_data_t *data)
{
    if (data == NULL || gps_mutex == NULL)
        return false;

    if (xSemaphoreTake(gps_mutex, pdMS_TO_TICKS(100)) != pdTRUE)
        return false;

    gps_data = *data;
    gps_data.last_update_ms = (uint32_t)(esp_timer_get_time() / 1000ULL);

    xSemaphoreGive(gps_mutex);

    return true;
}

/*--------------------------------------------------------------------
 * Public API
 *------------------------------------------------------------------*/

bool gps_init(void)
{
    memset(&gps_data, 0, sizeof(gps_data));

    gps_mutex = xSemaphoreCreateMutex();

    if (gps_mutex == NULL)
    {
        ESP_LOGE(TAG, "Failed to create mutex");
        return false;
    }

    return true;
}

bool gps_start(void)
{
    return gps_task_start();
}

bool gps_get_data(gps_data_t *data)
{
    if (data == NULL || gps_mutex == NULL)
        return false;

    if (xSemaphoreTake(gps_mutex, pdMS_TO_TICKS(100)) != pdTRUE)
        return false;

    *data = gps_data;

    xSemaphoreGive(gps_mutex);

    return gps_data.valid;
}

bool gps_get_location(double *lat, double *lon)
{
    if (lat == NULL || lon == NULL)
        return false;

    gps_data_t data;

    if (!gps_get_data(&data))
        return false;

    *lat = data.latitude;
    *lon = data.longitude;

    return true;
}

bool gps_get_time(struct tm *utc)
{
    if (utc == NULL)
        return false;

    gps_data_t data;

    if (!gps_get_data(&data))
        return false;

    /* Изчакваме валиден GPS Fix */
    if (!data.valid || !data.fix)
        return false;

    *utc = data.utc;

    return true;
}