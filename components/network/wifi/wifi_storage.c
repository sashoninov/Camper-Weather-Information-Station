#include "wifi_storage.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <string.h>

#define WIFI_NAMESPACE "wifi"

esp_err_t wifi_save_credentials(const char *ssid, const char *pass)
{
    if (!ssid || !pass)
        return ESP_ERR_INVALID_ARG;

    nvs_handle_t nvs;
    esp_err_t err = nvs_open(WIFI_NAMESPACE, NVS_READWRITE, &nvs);
    if (err != ESP_OK)
        return err;

    err = nvs_set_str(nvs, "ssid", ssid);
    if (err != ESP_OK) {
        nvs_close(nvs);
        return err;
    }

    err = nvs_set_str(nvs, "pass", pass);
    if (err != ESP_OK) {
        nvs_close(nvs);
        return err;
    }

    err = nvs_commit(nvs);
    nvs_close(nvs);

    return err;
}

esp_err_t wifi_load_credentials(char *ssid, char *pass)
{
    if (!ssid || !pass)
        return ESP_ERR_INVALID_ARG;

    nvs_handle_t nvs;
    esp_err_t err = nvs_open(WIFI_NAMESPACE, NVS_READONLY, &nvs);
    if (err != ESP_OK)
        return err;

    size_t ssid_len = WIFI_SSID_MAX;
    size_t pass_len = WIFI_PASS_MAX;

    err = nvs_get_str(nvs, "ssid", ssid, &ssid_len);
    if (err != ESP_OK) {
        nvs_close(nvs);
        return err;
    }

    err = nvs_get_str(nvs, "pass", pass, &pass_len);
    if (err != ESP_OK) {
        nvs_close(nvs);
        return err;
    }

    nvs_close(nvs);
    return ESP_OK;
}
