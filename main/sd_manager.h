#pragma once

#include <string>
#include <vector>

bool sd_init();
bool sd_present();
bool sd_ensure_structure();
bool sd_factory_reset_to_flash_defaults();
bool sd_unmount();
std::vector<std::string> sd_list_root();
std::vector<std::string> sd_list_settings_files();
bool sd_is_settings_file(const std::string& path);
std::string sd_settings_name(const std::string& path);
std::string sd_settings_date(const std::string& path);
