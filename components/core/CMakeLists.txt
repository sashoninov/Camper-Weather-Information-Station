idf_component_register(
    SRCS 
        "app_state.c"
        "app_events.c"
        "power_manager.c"
    INCLUDE_DIRS 
        "."

    REQUIRES 
        network
        drivers
        logic
        
)