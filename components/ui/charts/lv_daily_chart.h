#ifndef LV_DAILY_CHART_H
#define LV_DAILY_CHART_H

#define NUM_DAYS 7
#define MAX_DAILY_PRECIPITATION 20

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include <time.h>
#include <stdint.h>
#include <stdbool.h>

#define NUM_DAYS 7
#define MAX_DAILY_PRECIPITATION 20

#define LV_DAILY_CHART_LABEL_MAX_TEXT_LENGTH 16

// ===== COLORS (ако ги няма в проекта) =====
#define COLOR_BLACK 0x000000
#define COLOR_WHITE 0xFFFFFF
#define COLOR_BLUE  0x2196F3
#define COLOR_PINK  0xE91E63
#define COLOR_RED   0xF44336
#define COLOR_DARKBLUE 0x0D47A1
#define COLOR_LIGHTYELLOW 0xFFF176

// ===== DAY NAMES =====


typedef struct {
    struct tm dt;
    double low_temp;
    double high_temp;
    double rain;
    double snow;
    double pop;
    double sun;
} lv_daily_data;

typedef struct {
    lv_obj_t obj;

    lv_daily_data data_array[NUM_DAYS];
    bool has_data;

    int32_t max_temp;
    int32_t min_temp;
    uint32_t ticks_temp;

    int32_t max_precipitation;
    int32_t min_precipitation;
    uint32_t ticks_precipitation;

} lv_daily_chart_t;

typedef enum {
    LV_DAILY_CHART_AXIS_PRIMARY_Y = 0,
    LV_DAILY_CHART_AXIS_SECONDARY_Y
} lv_daily_chart_axis_t;

typedef enum {
    LV_DAILY_CHART_DRAW_PART_DIV_LINE_HOR,
    LV_DAILY_CHART_DRAW_PART_TEMP,
    LV_DAILY_CHART_DRAW_PART_CLOUDS,
    LV_DAILY_CHART_DRAW_PART_PRECIPITATION,
    LV_DAILY_CHART_DRAW_PART_TICK_LABEL,
} lv_daily_chart_draw_part_type_t;

extern const lv_obj_class_t lv_daily_chart_class;

lv_obj_t * lv_daily_chart_create(lv_obj_t * parent);
void lv_daily_chart_set_data(lv_obj_t * obj, const lv_daily_data *data);
void lv_daily_chart_refresh(lv_obj_t * obj);

#ifdef __cplusplus
}
#endif

#endif