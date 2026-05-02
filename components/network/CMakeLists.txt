file(GLOB_RECURSE SRC_FILES "*.c")

idf_component_register(
    SRCS ${SRC_FILES}
    INCLUDE_DIRS 
        "."
        "wifi"
        "weather"
        "location"
    REQUIRES 
        esp_wifi 
        nvs_flash 
        esp_event 
        esp_netif 
        esp_http_client
        esp_timer 
        esp-tls 
        cjson 
        core
        ui
        audio
        
)