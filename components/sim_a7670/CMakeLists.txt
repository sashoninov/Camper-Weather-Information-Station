idf_component_register(
    SRCS "sim_a7670.c"
    INCLUDE_DIRS "."
    REQUIRES driver esp_system freertos esp_timer
)
