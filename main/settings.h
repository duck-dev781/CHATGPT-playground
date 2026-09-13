#pragma once

#include <string>

struct PlayerSettings {
    std::string player_name = "ESP32 Music Player";
    int volume = 50;
    bool shuffle = false;
    bool repeat = false;
};

bool settings_init();
const PlayerSettings& settings_current();
const PlayerSettings& settings_factory_defaults();
bool settings_set(const PlayerSettings& value);
bool settings_save_factory_defaults();
bool settings_factory_reset();
