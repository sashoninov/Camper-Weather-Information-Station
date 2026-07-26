#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "weather.h"
#include "app_state.h"
#include "ui_weather.h"
#include "weather_coords.h"
#include "storage_coords.h"

#include "sim_a7670.h"
#include "network_manager.h"

#include <stdio.h>

extern weather_data_t g_weather;


extern bool gsm_ready;

// Обновяване на прогнозата на всеки 30 минути
#define WEATHER_INTERVAL_MS (30 * 60 * 1000)

// Ако няма никакви координати - нов опит след 5 минути
#define WEATHER_RETRY_MS    (5 * 60 * 1000)

void weather_task(void *arg)
{
    double lat = 0.0;
    double lon = 0.0;

    while (1)
    {
		app_state_lock();
		double cached_lat = app_state.location.lat;
		double cached_lon = app_state.location.lon;
		app_state_unlock();

        bool net_ok = network_has_internet();

        if (!net_ok)
        {
            printf("⏳ Waiting for GSM connection...\n");

            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        // Използваме последните координати от location_task
        lat = cached_lat;
        lon = cached_lon;

        // Ако няма GPS FIX - използваме последно записаните координати
        if (lat == 0.0 && lon == 0.0)
        {
            float nvs_lat;
            float nvs_lon;

            if (storage_load_coords(&nvs_lat, &nvs_lon))
            {
                lat = nvs_lat;
                lon = nvs_lon;

                printf("💾 Using stored coordinates: %.6f, %.6f\n", lat, lon);
            }
            else
            {
                printf("⚠️ No GPS fix and no stored coordinates. Retry in 5 minutes.\n");

                vTaskDelay(pdMS_TO_TICKS(WEATHER_RETRY_MS));
                continue;
            }
        }
        else
        {
            printf("📡 Using GPS coordinates: %.6f, %.6f\n", lat, lon);
        }

        // Изтегляне на прогнозата
        if (weather_process(lat, lon))
        {
            app_state_lock();
            app_state.weather = g_weather;
            app_state_unlock();

            lv_async_call(ui_update_weather, NULL);

            printf("✅ Weather updated successfully.\n");
        }
        else
        {
            printf("⚠️ Weather update failed.\n");
        }

        // Следващо обновяване след 30 минути
        vTaskDelay(pdMS_TO_TICKS(WEATHER_INTERVAL_MS));
    }
}