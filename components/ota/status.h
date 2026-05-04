#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float starter_v;   // INA3221 CH1

    float camper_v;    // Victron battery voltage
    float camper_a;    // Victron battery current

    float pitch;       // MPU6050
    float roll;        // MPU6050
} system_status_t;

extern system_status_t g_status;

#ifdef __cplusplus
}
#endif
