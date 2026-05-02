idf_component_register(
    SRCS
        "i2c/i2c_bus.c"
        "i2c/i2c_bus1.c"
        "sensors/scd41.c"
        "sensors/bh1750.c"
        "sensors/ina3221.c"
        "sensors/mpu6050.c"
        "display/display.c"
        "sensors/ds18b20.c"
        "sensors/victron_driver.c"
        "clock/ds3231.c" 
        "gps/gps.c"

    INCLUDE_DIRS
        "."
        "i2c"
        "sensors"
        "display"
        "clock"
        
        "gps"

    REQUIRES 
        esp_driver_i2c
        esp_driver_uart
        esp_lcd
        esp_lcd_touch
        esp_lcd_touch_gt911
        esp_lcd_jd9365
        esp_lvgl_port
        lvgl
        logic
        onewire_bus      
        ds18b20 
)