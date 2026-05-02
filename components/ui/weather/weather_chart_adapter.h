#pragma once

#include "lv_daily_chart.h"
#include "lv_hourly_chart.h"
#include "weather/weather.h"

void weather_to_daily_chart(const weather_data_t *w, lv_daily_data *out);
void weather_to_hourly_chart(const weather_data_t *w, lv_hourly_data *out);