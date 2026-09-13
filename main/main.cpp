#include "config.h"
#include "lcd.h"
#include "settings.h"
#include "sd_manager.h"
#include "wifi_manager.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

namespace { constexpr char TAG[] = "music_player"; }

extern "C" void app_main() {
    ESP_LOGI(TAG, "ESP32 Music Player 2.0 starting");
    const bool settings_ok = settings_init();
    const bool lcd_ok = lcd_init();
    const bool sd_ok = sd_init();
    const bool wifi_ok = wifi_init();

    if (!settings_ok) ESP_LOGE(TAG, "Settings initialization failed");
    if (!lcd_ok) ESP_LOGE(TAG, "LCD initialization failed");
    ESP_LOGI(TAG, "SD status: %s", sd_ok ? "mounted" : "not present");
    ESP_LOGI(TAG, "Wi-Fi startup: %s", wifi_ok ? "requested" : "not configured");

    if (lcd_ok) lcd_show_connect_code(PLAYER_CONNECT_CODE);

    while (true) {
        lcd_tick();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
