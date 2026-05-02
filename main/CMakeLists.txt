idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "."
    REQUIRES 
        core
        drivers
        ui
        network
        tasks
        logic
        audio
        sdcard
       
        
)
idf_component_get_property(lvgl_lib lvgl__lvgl COMPONENT_LIB)

target_include_directories(${lvgl_lib} PUBLIC ${PROJECT_DIR})


target_compile_definitions(${lvgl_lib} PUBLIC
    LV_CONF_INCLUDE_SIMPLE
    
)

target_link_libraries(${COMPONENT_LIB} "-u _printf_float")
target_compile_definitions(${COMPONENT_LIB} PRIVATE LV_CONF_INCLUDE_SIMPLE)