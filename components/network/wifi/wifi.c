#include "wifi.h"
#include "wifi_storage.h"
#include "app_state.h"
#include "ui_location.h"
#include "wifi_roaming.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "ota_update.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <string.h>

static const char *TAG = "WIFI";

static void wifi_watchdog_task(void *arg);

static bool s_connected = false;
static bool s_waiting_for_test = false;
static bool s_test_result = false;

extern volatile bool roaming_enabled;

// ⭐ Глобални за UI
char wifi_ssid[33] = {0};
char wifi_ip[16]   = {0};

// ===================== EVENT HANDLER =====================

static void wifi_event_handler(void *arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data)
{
    if (event_base == WIFI_EVENT) {

        if (event_id == WIFI_EVENT_STA_DISCONNECTED) {

            s_connected = false;

            if (s_waiting_for_test)
                s_test_result = false;

            app_state_lock();
            app_state.wifi_connected = false;
            app_state_unlock();

            wifi_ssid[0] = 0;
            wifi_ip[0] = 0;

            ui_update_wifi_icon();
        }

    } else if (event_base == IP_EVENT) {

        if (event_id == IP_EVENT_STA_GOT_IP) {

            s_connected = true;

            if (s_waiting_for_test)
                s_test_result = true;

            app_state_lock();
            app_state.wifi_connected = true;
            app_state_unlock();

            // ⭐ SSID (ESP-IDF 5.x няма ssid_len)
            wifi_ap_record_t info;
            if (esp_wifi_sta_get_ap_info(&info) == ESP_OK) {
                memcpy(wifi_ssid, info.ssid, sizeof(info.ssid));
                wifi_ssid[32] = 0;   // гарантираме нула
            }

            // ⭐ IP
            esp_netif_ip_info_t ip;
            esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"), &ip);
            sprintf(wifi_ip, IPSTR, IP2STR(&ip.ip));

            ui_update_wifi_icon();
            wifi_roaming_resume();

             // ⭐⭐ ТУК ДОБАВЯШ OTA ⭐⭐
             ESP_LOGI(TAG, "WiFi connected, starting OTA server...");
             ota_start_server();


        }
    }
}

// ===================== APPLY CREDENTIALS =====================

static esp_err_t wifi_apply_credentials(const char *ssid, const char *pass)
{
    wifi_config_t wifi_config = (wifi_config_t){ 0 };

    strlcpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));

    if (pass && pass[0] != '\0') {
        strlcpy((char *)wifi_config.sta.password, pass, sizeof(wifi_config.sta.password));
        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    } else {
        wifi_config.sta.password[0] = '\0';
        wifi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;
    }

    wifi_config.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;

    esp_wifi_disconnect();
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_connect();

    return ESP_OK;
}

// ===================== INIT =====================

esp_err_t wifi_init_sta(void)
{
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL);

    char ssid[32] = {0};
    char pass[64] = {0};

    wifi_config_t wifi_config = (wifi_config_t){ 0 };

    if (wifi_load_credentials(ssid, pass) == ESP_OK) {

        strlcpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
        strlcpy((char *)wifi_config.sta.password, pass, sizeof(wifi_config.sta.password));

        wifi_config.sta.threshold.authmode =
            (pass[0] != '\0') ? WIFI_AUTH_WPA2_PSK : WIFI_AUTH_OPEN;

        wifi_config.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;

        esp_wifi_set_mode(WIFI_MODE_STA);
        esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
        esp_wifi_start();
        esp_wifi_connect();

    } else {

        esp_wifi_set_mode(WIFI_MODE_STA);
        esp_wifi_start();
    }

    xTaskCreate(wifi_watchdog_task, "wifi_watchdog", 4096, NULL, 5, NULL);

    wifi_roaming_start();

    return ESP_OK;
}

// ===================== BASIC =====================

bool wifi_is_connected(void)
{
    return s_connected;
}

esp_err_t wifi_set_credentials(const char *ssid, const char *pass)
{
    wifi_roaming_pause();

    wifi_save_credentials(ssid, pass);

    return wifi_apply_credentials(ssid, pass);
}

// ===================== SCAN =====================

int wifi_scan(char ssids[][33], int max)
{
    wifi_scan_config_t scan_cfg = {0};
    esp_wifi_scan_start(&scan_cfg, true);

    uint16_t n = max;
    wifi_ap_record_t rec[20];
    esp_wifi_scan_get_ap_records(&n, rec);

    if (n > max) n = max;

    for (int i = 0; i < n; i++) {
        strncpy(ssids[i], (char *)rec[i].ssid, 32);
        ssids[i][32] = 0;
    }

    return n;
}

// ===================== TEST CONNECTION =====================

bool wifi_test_connection(const char *ssid, const char *pass)
{
    const int total_wait_ms = 9000;
    const int step_ms       = 300;

    wifi_roaming_pause();

    s_waiting_for_test = true;
    s_test_result = false;

    wifi_apply_credentials(ssid, pass);

    int steps = total_wait_ms / step_ms;
    for (int i = 0; i < steps; i++) {
        vTaskDelay(pdMS_TO_TICKS(step_ms));
        if (s_test_result) {
            s_waiting_for_test = false;
            wifi_roaming_resume();
            return true;
        }
    }

    s_waiting_for_test = false;
    wifi_roaming_resume();
    return false;
}

// ===================== WATCHDOG =====================

static void wifi_watchdog_task(void *arg)
{
    int counter = 0;

    while (1)
    {
        if (!roaming_enabled) {
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }

        if (wifi_is_connected()) {
            counter = 0;
        } else {
            counter++;
            if (counter >= 30) {
                esp_wifi_connect();
                counter = 0;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
