#pragma once

#include "esp_err.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DISPLAY_WIDTH   1280
#define DISPLAY_HEIGHT  800

esp_err_t display_init(void);
lv_disp_t *display_get(void);

void display_on(void);
void display_off(void);

void display_set_brightness(uint8_t percent);

#ifdef __cplusplus
}
#endif