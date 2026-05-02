#include "audio_events.h"
#include "audio_manager.h"

#include <stdint.h>
#include <stdio.h>

#define AUDIO_BASE "/sdcard/audio"

const char *audio_event_get_path(audio_event_t event)
{
    switch (event)
    {
        // SYSTEM
        case AUDIO_EVENT_BOOT:
            return AUDIO_BASE "/system/sysstart.wav";

        case AUDIO_EVENT_WELCOME:
            return AUDIO_BASE "/system/welcome.wav";

        case AUDIO_EVENT_DONE:
            return AUDIO_BASE "/system/done.wav";

        case AUDIO_EVENT_SAVED:
            return AUDIO_BASE "/system/saved.wav";

        case AUDIO_EVENT_CANCELED:
            return AUDIO_BASE "/system/cancel.wav";

        case AUDIO_EVENT_SETTINGS_APPLIED:
            return AUDIO_BASE "/system/setappl.wav";

        // CONNECTIVITY
        case AUDIO_EVENT_WIFI_CONNECTED:
            return AUDIO_BASE "/wifi/wifiok.wav";

        case AUDIO_EVENT_WIFI_DISCONNECTED:
            return AUDIO_BASE "/wifi/wifilost.wav";

        // GPS
        case AUDIO_EVENT_GPS_FIX:
            return AUDIO_BASE "/gps/gpsfix.wav";

        case AUDIO_EVENT_GPS_LOST:
            return AUDIO_BASE "/gps/gpslost.wav";

        // ALERTS
        case AUDIO_EVENT_ALARM:
            return AUDIO_BASE "/alerts/alarm.wav";

        case AUDIO_EVENT_CO2_HIGH:
            return AUDIO_BASE "/alerts/co2high.wav";

        case AUDIO_EVENT_TEMPERATURE_HIGH:
            return AUDIO_BASE "/camper/temphi.wav";

        // BATTERY
        case AUDIO_EVENT_BATTERY_LOW:
            return AUDIO_BASE "/battery/battlow.wav";

        case AUDIO_EVENT_BATTERY_CRITICAL:
            return AUDIO_BASE "/battery/battcrit.wav";

        case AUDIO_EVENT_START_BATTERY_LOW:
            return AUDIO_BASE "/battery/sbattlow.wav";

        case AUDIO_EVENT_START_BATTERY_CRITICAL:
            return AUDIO_BASE "/battery/sbattcrt.wav";

        // UI
        case AUDIO_EVENT_TOUCH:
            return AUDIO_BASE "/system/touch.wav";

        case AUDIO_EVENT_BUTTON:
            return AUDIO_BASE "/system/button.wav";

        // CAMPER
        case AUDIO_EVENT_TOILET_FULL:
            return AUDIO_BASE "/alerts/wcfull.wav";

        case AUDIO_EVENT_WATER_LOW:
            return AUDIO_BASE "/camper/waterlow.wav";

        // HOURS
        case AUDIO_EVENT_HOUR_01: return AUDIO_BASE "/hours/01.wav";
        case AUDIO_EVENT_HOUR_02: return AUDIO_BASE "/hours/02.wav";
        case AUDIO_EVENT_HOUR_03: return AUDIO_BASE "/hours/03.wav";
        case AUDIO_EVENT_HOUR_04: return AUDIO_BASE "/hours/04.wav";
        case AUDIO_EVENT_HOUR_05: return AUDIO_BASE "/hours/05.wav";
        case AUDIO_EVENT_HOUR_06: return AUDIO_BASE "/hours/06.wav";
        case AUDIO_EVENT_HOUR_07: return AUDIO_BASE "/hours/07.wav";
        case AUDIO_EVENT_HOUR_08: return AUDIO_BASE "/hours/08.wav";
        case AUDIO_EVENT_HOUR_09: return AUDIO_BASE "/hours/09.wav";
        case AUDIO_EVENT_HOUR_10: return AUDIO_BASE "/hours/10.wav";
        case AUDIO_EVENT_HOUR_11: return AUDIO_BASE "/hours/11.wav";
        case AUDIO_EVENT_HOUR_12: return AUDIO_BASE "/hours/12.wav";
        case AUDIO_EVENT_HOUR_13: return AUDIO_BASE "/hours/13.wav";
        case AUDIO_EVENT_HOUR_14: return AUDIO_BASE "/hours/14.wav";
        case AUDIO_EVENT_HOUR_15: return AUDIO_BASE "/hours/15.wav";
        case AUDIO_EVENT_HOUR_16: return AUDIO_BASE "/hours/16.wav";
        case AUDIO_EVENT_HOUR_17: return AUDIO_BASE "/hours/17.wav";
        case AUDIO_EVENT_HOUR_18: return AUDIO_BASE "/hours/18.wav";
        case AUDIO_EVENT_HOUR_19: return AUDIO_BASE "/hours/19.wav";
        case AUDIO_EVENT_HOUR_20: return AUDIO_BASE "/hours/20.wav";
        case AUDIO_EVENT_HOUR_21: return AUDIO_BASE "/hours/21.wav";
        case AUDIO_EVENT_HOUR_22: return AUDIO_BASE "/hours/22.wav";
        case AUDIO_EVENT_HOUR_23: return AUDIO_BASE "/hours/23.wav";
        case AUDIO_EVENT_HOUR_24: return AUDIO_BASE "/hours/24.wav";

        default:
            return NULL;
    }
}
