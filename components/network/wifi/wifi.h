/******************************************************************************
 *
 * wifi.h
 *
 * Camper Weather Station
 * ESP32-P4
 *
 * SoftAP ONLY
 *
 * Internet  : SIM A7670
 * Local WiFi: SoftAP
 *
 ******************************************************************************/

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "wifi_storage.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * Initialization
 ******************************************************************************/

/* Initialize WiFi subsystem */
esp_err_t wifi_init(void);

/* Start SoftAP */
esp_err_t wifi_start(void);

/* Stop SoftAP */
esp_err_t wifi_stop(void);

/* Restart SoftAP after configuration change */
esp_err_t wifi_restart(void);

/******************************************************************************
 * Status
 ******************************************************************************/

/* SoftAP is running */
bool wifi_is_running(void);

/* At least one client connected */
bool wifi_is_connected(void);

/* Number of connected clients */
uint8_t wifi_get_connected_clients(void);

/******************************************************************************
 * Information
 ******************************************************************************/

/* Fixed SSID */
const char *wifi_get_ssid(void);

/* SoftAP IP address */
const char *wifi_get_ip(void);

/* Configured WiFi channel */
uint8_t wifi_get_channel(void);

/* SoftAP MAC address as string */
void wifi_get_mac(char *buffer, size_t len);

/* Hostname */
const char *wifi_get_hostname(void);

/******************************************************************************
 * Configuration
 ******************************************************************************/

/* Read current configuration */
void wifi_get_config(softap_config_t *cfg);

/* Save configuration and restart SoftAP */
esp_err_t wifi_set_config(const softap_config_t *cfg);

#ifdef __cplusplus
}
#endif