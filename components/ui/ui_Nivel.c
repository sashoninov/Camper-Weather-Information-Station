#include "ui_Nivel.h"
#include "ui.h"
#include "app_state.h"
#include "wifi.h"
#include <stdio.h>

#include "audio_manager.h"
#include "audio_events.h"



extern char wifi_ssid[33];
extern char wifi_ip[16];

void ui_nivel_update(void)
{
    char buf[220];

    app_state_lock();

    bool gps_ok = app_state.gps.valid;
    double lat  = app_state.gps.lat;
    double lon  = app_state.gps.lon;
    float alt   = app_state.gps.altitude;
    int sats    = app_state.gps.satellites;

    const char *ssid = wifi_ssid[0] ? wifi_ssid : "No WiFi";
    const char *ip   = wifi_ip[0]   ? wifi_ip   : "0.0.0.0";

    app_state_unlock();

    // ===================== AUDIO GPS EVENTS =====================

	// FIX (valid + >=3 satellites)
	if (gps_ok && sats >= 3) {
		if (!app_state.gps_fix_alerted) {
			audio_play_event(AUDIO_EVENT_GPS_FIX);
			app_state.gps_fix_alerted = true;
			app_state.gps_lost_alerted = false;
		}
	}
	// LOST
	else {
		if (!app_state.gps_lost_alerted) {
			audio_play_event(AUDIO_EVENT_GPS_LOST);
			app_state.gps_lost_alerted = true;
			app_state.gps_fix_alerted = false;
		}
	}


    if (gps_ok && sats >= 3)
    {
        snprintf(buf, sizeof(buf),
                 "GPS: %d sats | %.1f m Lat: %.5f Lon: %.5f\n"
                 "WiFi: %s (%s)",
                 sats, alt, lat, lon,
                 ssid, ip);
    }
    else
    {
        snprintf(buf, sizeof(buf),
                 "GPS: No Fix\n"
                 "WiFi: %s (%s)",
                 ssid, ip);
    }

    lv_label_set_text(ui_Nivel, buf);
}
