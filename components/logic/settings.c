#include "settings.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <string.h>

#define SETTINGS_NAMESPACE "settings"

settings_t g_settings = {
    .manual_brightness = 80,
    .dim_level = 20,
    .autodim = true,
    .start_hour = 22,
    .start_min = 0,
    .end_hour = 7,
    .end_min = 0
};

bool settings_save(settings_t *s)
{
    nvs_handle_t nvs;

    if (nvs_open(SETTINGS_NAMESPACE, NVS_READWRITE, &nvs) != ESP_OK)
        return false;

    // записваме целия struct като blob
    if (nvs_set_blob(nvs, "cfg", s, sizeof(settings_t)) != ESP_OK) {
        nvs_close(nvs);
        return false;
    }

    nvs_commit(nvs);
    nvs_close(nvs);

    return true;
}

bool settings_load(settings_t *s)
{
    nvs_handle_t nvs;

    if (nvs_open(SETTINGS_NAMESPACE, NVS_READONLY, &nvs) != ESP_OK)
        return false;

    size_t required_size = sizeof(settings_t);

    if (nvs_get_blob(nvs, "cfg", s, &required_size) != ESP_OK) {
        nvs_close(nvs);
        return false;
    }

    nvs_close(nvs);
    return true;
}