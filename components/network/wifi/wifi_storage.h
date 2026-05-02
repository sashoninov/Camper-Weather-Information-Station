#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WIFI_SSID_MAX 32
#define WIFI_PASS_MAX 64

esp_err_t wifi_save_credentials(const char *ssid, const char *pass);
esp_err_t wifi_load_credentials(char *ssid, char *pass);

#ifdef __cplusplus
}
#endif
