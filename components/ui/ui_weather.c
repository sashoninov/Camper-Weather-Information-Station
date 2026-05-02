#include "app_state.h"
#include "ui.h"
#include "weather/weather.h"
#include "esp_log.h"

#include "lv_daily_chart.h"
#include "lv_hourly_chart.h"
#include "weather_chart_adapter.h"

static const char *TAG = "UI_WEATHER";

// =========================
// GLOBALS
// =========================
static lv_obj_t *daily_chart = NULL;
static lv_obj_t *hourly_chart = NULL;

static lv_daily_data daily_data[NUM_DAYS];
static lv_hourly_data hourly_data[NUM_HOURS];

// =========================
// ICONS
// =========================
LV_IMG_DECLARE(ui_img_01d_png);
LV_IMG_DECLARE(ui_img_01n_png);
LV_IMG_DECLARE(ui_img_02d_png);
LV_IMG_DECLARE(ui_img_02n_png);
LV_IMG_DECLARE(ui_img_03d_png);
LV_IMG_DECLARE(ui_img_03n_png);
LV_IMG_DECLARE(ui_img_04d_png);
LV_IMG_DECLARE(ui_img_04n_png);
LV_IMG_DECLARE(ui_img_09d_png);
LV_IMG_DECLARE(ui_img_09n_png);
LV_IMG_DECLARE(ui_img_10d_png);
LV_IMG_DECLARE(ui_img_10n_png);
LV_IMG_DECLARE(ui_img_11d_png);
LV_IMG_DECLARE(ui_img_11n_png);
LV_IMG_DECLARE(ui_img_13d_png);
LV_IMG_DECLARE(ui_img_13n_png);
LV_IMG_DECLARE(ui_img_50d_png);
LV_IMG_DECLARE(ui_img_50n_png);

// =========================
// ICON MAPPING
// =========================
static const void* get_weather_icon(int code, bool is_day)
{
    switch (code)
    {
        case 0: return is_day ? &ui_img_01d_png : &ui_img_01n_png;
        case 1:
        case 2: return is_day ? &ui_img_02d_png : &ui_img_02n_png;
        case 3: return is_day ? &ui_img_03d_png : &ui_img_03n_png;
        case 45:
        case 48: return is_day ? &ui_img_50d_png : &ui_img_50n_png;

        case 51: case 53: case 55:
        case 61: case 63: case 65:
        case 80: case 81: case 82:
            return is_day ? &ui_img_09d_png : &ui_img_09n_png;

        case 71: case 73: case 75:
        case 85: case 86:
            return is_day ? &ui_img_13d_png : &ui_img_13n_png;

        case 95: case 96: case 99:
            return is_day ? &ui_img_11d_png : &ui_img_11n_png;

        default:
            return is_day ? &ui_img_03d_png : &ui_img_03n_png;
    }
}

// =========================
// MAIN UPDATE
// =========================
void ui_update_weather(void * param)
{
    (void)param;

    if (!ui_TempCurrent) {
        ESP_LOGW(TAG, "UI not ready");
        return;
    }

    // ================= CURRENT =================
    lv_label_set_text_fmt(ui_TempCurrent, "%.1f", app_state.weather.current_temp);

    if (ui_WindSpeedCurrent)
        lv_label_set_text_fmt(ui_WindSpeedCurrent, "%.1f", app_state.weather.wind_speed);

    if (ui_WindGustCurrent)
        lv_label_set_text_fmt(ui_WindGustCurrent, "%.1f", app_state.weather.wind_gust);

    if (ui_CloudsCurrent)
        lv_label_set_text_fmt(ui_CloudsCurrent, "%.0f", app_state.weather.cloudcover);

    if (ui_UvCurrent)
        lv_label_set_text_fmt(ui_UvCurrent, "%.0f", app_state.weather.uv);

    if (ui_SunriseCurrent)
        lv_label_set_text(ui_SunriseCurrent, app_state.weather.sunrise[0]);

    if (ui_SunsetCurrent)
        lv_label_set_text(ui_SunsetCurrent, app_state.weather.sunset[0]);

    if (ui_Pressure4)
        lv_label_set_text_fmt(ui_Pressure4, "%.0f", app_state.weather.pressure);

    if (ui_WeatherIcon)
        lv_img_set_src(ui_WeatherIcon,
            get_weather_icon(app_state.weather.weather_code, app_state.weather.is_day));

    // ================= WIND ROTATION =================
    if (ui_WindDirectionCurrentIcon)
    {
        lv_img_set_pivot(ui_WindDirectionCurrentIcon,
            lv_obj_get_width(ui_WindDirectionCurrentIcon)/2,
            lv_obj_get_height(ui_WindDirectionCurrentIcon)/2);

        int angle = (int)((app_state.weather.wind_dir + 90.0f) * 10.0f);
        if (angle >= 3600) angle -= 3600;

        lv_img_set_angle(ui_WindDirectionCurrentIcon, angle);
    }

    // ================= SEA =================
    if (ui_TempSea)
    {
        if (app_state.weather.has_sea)
        {
            lv_obj_clear_flag(ui_PanelSea, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text_fmt(ui_TempSea, "%.1f°C", app_state.weather.sea_temp);
        }
        else
        {
            lv_obj_add_flag(ui_PanelSea, LV_OBJ_FLAG_HIDDEN);
        }
    }

    // ================= HOURLY CHART =================
    if (ui_HourlyChart)
    {
        if (!hourly_chart)
        {
            hourly_chart = lv_hourly_chart_create(ui_HourlyChart);
            lv_coord_t w = lv_obj_get_content_width(ui_HourlyChart);
			lv_coord_t h = lv_obj_get_content_height(ui_HourlyChart);
			
			// 🔥 shrink равномерно
			int shrink = 5;
			
			lv_obj_set_size(hourly_chart,
				w - shrink,
				h - shrink);
			
			// 🔥 центриране (автоматично балансира всички страни)
			lv_obj_center(hourly_chart);
        }

        weather_to_hourly_chart(&app_state.weather, hourly_data);
        lv_hourly_chart_set_data(hourly_chart, hourly_data);
		lv_hourly_chart_refresh(hourly_chart);
    }

    // ================= DAILY CHART =================
    if (ui_DailyChart)
    {
        if (!daily_chart)
        {
            daily_chart = lv_daily_chart_create(ui_DailyChart);
			lv_obj_set_style_pad_column(daily_chart, 4, 0);
			lv_coord_t w = lv_obj_get_content_width(ui_DailyChart);
			lv_coord_t h = lv_obj_get_content_height(ui_DailyChart);
			
			int shrink = 5;
			
			lv_obj_set_size(daily_chart,
				w - shrink,
				h - shrink);
			
			lv_obj_center(daily_chart);
        }

        weather_to_daily_chart(&app_state.weather, daily_data);
        lv_daily_chart_set_data(daily_chart, daily_data);
		lv_daily_chart_refresh(daily_chart);
    }

    ESP_LOGI(TAG, "Weather UI updated");
}