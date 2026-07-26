#include "weather.h"
#include "weather_http.h"
#include "seas.h"
#include "http_gateway.h"

#include <string.h>
#include <stdlib.h>

weather_data_t g_weather;

bool weather_process(float lat, float lon)
{
    memset(&g_weather, 0, sizeof(g_weather));

    if (!weather_http_fetch_main(lat, lon, &g_weather))
        return false;

    if (is_near_sea(lat, lon))
    {
        g_weather.has_sea = 1;

        // Marine API използва реалните GPS координати
        weather_http_fetch_sea(lat, lon, &g_weather);
    }
    else
    {
        g_weather.has_sea = 0;
    }

    return true;
}