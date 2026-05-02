#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "weather.h"
#include "app_state.h"
#include "ui_weather.h"
#include "weather_coords.h"
#include "storage_coords.h"

#include <stdio.h>

extern weather_data_t g_weather;

#define WEATHER_INTERVAL_MS (15 * 60 * 1000) // 15 мин

void weather_task(void *arg)
{
    double lat = 0, lon = 0;

    while (1)
    {
        bool wifi_ok, gps_ok;

        app_state_lock();
        wifi_ok = app_state.wifi_connected;
        gps_ok  = app_state.gps.valid;
        app_state_unlock();

        // ================================
        // 1) Ако няма WiFi → няма смисъл
        // ================================
        if (!wifi_ok) {
            printf("⏳ Waiting for WiFi...\n");
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        // ================================
        // 2) Вземаме координати
        // ================================

        if (gps_ok && app_state.gps.valid)
        {
            // GPS има FIX → използваме него
            lat = app_state.gps.lat;
            lon = app_state.gps.lon;

            printf("📡 Using GPS: %.6f, %.6f\n", lat, lon);
        }
        else
        {
            // GPS няма FIX → опитваме NVS
            float nvs_lat, nvs_lon;

            if (storage_load_coords(&nvs_lat, &nvs_lon))
            {
                lat = nvs_lat;
                lon = nvs_lon;

                printf("💾 Using stored coords: %.6f, %.6f\n", lat, lon);
            }
            else
            {
                // Няма GPS, няма NVS → изчакваме
                printf("⚠️ No GPS/NVS coords → delaying weather...\n");
                vTaskDelay(pdMS_TO_TICKS(60000)); // 1 минута
                continue;
            }
        }

        // ================================
        // 3) Заявка за времето
        // ================================
        if (weather_process(lat, lon))
        {
            app_state_lock();
            app_state.weather = g_weather;
            app_state_unlock();

            lv_async_call(ui_update_weather, NULL);
        }

        // ================================
        // 4) Интервал
        // ================================
        vTaskDelay(pdMS_TO_TICKS(WEATHER_INTERVAL_MS));
    }
}
