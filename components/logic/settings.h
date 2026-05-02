#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t manual_brightness;
    uint8_t dim_level;
    bool autodim;

    uint8_t start_hour;
    uint8_t start_min;
    uint8_t end_hour;
    uint8_t end_min;

    char wifi_ssid[32];
    char wifi_pass[64];
    
} settings_t;

extern settings_t g_settings;

bool settings_save(settings_t *s);
bool settings_load(settings_t *s);

#ifdef __cplusplus
}
#endif

#endif