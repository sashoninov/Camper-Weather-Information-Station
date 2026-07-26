idf_component_register(
    SRCS
        "bme680.c"

    INCLUDE_DIRS
        "."

    REQUIRES
        driver
        esp_driver_i2c
        core
        i2c_bus1
        bme68x_sensorapi_espidf
)