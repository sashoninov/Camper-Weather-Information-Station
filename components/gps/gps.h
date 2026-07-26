#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    bool valid;                // Валидна позиция
    bool fix;                  // GPS Fix

    double latitude;           // decimal degrees
    double longitude;          // decimal degrees

    float altitude;            // meters

    uint8_t satellites;        // satellites used for fix
    float hdop;                // Horizontal Dilution of Precision

    struct tm utc;             // UTC date & time

    uint32_t last_update_ms;   // esp_timer / 1000

} gps_data_t;


/* Initialize GPS component */
bool gps_init(void);

/* Start GPS task */
bool gps_start(void);

/* Get complete GPS data */
bool gps_get_data(gps_data_t *data);

/* Get only coordinates */
bool gps_get_location(double *lat, double *lon);

/* Get UTC date & time */
bool gps_get_time(struct tm *utc);

#ifdef __cplusplus
}
#endif