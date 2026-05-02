#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <math.h>
#include <stdio.h>
#include <stdbool.h>
#include <esp_timer.h>

#include "app_state.h"
#include "location.h"

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

    double a = sin(dLat/2) * sin(dLat/2) +
               cos(deg2rad(lat1)) * cos(deg2rad(lat2)) *
               sin(dLon/2) * sin(dLon/2);

    double c = 2 * atan2(sqrt(a), sqrt(1-a));

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
        // 1) Ако няма WiFi → няма смисъл да викаме API
        app_state_lock();
        bool wifi_ok = app_state.wifi_connected;
        app_state_unlock();

        if (!wifi_ok)
        {
            vTaskDelay(pdMS_TO_TICKS(LOCATION_CHECK_INTERVAL_MS));
            continue;
        }

        int64_t now = esp_timer_get_time() / 1000;

        double lat = 0;
        double lon = 0;
        bool gps_valid = false;

        // 2) Четем GPS от app_state
        app_state_lock();
        lat = app_state.gps.lat;
        lon = app_state.gps.lon;
        gps_valid = app_state.gps.valid;
        app_state_unlock();

        bool need_update = false;

        // 👉 първо стартиране
        if (!has_fix && gps_valid)
        {
            need_update = true;
        }

        // 👉 на 6 часа
        if ((now - last_update) > LOCATION_REFRESH_INTERVAL_MS)
        {
            need_update = true;
        }

        // 👉 движение > 30 км
        if (gps_valid && has_fix)
        {
            double dist = distance_km(last_lat, last_lon, lat, lon);

            if (dist > LOCATION_MOVE_THRESHOLD_KM)
            {
                printf("📍 GPS moved: %.2f km\n", dist);
                need_update = true;
            }
        }

        // ==========================
        // UPDATE
        // ==========================
        if (need_update && gps_valid)
        {
            printf("📍 Updating location via Open-Meteo...\n");

            // записваме GPS координати
            app_state_lock();
            app_state.location.lat = lat;
            app_state.location.lon = lon;
            app_state_unlock();

            printf("📡 GPS coords: %.5f, %.5f\n", lat, lon);

            // ⭐ ТУК ВИКАМЕ Open-Meteo
            if (location_fetch() == ESP_OK)
            {
                printf("🌍 Open-Meteo location updated\n");
            }
            else
            {
                printf("⚠️ Open-Meteo location failed\n");
            }

            // update cache
            last_lat = lat;
            last_lon = lon;

            last_update = now;
            has_fix = true;
        }

        vTaskDelay(pdMS_TO_TICKS(LOCATION_CHECK_INTERVAL_MS));
    }
}
