/******************************************************************************
 *
 * wifi_storage.c
 *
 * Camper Weather Station
 * ESP32-P4
 *
 * SoftAP configuration storage
 *
 ******************************************************************************/

#include "wifi_storage.h"

#include "nvs.h"
#include "nvs_flash.h"

#include <string.h>

#define WIFI_NAMESPACE    "wifi"

/******************************************************************************
 * Factory defaults
 ******************************************************************************/

static void wifi_set_defaults(softap_config_t *cfg)
{
    memset(cfg, 0, sizeof(softap_config_t));

    strlcpy(cfg->password,
            WIFI_DEFAULT_PASSWORD,
            sizeof(cfg->password));
}

/******************************************************************************
 * Load configuration
 ******************************************************************************/

esp_err_t wifi_load_ap_config(softap_config_t *cfg)
{
    if (cfg == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    wifi_set_defaults(cfg);

    nvs_handle_t nvs;

    esp_err_t err = nvs_open(WIFI_NAMESPACE,
                             NVS_READONLY,
                             &nvs);

    if (err != ESP_OK)
    {
        /* First boot -> use defaults */
        return ESP_OK;
    }

    size_t len;

    len = sizeof(cfg->password);

    err = nvs_get_str(nvs,
                      "password",
                      cfg->password,
                      &len);

    if (err != ESP_OK)
    {
        strlcpy(cfg->password,
                WIFI_DEFAULT_PASSWORD,
                sizeof(cfg->password));
    }

    nvs_close(nvs);

    return ESP_OK;
}

/******************************************************************************
 * Save configuration
 ******************************************************************************/

esp_err_t wifi_save_ap_config(const softap_config_t *cfg)
{
    if (cfg == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t nvs;

    esp_err_t err = nvs_open(WIFI_NAMESPACE,
                             NVS_READWRITE,
                             &nvs);

    if (err != ESP_OK)
    {
        return err;
    }

    err = nvs_set_str(nvs,
                      "password",
                      cfg->password);
				  

    if (err != ESP_OK)
    {
        nvs_close(nvs);
        return err;
    }



    err = nvs_commit(nvs);

    nvs_close(nvs);

    return err;
}

/******************************************************************************
 * Restore factory defaults
 ******************************************************************************/

esp_err_t wifi_reset_ap_config(void)
{
    softap_config_t cfg;

    wifi_set_defaults(&cfg);

    return wifi_save_ap_config(&cfg);
}