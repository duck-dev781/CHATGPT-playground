#pragma once

// Change this value for each physical ESP32 player.
#define PLAYER_CONNECT_CODE "X7K2Q9"

// 1602 I2C LCD
#define LCD_I2C_ADDRESS 0x27
#define LCD_SDA_GPIO 21
#define LCD_SCL_GPIO 22

// Optional SD card over the ESP32 SDMMC peripheral.
#define SD_MOUNT_POINT "/sdcard"

// SD layout
#define SD_BACKUP_DIR "/sdcard/BACKUP"
#define SD_SET_DIR "/sdcard/SET"
#define SD_SETTINGS_DIR "/sdcard/SET/REMP09/sys/files"

// Six settings profiles are retained.
#define MAX_SETTINGS_FILES 6

// Wi-Fi defaults. Leave SSID/password empty to boot without station setup.
#define WIFI_DEFAULT_SSID ""
#define WIFI_DEFAULT_PASSWORD ""

// LCD refresh interval.
#define LCD_REFRESH_MS 250
