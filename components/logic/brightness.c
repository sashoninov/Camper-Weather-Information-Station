#include "brightness.h"
#include "settings.h"
#include "esp_timer.h"
#include <time.h>
#include <math.h>
#include "app_state.h"
#include "power_manager.h"   // ⭐ важно

static bool wake_override = false;
static int64_t last_touch = 0;

#define WAKE_TIMEOUT_MS 30000
#define MIN_BRIGHTNESS  5

static float lux_filtered = -1.0f;
static uint8_t last_brightness = 0;

#define BRIGHTNESS_HYSTERESIS 3

static bool is_dim_time()
{
    time_t now;
    time(&now);

    struct tm t;
    localtime_r(&now, &t);

    int now_m = t.tm_hour * 60 + t.tm_min;
    int start = g_settings.start_hour * 60 + g_settings.start_min;
    int end   = g_settings.end_hour * 60 + g_settings.end_min;

    if (start < end)
        return (now_m >= start && now_m < end);
    else
        return (now_m >= start || now_m < end);
}

void brightness_touch_event()
{
    if (is_dim_time()) {
        last_touch = esp_timer_get_time() / 1000;
        wake_override = true;
    }
}

static bool is_wake_active()
{
    int64_t now = esp_timer_get_time() / 1000;

    if (wake_override && (now - last_touch < WAKE_TIMEOUT_MS))
        return true;

    wake_override = false;
    return false;
}

static float filter_lux(float lux)
{
    const float alpha = 0.15f;

    if (lux_filtered < 0.0f) {
        lux_filtered = lux;
        return lux_filtered;
    }

    lux_filtered = lux_filtered * (1.0f - alpha) + lux * alpha;
    return lux_filtered;
}

static uint8_t calc_auto(float lux)
{
    if (lux < 0.1f) lux = 0.1f;
    if (lux > 1000.0f) lux = 1000.0f;

    float log_lux = log10f(lux);
    float norm = (log_lux + 1.0f) / 3.0f;

    if (norm < 0) norm = 0;
    if (norm > 1) norm = 1;

    return 20 + norm * 80;
}

uint8_t brightness_calculate(float lux)
{
    // ⭐ КРИТИЧНО: ако сме в sleep → яркост = 0
    if (power_sleep_active) {
        last_brightness = 0;
        return 0;
    }

    lux = filter_lux(lux);

    uint8_t auto_brightness = calc_auto(lux);

    uint8_t scaled =
        (auto_brightness * g_settings.manual_brightness) / 100;

    if (scaled < MIN_BRIGHTNESS)
        scaled = MIN_BRIGHTNESS;

    if (!g_settings.autodim)
    {
        app_state_lock();
        app_state.dimming_active = false;
        app_state_unlock();

        if (abs((int)scaled - (int)last_brightness) < BRIGHTNESS_HYSTERESIS)
            return last_brightness;

        last_brightness = scaled;
        return scaled;
    }

    bool dim_active = is_dim_time();
    bool wake = is_wake_active();

    if (dim_active)
    {
        if (wake)
        {
            app_state_lock();
            app_state.dimming_active = false;
            app_state_unlock();

            if (abs((int)scaled - (int)last_brightness) < BRIGHTNESS_HYSTERESIS)
                return last_brightness;

            last_brightness = scaled;
            return scaled;
        }
        else
        {
            app_state_lock();
            app_state.dimming_active = true;
            app_state_unlock();

            last_brightness = g_settings.dim_level;
            return g_settings.dim_level;
        }
    }

    app_state_lock();
    app_state.dimming_active = false;
    app_state_unlock();

    if (abs((int)scaled - (int)last_brightness) < BRIGHTNESS_HYSTERESIS)
        return last_brightness;

    last_brightness = scaled;
    return scaled;
}
