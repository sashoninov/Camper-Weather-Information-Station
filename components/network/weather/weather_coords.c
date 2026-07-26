#include "weather_coords.h"
#include "app_state.h"
#include "storage_coords.h"
#include "sim_a7670.h"
#include <stdio.h>

bool weather_get_coordinates(double *lat, double *lon)
{
    if (!lat || !lon)
        return false;

    double glat = 0.0, glon = 0.0;

    // ================================
    // 1) Опитваме GNSS модема
    // ================================
    if (sim_a7670_get_location(&glat, &glon))
    {
        *lat = glat;
        *lon = glon;

        // Записваме в глобалния state
        app_state_lock();
        app_state.location.lat = glat;
        app_state.location.lon = glon;
        app_state_unlock();

        // Записваме в NVS
        storage_save_coords(glat, glon);

        printf("📡 Weather coords from GNSS: %.6f, %.6f\n", glat, glon);
        return true;
    }

    // ================================
    // 2) Fallback → NVS координати
    // ================================
    float nvs_lat, nvs_lon;

    if (storage_load_coords(&nvs_lat, &nvs_lon))
    {
        *lat = nvs_lat;
        *lon = nvs_lon;

        printf("💾 Weather coords from NVS: %.6f, %.6f\n", nvs_lat, nvs_lon);
        return true;
    }

    // ================================
    // 3) Няма координати
    // ================================
    printf("⚠️ Weather: no GNSS/NVS coordinates\n");
    return false;
}
