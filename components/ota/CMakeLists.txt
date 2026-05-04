idf_component_register(
    SRCS "status.c" "ota_update.c"
    INCLUDE_DIRS "."
    REQUIRES esp_http_server app_update esp_wifi cjson esp_app_format esp_system
)
