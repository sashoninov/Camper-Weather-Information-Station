#include "calibration.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <string.h>
#include <stdbool.h>

#define NAMESPACE "calib"

static calibration_data_t calib = {
    .pitch_offset = 0,
    .roll_offset = 0
};

// ===================== SET =====================

void calibration_set(float pitch, float roll)
{
    calib.pitch_offset = pitch;
    calib.roll_offset  = roll;
}

// ===================== GET =====================

void calibration_get(calibration_data_t *out)
{
    if (out) {
        *out = calib;
    }
}

// ===================== SAVE =====================

bool calibration_save(void)
{
    nvs_handle_t handle;

    if (nvs_open(NAMESPACE, NVS_READWRITE, &handle) != ESP_OK)
        return false;

    nvs_set_blob(handle, "mpu", &calib, sizeof(calib));
    nvs_commit(handle);
    nvs_close(handle);

    return true;
}

// ===================== LOAD =====================

bool calibration_load(void)
{
    nvs_handle_t handle;

    if (nvs_open(NAMESPACE, NVS_READONLY, &handle) != ESP_OK)
        return false;

    size_t size = sizeof(calib);

    if (nvs_get_blob(handle, "mpu", &calib, &size) != ESP_OK)
    {
        nvs_close(handle);
        return false;
    }

    nvs_close(handle);
    return true;
}