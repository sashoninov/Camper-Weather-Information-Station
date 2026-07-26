/******************************************************************************
 *
 * wifi_storage.h
 *
 * Camper Weather Station
 * ESP32-P4
 *
 * SoftAP configuration storage
 *
 ******************************************************************************/

#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Fixed SoftAP configuration
 ******************************************************************************/

#define WIFI_AP_SSID           "Camper Station"
#define WIFI_AP_HOSTNAME       "camper-station"
#define WIFI_AP_IP             "192.168.4.1"

#define WIFI_AP_PASSWORD_MAX   64

/******************************************************************************
 * Factory defaults
 ******************************************************************************/

#define WIFI_DEFAULT_PASSWORD      "12345678"
#define WIFI_AP_CHANNEL       6
#define WIFI_AP_MAX_CLIENTS   5

/******************************************************************************
 * SoftAP configuration stored in NVS
 *
 * SSID is NOT stored.
 * SSID is fixed to WIFI_AP_SSID.
 ******************************************************************************/

typedef struct
{
    char password[65];

} softap_config_t;

/******************************************************************************
 * Load configuration
 ******************************************************************************/

esp_err_t wifi_load_ap_config(softap_config_t *cfg);

/******************************************************************************
 * Save configuration
 ******************************************************************************/

esp_err_t wifi_save_ap_config(const softap_config_t *cfg);

/******************************************************************************
 * Restore factory defaults
 ******************************************************************************/

esp_err_t wifi_reset_ap_config(void);

#ifdef __cplusplus
}
#endif