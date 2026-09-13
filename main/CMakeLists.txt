idf_component_register(
    SRCS
        "main.cpp"
        "config.cpp"
        "settings.cpp"
        "sd_manager.cpp"
        "lcd.cpp"
        "wifi_manager.cpp"
    INCLUDE_DIRS "."
    REQUIRES
        nvs_flash
        esp_wifi
        esp_event
        esp_netif
        driver
        fatfs
        sdmmc
)
