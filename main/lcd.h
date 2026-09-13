#pragma once

#include <string>

bool lcd_init();
void lcd_set_status(const std::string& line1, const std::string& line2);
void lcd_show_connect_code(const char* code);
void lcd_show_sd(bool present);
void lcd_show_wifi(bool connected);
void lcd_tick();
