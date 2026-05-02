#include "ui.h"
#include "app_state.h"

void ui_update_scd41(void)
{
    if (app_state.scd_valid)
    {
        lv_label_set_text_fmt(ui_VOX, "%d ppm", app_state.co2);
        lv_label_set_text_fmt(ui_TempInsaid, "%.1f", app_state.scd_temp);
        lv_label_set_text_fmt(ui_HumidityInsaid, "%.0f", app_state.scd_humidity);

        // =========================
        // 🎨 CO2 COLOR LOGIC
        // =========================
        lv_color_t color;

        if (app_state.co2 < 800)
        {
            color = lv_color_hex(0x00FF00); // зелено
        }
        else if (app_state.co2 < 1200)
        {
            color = lv_color_hex(0xFFA500); // оранжево
        }
        else if (app_state.co2 < 2000)
        {
            color = lv_color_hex(0xFF0000); // червено
        }
        else
        {
            color = lv_color_hex(0x800080); // лилаво (опасно)
        }

        lv_obj_set_style_text_color(ui_VOX, color, 0);
    }
    else
    {
        lv_label_set_text(ui_VOX, "--");
        lv_label_set_text(ui_TempInsaid, "--");
        lv_label_set_text(ui_HumidityInsaid, "--");

        // сив цвят ако няма данни
        lv_obj_set_style_text_color(ui_VOX, lv_color_hex(0x888888), 0);
    }
}