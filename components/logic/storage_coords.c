#include "storage_coords.h"
#include "nvs_flash.h"
#include "nvs.h"

#define NAMESPACE "storage"

bool storage_save_coords(float lat, float lon)
{
    nvs_handle_t nvs;
    if (nvs_open(NAMESPACE, NVS_READWRITE, &nvs) != ESP_OK)
        return false;

    int32_t lat_i = (int32_t)(lat * 1e6);
    int32_t lon_i = (int32_t)(lon * 1e6);

    nvs_set_i32(nvs, "gps_lat", lat_i);
    nvs_set_i32(nvs, "gps_lon", lon_i);

    esp_err_t err = nvs_commit(nvs);
    nvs_close(nvs);

    return err == ESP_OK;
}

bool storage_load_coords(float *lat, float *lon)
{
    nvs_handle_t nvs;
    if (nvs_open(NAMESPACE, NVS_READONLY, &nvs) != ESP_OK)
        return false;

    int32_t lat_i, lon_i;

    if (nvs_get_i32(nvs, "gps_lat", &lat_i) != ESP_OK ||
        nvs_get_i32(nvs, "gps_lon", &lon_i) != ESP_OK)
    {
        nvs_close(nvs);
        return false;
    }

    nvs_close(nvs);

    *lat = lat_i / 1e6f;
    *lon = lon_i / 1e6f;
    return true;
}
