#pragma once

#include <time.h>
#include <stdbool.h>

#define HOURLY_POINTS 48
#define DAILY_POINTS 7

typedef struct {

    // =========================
    // CURRENT
    // =========================
    float current_temp;
    int weather_code;
    bool is_day;

    float wind_speed;
    float wind_gust;
    float wind_dir;

    float pressure;
    float uv;
    float cloudcover;
    float humidity;

    // =========================
    // HOURLY
    // =========================
    float hourly_temp[HOURLY_POINTS];
    float rain[HOURLY_POINTS];
    float rain_prob[HOURLY_POINTS];
    float cloud[HOURLY_POINTS];
    float snow[HOURLY_POINTS];
    time_t hourly_time[HOURLY_POINTS];

    // =========================
    // DAILY
    // =========================
    float daily_max[DAILY_POINTS];
    float daily_min[DAILY_POINTS];

    char sunrise[DAILY_POINTS][6];  // HH:MM
    char sunset[DAILY_POINTS][6];   // HH:MM

    // =========================
    // SEA
    // =========================
    float sea_temp;
    int has_sea;

    // =========================
    // DAILY EXTRA (Kreuzer)
    // =========================
    float daily_rain[DAILY_POINTS];
    float daily_snow[DAILY_POINTS];
    float daily_pop[DAILY_POINTS];
    float daily_cloud[DAILY_POINTS];
    time_t daily_time[DAILY_POINTS];

} weather_data_t;

bool weather_process(float lat, float lon);
