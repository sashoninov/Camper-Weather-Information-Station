#include "time_sync.h"
#include "esp_log.h"
#include "gps.h"
#include "ds3231.h"
#include "time_format.h"
#include "app_state.h"

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
    // EET/EEST → Bulgaria, Greece, Romania
    { "EET-2EEST,M3.5.0/3,M10.5.0/4",
      34.0, 48.5, 19.0, 29.7 },

    // CET/CEST → Central & Western Europe
    { "CET-1CEST,M3.5.0/2,M10.5.0/3",
      35.0, 70.0, -10.0, 30.0 },

    // TRT → Turkey
    { "TRT-3",
      35.8, 42.1, 25.6, 44.8 },
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
// GPS SYNC (GPS = UTC)
// ===============================
static bool sync_rtc_from_gps(void)
{
    gps_data_t gps;

    if (!gps_read(&gps))
        return false;

    if (!gps.valid_time) {
        ESP_LOGW(TAG, "GPS has NO VALID TIME → skip GPS sync");
        return false;
    }

    // Ако GPS няма дата → ползваме датата от DS3231
    if (gps.year == 0 || gps.month == 0 || gps.day == 0) {
        struct tm rtc_tm = {};
        if (ds3231_get_time(&rtc_tm)) {
            gps.year  = rtc_tm.tm_year + 1900;
            gps.month = rtc_tm.tm_mon + 1;
            gps.day   = rtc_tm.tm_mday;

            ESP_LOGW(TAG,
                     "GPS has NO DATE → using RTC date %04d-%02d-%02d",
                     gps.year, gps.month, gps.day);
        } else {
            ESP_LOGW(TAG, "GPS has NO DATE and RTC invalid → skipping GPS sync");
            return false;
        }
    }

    struct tm t = {
        .tm_year = gps.year - 1900,
        .tm_mon  = gps.month - 1,
        .tm_mday = gps.day,
        .tm_hour = gps.hour,
        .tm_min  = gps.min,
        .tm_sec  = gps.sec
    };

    // GPS UTC → epoch UTC
    char old_tz[64] = {0};
    char *env_tz = getenv("TZ");
    if (env_tz)
        strncpy(old_tz, env_tz, sizeof(old_tz) - 1);

    setenv("TZ", "UTC", 1);
    tzset();

    time_t epoch = mktime(&t);

    if (old_tz[0])
        setenv("TZ", old_tz, 1);
    else
        unsetenv("TZ");
    tzset();

    if (epoch <= 0)
        return false;

    // Пишем UTC в DS3231
    ds3231_set_time(&t);

    // Системен часовник = UTC
    struct timeval tv = { .tv_sec = epoch, .tv_usec = 0 };
    settimeofday(&tv, NULL);

    // Приложи TZ по GPS координати
    apply_timezone_from_gps(gps.lat, gps.lon);
    tzset();   // <<< FIX: гарантира локално време за всички задачи
    update_ui_time();

    g_time_from_gps = true;
    g_time_ready = true;

    ESP_LOGI(TAG, "GPS → RTC sync complete (UTC)");
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
        ESP_LOGW(TAG, "No stored coords → TZ=UTC until GPS fix");
    }

    if (load_time_from_rtc()) {
        tzset();   // <<< FIX: прилагаме TZ след RTC
        ESP_LOGI(TAG, "Time loaded from RTC");
        return;
    }

    ESP_LOGW(TAG, "RTC has no valid time, waiting for GPS...");
}

// ===============================
// TASK
// ===============================
void time_sync_task(void *arg)
{
    ESP_LOGI(TAG, "Time sync task started");

    if (g_time_ready) {
        for (int i = 0; i < 30; i++) {
            if (sync_rtc_from_gps()) {
                ESP_LOGI(TAG, "Startup GPS sync → DS3231 updated");
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }

    while (!g_time_ready) {
        if (sync_rtc_from_gps()) {
            ESP_LOGI(TAG, "Initial GPS sync complete");
            tzset();   // <<< FIX: гарантира локално време
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
		
		// Ако екранът е димиран → не казваме часа
		if (app_state.dimming_active)
		{
			// Обновяваме last_hour, за да не се натрупва
			if (local.tm_hour != last_hour)
				last_hour = local.tm_hour;
		
			// НЕ правим continue → оставяме задачата да стигне до vTaskDelay()
		}
		else
		{
			// Ако часът се е сменил → казваме го
			if (local.tm_hour != last_hour)
			{
				last_hour = local.tm_hour;
		
				int hour = local.tm_hour;  // 0–23
				int hour_index = (hour == 0) ? 23 : (hour - 1);
		
				ESP_LOGI("TIME", "HOURLY CHIME: hour=%d (index=%d)",
						hour, hour_index);
		
				audio_play_event(AUDIO_EVENT_HOUR_01 + hour_index);
			}
		}





        // ===================== DAILY GPS SYNC =====================
        time_t now;
        time(&now);

        if (now - last_sync > 24 * 3600) {
            if (sync_rtc_from_gps()) {
                last_sync = now;
                ESP_LOGI(TAG, "Daily GPS sync done");
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
