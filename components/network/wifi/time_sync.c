#include "time_sync.h"
#include "esp_log.h"
#include "ds3231.h"
#include "time_format.h"
#include "app_state.h"
#include "sim_a7670.h"
#include "gps.h"

#include <string.h>
#include <sys/time.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "storage_coords.h"
#include "audio_manager.h"
#include "audio_events.h"

static const char *TAG = "TIME_SYNC";

volatile bool g_time_ready = false;
volatile bool g_time_from_gps = false;
volatile bool g_time_from_rtc = false;

// ===============================
// TIMEZONE TABLE (GPS → TZ)
// ===============================
typedef struct {
    const char *tz;
    float lat_min;
    float lat_max;
    float lon_min;
    float lon_max;
} tz_zone_t;

// Балкани: 3 часови зони
static const tz_zone_t tz_zones[] = {
    { "EET-2EEST,M3.5.0/3,M10.5.0/4", 34.0, 48.5, 19.0, 29.7 },   // BG/GR/RO
    { "CET-1CEST,M3.5.0/2,M10.5.0/3", 35.0, 70.0, -10.0, 30.0 }, // Central EU
    { "TRT-3",                      35.8, 42.1, 25.6, 44.8 },    // Turkey
};

// ===============================
// APPLY TIMEZONE FROM GPS
// ===============================
static void apply_timezone_from_gps(double lat, double lon)
{
    for (int i = 0; i < sizeof(tz_zones)/sizeof(tz_zones[0]); i++) {
        if (lat >= tz_zones[i].lat_min && lat <= tz_zones[i].lat_max &&
            lon >= tz_zones[i].lon_min && lon <= tz_zones[i].lon_max)
        {
            setenv("TZ", tz_zones[i].tz, 1);
            tzset();
            ESP_LOGI(TAG, "Timezone set by GPS: %s", tz_zones[i].tz);
            return;
        }
    }

    // Default → EET/EEST
    setenv("TZ", "EET-2EEST,M3.5.0/3,M10.5.0/4", 1);
    tzset();
    ESP_LOGI(TAG, "Timezone set: Default Europe");
}

// ===============================
// UI UPDATE
// ===============================
static void update_ui_time(void)
{
    time_t now;
    time(&now);

    struct tm local;
    localtime_r(&now, &local);

    app_state_lock();
    format_time_only(&local, false);
    format_date(&local);
    format_weekday(&local);
    app_state_unlock();
}

// ===============================
// LOAD FROM DS3231 (DS3231 = UTC)
// ===============================
static bool load_time_from_rtc(void)
{
    struct tm t = {};
    if (!ds3231_get_time(&t))
        return false;

    // DS3231 пази UTC → временно TZ=UTC
    char old_tz[64] = {0};
    char *env_tz = getenv("TZ");
    if (env_tz)
        strncpy(old_tz, env_tz, sizeof(old_tz) - 1);

    setenv("TZ", "UTC", 1);
    tzset();

    time_t epoch = mktime(&t);

    // Връщаме локалната зона
    if (old_tz[0])
        setenv("TZ", old_tz, 1);
    else
        unsetenv("TZ");
    tzset();

    if (epoch <= 0)
        return false;

    struct timeval tv = { .tv_sec = epoch, .tv_usec = 0 };
    settimeofday(&tv, NULL);

    update_ui_time();

    g_time_from_rtc = true;
    g_time_ready = true;

    ESP_LOGI(TAG, "RTC time loaded (UTC)");
    return true;
}

// ===============================
// GNSS SYNC (GNSS = UTC)
// ===============================
static bool sync_rtc_from_gps(void)
{
    double lat = 0.0, lon = 0.0;
    struct tm utc = {};

    // 1) GNSS координати
    if (!gps_get_location(&lat, &lon)
) {
        ESP_LOGW(TAG, "GNSS: no coordinates → skip sync");
        return false;
    }

    // 🔥 ЗАПИСВАМЕ КООРДИНАТИТЕ В app_state
    app_state_lock();
    app_state.location.lat = lat;
    app_state.location.lon = lon;
    app_state_unlock();

    // 2) GNSS време
    if (!gps_get_time(&utc)) {
        ESP_LOGW(TAG, "GNSS: no time → skip sync");
        return false;
    }

    // 3) GNSS UTC → epoch
    setenv("TZ", "UTC", 1);
    tzset();
    time_t epoch = mktime(&utc);

    if (epoch <= 0) {
        ESP_LOGW(TAG, "GNSS: invalid epoch");
        return false;
    }

    // 4) Пишем UTC в DS3231
    ds3231_set_time(&utc);

    // 5) Системен часовник = UTC
    struct timeval tv = { .tv_sec = epoch, .tv_usec = 0 };
    settimeofday(&tv, NULL);

    // 6) Приложи timezone според координатите
    apply_timezone_from_gps(lat, lon);
    tzset();

    // 7) Обнови UI
    update_ui_time();

    g_time_from_gps = true;
    g_time_ready = true;

    ESP_LOGI(TAG, "GNSS → RTC sync complete (UTC)");
    return true;
}

// ===============================
// INIT
// ===============================
void time_sync_init(void)
{
    ESP_LOGI(TAG, "Initializing time sync");

    float lat, lon;
    if (storage_load_coords(&lat, &lon)) {
        apply_timezone_from_gps(lat, lon);
        ESP_LOGI(TAG, "Loaded TZ from stored coords: %.6f %.6f", lat, lon);
    } else {
        setenv("TZ", "UTC", 1);
        tzset();
        ESP_LOGW(TAG, "No stored coords → TZ=UTC until GNSS fix");
    }

    if (load_time_from_rtc()) {
        tzset();
        ESP_LOGI(TAG, "Time loaded from RTC");
        return;
    }

    ESP_LOGW(TAG, "RTC has no valid time, waiting for GNSS...");
}

// ===============================
// TASK
// ===============================
void time_sync_task(void *arg)
{
    ESP_LOGI(TAG, "Time sync task started");

    // Винаги чакаме първия GPS Fix, дори ако RTC вече е зареден.
    while (!g_time_from_gps)
    {
        if (sync_rtc_from_gps())
        {
            ESP_LOGI(TAG, "Initial GNSS sync complete");
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    time_t last_sync = 0;
    time(&last_sync);

    for (;;) {
        update_ui_time();

        // ===================== HOURLY CHIME =====================
        time_t now_epoch;
        time(&now_epoch);

        struct tm local;
        localtime_r(&now_epoch, &local);

        static int last_hour = -1;

        if (app_state.dimming_active) {
            if (local.tm_hour != last_hour)
                last_hour = local.tm_hour;
        } else {
            if (local.tm_hour != last_hour) {
                last_hour = local.tm_hour;

                int hour = local.tm_hour;
                int hour_index = (hour == 0) ? 23 : (hour - 1);

                ESP_LOGI("TIME", "HOURLY CHIME: hour=%d (index=%d)",
                         hour, hour_index);

                audio_play_event(AUDIO_EVENT_HOUR_01 + hour_index);
            }
        }

        // ===================== DAILY GNSS SYNC =====================
        time_t now;
        time(&now);

        if (now - last_sync > 24 * 3600) {
            if (sync_rtc_from_gps()) {
                last_sync = now;
                ESP_LOGI(TAG, "Daily GNSS sync done");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// ===============================
// API
// ===============================
bool time_sync_is_valid(void)
{
    return g_time_ready;
}

bool time_sync_is_gps(void)
{
    return g_time_from_gps;
}

bool time_sync_is_rtc(void)
{
    return g_time_from_rtc;
}
