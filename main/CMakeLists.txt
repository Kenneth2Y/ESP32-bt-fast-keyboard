idf_component_register(
    SRCS
        "main.c"
        "esp_hid_gap.c"
    INCLUDE_DIRS "."
    REQUIRES esp_hid bt nvs_flash esp_driver_gpio spi_flash
)
