#include "gps.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "app_state.h"
#include <string.h>
#include <stdlib.h>

#define UART_PORT UART_NUM_1
#define TX_PIN    22
#define RX_PIN    21
#define BUF_SIZE  2048

static const char *TAG = "GPS";

// Включи/изключи подробни логове
#define GPS_DEBUG 0

static char line[256];
static int  line_pos = 0;

// Последни валидни данни
gps_data_t gps_last = {0};

// Акумулатор за комбиниране RMC + GGA
static gps_data_t accum = {0};
static bool have_rmc = false;
static bool have_gga = false;

void gps_init(void)
{
    uart_config_t cfg = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };

    uart_driver_install(UART_PORT, BUF_SIZE, 0, 0, NULL, 0);
    uart_param_config(UART_PORT, &cfg);
    uart_set_pin(UART_PORT, TX_PIN, RX_PIN,
                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    ESP_LOGI(TAG, "GPS UART initialized (GN02D)");
}

static float convert_deg(const char *raw)
{
    if (!raw || !raw[0]) return 0.0f;

    float val = atof(raw);
    int   deg = (int)(val / 100.0f);
    float min = val - deg * 100.0f;
    return deg + min / 60.0f;
}

static bool try_emit_fix(gps_data_t *out)
{
    if (!have_rmc || !have_gga)
        return false;

    if (!accum.valid)
        return false;

    if (!accum.valid_time)
        return false;

    if (accum.lat == 0.0f || accum.lon == 0.0f)
        return false;

    if (accum.satellites < 3)
        return false;

    gps_last = accum;
    *out     = gps_last;

    ESP_LOGI(TAG,
             "FIX → sats=%d alt=%.2f lat=%.6f lon=%.6f",
             gps_last.satellites,
             gps_last.altitude,
             gps_last.lat,
             gps_last.lon);

    // Обновяваме app_state
    app_state_lock();
    app_state.gps = gps_last;
    app_state_unlock();

    have_rmc = false;
    have_gga = false;

    return true;
}

bool gps_read(gps_data_t *out)
{
    uint8_t buf[64];
    int len = uart_read_bytes(UART_PORT, buf, sizeof(buf),
                              200 / portTICK_PERIOD_MS);
    if (len <= 0) return false;

    for (int i = 0; i < len; i++)
    {
        char c = buf[i];

        if (c == '\n')
        {
            line[line_pos] = 0;
            line_pos = 0;

            bool is_gga = strstr(line, "GGA") != NULL;
            bool is_rmc = strstr(line, "RMC") != NULL;

            if (!is_gga && !is_rmc)
                continue;

#if GPS_DEBUG
            ESP_LOGI(TAG, "RAW: %s", line);
#endif

            char *token;
            char *saveptr = NULL;
            int   field   = 0;

            char time_raw[16] = {0};
            char date_raw[16] = {0};
            char lat_raw[16]  = {0};
            char lon_raw[16]  = {0};
            char lat_dir = 0;
            char lon_dir = 0;

            token = strtok_r(line, ",", &saveptr);

            while (token)
            {
                field++;

                if (is_gga)
                {
                    if (field == 2) strcpy(time_raw, token);
                    if (field == 3) strcpy(lat_raw,  token);
                    if (field == 4) lat_dir = token[0];
                    if (field == 5) strcpy(lon_raw,  token);
                    if (field == 6) lon_dir = token[0];

                    if (field == 8)  accum.satellites = atoi(token);
                    if (field == 10) accum.altitude   = atof(token);
                }

                if (is_rmc)
                {
                    if (field == 2) strcpy(time_raw, token);
                    if (field == 3) accum.valid = (token[0] == 'A');
                    if (field == 4) strcpy(lat_raw,  token);
                    if (field == 5) lat_dir = token[0];
                    if (field == 6) strcpy(lon_raw,  token);
                    if (field == 7) lon_dir = token[0];
                    if (field == 10) strcpy(date_raw, token);
                }

                token = strtok_r(NULL, ",", &saveptr);
            }

            // време
            if (strlen(time_raw) >= 6)
            {
                accum.hour = (time_raw[0]-'0')*10 + (time_raw[1]-'0');
                accum.min  = (time_raw[2]-'0')*10 + (time_raw[3]-'0');
                accum.sec  = (time_raw[4]-'0')*10 + (time_raw[5]-'0');
                accum.valid_time = true;
            }

            // дата
            if (strlen(date_raw) == 6)
            {
                accum.day   = (date_raw[0]-'0')*10 + (date_raw[1]-'0');
                accum.month = (date_raw[2]-'0')*10 + (date_raw[3]-'0');
                int yy      = (date_raw[4]-'0')*10 + (date_raw[5]-'0');
                accum.year  = (yy < 80) ? (2000 + yy) : (1900 + yy);
                accum.valid_time = true;
            }

            // координати
            if (lat_raw[0])
            {
                float lat = convert_deg(lat_raw);
                if (lat_dir == 'S') lat = -lat;
                accum.lat = lat;
            }

            if (lon_raw[0])
            {
                float lon = convert_deg(lon_raw);
                if (lon_dir == 'W') lon = -lon;
                accum.lon = lon;
            }

            if (is_rmc)
                have_rmc = true;

            if (is_gga)
                have_gga = true;

            if (try_emit_fix(out))
                return true;

            continue;
        }

        if (line_pos < (int)sizeof(line) - 1)
            line[line_pos++] = c;
    }

    return false;
}
