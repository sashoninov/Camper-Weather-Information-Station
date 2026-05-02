#include "weather_chart_adapter.h"
#include <time.h>

// ================= DAILY =================
void weather_to_daily_chart(const weather_data_t *w, lv_daily_data *out)
{
    for (int i = 0; i < NUM_DAYS; i++)
    {
        struct tm tm = {0};
        localtime_r(&w->daily_time[i], &tm);

        out[i].dt = tm;
        out[i].high_temp = w->daily_max[i];
        out[i].low_temp  = w->daily_min[i];
        out[i].rain = w->daily_rain[i];
        out[i].snow = w->daily_snow[i];
        out[i].pop  = w->daily_pop[i] / 100.0f;


        
        out[i].sun = 1.0f - (w->daily_cloud[i] / 100.0f);
    }
}

// ================= HOURLY =================
void weather_to_hourly_chart(const weather_data_t *w, lv_hourly_data *out)
{
    for (int i = 0; i < NUM_HOURS; i++)
    {
        time_t t = w->hourly_time[i];
        struct tm tm;
        localtime_r(&t, &tm);

        out[i].dt = tm;
        out[i].temp = w->hourly_temp[i];
        out[i].rain = w->rain[i];
        out[i].snow = w->snow[i];
        out[i].pop  = w->rain_prob[i] / 100.0f;
        out[i].sun  = 1.0f - (w->cloud[i] / 100.0f);
    }
}
