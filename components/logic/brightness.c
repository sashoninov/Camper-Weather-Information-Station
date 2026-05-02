#include "brightness.h"
#include "settings.h"
#include "esp_timer.h"
#include <time.h>
#include <math.h>
#include "app_state.h"

static bool wake_override = false;
static int64_t last_touch = 0;

#define WAKE_TIMEOUT_MS 30000
#define MIN_BRIGHTNESS  10

// ====================== FILTER STATE ======================

static float lux_filtered = -1.0f;
static uint8_t last_brightness = 0;

// Хистерезис: промяна само ако разликата е >= 3%
#define BRIGHTNESS_HYSTERESIS 3

// ====================== TIME CHECK ======================

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

// ====================== TOUCH WAKE ======================

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

// ====================== LUX FILTER (EMA) ======================

static float filter_lux(float lux)
{
    const float alpha = 0.15f;  // плавно, но бързо реагира

    if (lux_filtered < 0.0f) {
        lux_filtered = lux;
        return lux_filtered;
    }

    lux_filtered = lux_filtered * (1.0f - alpha) + lux * alpha;
    return lux_filtered;
}

// ====================== AUTO CURVE ======================

static uint8_t calc_auto(float lux)
{
    if (lux < 0.1f) lux = 0.1f;
    if (lux > 1000.0f) lux = 1000.0f;

    float log_lux = log10f(lux);

    float norm = (log_lux + 1.0f) / 3.0f;

    if (norm < 0) norm = 0;
    if (norm > 1) norm = 1;

    // 20–100%
    return 20 + norm * 80;
}

// ====================== MAIN ======================

uint8_t brightness_calculate(float lux)
{
    // 1) Филтрираме входа
    lux = filter_lux(lux);

    uint8_t auto_brightness = calc_auto(lux);

    // 2) USER LIMIT (MAX)
    uint8_t scaled =
        (auto_brightness * g_settings.manual_brightness) / 100;

    if (scaled < MIN_BRIGHTNESS)
        scaled = MIN_BRIGHTNESS;

    // 3) AUTODIM OFF
    if (!g_settings.autodim)
    {
        // Обновяваме флага → НЕ димираме
        app_state_lock();
        app_state.dimming_active = false;
        app_state_unlock();

        // Хистерезис
        if (abs((int)scaled - (int)last_brightness) < BRIGHTNESS_HYSTERESIS)
            return last_brightness;

        last_brightness = scaled;
        return scaled;
    }

    // 4) AUTODIM ON
    bool dim_active = is_dim_time();
    bool wake = is_wake_active();

    if (dim_active)
    {
        if (wake)
        {
            // НЕ димираме → wake override
            app_state_lock();
            app_state.dimming_active = false;
            app_state_unlock();

            // Хистерезис
            if (abs((int)scaled - (int)last_brightness) < BRIGHTNESS_HYSTERESIS)
                return last_brightness;

            last_brightness = scaled;
            return scaled;
        }
        else
        {
            // ДИМИРАНЕ АКТИВНО
            app_state_lock();
            app_state.dimming_active = true;
            app_state_unlock();

            last_brightness = g_settings.dim_level;
            return g_settings.dim_level;
        }
    }

    // 5) Нормален режим → НЕ димираме
    app_state_lock();
    app_state.dimming_active = false;
    app_state_unlock();

    // Хистерезис
    if (abs((int)scaled - (int)last_brightness) < BRIGHTNESS_HYSTERESIS)
        return last_brightness;

    last_brightness = scaled;
    return scaled;
}
