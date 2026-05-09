#include "ui.h"
#include "app_state.h"

#include "audio_manager.h"
#include "audio_events.h"



void ui_update_ds18b20(void)
{
    //// Temp Vun
    if (app_state.ds_valid[0])
        lv_label_set_text_fmt(ui_TempCurrent, "%.1f", app_state.ds_temp[0]);
    else
        lv_label_set_text(ui_TempCurrent, "--");
	//
    //// Frizer
    //if (app_state.ds_valid[2])
    //    lv_label_set_text_fmt(ui_TempFrige, "%.0f", app_state.ds_temp[2]);
    //else
    //    lv_label_set_text(ui_TempFrige, "--");
	//
    // Test
    // Frizer

    if (app_state.ds_valid[1])
    {
        float temp = app_state.ds_temp[1];
    
        lv_label_set_text_fmt(ui_TempFrige, "%.0f", temp);

        // ===================== AUDIO EVENT: FRIDGE2 HIGH TEMP =====================
		if (app_state.ds_valid[1]) {
			float t = app_state.ds_temp[1];
		
			if (t > 4.0f) {
				if (!app_state.fridge2_temp_alerted) {
					audio_play_event(AUDIO_EVENT_TEMPERATURE_HIGH);
					app_state.fridge2_temp_alerted = true;
				}
			} else {
				app_state.fridge2_temp_alerted = false;
			}
		}
    
        // 🔥 логика за аларма
        if (temp > 4.0f)
        {
            // ЧЕРВЕНО
            lv_obj_set_style_text_color(ui_TempFrige,
                                       lv_color_hex(0xF44336), 0);
        }
        else
        {
            // ЗЕЛЕНО
            lv_obj_set_style_text_color(ui_TempFrige,
                                       lv_color_hex(0x4CAF50), 0);
        }
    }
    else
    {
        lv_label_set_text(ui_TempFrige, "--");
    
        // сиво ако няма данни
        lv_obj_set_style_text_color(ui_TempFrige,
                                   lv_color_hex(0x888888), 0);
    }
}