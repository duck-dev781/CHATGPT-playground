#include "lcd.h"
#include "config.h"

#include "driver/i2c.h"
#include "esp_log.h"
#include <algorithm>
#include <cstring>

namespace {
constexpr char TAG[] = "lcd";
constexpr uint8_t LCD_ADDR = LCD_I2C_ADDRESS;
uint8_t backlight = 0x08;
uint8_t display[32] = {};
uint32_t last_scroll = 0;
std::string scroll_text;
size_t scroll_pos = 0;

void expander_write(uint8_t value) {
    i2c_master_write_to_device(I2C_NUM_0, LCD_ADDR, &value, 1, pdMS_TO_TICKS(100));
}

void pulse(uint8_t value) {
    expander_write(value | 0x04 | backlight);
    esp_rom_delay_us(1);
    expander_write((value & ~0x04) | backlight);
    esp_rom_delay_us(50);
}

void send4(uint8_t value) {
    pulse(value & 0xF0);
}

void send(uint8_t value, bool data) {
    uint8_t high = value & 0xF0;
    uint8_t low = (value << 4) & 0xF0;
    pulse(high | (data ? 0x01 : 0));
    pulse(low | (data ? 0x01 : 0));
}

void command(uint8_t c) { send(c, false); }
void write_char(char c) { send(static_cast<uint8_t>(c), true); }
void write_text(const std::string& s) { for (char c : s) write_char(c); }
void set_cursor(uint8_t row, uint8_t col) { command(0x80 | (row ? 0x40 : 0) | col); }
}

bool lcd_init() {
    i2c_config_t cfg{};
    cfg.mode = I2C_MODE_MASTER;
    cfg.sda_io_num = static_cast<gpio_num_t>(LCD_SDA_GPIO);
    cfg.scl_io_num = static_cast<gpio_num_t>(LCD_SCL_GPIO);
    cfg.sda_pullup_en = GPIO_PULLUP_ENABLE;
    cfg.scl_pullup_en = GPIO_PULLUP_ENABLE;
    cfg.master.clk_speed = 100000;
    if (i2c_param_config(I2C_NUM_0, &cfg) != ESP_OK) return false;
    if (i2c_driver_install(I2C_NUM_0, cfg.mode, 0, 0, 0) != ESP_OK) return false;

    vTaskDelay(pdMS_TO_TICKS(50));
    send4(0x30); vTaskDelay(pdMS_TO_TICKS(5));
    send4(0x30); vTaskDelay(pdMS_TO_TICKS(1));
    send4(0x30); vTaskDelay(pdMS_TO_TICKS(1));
    send4(0x20);
    command(0x28); command(0x0C); command(0x06); command(0x01);
    vTaskDelay(pdMS_TO_TICKS(2));
    lcd_set_status("ESP32 Music Player", "Ready");
    ESP_LOGI(TAG, "LCD ready");
    return true;
}

void lcd_set_status(const std::string& line1, const std::string& line2) {
    std::string a = line1.substr(0, 16);
    std::string b = line2.substr(0, 16);
    a.resize(16, ' ');
    b.resize(16, ' ');
    set_cursor(0, 0); write_text(a);
    set_cursor(1, 0); write_text(b);
}

void lcd_show_connect_code(const char* code) {
    std::string c = code ? code : "------";
    lcd_set_status("Connect code:", c.substr(0, 6));
}

void lcd_show_sd(bool present) {
    lcd_set_status(present ? "SD: connected" : "SD: - - -", "Ready");
}

void lcd_show_wifi(bool connected) {
    lcd_set_status(connected ? "WiFi: connected" : "WiFi: offline", "Ready");
}

void lcd_tick() {
    // Reserved for the future rotating disc / smooth title animation.
}
