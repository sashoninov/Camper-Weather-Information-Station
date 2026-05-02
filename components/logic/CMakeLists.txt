idf_component_register(
    SRCS
        "calibration/calibration.c"
		"calibration/tank_levels.c"
		"time/time_format.c"
	    "settings.c"
        "storage.c"
        "brightness.c"
        "gpio_monitor.c"
        "storage_coords.c"



    INCLUDE_DIRS 
         "."
        "calibration"
		"time"

    REQUIRES
        esp_common 
		core
        audio
)