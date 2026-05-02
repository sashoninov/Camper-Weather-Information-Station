#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    bool valid;          // RMC status: A = valid, V = invalid
    bool valid_time;     // true when RMC time is valid (status A)

    double lat;
    double lon;

    float altitude;      // from GGA
    int satellites;      // from GGA

    int hour;
    int min;
    int sec;

    int day;
    int month;
    int year;

} gps_data_t;

// Инициализация на GPS (UART + задача)
void gps_init(void);

// Връща последния FIX, ако има валиден
bool gps_read(gps_data_t *out);

// Последен FIX и време на FIX-а (в тикове)
extern gps_data_t gps_last;
extern uint32_t gps_last_fix_ms;

#ifdef __cplusplus
}
#endif
