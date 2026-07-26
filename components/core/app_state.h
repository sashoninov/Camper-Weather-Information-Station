#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "weather.h"

#ifdef __cplusplus
extern "C" {
#endif

// =====================
// SENSOR DATA
// =====================
typedef struct
{
    bool valid;

    // Environmental data
    float temperature;
    float humidity;
    float pressure;

    // BME680
    float gas_resistance;      // Ω
    uint16_t iaq;              // Air Quality Index
    uint8_t air_quality_level; // 0..4

} sensor_data_t;

// =====================
// MPU DATA
// =====================
typedef struct {
    float ax, ay, az;
    float gx, gy, gz;
    float pitch;
    float roll;
} mpu_data_t;

// =====================
// LOCATION DATA
// =====================
typedef struct {
    float lat;
    float lon;
    char city[32];
    char region[32];
    char country[32];
} location_data_t;

// =====================
// GLOBAL STATE
// =====================
typedef struct {

    sensor_data_t sensors;
    weather_data_t weather;
    location_data_t location;

    mpu_data_t mpu;

    // 🔥 INA
    float ina_voltage[3];

    // 🔊 Starter battery audio flags
    bool starter_batt_low_alerted;
    bool starter_batt_critical_alerted;

    // 🔥 CALIBRATION
    bool request_calibration;
    bool calibration_done;

    // 🔥 TIME
    char time_str[6];
    char date_str[32];
    char weekday_str[32];

    bool hourly_chime_enabled;
    int last_hour_chime;
    bool dimming_active;

    bool wifi_connected;
    bool time_synced;



    // 🌡️ DS18B20 (3 сензора)
    #define DS_MAX 3
    bool ds_valid[DS_MAX];
    float ds_temp[DS_MAX];

    // ❄️ Fridge #2 alert flag
    bool fridge2_temp_alerted;

    // 🔋 Victron MPPT
    bool mppt_valid;
    float battery_voltage;
    float battery_current;
    float solar_voltage;
    int solar_power;
    int charge_state;

    // 🔊 Battery audio alert flags
    bool battery_low_alerted;
    bool battery_critical_alerted;

    // 💧 Water alert flag
    bool water_low_alerted;

    // 🚽 WC alert flag
    bool wc_full_alerted;

    // 🔊 GPS audio flags (остават, но вече не са свързани с gps_data_t)
    bool gps_fix_alerted;
    bool gps_lost_alerted;

    // 🔊 WiFi audio flags
    bool wifi_alerted_connected;
    bool wifi_alerted_lost;

    bool victron_batt_low_alerted;
    bool victron_batt_critical_alerted;

} app_state_t;

// =====================
// API
// =====================
void app_state_init(void);
void app_state_lock(void);
void app_state_unlock(void);
app_state_t* app_state_get(void);

extern app_state_t app_state;

#ifdef __cplusplus
}
#endif
