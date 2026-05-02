#include "lvgl.h"
#include "brightness.h"

void global_touch_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);

    // 🔥 ТУК слагаш разширения вариант
    if(code == LV_EVENT_PRESSED || code == LV_EVENT_CLICKED) {
        brightness_touch_event();
    }
}