#include "nmea.h"
#include "gps_private.h"

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "esp_log.h"
#include <stdint.h>

static const char *TAG = "NMEA";

/*----------------------------------------------------------
 * Private functions
 *---------------------------------------------------------*/

static bool nmea_checksum_ok(const char *line);

static bool nmea_get_field(const char *line,
                           int field,
                           char *out,
                           size_t out_size);

static double parse_latitude(const char *value, const char *ns);
static double parse_longitude(const char *value, const char *ew);

static void parse_rmc(const char *line);
static void parse_gga(const char *line);

/*----------------------------------------------------------
 * Public
 *---------------------------------------------------------*/

void nmea_parse(const char *line)
{
    if (line == NULL)
        return;

    if (line[0] != '$')
        return;

    if (!nmea_checksum_ok(line))
        return;

    if (strncmp(line, "$GNRMC", 6) == 0 ||
        strncmp(line, "$GPRMC", 6) == 0)
    {
        parse_rmc(line);
    }
    else if (strncmp(line, "$GNGGA", 6) == 0 ||
             strncmp(line, "$GPGGA", 6) == 0)
    {
        parse_gga(line);
    }
}

static bool nmea_checksum_ok(const char *line)
{
    if (line == NULL)
        return false;

    if (*line != '$')
        return false;

    const char *star = strchr(line, '*');

    if (star == NULL)
        return false;

    uint8_t checksum = 0;

    for (const char *p = line + 1; p < star; p++)
        checksum ^= (uint8_t)*p;

    char *endptr;

    long expected = strtol(star + 1, &endptr, 16);

    if (endptr != star + 3)
        return false;

    return checksum == (uint8_t)expected;
}

static bool nmea_get_field(const char *line,
                           int field,
                           char *out,
                           size_t out_size)
{
    if (line == NULL || out == NULL || out_size == 0)
        return false;

    int current = 0;
    const char *p = line;

    /* Skip '$' */
    if (*p == '$')
        p++;

    while (*p)
    {
        if (current == field)
        {
            size_t len = 0;

            while (*p && *p != ',' && *p != '*')
            {
                if (len < out_size - 1)
                    out[len++] = *p;

                p++;
            }

            out[len] = '\0';
            return true;
        }

        while (*p && *p != ',' && *p != '*')
            p++;

        if (*p == ',')
        {
            current++;
            p++;
            continue;
        }

        break;
    }

    out[0] = '\0';
    return false;
}

static double parse_latitude(const char *value, const char *ns)
{
    if (value == NULL || ns == NULL)
        return 0.0;

    double v = atof(value);

    int deg = (int)(v / 100.0);
    double min = v - (deg * 100.0);

    double lat = deg + (min / 60.0);

    if (ns[0] == 'S')
        lat = -lat;

    return lat;
}

static double parse_longitude(const char *value, const char *ew)
{
    if (value == NULL || ew == NULL)
        return 0.0;

    double v = atof(value);

    int deg = (int)(v / 100.0);
    double min = v - (deg * 100.0);

    double lon = deg + (min / 60.0);

    if (ew[0] == 'W')
        lon = -lon;

    return lon;
}

static void parse_rmc(const char *line)
{
    gps_data_t gps;

    /* Вземаме текущите данни, за да не загубим altitude, satellites и др. */
    if (!gps_get_data(&gps))
        memset(&gps, 0, sizeof(gps));

    char field[32];
    char ns[2] = {0};
    char ew[2] = {0};

    /* Status (A = valid, V = invalid) */
    if (nmea_get_field(line, 2, field, sizeof(field)))
        gps.valid = (field[0] == 'A');

    /* Latitude */
    if (nmea_get_field(line, 3, field, sizeof(field)))
    {
        nmea_get_field(line, 4, ns, sizeof(ns));
        gps.latitude = parse_latitude(field, ns);
    }

    /* Longitude */
    if (nmea_get_field(line, 5, field, sizeof(field)))
    {
        nmea_get_field(line, 6, ew, sizeof(ew));
        gps.longitude = parse_longitude(field, ew);
    }

    /* UTC Time (hhmmss.sss) */
    if (nmea_get_field(line, 1, field, sizeof(field)))
    {
        if (strlen(field) >= 6)
        {
            gps.utc.tm_hour = (field[0]-'0')*10 + (field[1]-'0');
            gps.utc.tm_min  = (field[2]-'0')*10 + (field[3]-'0');
            gps.utc.tm_sec  = (field[4]-'0')*10 + (field[5]-'0');
        }
    }

    /* Date (ddmmyy) */
    if (nmea_get_field(line, 9, field, sizeof(field)))
    {
        if (strlen(field) >= 6)
        {
            gps.utc.tm_mday = (field[0]-'0')*10 + (field[1]-'0');
            gps.utc.tm_mon  = ((field[2]-'0')*10 + (field[3]-'0')) - 1;
            gps.utc.tm_year = ((field[4]-'0')*10 + (field[5]-'0')) + 100;
        }
    }

    gps_update_data(&gps);
}

static void parse_gga(const char *line)
{
    gps_data_t gps;

    /* Вземаме текущите данни */
    if (!gps_get_data(&gps))
        memset(&gps, 0, sizeof(gps));

    char field[32];

    /* Fix Quality
       0 = invalid
       1 = GPS
       2 = DGPS
       4 = RTK
       ...
    */
    if (nmea_get_field(line, 6, field, sizeof(field)))
    {
        gps.fix = (atoi(field) > 0);
    }

    /* Satellites used */
    if (nmea_get_field(line, 7, field, sizeof(field)))
    {
        gps.satellites = (uint8_t)atoi(field);
    }

    /* HDOP */
    if (nmea_get_field(line, 8, field, sizeof(field)))
    {
        gps.hdop = strtof(field, NULL);
    }

    /* Altitude */
    if (nmea_get_field(line, 9, field, sizeof(field)))
    {
        gps.altitude = strtof(field, NULL);
    }

    gps_update_data(&gps);
}