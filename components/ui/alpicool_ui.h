#pragma once
#include "lvgl.h"
#include "alpicool_ble.h"

#ifdef __cplusplus
extern "C" {
#endif

/* SquareLine objects */
extern lv_obj_t * ui_ImgButton3;     // Power ON/OFF
extern lv_obj_t * ui_ImgButton4;     // ECO/MAX
extern lv_obj_t * ui_SliderTemp;     // Temperature slider
extern lv_obj_t * ui_LabelTemp;      // Set temperature
extern lv_obj_t * ui_LabelTemp1;     // Real temperature

/* Icons */
LV_IMG_DECLARE(ui_img_147745890);      // ECO
LV_IMG_DECLARE(ui_img_1461797723);     // MAX
LV_IMG_DECLARE(ui_img_669777041);      // Power OFF
LV_IMG_DECLARE(ui_img_488183857);       // Power ON

/* Event callbacks */
void ui_event_power(lv_event_t * e);
void ui_event_mode(lv_event_t * e);
void ui_event_temp(lv_event_t * e);

/* Update UI from BLE status */
void alpicool_ui_update(const alpicool_status_t *s);

#ifdef __cplusplus
}
#endif
