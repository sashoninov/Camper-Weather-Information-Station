#include "weather_http.h"
#include "weather.h"
#include "seas.h"
#include <math.h>


#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_crt_bundle.h"

#include "cJSON.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

static const char *TAG = "WEATHER_HTTP";

typedef struct {
    char *buffer;
    int length;
} http_response_t;

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    http_response_t *res = (http_response_t *)evt->user_data;

    if (evt->event_id == HTTP_EVENT_ON_DATA) {
        char *new_buf = realloc(res->buffer, res->length + evt->data_len + 1);
        if (!new_buf) return ESP_FAIL;

        res->buffer = new_buf;
        memcpy(res->buffer + res->length, evt->data, evt->data_len);
        res->length += evt->data_len;
        res->buffer[res->length] = 0;
    }

    return ESP_OK;
}

static char *http_get(const char *url)
{
    http_response_t res = {0};

    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_event_handler,
        .user_data = &res,
        .timeout_ms = 10000,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);

    if (esp_http_client_perform(client) != ESP_OK) {
        ESP_LOGE(TAG, "HTTP failed");
        esp_http_client_cleanup(client);
        free(res.buffer);
        return NULL;
    }

    esp_http_client_cleanup(client);
    return res.buffer;
}

bool weather_http_fetch_main(float lat, float lon, weather_data_t *out)
{
    if (!out) return false;

    char url[512];

    // ⭐ FIX: винаги искаме DAILY_POINTS дни (7)
    snprintf(url, sizeof(url),
        "https://api.open-meteo.com/v1/forecast?"
        "latitude=%.6f&longitude=%.6f"
        "&current=temperature_2m,relative_humidity_2m,weather_code,"
        "wind_speed_10m,wind_gusts_10m,wind_direction_10m,uv_index,pressure_msl,is_day"
        "&hourly=temperature_2m,precipitation_probability,cloudcover,precipitation"
        "&daily=temperature_2m_max,temperature_2m_min,precipitation_sum,"
        "precipitation_probability_max,weather_code,sunrise,sunset,cloudcover_mean"
        "&forecast_days=%d"
        "&timezone=auto",
        lat, lon, DAILY_POINTS);

    char *response = http_get(url);

    if (!response || strlen(response) == 0) {
        free(response);
        return false;
    }

    cJSON *root = cJSON_Parse(response);
    free(response);

    if (!root) return false;

    // =========================
    // CURRENT
    // =========================
    cJSON *current = cJSON_GetObjectItem(root, "current");

    if (current) {
        cJSON *temp = cJSON_GetObjectItem(current, "temperature_2m");
        cJSON *hum  = cJSON_GetObjectItem(current, "relative_humidity_2m");
        cJSON *code = cJSON_GetObjectItem(current, "weather_code");
        cJSON *wind = cJSON_GetObjectItem(current, "wind_speed_10m");
        cJSON *gust = cJSON_GetObjectItem(current, "wind_gusts_10m");
        cJSON *dir  = cJSON_GetObjectItem(current, "wind_direction_10m");
        cJSON *uv   = cJSON_GetObjectItem(current, "uv_index");
        cJSON *pres = cJSON_GetObjectItem(current, "pressure_msl");
        cJSON *day  = cJSON_GetObjectItem(current, "is_day");

        if (temp) out->current_temp = temp->valuedouble;
        if (hum)  out->humidity     = hum->valuedouble;
        if (code) out->weather_code = code->valueint;
        if (wind) out->wind_speed   = wind->valuedouble;
        if (gust) out->wind_gust    = gust->valuedouble;
        if (dir)  out->wind_dir     = dir->valuedouble;
        if (uv)   out->uv           = uv->valuedouble;
        if (pres) out->pressure     = pres->valuedouble;
        if (day)  out->is_day       = day->valueint;
    }

    // =========================
    // HOURLY
    // =========================
    cJSON *hourly = cJSON_GetObjectItem(root, "hourly");

    if (hourly) {
        cJSON *hourly_time  = cJSON_GetObjectItem(hourly, "time");
        cJSON *temp  = cJSON_GetObjectItem(hourly, "temperature_2m");
        cJSON *rainp = cJSON_GetObjectItem(hourly, "precipitation_probability");
        cJSON *rain  = cJSON_GetObjectItem(hourly, "precipitation");
        cJSON *cloud = cJSON_GetObjectItem(hourly, "cloudcover");

        if (hourly_time && temp && rainp && rain && cloud) {
            int total_time  = cJSON_GetArraySize(hourly_time);
            int total_temp  = cJSON_GetArraySize(temp);
            int total_rainp = cJSON_GetArraySize(rainp);
            int total_rain  = cJSON_GetArraySize(rain);
            int total_cloud = cJSON_GetArraySize(cloud);

            int total = total_time;
            if (total_temp  < total) total = total_temp;
            if (total_rainp < total) total = total_rainp;
            if (total_rain  < total) total = total_rain;
            if (total_cloud < total) total = total_cloud;

            time_t now = time(NULL);
            struct tm tm_now;
            localtime_r(&now, &tm_now);
            int current_hour = tm_now.tm_hour;

            for (int i = 0; i < HOURLY_POINTS; i++) {
                int idx = i + current_hour;
                if (idx >= total) break;

                cJSON *titem = cJSON_GetArrayItem(hourly_time, idx);
                if (titem && titem->valuestring) {
                    struct tm tm = {0};
                    strptime(titem->valuestring, "%Y-%m-%dT%H:%M", &tm);
                    out->hourly_time[i] = mktime(&tm);
                }

                cJSON *v;

                v = cJSON_GetArrayItem(temp, idx);
                if (v) out->hourly_temp[i] = v->valuedouble;

                v = cJSON_GetArrayItem(rainp, idx);
                if (v) out->rain_prob[i] = v->valuedouble;

                v = cJSON_GetArrayItem(rain, idx);
                if (v) out->rain[i] = v->valuedouble;

                v = cJSON_GetArrayItem(cloud, idx);
                if (v) out->cloud[i] = v->valuedouble;
            }

            out->cloudcover = out->cloud[0];
        }
    }

    // =========================
    // DAILY
    // =========================
    cJSON *daily = cJSON_GetObjectItem(root, "daily");

    if (daily) {
        cJSON *max  = cJSON_GetObjectItem(daily, "temperature_2m_max");
        cJSON *min  = cJSON_GetObjectItem(daily, "temperature_2m_min");
        cJSON *rain = cJSON_GetObjectItem(daily, "precipitation_sum");
        cJSON *pop  = cJSON_GetObjectItem(daily, "precipitation_probability_max");
        cJSON *time = cJSON_GetObjectItem(daily, "time");
        cJSON *sunrise = cJSON_GetObjectItem(daily, "sunrise");
        cJSON *sunset  = cJSON_GetObjectItem(daily, "sunset");
        cJSON *cloud = cJSON_GetObjectItem(daily, "cloudcover_mean");

        int total_time    = time    ? cJSON_GetArraySize(time)    : 0;
        int total_max     = max     ? cJSON_GetArraySize(max)     : 0;
        int total_min     = min     ? cJSON_GetArraySize(min)     : 0;
        int total_rain    = rain    ? cJSON_GetArraySize(rain)    : 0;
        int total_pop     = pop     ? cJSON_GetArraySize(pop)     : 0;
        int total_cloud   = cloud   ? cJSON_GetArraySize(cloud)   : 0;
        int total_sunrise = sunrise ? cJSON_GetArraySize(sunrise) : 0;
        int total_sunset  = sunset  ? cJSON_GetArraySize(sunset)  : 0;

        for (int i = 0; i < DAILY_POINTS; i++) {
            cJSON *v;

            if (i < total_max) {
                v = cJSON_GetArrayItem(max, i);
                if (v) out->daily_max[i] = v->valuedouble;
            }

            if (i < total_min) {
                v = cJSON_GetArrayItem(min, i);
                if (v) out->daily_min[i] = v->valuedouble;
            }

            if (i < total_rain) {
                v = cJSON_GetArrayItem(rain, i);
                if (v) out->daily_rain[i] = v->valuedouble;
            }

            if (i < total_pop) {
                v = cJSON_GetArrayItem(pop, i);
                if (v) out->daily_pop[i] = v->valuedouble;
            }

            if (i < total_cloud) {
                v = cJSON_GetArrayItem(cloud, i);
                if (v) out->daily_cloud[i] = v->valuedouble;
            }

            if (time && i < total_time) {
                cJSON *titem = cJSON_GetArrayItem(time, i);
                if (titem && titem->valuestring) {
                    struct tm tm = {0};
                    strptime(titem->valuestring, "%Y-%m-%d", &tm);
                    out->daily_time[i] = mktime(&tm);
                }
            }

            if (sunrise && i < total_sunrise) {
                cJSON *sitem = cJSON_GetArrayItem(sunrise, i);
                if (sitem && sitem->valuestring) {
                    const char *s = sitem->valuestring;
                    if (strlen(s) >= 16) {
                        strncpy(out->sunrise[i], s + 11, 5);
                        out->sunrise[i][5] = '\0';
                    }
                }
            }

            if (sunset && i < total_sunset) {
                cJSON *sitem = cJSON_GetArrayItem(sunset, i);
                if (sitem && sitem->valuestring) {
                    const char *s = sitem->valuestring;
                    if (strlen(s) >= 16) {
                        strncpy(out->sunset[i], s + 11, 5);
                        out->sunset[i][5] = '\0';
                    }
                }
            }
        }
    }

    cJSON_Delete(root);

    ESP_LOGI(TAG, "Weather FULL parsed OK");

    return true;
}

//
// ⭐ Реална морска температура от Open‑Meteo Marine API
//
bool weather_http_fetch_sea(float lat, float lon, weather_data_t *out)
{
    if (!out) return false;

    float sea_lat, sea_lon;

    // 1) Намираме най-близкото море
    int idx = find_nearest_sea(lat, lon, &sea_lat, &sea_lon);

    if (idx < 0) {
        // Твърде далеч от море
        out->has_sea = 0;
        return true;
    }

    // 2) Изчисляваме разстоянието до морето
    float dlat = lat - sea_lat;
    float dlon = lon - sea_lon;
    float dist = sqrtf(dlat*dlat + dlon*dlon);

    // 3) Ако сме над 50 км → не показваме море
    if (dist > 0.50f) {   // 0.50 градуса ≈ 55 км
        out->has_sea = 0;
        return true;
    }

    // 4) Теглим реална морска температура за точните GPS координати
    char url[512];

    snprintf(url, sizeof(url),
        "https://marine-api.open-meteo.com/v1/marine?"
        "latitude=%.6f&longitude=%.6f"
        "&daily=sea_surface_temperature_max"
        "&forecast_days=1"
        "&timezone=auto",
        lat, lon);

    char *response = http_get(url);

    if (!response || strlen(response) == 0) {
        free(response);
        out->has_sea = 0;
        return true;
    }

    cJSON *root = cJSON_Parse(response);
    free(response);

    if (!root) {
        out->has_sea = 0;
        return true;
    }

    cJSON *daily = cJSON_GetObjectItem(root, "daily");
    if (!daily) {
        cJSON_Delete(root);
        out->has_sea = 0;
        return true;
    }

    cJSON *temp = cJSON_GetObjectItem(daily, "sea_surface_temperature_max");
    if (!temp || cJSON_GetArraySize(temp) == 0) {
        cJSON_Delete(root);
        out->has_sea = 0;
        return true;
    }

    cJSON *v = cJSON_GetArrayItem(temp, 0);
    if (v) {
        out->sea_temp = v->valuedouble;
        out->has_sea = 1;
    } else {
        out->has_sea = 0;
    }

    cJSON_Delete(root);
    return true;
}
