#include "app_events.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

static QueueHandle_t event_queue;

void app_event_request_weather_update(void)
{
    int evt = 1;
    xQueueSend(event_queue, &evt, 0);
}