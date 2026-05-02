#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "weather.h"
#include "gps.h"

#ifdef __cplusplus
extern "C" {
#endif

// =====================
// SENSOR DATA
// =====================
typedef struct {
    float temperature;
    float humidity;
    float pressure;
    int co2;
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

    gps_data_t gps; 
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

    bool hourly_chime_enabled;   // 🔔 от настройките
    int last_hour_chime;         // последният час, за който е пуснат звук
    bool dimming_active;         // дали екранът е димиран

    bool wifi_connected;
    bool time_synced;

    // 🌬️ SCD41
    bool scd_valid;
    uint16_t co2;
    float scd_temp;
    float scd_humidity;

    // 🌡️ DS18B20 (3 сензора по име)
    #define DS_MAX 3
    bool ds_valid[DS_MAX];
    float ds_temp[DS_MAX];

    // ❄️ Fridge #2 (хладилна камера) alert flag
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

    // 🔊 GPS audio flags
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
