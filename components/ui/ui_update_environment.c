#include "ui.h"
#include "app_state.h"
#include "ui_update_environment.h"

void ui_update_environment(void)
{
    if (app_state.sensors.valid)
    {
        lv_label_set_text_fmt(ui_TempInsaid, "%.1f",
                              app_state.sensors.temperature);

        lv_label_set_text_fmt(ui_HumidityInsaid, "%.0f",
                              app_state.sensors.humidity);
							  
	   lv_label_set_text_fmt(ui_Pressure4, "%.0f",
							  app_state.sensors.pressure);					  

        switch (app_state.sensors.air_quality_level)
        {
            case 0:
                lv_label_set_text(ui_VOX, "Excellent");
                lv_obj_set_style_text_color(ui_VOX, lv_color_hex(0x00C853), 0);
                break;

            case 1:
                lv_label_set_text(ui_VOX, "Good");
                lv_obj_set_style_text_color(ui_VOX, lv_color_hex(0x64DD17), 0);
                break;

            case 2:
                lv_label_set_text(ui_VOX, "Moderate");
                lv_obj_set_style_text_color(ui_VOX, lv_color_hex(0xFFD600), 0);
                break;

            case 3:
                lv_label_set_text(ui_VOX, "Poor");
                lv_obj_set_style_text_color(ui_VOX, lv_color_hex(0xFF6D00), 0);
                break;

            default:
                lv_label_set_text(ui_VOX, "Very Poor");
                lv_obj_set_style_text_color(ui_VOX, lv_color_hex(0xD50000), 0);
                break;
        }
    }
    else
    {
        lv_label_set_text(ui_TempInsaid, "--");
        lv_label_set_text(ui_HumidityInsaid, "--");
        lv_label_set_text(ui_VOX, "--");

        lv_obj_set_style_text_color(ui_VOX,
                                    lv_color_hex(0x888888),
                                    0);
    }
}