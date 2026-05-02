#include <stdio.h>
#include <string.h>

#include "esp_http_client.h"
#include "esp_err.h"
#include "cJSON.h"

#include "app_state.h"
#include "ui_location.h"

// ==============================
// RESPONSE BUFFER
// ==============================
static char response_buffer[4096];

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    static int output_len = 0;

    switch (evt->event_id)
    {
        case HTTP_EVENT_ON_DATA:
            if (output_len + evt->data_len < sizeof(response_buffer)) {
                memcpy(response_buffer + output_len, evt->data, evt->data_len);
                output_len += evt->data_len;
            }
            break;

        case HTTP_EVENT_ON_FINISH:
            response_buffer[output_len] = 0;
            output_len = 0;
            break;

        default:
            break;
    }

    return ESP_OK;
}

// ==============================
// MAIN FUNCTION
// ==============================
esp_err_t location_fetch(void)
{
    double lat = app_state.location.lat;
    double lon = app_state.location.lon;

    if (lat == 0 || lon == 0) {
        printf("❌ Няма GPS координати\n");
        return ESP_FAIL;
    }

    // BigDataCloud Reverse Geocoding (HTTP → работи на ESP32-P4)
    char url[512];
    snprintf(url, sizeof(url),
             "http://api-bdc.io/data/reverse-geocode-client?latitude=%f&longitude=%f&localityLanguage=bg",
             lat, lon);

    printf("🌍 Reverse URL: %s\n", url);

    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_event_handler,
        .timeout_ms = 8000,
        .method = HTTP_METHOD_GET,
        .user_agent = "camper-weather-v2"
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        printf("❌ esp_http_client_init failed\n");
        return ESP_FAIL;
    }

    // ESP32-P4 WiFi6 → задължително Connection: close
    esp_http_client_set_header(client, "Connection", "close");

    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK) {
        printf("❌ HTTP request failed: %s\n", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    // ==============================
    // PARSE JSON
    // ==============================
    cJSON *root = cJSON_Parse(response_buffer);
    if (!root) {
        printf("❌ JSON parse error\n");
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    const cJSON *city    = cJSON_GetObjectItem(root, "city");
    const cJSON *region  = cJSON_GetObjectItem(root, "principalSubdivision");
    const cJSON *country = cJSON_GetObjectItem(root, "countryName");

    app_state_lock();

    snprintf(app_state.location.city, sizeof(app_state.location.city), "%s",
             cJSON_IsString(city) ? city->valuestring : "Неизвестно");

    snprintf(app_state.location.region, sizeof(app_state.location.region), "%s",
             cJSON_IsString(region) ? region->valuestring : "");

    snprintf(app_state.location.country, sizeof(app_state.location.country), "%s",
             cJSON_IsString(country) ? country->valuestring : "");

    app_state_unlock();

    ui_update_location();

    cJSON_Delete(root);
    esp_http_client_cleanup(client);

    printf("✅ Location updated: %s, %s, %s\n",
           app_state.location.city,
           app_state.location.region,
           app_state.location.country);

    return ESP_OK;
}
