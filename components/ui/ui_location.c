#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "app_state.h"
#include "ui.h"   // SquareLine генериран





void ui_update_location(void)
{
    char buf[128];
    buf[0] = '\0';

    app_state_lock();

    if (app_state.location.city[0]) {
        strncat(buf, app_state.location.city, sizeof(buf) - strlen(buf) - 1);
    }

    if (app_state.location.region[0]) {
        if (buf[0]) strncat(buf, ", ", sizeof(buf) - strlen(buf) - 1);
        strncat(buf, app_state.location.region, sizeof(buf) - strlen(buf) - 1);
    }

    if (app_state.location.country[0]) {
        if (buf[0]) strncat(buf, ", ", sizeof(buf) - strlen(buf) - 1);
        strncat(buf, app_state.location.country, sizeof(buf) - strlen(buf) - 1);
    }

    app_state_unlock();

    lvgl_port_lock(0);
    lv_label_set_text(ui_Label7, buf);
    lvgl_port_unlock();
}

