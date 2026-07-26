/******************************************************************************
 *
 * wifi.c
 *
 * Camper Weather Station
 * ESP32-P4
 *
 * SoftAP ONLY
 *
 ******************************************************************************/

#include "wifi.h"

#include "app_state.h"
#include "ota_update.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_netif_ip_addr.h"
#include "esp_wifi.h"
#include "lwip/ip4_addr.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <string.h>

static const char *TAG = "SOFTAP";

/******************************************************************************
 * Globals
 ******************************************************************************/

static esp_netif_t *s_ap_netif = NULL;

static softap_config_t s_cfg;

static bool s_running = false;

static uint8_t s_clients = 0;

/******************************************************************************
 * Forward declarations
 ******************************************************************************/

static void wifi_event_handler(
        void *arg,
        esp_event_base_t event_base,
        int32_t event_id,
        void *event_data);

static esp_err_t wifi_configure_network(void);

static esp_err_t wifi_configure_ap(void);

/******************************************************************************
 * WiFi Event Handler
 ******************************************************************************/

static void wifi_event_handler(
        void *arg,
        esp_event_base_t event_base,
        int32_t event_id,
        void *event_data)
{
    (void)arg;

    if (event_base != WIFI_EVENT)
        return;

    switch (event_id)
    {
        case WIFI_EVENT_AP_START:

            ESP_LOGI(TAG, "SoftAP started");

            s_running = true;

            app_state_lock();
            app_state.wifi_connected = false;
            app_state_unlock();

            ota_start_server();

            break;

        case WIFI_EVENT_AP_STOP:

            ESP_LOGI(TAG, "SoftAP stopped");

            s_running = false;
            s_clients = 0;

            app_state_lock();
            app_state.wifi_connected = false;
            app_state_unlock();

            break;

        case WIFI_EVENT_AP_STACONNECTED:
        {
            wifi_event_ap_staconnected_t *ev =
                (wifi_event_ap_staconnected_t *)event_data;

            s_clients++;

            ESP_LOGI(TAG,
                     "Client connected "
                     MACSTR
                     " AID=%d (%u clients)",
                     MAC2STR(ev->mac),
                     ev->aid,
                     s_clients);

            app_state_lock();
            app_state.wifi_connected = true;
            app_state_unlock();

            break;
        }

        case WIFI_EVENT_AP_STADISCONNECTED:
        {
            wifi_event_ap_stadisconnected_t *ev =
                (wifi_event_ap_stadisconnected_t *)event_data;

            if (s_clients)
                s_clients--;

            ESP_LOGI(TAG,
                     "Client disconnected "
                     MACSTR
                     " AID=%d (%u clients)",
                     MAC2STR(ev->mac),
                     ev->aid,
                     s_clients);

            app_state_lock();
            app_state.wifi_connected = (s_clients != 0);
            app_state_unlock();

            break;
        }

        default:
            break;
    }
}

/******************************************************************************
 * Configure SoftAP network
 ******************************************************************************/

static esp_err_t wifi_configure_network(void)
{
    esp_netif_ip_info_t ip;

    IP4_ADDR(&ip.ip,      192,168,4,1);
    IP4_ADDR(&ip.gw,      192,168,4,1);
    IP4_ADDR(&ip.netmask, 255,255,255,0);

    ESP_ERROR_CHECK(esp_netif_dhcps_stop(s_ap_netif));

    ESP_ERROR_CHECK(
        esp_netif_set_ip_info(
            s_ap_netif,
            &ip));

    ESP_ERROR_CHECK(
        esp_netif_dhcps_start(
            s_ap_netif));

    ESP_LOGI(TAG,
             "SoftAP IP: %s",
             WIFI_AP_IP);

    return ESP_OK;
}

/******************************************************************************
 * Configure SoftAP
 ******************************************************************************/

static esp_err_t wifi_configure_ap(void)
{
    wifi_load_ap_config(&s_cfg);
	ESP_LOGI(TAG, "Loaded password: '%s' len=%u",
         s_cfg.password,
         strlen(s_cfg.password));

    wifi_config_t cfg = {0};

    strlcpy((char *)cfg.ap.ssid,
            WIFI_AP_SSID,
            sizeof(cfg.ap.ssid));

    strlcpy((char *)cfg.ap.password,
            s_cfg.password,
            sizeof(cfg.ap.password));

    cfg.ap.ssid_len = strlen(WIFI_AP_SSID);

    cfg.ap.channel = WIFI_AP_CHANNEL;

    cfg.ap.max_connection = WIFI_AP_MAX_CLIENTS;

    cfg.ap.pmf_cfg.required = false;

    if (strlen(s_cfg.password) >= 8)
    {
        cfg.ap.authmode = WIFI_AUTH_WPA2_PSK;
    }
    else
    {
        cfg.ap.authmode = WIFI_AUTH_OPEN;
        cfg.ap.password[0] = 0;
    }

    ESP_ERROR_CHECK(
        esp_wifi_set_mode(WIFI_MODE_AP));

    ESP_ERROR_CHECK(
        esp_wifi_set_config(
            WIFI_IF_AP,
            &cfg));
			
	ESP_LOGI(TAG, "WiFi password: '%s' len=%u",
         cfg.ap.password,
         strlen((char *)cfg.ap.password));		

    ESP_LOGI(TAG,
             "SSID      : %s",
             WIFI_AP_SSID);


    return ESP_OK;
}

/******************************************************************************
 * Initialize
 ******************************************************************************/

esp_err_t wifi_init(void)
{
    ESP_LOGI(TAG, "Initializing SoftAP...");

    s_ap_netif = esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(
        esp_event_handler_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            &wifi_event_handler,
            NULL));

    ESP_ERROR_CHECK(wifi_configure_ap());

    ESP_ERROR_CHECK(wifi_configure_network());

    return ESP_OK;
}

/******************************************************************************
 * Start
 ******************************************************************************/

esp_err_t wifi_start(void)
{
    if (s_running)
    {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Starting SoftAP");

    return esp_wifi_start();
}

/******************************************************************************
 * Stop
 ******************************************************************************/

esp_err_t wifi_stop(void)
{
    if (!s_running)
    {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Stopping SoftAP");

    return esp_wifi_stop();
}

/******************************************************************************
 * Restart
 ******************************************************************************/

esp_err_t wifi_restart(void)
{
    ESP_ERROR_CHECK(wifi_stop());

    vTaskDelay(pdMS_TO_TICKS(300));

    ESP_ERROR_CHECK(wifi_configure_ap());

    return wifi_start();
}

/******************************************************************************
 * Status
 ******************************************************************************/

bool wifi_is_running(void)
{
    return s_running;
}

bool wifi_is_connected(void)
{
    return (s_clients > 0);
}

uint8_t wifi_get_connected_clients(void)
{
    return s_clients;
}

/******************************************************************************
 * Information
 ******************************************************************************/

const char *wifi_get_ssid(void)
{
    return WIFI_AP_SSID;
}

const char *wifi_get_ip(void)
{
    return WIFI_AP_IP;
}

uint8_t wifi_get_channel(void)
{
    return WIFI_AP_CHANNEL;
}

const char *wifi_get_hostname(void)
{
    return WIFI_AP_HOSTNAME;
}

/******************************************************************************
 * MAC address
 ******************************************************************************/

void wifi_get_mac(char *buffer, size_t len)
{
    if (buffer == NULL || len == 0)
    {
        return;
    }

    uint8_t mac[6];

    if (esp_wifi_get_mac(WIFI_IF_AP, mac) != ESP_OK)
    {
        buffer[0] = '\0';
        return;
    }

    snprintf(buffer,
             len,
             "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0],
             mac[1],
             mac[2],
             mac[3],
             mac[4],
             mac[5]);
}

/******************************************************************************
 * Configuration
 ******************************************************************************/

void wifi_get_config(softap_config_t *cfg)
{
    if (cfg == NULL)
    {
        return;
    }

    memcpy(cfg,
           &s_cfg,
           sizeof(softap_config_t));
}

esp_err_t wifi_set_config(const softap_config_t *cfg)
{
    if (cfg == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    memcpy(&s_cfg,
           cfg,
           sizeof(softap_config_t));

    ESP_ERROR_CHECK(
        wifi_save_ap_config(&s_cfg));

    return wifi_restart();
}

/******************************************************************************
 * End of file
 ******************************************************************************/