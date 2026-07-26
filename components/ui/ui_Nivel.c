#include "ui_Nivel.h"
#include "ui.h"
#include "app_state.h"
#include "wifi.h"
#include "sim_a7670.h"

#include <stdio.h>

#include "audio_manager.h"
#include "audio_events.h"

#include "gps.h"


void ui_nivel_update(void)
{
    char buf[256];

    gps_data_t gps;
    bool gps_ok = gps_get_data(&gps);


    
    if (gps_ok && gps.fix)
    {
        if (!app_state.gps_fix_alerted)
        {
            audio_play_event(AUDIO_EVENT_GPS_FIX);
            app_state.gps_fix_alerted  = true;
            app_state.gps_lost_alerted = false;
        }
    }
    else
    {
        if (!app_state.gps_lost_alerted)
        {
            audio_play_event(AUDIO_EVENT_GPS_LOST);
            app_state.gps_lost_alerted = true;
            app_state.gps_fix_alerted  = false;
        }
    }
    

	if (gps_ok && gps.fix)
	{
		snprintf(buf, sizeof(buf),
				"SAT:%u HDOP:%.1f ALT:%.0fm LAT:%.4f LON:%.4f\n"
				"%s %s    Camper Station http://192.168.4.1",
				gps.satellites,
				gps.hdop,
				gps.altitude,
				gps.latitude,
				gps.longitude,
				gsm_status.operator_name,
				gsm_status.network);

	}
	else
	{
		snprintf(buf, sizeof(buf),
				"GPS: No Fix\n"
				"%s %s    Camper Station http://192.168.4.1",
				gsm_status.operator_name,
				gsm_status.network);
	}
    lv_label_set_text(ui_Nivel, buf);
	
	if (!gsm_status.online)
	{
		lv_img_set_src(ui_Image9, &ui_img_530950535);
	}
	else
	{
		switch (gsm_status.bars)
		{
			case 0:
				lv_img_set_src(ui_Image9, &ui_img_998318497);
				break;
	
			case 1:
				lv_img_set_src(ui_Image9, &ui_img_998489662);
				break;
	
			case 2:
				lv_img_set_src(ui_Image9, &ui_img_998386271);
				break;
	
			case 3:
				lv_img_set_src(ui_Image9, &ui_img_998565636);
				break;
	
			case 4:
			case 5:
				lv_img_set_src(ui_Image9, &ui_img_998461989);
				break;
	
			default:
				lv_img_set_src(ui_Image9, &ui_img_530950535);
				break;
		}
	}
}