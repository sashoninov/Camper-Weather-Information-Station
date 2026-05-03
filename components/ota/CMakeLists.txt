idf_component_register(
    SRCS "ota_update.c"
    INCLUDE_DIRS "."
    REQUIRES esp_http_server app_update esp_wifi
)
