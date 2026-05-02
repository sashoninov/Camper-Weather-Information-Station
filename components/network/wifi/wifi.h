#pragma once

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t wifi_init_sta(void);
bool wifi_is_connected(void);
bool wifi_test_connection(const char *ssid, const char *pass);

esp_err_t wifi_set_credentials(const char *ssid, const char *pass);

// 🔥 NEW
int wifi_scan(char ssids[][33], int max);
void wifi_try_known_networks(void);
void wifi_add_known(const char *ssid, const char *pass);

#ifdef __cplusplus
}
#endif