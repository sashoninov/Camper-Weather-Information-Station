#include "ui_update_ina.h"
#include "ui.h"
#include "lvgl.h"
#include "tank_levels.h"
#include "app_state.h"
#include "audio_events.h"
#include "audio_manager.h"

#include <stdio.h>

void ui_update_ina(float ch1, float grey, float clean)
{
    char buf[32];

    // ===================== BATTERY =====================
    snprintf(buf, sizeof(buf), "%.1f", ch1);
    lv_label_set_text(ui_StartBattery, buf);

    // 👉 ТУК ВЕЧЕ ПОДАВАМЕ ВОЛТАЖ, НЕ ПРОЦЕНТ
    update_battery_bar(ch1);

    // ===================== AUDIO STARTER BATTERY EVENTS =====================

    if (ch1 < 11.6f) {
        if (!app_state.starter_batt_critical_alerted) {
            audio_play_event(AUDIO_EVENT_START_BATTERY_CRITICAL);
            app_state.starter_batt_critical_alerted = true;
            app_state.starter_batt_low_alerted = true;
        }
    }
    else if (ch1 < 12.0f) {
        if (!app_state.starter_batt_low_alerted) {
            audio_play_event(AUDIO_EVENT_START_BATTERY_LOW);
            app_state.starter_batt_low_alerted = true;
        }
    }
    else {
        app_state.starter_batt_low_alerted = false;
        app_state.starter_batt_critical_alerted = false;
    }

    // 🎨 COLORS
    if (ch1 < 12.0f)
    {
        lv_obj_set_style_text_color(ui_StartBattery,
            lv_palette_main(LV_PALETTE_RED), 0);
    }
    else if (ch1 < 12.4f)
    {
        lv_obj_set_style_text_color(ui_StartBattery,
            lv_palette_main(LV_PALETTE_ORANGE), 0);
    }
    else
    {
        lv_obj_set_style_text_color(ui_StartBattery,
            lv_color_black(), 0);
    }

    // ===================== DIRTY WATER =====================
    snprintf(buf, sizeof(buf), "%.0f %%", grey);
    lv_label_set_text(ui_GreyWather, buf);

    update_dirty_water_bar((int)grey);

    if (grey > 50.0f)
    {
        lv_obj_set_style_text_color(ui_GreyWather,
            lv_palette_main(LV_PALETTE_RED), 0);
    }
    else
    {
        lv_obj_set_style_text_color(ui_GreyWather,
            lv_color_black(), 0);
    }

    // ===================== CLEAN WATER =====================
    snprintf(buf, sizeof(buf), "%.0f L", clean);
    lv_label_set_text(ui_ClearWather, buf);

    update_clean_water_bar(clean);

    if (clean < 10.0f) {
        if (!app_state.water_low_alerted) {
            audio_play_event(AUDIO_EVENT_WATER_LOW);
            app_state.water_low_alerted = true;
        }
    }
    else {
        app_state.water_low_alerted = false;
    }

    if (clean < 10.0f)
    {
        lv_obj_set_style_text_color(ui_ClearWather,
            lv_palette_main(LV_PALETTE_RED), 0);
    }
    else
    {
        lv_obj_set_style_text_color(ui_ClearWather,
            lv_color_black(), 0);
    }
}
