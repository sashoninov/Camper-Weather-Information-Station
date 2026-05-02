#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <time.h>

#include "time_format.h"
#include "ui_time.h"
#include "app_state.h"

void time_task(void *arg)
{
    while (1)
    {
        time_t now;
        struct tm timeinfo;

        time(&now);
        localtime_r(&now, &timeinfo);

        // формат
        format_time_only(&timeinfo, true);
        format_date(&timeinfo);
        format_weekday(&timeinfo);

        // UI update
        ui_update_time();

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}