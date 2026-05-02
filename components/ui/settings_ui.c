#include "settings.h"
#include "ui.h"
#include "lvgl.h"
#include "app_state.h"
#include "wifi.h"
#include "wifi_storage.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_wifi.h"      // ⭐ ЛИПСВАЩИЯТ include
#include "esp_log.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "audio_manager.h"
#include "audio_events.h"



#define MAX_WIFI 20
static char wifi_list[MAX_WIFI][33];

// ===================== SAFE COPY HELPERS =====================

static void safe_copy(char *dst, const char *src, size_t size)
{
    size_t len = strlen(src);
    if (len >= size) len = size - 1;
    memcpy(dst, src, len);
    dst[len] = '\0';
}

static void safe_append(char *dst, const char *src, size_t size)
{
    size_t len = strlen(dst);
    size_t slen = strlen(src);
    if (len + slen + 1 >= size) slen = size - len - 1;
    memcpy(dst + len, src, slen);
    dst[len + slen] = '\0';
}

// ===================== TEXT COLOR FLASH =====================

static lv_color_t original_text_color;
static bool original_color_saved = false;

typedef struct {
    bool success;
} flash_ctx_t;

static void flash_text_timer_cb(lv_timer_t *t)
{
    lv_obj_set_style_text_color(ui_Label2, original_text_color, LV_PART_MAIN | LV_STATE_DEFAULT);

    free(t->user_data);
    lv_timer_del(t);
}

static void flash_text(bool success)
{
    if (!original_color_saved) {
        original_text_color = lv_obj_get_style_text_color(ui_Label2, LV_PART_MAIN | LV_STATE_DEFAULT);
        original_color_saved = true;
    }

    if (success) {
        lv_obj_set_style_text_color(ui_Label2, lv_color_hex(0x00CC00), LV_PART_MAIN | LV_STATE_DEFAULT);
    } else {
        lv_obj_set_style_text_color(ui_Label2, lv_color_hex(0xCC0000), LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    flash_ctx_t *ctx = malloc(sizeof(flash_ctx_t));
    ctx->success = success;

    lv_timer_create(flash_text_timer_cb, 3000, ctx);
}

// ===================== UI UPDATE =====================

void ui_update_settings_screen(void)
{
    char buf[16];

    lv_slider_set_value(ui_SliderBrightness, g_settings.manual_brightness, LV_ANIM_OFF);
    lv_slider_set_value(ui_SliderDim, g_settings.dim_level, LV_ANIM_OFF);

    if (g_settings.autodim)
        lv_obj_add_state(ui_SwitchAutodim, LV_STATE_CHECKED);
    else
        lv_obj_clear_state(ui_SwitchAutodim, LV_STATE_CHECKED);

    snprintf(buf, sizeof(buf), "%02d:%02d", g_settings.start_hour, g_settings.start_min);
    lv_textarea_set_text(ui_textareaStart, buf);

    snprintf(buf, sizeof(buf), "%02d:%02d", g_settings.end_hour, g_settings.end_min);
    lv_textarea_set_text(ui_textareaEnd, buf);

    // WiFi password
    lv_textarea_set_text(ui_TextAreaPassword, g_settings.wifi_pass);

    // ⭐ Поправена логика за SSID – не ползваме lv_dropdown_set_text ⭐
    const char *opts = lv_dropdown_get_options(ui_DropdownSSID);

    if (strlen(g_settings.wifi_ssid) > 0 && strlen(opts) > 0)
    {
        int index = 0;
        const char *p = opts;

        while (*p)
        {
            if (strcmp(p, g_settings.wifi_ssid) == 0) {
                lv_dropdown_set_selected(ui_DropdownSSID, index);
                break;
            }

            p += strlen(p) + 1;
            index++;
        }
    }
}

// ===================== TIME PARSE =====================

static void parse_time(const char *txt, uint8_t *h, uint8_t *m)
{
    int hh = 0, mm = 0;

    if (sscanf(txt, "%d:%d", &hh, &mm) == 2) {
        if (hh >= 0 && hh <= 23 && mm >= 0 && mm <= 59) {
            *h = hh;
            *m = mm;
        }
    }
}

// ===================== WIFI SCAN =====================

static void wifi_scan_cb(lv_event_t * e)
{
    // ⭐ FIX за ESP32-P4 WiFi6 remote driver:
    //    трябва да рестартираме WiFi преди scan, иначе връща 12294
    esp_wifi_disconnect();
    vTaskDelay(pdMS_TO_TICKS(100));

    esp_wifi_stop();
    vTaskDelay(pdMS_TO_TICKS(100));

    esp_wifi_start();
    vTaskDelay(pdMS_TO_TICKS(100));

    int count = wifi_scan(wifi_list, MAX_WIFI);

    char options[512] = {0};

    for (int i = 0; i < count; i++) {
        safe_append(options, wifi_list[i], sizeof(options));
        if (i < count - 1) safe_append(options, "\n", sizeof(options));
    }

    lv_dropdown_set_options(ui_DropdownSSID, options);

    if (count > 0)
        lv_dropdown_set_selected(ui_DropdownSSID, 0);
}

// ===================== WIFI CONNECT TASK =====================

typedef struct {
    char ssid[33];
    char pass[64];
} wifi_task_params_t;

static void wifi_connect_task(void *pvParameter)
{
    wifi_task_params_t *p = (wifi_task_params_t *)pvParameter;

    printf("Connecting to %s...\n", p->ssid);

    if (wifi_test_connection(p->ssid, p->pass)) {

        audio_play_event(AUDIO_EVENT_SAVED);
        flash_text(true);

        safe_copy(g_settings.wifi_ssid, p->ssid, sizeof(g_settings.wifi_ssid));
        safe_copy(g_settings.wifi_pass, p->pass, sizeof(g_settings.wifi_pass));

        settings_save(&g_settings);
        wifi_save_credentials(p->ssid, p->pass);

        wifi_set_credentials(p->ssid, p->pass);

        printf("WiFi OK\n");
        

    } else {

        audio_play_event(AUDIO_EVENT_CANCELED);
        flash_text(false);
        printf("WiFi FAILED\n");
        
    }

    free(p);
    vTaskDelete(NULL);
}

// ===================== WIFI VALIDATE =====================

static void wifi_validate_cb(lv_event_t * e)
{
    char ssid[33];
    char pass[64];

    lv_dropdown_get_selected_str(ui_DropdownSSID, ssid, sizeof(ssid));

    if (ssid[0] == '\0') {
        flash_text(false);
        return;
    }

    const char *txt = lv_textarea_get_text(ui_TextAreaPassword);
    safe_copy(pass, txt, sizeof(pass));

    wifi_task_params_t *params = malloc(sizeof(wifi_task_params_t));
    if (!params) {
        flash_text(false);
        return;
    }

    safe_copy(params->ssid, ssid, sizeof(params->ssid));
    safe_copy(params->pass, pass, sizeof(params->pass));

    xTaskCreate(wifi_connect_task, "wifi_task", 4096, params, 5, NULL);
}

// ===================== SAVE SETTINGS =====================

static void save_settings_cb(lv_event_t * e)
{
    const char *start_txt = lv_textarea_get_text(ui_textareaStart);
    parse_time(start_txt, &g_settings.start_hour, &g_settings.start_min);

    const char *end_txt = lv_textarea_get_text(ui_textareaEnd);
    parse_time(end_txt, &g_settings.end_hour, &g_settings.end_min);

    settings_save(&g_settings);
}

// ===================== SLIDERS =====================

static void brightness_cb(lv_event_t * e)
{
    g_settings.manual_brightness = lv_slider_get_value(ui_SliderBrightness);
    settings_save(&g_settings);
}

static void dim_cb(lv_event_t * e)
{
    g_settings.dim_level = lv_slider_get_value(ui_SliderDim);
    settings_save(&g_settings);
}

// ===================== SWITCH =====================

static void autodim_cb(lv_event_t * e)
{
    g_settings.autodim =
        lv_obj_has_state(ui_SwitchAutodim, LV_STATE_CHECKED);

    settings_save(&g_settings);
}

// ===================== TIME =====================

static void start_time_cb(lv_event_t * e)
{
    const char *txt = lv_textarea_get_text(ui_textareaStart);
    parse_time(txt, &g_settings.start_hour, &g_settings.start_min);
    settings_save(&g_settings);
}

static void end_time_cb(lv_event_t * e)
{
    const char *txt = lv_textarea_get_text(ui_textareaEnd);
    parse_time(txt, &g_settings.end_hour, &g_settings.end_min);
    settings_save(&g_settings);
}

// ===================== CALIBRATION =====================

static void calibrate_cb(lv_event_t * e)
{
    app_state_lock();
    app_state.request_calibration = true;
    app_state_unlock();
}

// ===================== INIT EVENTS =====================

void ui_settings_init_events(void)
{
    lv_obj_add_event_cb(ui_SliderBrightness, brightness_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(ui_SliderDim, dim_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_add_event_cb(ui_SwitchAutodim, autodim_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_add_event_cb(ui_textareaStart, start_time_cb, LV_EVENT_DEFOCUSED, NULL);
    lv_obj_add_event_cb(ui_textareaEnd, end_time_cb, LV_EVENT_DEFOCUSED, NULL);

    lv_obj_add_event_cb(ui_CalibrateLevel, calibrate_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_add_event_cb(ui_SaveButton, save_settings_cb, LV_EVENT_CLICKED, NULL);

    // WiFi
    lv_obj_add_event_cb(ui_ButtonSCAN, wifi_scan_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui_ButtonENTER, wifi_validate_cb, LV_EVENT_CLICKED, NULL);
}
