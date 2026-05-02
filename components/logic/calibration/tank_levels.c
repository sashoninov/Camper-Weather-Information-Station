#include "lvgl.h"
#include "ui.h"

// ====================== CONFIG ======================

// Батерия AGM
#define BATT_MIN_V 11.8f
#define BATT_MAX_V 12.8f

// Чиста вода (литри)
#define CLEAN_WATER_MAX 80.0f

// ====================== HELPERS ======================

static int clamp_percent(float value)
{
    if (value < 0) return 0;
    if (value > 100) return 100;
    return (int)value;
}

// ====================== BATTERY ======================

void update_battery_bar(float voltage)
{
    float percent = (voltage - BATT_MIN_V) / (BATT_MAX_V - BATT_MIN_V) * 100.0f;
    int p = clamp_percent(percent);

    lv_bar_set_value(ui_BarStartBattery, p, LV_ANIM_ON);

    lv_color_t color;

    if (p < 20)
        color = lv_palette_main(LV_PALETTE_RED);
    else if (p < 50)
        color = lv_palette_main(LV_PALETTE_ORANGE);
    else
        color = lv_palette_main(LV_PALETTE_GREEN);

    lv_obj_set_style_bg_color(ui_BarStartBattery, color, LV_PART_INDICATOR);
}

// ====================== CLEAN WATER ======================

void update_clean_water_bar(float liters)
{
    float percent = (liters / CLEAN_WATER_MAX) * 100.0f;
    int p = clamp_percent(percent);

    lv_bar_set_value(ui_BarClearWather, p, LV_ANIM_ON);

    lv_color_t color;

    if (p < 20)
        color = lv_palette_main(LV_PALETTE_RED);
    else if (p < 50)
        color = lv_palette_main(LV_PALETTE_BLUE);
    else
        color = lv_palette_main(LV_PALETTE_GREEN);

    lv_obj_set_style_bg_color(ui_BarClearWather, color, LV_PART_INDICATOR);
}

// ====================== DIRTY WATER ======================

void update_dirty_water_bar(int percent_step)
{
    int p = clamp_percent(percent_step);

    lv_bar_set_value(ui_BarGreyWather, p, LV_ANIM_ON);

    lv_color_t color;

    if (p <= 25)
        color = lv_palette_main(LV_PALETTE_GREEN);
    else if (p <= 50)
        color = lv_palette_main(LV_PALETTE_YELLOW);
    else if (p <= 75)
        color = lv_palette_main(LV_PALETTE_ORANGE);
    else
        color = lv_palette_main(LV_PALETTE_RED);

    lv_obj_set_style_bg_color(ui_BarGreyWather, color, LV_PART_INDICATOR);
}

// ====================== CONVERSIONS ======================

// 👉 ЧИСТА ВОДА (реална калибрация)
float convert_clean_water_liters(float v)
{
    if (v < 0.70f) return 0;
    else if (v < 1.12f) return 5;
    else if (v < 1.37f) return 10;
    else if (v < 1.58f) return 15;
    else if (v < 1.77f) return 20;
    else if (v < 1.95f) return 25;
    else if (v < 2.13f) return 30;
    else if (v < 2.23f) return 35;
    else if (v < 2.30f) return 40;
    else if (v < 2.48f) return 45;
    else if (v < 2.60f) return 50;
    else if (v < 2.69f) return 55;
    else if (v < 2.80f) return 60;
    else if (v < 2.91f) return 65;
    else if (v < 3.06f) return 70;
    else if (v < 3.19f) return 75;
    else return 80;
}

// 👉 МРЪСНА ВОДА (реална калибрация)
float convert_grey_water_percent(float v)
{
    if (v < 0.3f) return 0;
    else if (v < 0.92f) return 25;
    else if (v < 1.37f) return 50;
    else if (v < 1.80f) return 75;
    else if (v < 2.92f) return 100;
    else return 100;
}