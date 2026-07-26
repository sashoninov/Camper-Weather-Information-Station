#include <stdio.h>
#include <string.h>

#include "esp_err.h"
#include "cJSON.h"
#include "app_state.h"
#include "ui_location.h"
#include "http_gateway.h"
#include "gps.h"

static char response_buffer[4096];

esp_err_t location_fetch(void)
{
    double lat = app_state.location.lat;
    double lon = app_state.location.lon;

    // Ако координатите липсват → вземи ги от модема
    if (lat == 0 || lon == 0) {
        double glat = 0.0, glon = 0.0;

        if (gps_get_location(&glat, &glon)) {
            lat = glat;
            lon = glon;

            app_state_lock();
            app_state.location.lat = lat;
            app_state.location.lon = lon;
            app_state_unlock();

            printf("📡 GNSS fallback coords: %.6f, %.6f\n", lat, lon);
        } else {
            printf("❌ Няма GPS координати\n");
            return ESP_FAIL;
        }
    }

    char url[512];
    snprintf(url, sizeof(url),
             "http://api-bdc.io/data/reverse-geocode-client?latitude=%f&longitude=%f&localityLanguage=bg",
             lat, lon);

    printf("🌍 Reverse URL: %s\n", url);

    if (!http_get(url, response_buffer, sizeof(response_buffer))) {
        printf("❌ HTTP failed\n");
        return ESP_FAIL;
    }

    cJSON *root = cJSON_Parse(response_buffer);
    if (!root) {
        printf("❌ JSON parse error\n");
        return ESP_FAIL;
    }

    const cJSON *city    = cJSON_GetObjectItem(root, "city");
    const cJSON *region  = cJSON_GetObjectItem(root, "principalSubdivision");
    const cJSON *country = cJSON_GetObjectItem(root, "countryName");

    app_state_lock();

    snprintf(app_state.location.city, sizeof(app_state.location.city),
             "%s", cJSON_IsString(city) ? city->valuestring : "Неизвестно");

    snprintf(app_state.location.region, sizeof(app_state.location.region),
             "%s", cJSON_IsString(region) ? region->valuestring : "");

    snprintf(app_state.location.country, sizeof(app_state.location.country),
             "%s", cJSON_IsString(country) ? country->valuestring : "");

    app_state_unlock();

    ui_update_location();

    cJSON_Delete(root);

    printf("✅ Location updated: %s, %s, %s\n",
           app_state.location.city,
           app_state.location.region,
           app_state.location.country);

    return ESP_OK;
}
