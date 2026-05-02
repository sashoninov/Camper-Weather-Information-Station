idf_component_register(
    SRCS
        "time_task.c"
        "weather_task.c"
        "location_task.c"
		"sensor_task.c"
		"brightness_task.c"
        "scd41_task.c"
        "ds18b20_task.c"
        "victron_task.c"
        "gps_raw_logger.c"
        
        

    INCLUDE_DIRS "."
    REQUIRES core ui logic network driver
)