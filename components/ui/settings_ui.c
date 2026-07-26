//==============================
//
// settings_ui.c
//
// PART 1
//
//==============================

#include "settings.h"
#include "ui.h"
#include "lvgl.h"
#include "app_state.h"

#include "wifi.h"
#include "wifi_storage.h"

#include "audio_manager.h"
#include "audio_events.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//---------------------------------------------------------
// SoftAP configuration
//---------------------------------------------------------

static softap_config_t ap_cfg;

//---------------------------------------------------------
// Helpers
//---------------------------------------------------------

static void safe_copy(char *dst, const char *src, size_t size)
{
    if (!dst || !src || size == 0)
        return;

    size_t len = strlen(src);

    if (len >= size)
        len = size - 1;

    memcpy(dst, src, len);
    dst[len] = 0;
}

//---------------------------------------------------------
// Flash label
//---------------------------------------------------------

//static lv_color_t original_text_color;
//static bool original_color_saved = false;
//
//static void flash_timer_cb(lv_timer_t *t)
//{
//    lv_obj_set_style_text_color(
//        ui_Label2,
//        original_text_color,
//        LV_PART_MAIN | LV_STATE_DEFAULT);
//
//    lv_timer_del(t);
//}
//
//static void flash_label(bool ok)
//{
//    if (!original_color_saved)
//    {
//        original_text_color =
//            lv_obj_get_style_text_color(
//                ui_Label2,
//                LV_PART_MAIN | LV_STATE_DEFAULT);
//
//        original_color_saved = true;
//    }
//
//    lv_obj_set_style_text_color(
//        ui_Label2,
//        ok ? lv_color_hex(0x00CC00)
//           : lv_color_hex(0xCC0000),
//        LV_PART_MAIN | LV_STATE_DEFAULT);
//
//    lv_timer_create(
//        flash_timer_cb,
//        2500,
//        NULL);
//}

//---------------------------------------------------------
// Time parser
//---------------------------------------------------------

static void parse_time(
    const char *txt,
    uint8_t *hour,
    uint8_t *min)
{
    int h = 0;
    int m = 0;

    if (sscanf(txt, "%d:%d", &h, &m) != 2)
        return;

    if (h < 0 || h > 23)
        return;

    if (m < 0 || m > 59)
        return;

    *hour = h;
    *min = m;
}

//---------------------------------------------------------
// Update screen
//---------------------------------------------------------

void ui_update_settings_screen(void)

{
    char buf[16];
	
	wifi_load_ap_config(&ap_cfg);

    lv_slider_set_value(
        ui_SliderBrightness,
        g_settings.manual_brightness,
        LV_ANIM_OFF);

    lv_slider_set_value(
        ui_SliderDim,
        g_settings.dim_level,
        LV_ANIM_OFF);

    if (g_settings.autodim)
        lv_obj_add_state(
            ui_SwitchAutodim,
            LV_STATE_CHECKED);
    else
        lv_obj_clear_state(
            ui_SwitchAutodim,
            LV_STATE_CHECKED);

    snprintf(
        buf,
        sizeof(buf),
        "%02d:%02d",
        g_settings.start_hour,
        g_settings.start_min);

    lv_textarea_set_text(
        ui_textareaStart,
        buf);

    snprintf(
        buf,
        sizeof(buf),
        "%02d:%02d",
        g_settings.end_hour,
        g_settings.end_min);

    lv_textarea_set_text(
        ui_textareaEnd,
        buf);


    lv_textarea_set_text(
        ui_TextAreaPassword,
        ap_cfg.password);
}

//---------------------------------------------------------
// Brightness
//---------------------------------------------------------

static void brightness_cb(lv_event_t *e)
{
    (void)e;

    g_settings.manual_brightness =
        lv_slider_get_value(ui_SliderBrightness);
}

//---------------------------------------------------------
// Dim level
//---------------------------------------------------------

static void dim_cb(lv_event_t *e)
{
    (void)e;

    g_settings.dim_level =
        lv_slider_get_value(ui_SliderDim);
}

//---------------------------------------------------------
// Auto dim
//---------------------------------------------------------

static void autodim_cb(lv_event_t *e)
{
    (void)e;

    g_settings.autodim =
        lv_obj_has_state(
            ui_SwitchAutodim,
            LV_STATE_CHECKED);
}

//---------------------------------------------------------
// Time callbacks
//---------------------------------------------------------

static void start_time_cb(lv_event_t *e)
{
    (void)e;

    parse_time(
        lv_textarea_get_text(ui_textareaStart),
        &g_settings.start_hour,
        &g_settings.start_min);
}

static void end_time_cb(lv_event_t *e)
{
    (void)e;

    parse_time(
        lv_textarea_get_text(ui_textareaEnd),
        &g_settings.end_hour,
        &g_settings.end_min);
}

//---------------------------------------------------------
// Calibration
//---------------------------------------------------------

static void calibrate_cb(lv_event_t *e)
{
    (void)e;

    app_state_lock();

    app_state.request_calibration = true;

    app_state_unlock();
}

//---------------------------------------------------------
// Save settings
//---------------------------------------------------------

static void save_settings_cb(lv_event_t *e)
{
    (void)e;

    parse_time(
        lv_textarea_get_text(ui_textareaStart),
        &g_settings.start_hour,
        &g_settings.start_min);

    parse_time(
        lv_textarea_get_text(ui_textareaEnd),
        &g_settings.end_hour,
        &g_settings.end_min);

    safe_copy(
        ap_cfg.password,
        lv_textarea_get_text(ui_TextAreaPassword),
        sizeof(ap_cfg.password));

	esp_err_t err = wifi_save_ap_config(&ap_cfg);
	
	if (err != ESP_OK)
	{
		printf("wifi_save_ap_config() failed: %s\n", esp_err_to_name(err));
	
		audio_play_event(AUDIO_EVENT_CANCELED);
		
		return;
	}

    if (!settings_save(&g_settings))
    {
        audio_play_event(AUDIO_EVENT_CANCELED);
        
        return;
    }

    wifi_restart();

    audio_play_event(AUDIO_EVENT_SAVED);
    
}

//---------------------------------------------------------
// Init Events
//---------------------------------------------------------

void ui_settings_init_events(void)
{
    lv_obj_add_event_cb(
        ui_SliderBrightness,
        brightness_cb,
        LV_EVENT_VALUE_CHANGED,
        NULL);

    lv_obj_add_event_cb(
        ui_SliderDim,
        dim_cb,
        LV_EVENT_VALUE_CHANGED,
        NULL);

    lv_obj_add_event_cb(
        ui_SwitchAutodim,
        autodim_cb,
        LV_EVENT_VALUE_CHANGED,
        NULL);

    lv_obj_add_event_cb(
        ui_textareaStart,
        start_time_cb,
        LV_EVENT_DEFOCUSED,
        NULL);

    lv_obj_add_event_cb(
        ui_textareaEnd,
        end_time_cb,
        LV_EVENT_DEFOCUSED,
        NULL);

    lv_obj_add_event_cb(
        ui_CalibrateLevel,
        calibrate_cb,
        LV_EVENT_CLICKED,
        NULL);

    lv_obj_add_event_cb(
        ui_SaveButton,
        save_settings_cb,
        LV_EVENT_CLICKED,
        NULL);
}