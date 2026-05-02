#pragma once

#include "esp_err.h"
#include "esp_lcd_touch.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t touch_init(void);
esp_lcd_touch_handle_t touch_get_handle(void);

#ifdef __cplusplus
}
#endif