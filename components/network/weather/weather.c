#include "weather.h"
#include "weather_http.h"
#include "seas.h"

#include <stdbool.h>

#include <string.h>
#include <stdlib.h>

#include "esp_http_client.h"
#include "esp_err.h"

weather_data_t g_weather;


bool weather_process(float lat, float lon)
{
    memset(&g_weather, 0, sizeof(g_weather));

    if (!weather_http_fetch_main(lat, lon, &g_weather))
    {
        return false;
    }

    float sea_lat, sea_lon;
    int sea_index = find_nearest_sea(lat, lon, &sea_lat, &sea_lon);

    if (sea_index >= 0) {
        g_weather.has_sea = 1;
        weather_http_fetch_sea(sea_lat, sea_lon, &g_weather);
    } else {
        g_weather.has_sea = 0;
    }

    return true;
}

static char buffer[4096];

char* http_get(const char *url)
{
    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    if (esp_http_client_perform(client) != ESP_OK) {
        esp_http_client_cleanup(client);
        return NULL;
    }

    int len = esp_http_client_read_response(client, buffer, sizeof(buffer)-1);
    buffer[len] = 0;

    esp_http_client_cleanup(client);

    return strdup(buffer);
}