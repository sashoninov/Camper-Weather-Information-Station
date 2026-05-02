idf_component_register(
    SRCS
        "storage.c"
        "nvs_storage.c"
        
    INCLUDE_DIRS
        "."
    REQUIRES
        nvs_flash
        logic
)