#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "app_state.h"
#include "ui.h"   // SquareLine генериран

#include "audio_manager.h"
#include "audio_events.h"



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

void ui_update_wifi_icon(void)
{
    bool wifi_ok;

    app_state_lock();
    wifi_ok = app_state.wifi_connected;
    app_state_unlock();

        // ===================== AUDIO WIFI EVENTS =====================

    if (wifi_ok) {
        // CONNECTED
        if (!app_state.wifi_alerted_connected) {
            audio_play_event(AUDIO_EVENT_WIFI_CONNECTED);
            app_state.wifi_alerted_connected = true;
            app_state.wifi_alerted_lost = false;
        }
    } else {
        // LOST
        if (!app_state.wifi_alerted_lost) {
            audio_play_event(AUDIO_EVENT_WIFI_DISCONNECTED);
            app_state.wifi_alerted_lost = true;
            app_state.wifi_alerted_connected = false;
        }
    }

    // ===================== UI UPDATE =====================

    lvgl_port_lock(0);

    if (wifi_ok) {
        lv_img_set_src(ui_Image9, &ui_img_wifi1_png);
        
        
    } else {
        lv_img_set_src(ui_Image9, &ui_img_wifi_png);
        
    }

    lvgl_port_unlock();
}