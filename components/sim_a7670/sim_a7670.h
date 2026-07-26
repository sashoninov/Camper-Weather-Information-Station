#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * GSM STATUS
 ******************************************************************************/

typedef struct
{
    /* General state */

    bool online;          // Modem responds to AT
    bool ready;           // Modem is ready for data (registered)
    bool registered;      // Registered to cellular network
    bool gnss;            // GNSS engine enabled

    /* Signal */

    int rssi;
    int rsrp;
    int rsrq;
    int bars;

    /* Network */

    char operator_name[32];
    char network[16];

    /* Statistics */

    uint32_t reconnects;

} gsm_status_t;

/******************************************************************************
 * GLOBAL STATUS
 ******************************************************************************/

extern gsm_status_t gsm_status;

/******************************************************************************
 * DRIVER
 ******************************************************************************/

bool sim_a7670_init(void);

bool sim_a7670_start(void);

void sim_a7670_monitor_start(void);

/******************************************************************************
 * HTTP
 ******************************************************************************/

int sim_a7670_http_get(
    const char *url,
    char *buf,
    size_t buf_len);

/******************************************************************************
 * GNSS
 ******************************************************************************/

bool sim_a7670_get_location(
    double *lat,
    double *lon);

bool sim_a7670_get_time(
    struct tm *utc);

#ifdef __cplusplus
}
#endif