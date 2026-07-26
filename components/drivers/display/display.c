#include "display.h"
#include "i2c_bus.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_check.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_ldo_regulator.h"
#include "driver/i2c_master.h"

#include "esp_lcd_hx8394.h"
#include "esp_lcd_touch_gt911.h"
#include "esp_lvgl_port.h"

static const char *TAG = "display";

/*
 * Физическа резолюция на панела HX8394.
 * Панелът е ПОРТРЕТЕН по природа: 720x1280.
 * Тук НЕ правим landscape — това ще е следващата стъпка.
 */
#define PHYS_H_RES  720
#define PHYS_V_RES  1280

#define BL_I2C_ADDR        0x45
#define BL_BRIGHTNESS_REG  0x96
#define TOUCH_I2C_ADDR ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS

static i2c_master_dev_handle_t  s_bl_dev  = NULL;
static esp_lcd_panel_handle_t   s_panel   = NULL;
static esp_lcd_dsi_bus_handle_t s_dsi_bus = NULL;
static esp_lcd_touch_handle_t   s_touch   = NULL;
static lv_disp_t *s_lvgl_display = NULL;

/* =========================================================
   BACKLIGHT CONTROL
   ========================================================= */
static esp_err_t init_backlight(void)
{
    i2c_master_bus_handle_t bus = i2c_bus_get_handle();

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = BL_I2C_ADDR,
        .scl_speed_hz    = 100000,
    };

    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(bus, &dev_cfg, &s_bl_dev),
                        TAG, "Backlight device fail");

    // Инициализация на LED драйвера
    uint8_t cmd1[] = {0x95, 0x11};
    i2c_master_transmit(s_bl_dev, cmd1, sizeof(cmd1), 50);

    uint8_t cmd2[] = {0x95, 0x17};
    i2c_master_transmit(s_bl_dev, cmd2, sizeof(cmd2), 50);

    uint8_t cmd3[] = {0x96, 0xFF};
    i2c_master_transmit(s_bl_dev, cmd3, sizeof(cmd3), 50);

    vTaskDelay(pdMS_TO_TICKS(200));
    return ESP_OK;
}

void display_set_brightness(uint8_t percent)
{
    if (!s_bl_dev) return;
    if (percent > 100) percent = 100;

    uint8_t value = (percent * 255) / 100;
    uint8_t cmd[] = {BL_BRIGHTNESS_REG, value};

    i2c_master_transmit(s_bl_dev, cmd, sizeof(cmd), 50);
}

/* =========================================================
   DISPLAY PANEL (HX8394)
   ========================================================= */
static esp_err_t init_display_panel(void)
{
    /*
     * HX8394 изисква 2.5V LDO за DSI PHY.
     */
    esp_ldo_channel_handle_t phy_pwr = NULL;

    esp_ldo_channel_config_t ldo_cfg = {
        .chan_id    = 3,
        .voltage_mv = 2500,
    };

    ESP_RETURN_ON_ERROR(esp_ldo_acquire_channel(&ldo_cfg, &phy_pwr),
                        TAG, "LDO fail");

    /*
     * DSI BUS — 2 lanes, конфигурацията идва от официалния макрос.
     */
    esp_lcd_dsi_bus_config_t bus_cfg = HX8394_PANEL_BUS_DSI_2CH_CONFIG();
    ESP_RETURN_ON_ERROR(esp_lcd_new_dsi_bus(&bus_cfg, &s_dsi_bus),
                        TAG, "DSI bus fail");

    /*
     * DBI канал за командите към панела.
     */
    esp_lcd_panel_io_handle_t dbi_io = NULL;
    esp_lcd_dbi_io_config_t dbi_cfg = HX8394_PANEL_IO_DBI_CONFIG();
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_dbi(s_dsi_bus, &dbi_cfg, &dbi_io),
                        TAG, "DBI fail");

    /*
     * DPI тайминг — ПОРТРЕТЕН.
     * НЕ ГО ПИПАМЕ — HX8394 драйверът не поддържа landscape DPI.
     */
    esp_lcd_dpi_panel_config_t dpi_cfg =
        HX8394_720_1280_PANEL_30HZ_DPI_CONFIG(LCD_COLOR_PIXEL_FORMAT_RGB565);

    dpi_cfg.num_fbs = 1;

    hx8394_vendor_config_t vendor_cfg = {
        .init_cmds      = NULL,
        .init_cmds_size = 0,
        .mipi_config = {
            .dsi_bus    = s_dsi_bus,
            .dpi_config = &dpi_cfg,
            .lane_num   = 2,
        },
    };

    esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = GPIO_NUM_NC,
        .rgb_ele_order  = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
        .vendor_config  = &vendor_cfg,
    };

    ESP_RETURN_ON_ERROR(
        esp_lcd_new_panel_hx8394(dbi_io, &panel_cfg, &s_panel),
        TAG, "Panel fail"
    );

    esp_lcd_panel_reset(s_panel);
    esp_lcd_panel_init(s_panel);
    esp_lcd_panel_disp_on_off(s_panel, true);

    return ESP_OK;
}

/* =========================================================
   TOUCH (GT911)
   ========================================================= */
static esp_err_t init_touch(void)
{
    i2c_master_bus_handle_t bus = i2c_bus_get_handle();

    esp_lcd_panel_io_handle_t tp_io = NULL;

    esp_lcd_panel_io_i2c_config_t tp_io_cfg = {
        .dev_addr            = TOUCH_I2C_ADDR,
        .control_phase_bytes = 1,
        .lcd_cmd_bits        = 16,
        .flags = { .disable_control_phase = 1 },
        .scl_speed_hz        = 400000,
    };

    ESP_RETURN_ON_ERROR(
        esp_lcd_new_panel_io_i2c(bus, &tp_io_cfg, &tp_io),
        TAG, "Touch IO fail"
    );

    esp_lcd_touch_config_t cfg = {
        .x_max        = PHYS_H_RES,
        .y_max        = PHYS_V_RES,
        .rst_gpio_num = -1,
        .int_gpio_num = -1,
    };

    ESP_RETURN_ON_ERROR(
        esp_lcd_touch_new_i2c_gt911(tp_io, &cfg, &s_touch),
        TAG, "Touch fail"
    );

    return ESP_OK;
}

/* =========================================================
   LVGL (ПОРТРЕТЕН РЕЖИМ)
   ========================================================= */
static esp_err_t init_lvgl(void)
{
    ESP_LOGI(TAG, "Initializing LVGL");

    const lvgl_port_cfg_t port_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_RETURN_ON_ERROR(lvgl_port_init(&port_cfg), TAG, "LVGL port init failed");

    ESP_LOGI(TAG, "Registering display (720x1280 portrait)");

    /*
     * Тук LVGL работи в портрет.
     * НЯМА landscape логика — ще я добавим след това.
     */
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle      = NULL,
        .panel_handle   = s_panel,
        .control_handle = NULL,

        .buffer_size    = PHYS_H_RES * 400,
        .double_buffer  = false,

        .hres           = 720,
        .vres           = 1280,

        .monochrome     = false,

        .rotation = {
            .swap_xy  = false,
            .mirror_x = false,
            .mirror_y = false,
        },

        .flags = {
            .buff_dma    = false,
            .buff_spiram = true,
            .sw_rotate   = true,
        },
    };

    const lvgl_port_display_dsi_cfg_t dsi_cfg = {
        .flags = { .avoid_tearing = false },
    };
	
	ESP_LOGI(TAG, "buffer_size=%u", (unsigned)disp_cfg.buffer_size);
	ESP_LOGI(TAG, "hres=%u", (unsigned)disp_cfg.hres);
	ESP_LOGI(TAG, "vres=%u", (unsigned)disp_cfg.vres);


    s_lvgl_display = lvgl_port_add_disp_dsi(&disp_cfg, &dsi_cfg);
    lv_disp_set_rotation(s_lvgl_display, LV_DISPLAY_ROTATION_90);

    ESP_RETURN_ON_FALSE(s_lvgl_display, ESP_FAIL, TAG, "Failed to add display");

    /*
     * TOUCH — без ротация.
     * Ще добавим landscape touch mapping след това.
     */
    ESP_LOGI(TAG, "Registering touch input");
    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp   = s_lvgl_display,
        .handle = s_touch,
    };

    lvgl_port_add_touch(&touch_cfg);

    ESP_LOGI(TAG, "LVGL initialized");
    return ESP_OK;
}

/* =========================================================
   PUBLIC API
   ========================================================= */
esp_err_t display_init(void)
{
    ESP_LOGI(TAG, "Display init (PORTRAIT)");

    ESP_RETURN_ON_ERROR(init_backlight(),     TAG, "BL fail");
    ESP_RETURN_ON_ERROR(init_display_panel(), TAG, "Panel fail");
    ESP_RETURN_ON_ERROR(init_touch(),         TAG, "Touch fail");
    ESP_RETURN_ON_ERROR(init_lvgl(),          TAG, "LVGL fail");

    ESP_LOGI(TAG, "===== Display subsystem ready =====");
    return ESP_OK;
}

lv_disp_t *display_get(void)
{
    return s_lvgl_display;
}

void display_off(void)
{
    display_set_brightness(0);
    if (s_panel) esp_lcd_panel_disp_on_off(s_panel, false);
}

void display_on(void)
{
    if (s_panel) esp_lcd_panel_disp_on_off(s_panel, true);
    display_set_brightness(100);
}
