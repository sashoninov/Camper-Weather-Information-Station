#pragma once

#include "weather.h"

bool weather_http_fetch_main(float lat, float lon, weather_data_t *out);
bool weather_http_fetch_sea(float lat, float lon, weather_data_t *out);