file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/../games/*/main.cpp"
)

idf_component_register(
    SRCS
        "main.cpp"
        "config.cpp"
        "settings.cpp"
        "sd_manager.cpp"
        "lcd.cpp"
        "wifi_manager.cpp"
        "ble_manager.cpp"
        "game_manager.cpp"
        ${GAME_SOURCES}
    INCLUDE_DIRS "." "../games"
    REQUIRES
        nvs_flash
        esp_wifi
        esp_event
        esp_netif
        driver
        fatfs
        sdmmc
        bt
)