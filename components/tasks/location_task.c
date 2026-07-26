#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <math.h>
#include <stdio.h>
#include <stdbool.h>
#include <esp_timer.h>

#include "app_state.h"
#include "location.h"
#include "network_manager.h"

#include "sim_a7670.h"
#include "gps.h"



extern bool gsm_ready;

// ==============================
// CONFIG
// ==============================
#define LOCATION_CHECK_INTERVAL_MS      (60000)                // 1 минута
#define LOCATION_REFRESH_INTERVAL_MS    (6 * 60 * 60 * 1000)   // 6 часа
#define LOCATION_MOVE_THRESHOLD_KM      30.0

#define EARTH_RADIUS_KM 6371.0

// ==============================
// HELPERS
// ==============================
static double deg2rad(double deg)
{
    return deg * (M_PI / 180.0);
}

static double distance_km(double lat1, double lon1, double lat2, double lon2)
{
    double dLat = deg2rad(lat2 - lat1);
    double dLon = deg2rad(lon2 - lon1);

    double a = sin(dLat / 2) * sin(dLat / 2) +
               cos(deg2rad(lat1)) * cos(deg2rad(lat2)) *
               sin(dLon / 2) * sin(dLon / 2);

    double c = 2 * atan2(sqrt(a), sqrt(1 - a));

    return EARTH_RADIUS_KM * c;
}

// ==============================
// MAIN TASK
// ==============================
void location_task(void *arg)
{
    double last_lat = 0;
    double last_lon = 0;
    bool has_fix = false;

    int64_t last_update = 0;

    while (1)
    {
        // ================================
        // 1) Проверка за мрежа (WiFi или GSM)
        // ================================


        bool net_ok = network_has_internet();
        if (!net_ok)
        {
            vTaskDelay(pdMS_TO_TICKS(LOCATION_CHECK_INTERVAL_MS));
            continue;
        }

        int64_t now = esp_timer_get_time() / 1000;

        double lat = 0.0;
        double lon = 0.0;

        // ================================
        // 2) Вземаме координати от GPS
        // ================================
        if (!gps_get_location(&lat, &lon))
        {
            printf("❌ GNSS: no coordinates\n");
            vTaskDelay(pdMS_TO_TICKS(LOCATION_CHECK_INTERVAL_MS));
            continue;
        }

        // Записваме координатите глобално
        app_state_lock();
        app_state.location.lat = lat;
        app_state.location.lon = lon;
        app_state_unlock();

        bool need_update = false;

        // Първо стартиране
        if (!has_fix)
        {
            need_update = true;
        }

        // Обновяване на всеки 6 часа
        if ((now - last_update) > LOCATION_REFRESH_INTERVAL_MS)
        {
            need_update = true;
        }

        // Проверка за движение > 30 km
        if (has_fix)
        {
            double dist = distance_km(last_lat, last_lon, lat, lon);

            if (dist > LOCATION_MOVE_THRESHOLD_KM)
            {
                printf("📍 GNSS moved: %.2f km\n", dist);
                need_update = true;
            }
        }

        // ================================
        // UPDATE
        // ================================
        if (need_update)
        {
            printf("📍 Updating location via Open-Meteo...\n");
            printf("📡 GNSS coords: %.6f, %.6f\n", lat, lon);

            if (location_fetch() == ESP_OK)
            {
                printf("🌍 Open-Meteo location updated\n");
            }
            else
            {
                printf("⚠️ Open-Meteo location failed\n");
            }

            last_lat = lat;
            last_lon = lon;

            last_update = now;
            has_fix = true;
        }

        vTaskDelay(pdMS_TO_TICKS(LOCATION_CHECK_INTERVAL_MS));
    }
}