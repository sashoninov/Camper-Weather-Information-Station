#ifndef LV_HOURLY_CHART_H
#define LV_HOURLY_CHART_H

#define NUM_HOURS 48
#define MAX_HOURLY_PRECIPITATION 5

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include <time.h>
#include <stdint.h>
#include <stdbool.h>

#define NUM_HOURS 48
#define MAX_HOURLY_PRECIPITATION 5

#define LV_HOURLY_CHART_LABEL_MAX_TEXT_LENGTH 16

// ===== COLORS (ако ги няма в проекта ти) =====
//#define COLOR_BLACK 0x000000
//#define COLOR_WHITE 0xFFFFFF
//#define COLOR_BLUE  0x2196F3
//#define COLOR_PINK  0xE91E63
//#define COLOR_ORANGE 0xFF9800
//#define COLOR_LIGHTYELLOW 0xFFF176

typedef struct {
    struct tm dt;
    double temp;
    double rain;
    double snow;
    double pop;
    double sun;
} lv_hourly_data;

typedef struct {
    lv_obj_t obj;

    lv_hourly_data data_array[NUM_HOURS];
    bool has_data;

    int32_t max_temp;
    int32_t min_temp;
    uint32_t ticks_temp;

    int32_t max_precipitation;
    int32_t min_precipitation;
    uint32_t ticks_precipitation;

} lv_hourly_chart_t;

typedef enum {
    LV_HOURLY_CHART_AXIS_PRIMARY_Y = 0,
    LV_HOURLY_CHART_AXIS_SECONDARY_Y
} lv_hourly_chart_axis_t;

typedef enum {
    LV_HOURLY_CHART_DRAW_PART_DIV_LINE_HOR,
    LV_HOURLY_CHART_DRAW_PART_TEMP,
    LV_HOURLY_CHART_DRAW_PART_CLOUDS,
    LV_HOURLY_CHART_DRAW_PART_PRECIPITATION,
    LV_HOURLY_CHART_DRAW_PART_TICK_LABEL,
} lv_hourly_chart_draw_part_type_t;

extern const lv_obj_class_t lv_hourly_chart_class;

lv_obj_t * lv_hourly_chart_create(lv_obj_t * parent);
void lv_hourly_chart_set_data(lv_obj_t * obj, const lv_hourly_data *data);
void lv_hourly_chart_refresh(lv_obj_t * obj);

#ifdef __cplusplus
}
#endif

#endif