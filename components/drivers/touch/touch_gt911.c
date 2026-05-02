#include "touch_gt911.h"
#include "i2c_bus.h"

#include "esp_log.h"
#include "esp_check.h"
#include "esp_lcd_touch_gt911.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_io_i2c.h"

#include "esp_lvgl_port.h"   // 🔥 важно

#define TOUCH_I2C_ADDR ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS

static const char *TAG = "TOUCH";

static esp_lcd_touch_handle_t s_touch = NULL;

/* =========================================================
   INIT (ОРИГИНАЛЕН СТИЛ)
   ========================================================= */

esp_err_t touch_init(void)
{
    ESP_LOGI(TAG, "Initializing GT911 touch");

    i2c_master_bus_handle_t bus = i2c_bus_get_handle();

    esp_lcd_panel_io_handle_t tp_io = NULL;

    esp_lcd_panel_io_i2c_config_t tp_io_cfg = {
        .dev_addr            = TOUCH_I2C_ADDR,
        .control_phase_bytes = 1,
        .dc_bit_offset       = 0,
        .lcd_cmd_bits        = 16,
        .flags = {
            .disable_control_phase = 1,
        },
        .scl_speed_hz = 400000,
    };

    ESP_RETURN_ON_ERROR(
        esp_lcd_new_panel_io_i2c(bus, &tp_io_cfg, &tp_io),
        TAG, "Failed to create touch IO"
    );

    esp_lcd_touch_config_t touch_cfg = {
        .x_max        = 800,
        .y_max        = 1280,
        .rst_gpio_num = -1,
        .int_gpio_num = -1,
        .levels = {
            .reset     = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy  = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
    };

    ESP_RETURN_ON_ERROR(
        esp_lcd_touch_new_i2c_gt911(tp_io, &touch_cfg, &s_touch),
        TAG, "Failed to create touch driver"
    );

    ESP_LOGI(TAG, "Touch controller initialized");

    /* =========================================================
       🔥 ВРЪЗКА КЪМ LVGL (ОРИГИНАЛНИЯ НАЧИН)
       ========================================================= */

    const lvgl_port_touch_cfg_t lvgl_touch_cfg = {
        .disp   = NULL,              // 🔥 важно
        .handle = s_touch,
    };

    lv_indev_t *indev = lvgl_port_add_touch(&lvgl_touch_cfg);

    if (indev == NULL) {
        ESP_LOGW(TAG, "Failed to add touch to LVGL");
    } else {
        ESP_LOGI(TAG, "Touch registered to LVGL");
    }

    return ESP_OK;
}