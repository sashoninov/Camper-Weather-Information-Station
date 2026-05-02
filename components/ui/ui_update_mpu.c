#include "ui.h"
#include "lvgl.h"
#include <stdio.h>
#include <math.h>

static lv_color_t get_level_color(float angle)
{
    float a = fabsf(angle);

    if (a < 2.0f)
        return lv_palette_main(LV_PALETTE_GREEN);
    else if (a < 5.0f)
        return lv_palette_main(LV_PALETTE_YELLOW);
    else
        return lv_palette_main(LV_PALETTE_RED);
}

void ui_update_mpu(float pitch, float roll)
{
    char buf[32];

    // текст (ако имаш label-и)
    snprintf(buf, sizeof(buf), "%.1f°", pitch);
    lv_label_set_text(ui_LabelPitch, buf);

    snprintf(buf, sizeof(buf), "%.1f°", roll);
    lv_label_set_text(ui_LabelRoll, buf);

    // 👉 SLIDER управление

    // 👉 директно използваме -100 до +100
    int pitch_val = (int)(pitch * 2.0f);
    int roll_val  = (int)(roll  * 2.0f);

    // ограничаване
    if (pitch_val > 100) pitch_val = 100;
    if (pitch_val < -100) pitch_val = -100;

    if (roll_val > 100) roll_val = 100;
    if (roll_val < -100) roll_val = -100;

    lv_slider_set_value(ui_SliderPitch, pitch_val, LV_ANIM_OFF);
    lv_slider_set_value(ui_SliderRoll, roll_val, LV_ANIM_OFF);


    lv_color_t pitch_color = get_level_color(pitch);
    lv_color_t roll_color  = get_level_color(roll);

    // оцветяване на бара (индикатора)
    //lv_obj_set_style_bg_color(ui_SliderPitch, pitch_color, LV_PART_INDICATOR);
    //lv_obj_set_style_bg_color(ui_SliderRoll,  roll_color,  LV_PART_INDICATOR);

    //оцветяване на копчето
    lv_obj_set_style_bg_color(ui_SliderPitch, pitch_color, LV_PART_KNOB);
    lv_obj_set_style_bg_color(ui_SliderRoll,  roll_color,  LV_PART_KNOB);

}