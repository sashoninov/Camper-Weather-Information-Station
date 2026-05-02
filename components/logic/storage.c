#include "storage.h"
#include "nvs_flash.h"
#include "nvs.h"

#define NAMESPACE "storage"
#define KEY "calib"

bool storage_save_calibration(const calibration_data_t *calib)
{
    nvs_handle_t nvs;

    if (nvs_open(NAMESPACE, NVS_READWRITE, &nvs) != ESP_OK)
        return false;

    esp_err_t err = nvs_set_blob(nvs, KEY, calib, sizeof(*calib));

    if (err == ESP_OK) {
        err = nvs_commit(nvs);
    }

    nvs_close(nvs);
    return err == ESP_OK;
}

bool storage_load_calibration(calibration_data_t *calib)
{
    nvs_handle_t nvs;
    size_t size = sizeof(*calib);

    if (nvs_open(NAMESPACE, NVS_READONLY, &nvs) != ESP_OK)
        return false;

    esp_err_t err = nvs_get_blob(nvs, KEY, calib, &size);

    nvs_close(nvs);
    return err == ESP_OK;
}