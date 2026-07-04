idf_component_register(
    SRCS
        "main.c"
        "esp_hid_gap.c"
    INCLUDE_DIRS "."
    REQUIRES esp_hid bt nvs_flash esp_driver_gpio esp_driver_spi esp_lcd esp_timer spi_flash lvgl
)
