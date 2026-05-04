#include "ui.h"
#include "app_state.h"

#include "audio_manager.h"
#include "audio_events.h"
#include "status.h"

// 🔋 Battery % функция (12V система)
static int battery_percent(float v)
{
    if (v >= 12.7) return 100;
    if (v >= 12.5) return 90;
    if (v >= 12.3) return 75;
    if (v >= 12.2) return 60;
    if (v >= 12.0) return 40;
    if (v >= 11.8) return 20;
    if (v >= 11.6) return 10;
    return 5;
}

void ui_update_victron(void)
{
    if (app_state.mppt_valid)
    {
        float v = app_state.battery_voltage;
        int percent = battery_percent(v);

        // camper battery Web
        g_status.camper_v = v;
        g_status.camper_a = app_state.battery_current;

        // 🔋 Battery Voltage
        lv_label_set_text_fmt(ui_CampBattery, "%.1f", v);

        // 🔋 Battery BAR
        lv_bar_set_value(ui_BarCampBattery, percent, LV_ANIM_ON);

        // 🔌 Battery Current
        lv_label_set_text_fmt(ui_SolarCurent, "%.1f A",
                              app_state.battery_current);

        // ☀️ Solar Voltage
        lv_label_set_text_fmt(ui_SolarVolt, "%.2f V",
                              app_state.solar_voltage);

        // ⚡ Solar Power
        lv_label_set_text_fmt(ui_SolarWat, "%d W",
                              app_state.solar_power);

        // 🔄 Charge State
        switch (app_state.charge_state)
        {
            case 0: lv_label_set_text(ui_SolarStatus, "OFF"); break;
            case 2: lv_label_set_text(ui_SolarStatus, "FAULT"); break;
            case 3: lv_label_set_text(ui_SolarStatus, "BULK"); break;
            case 4: lv_label_set_text(ui_SolarStatus, "ABS"); break;
            case 5: lv_label_set_text(ui_SolarStatus, "FLOAT"); break;
            default: lv_label_set_text(ui_SolarStatus, "IDLE"); break;
        }

        // ===================== AUDIO BATTERY EVENTS =====================

        // CRITICAL < 11.6V
        if (v < 11.6f)
        {
            if (!app_state.victron_batt_critical_alerted)
            {
                audio_play_event(AUDIO_EVENT_BATTERY_CRITICAL);

                app_state.victron_batt_critical_alerted = true;
                app_state.victron_batt_low_alerted = true;   // блокира LOW
            }
        }
        // LOW < 12.0V
        else if (v < 12.0f)
        {
            if (!app_state.victron_batt_low_alerted)
            {
                audio_play_event(AUDIO_EVENT_BATTERY_LOW);

                app_state.victron_batt_low_alerted = true;
            }
        }
        // NORMAL ≥ 12.0V → нулираме флаговете
        else
        {
            app_state.victron_batt_low_alerted = false;
            app_state.victron_batt_critical_alerted = false;
        }

        // 🎨 BATTERY COLOR LOGIC
        if (v < 12.0f)
        {
            // 🔴 разредена
            lv_obj_set_style_text_color(ui_CampBattery,
                                       lv_color_hex(0xF44336), 0);

            lv_obj_set_style_bg_color(ui_BarCampBattery,
                                     lv_color_hex(0xF44336),
                                     LV_PART_INDICATOR);
        }
        else if (v < 12.4f)
        {
            // 🟡 средна
            lv_obj_set_style_text_color(ui_CampBattery,
                                       lv_color_hex(0xFFC107), 0);

            lv_obj_set_style_bg_color(ui_BarCampBattery,
                                     lv_color_hex(0xFFC107),
                                     LV_PART_INDICATOR);
        }
        else
        {
            // ⚫ заредена (черен текст)
            lv_obj_set_style_text_color(ui_CampBattery,
                                       lv_color_hex(0x000000), 0);

            lv_obj_set_style_bg_color(ui_BarCampBattery,
                                     lv_color_hex(0x4CAF50),
                                     LV_PART_INDICATOR);
        }

        // 🎨 Solar power color
        if (app_state.solar_power > 100)
        {
            lv_obj_set_style_text_color(ui_SolarWat,
                                       lv_color_hex(0x4CAF50), 0);
        }
        else if (app_state.solar_power > 20)
        {
            lv_obj_set_style_text_color(ui_SolarWat,
                                       lv_color_hex(0xFFC107), 0);
        }
        else
        {
            lv_obj_set_style_text_color(ui_SolarWat,
                                       lv_color_hex(0x888888), 0);
        }
    }
    else
    {
        lv_label_set_text(ui_CampBattery, "--");
        lv_label_set_text(ui_SolarCurent, "--");
        lv_label_set_text(ui_SolarVolt, "--");
        lv_label_set_text(ui_SolarWat, "--");
        lv_label_set_text(ui_SolarStatus, "--");

        lv_bar_set_value(ui_BarCampBattery, 0, LV_ANIM_OFF);
    }
}
