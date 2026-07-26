#include "lvgl.h"
#include "brightness.h"

void global_touch_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    ESP_LOGI("TOUCH", "event=%d target=%p",
             code,
             lv_event_get_target(e));

    if (code == LV_EVENT_PRESSED ||
        code == LV_EVENT_CLICKED)
    {
        brightness_touch_event();
    }
}