idf_component_register(
    SRCS
        "gps.c"
        "gps_task.c"
        "nmea.c"

    INCLUDE_DIRS
        "."

    REQUIRES
        driver
        freertos
        esp_timer
)