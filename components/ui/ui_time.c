#include "ui.h"
#include "app_state.h"
#include "lvgl.h"

void ui_update_time(void)
{
    lv_label_set_text(ui_Label4, app_state.time_str);
    lv_label_set_text(ui_Label8, app_state.date_str);
    lv_label_set_text(ui_Label6, app_state.weekday_str);
}