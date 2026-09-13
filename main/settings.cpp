#include "settings.h"

#include "nvs.h"
#include "nvs_flash.h"

namespace {
constexpr const char* kNamespace = "player";
constexpr const char* kFactoryNamespace = "factory";
PlayerSettings current;
PlayerSettings factory_defaults;

bool load_from(const char* ns, PlayerSettings& out) {
    nvs_handle_t h{};
    if (nvs_open(ns, NVS_READONLY, &h) != ESP_OK) return false;

    char name[96];
    size_t name_len = sizeof(name);
    int32_t volume = 50;
    uint8_t shuffle = 0;
    uint8_t repeat = 0;

    esp_err_t e = nvs_get_str(h, "name", name, &name_len);
    if (e == ESP_OK) out.player_name = name;
    nvs_get_i32(h, "volume", &volume);
    nvs_get_u8(h, "shuffle", &shuffle);
    nvs_get_u8(h, "repeat", &repeat);
    nvs_close(h);

    out.volume = volume < 0 ? 0 : (volume > 100 ? 100 : volume);
    out.shuffle = shuffle != 0;
    out.repeat = repeat != 0;
    return e == ESP_OK;
}

bool save_to(const char* ns, const PlayerSettings& s) {
    nvs_handle_t h{};
    if (nvs_open(ns, NVS_READWRITE, &h) != ESP_OK) return false;
    esp_err_t e = nvs_set_str(h, "name", s.player_name.c_str());
    if (e == ESP_OK) e = nvs_set_i32(h, "volume", s.volume);
    if (e == ESP_OK) e = nvs_set_u8(h, "shuffle", s.shuffle ? 1 : 0);
    if (e == ESP_OK) e = nvs_set_u8(h, "repeat", s.repeat ? 1 : 0);
    if (e == ESP_OK) e = nvs_commit(h);
    nvs_close(h);
    return e == ESP_OK;
}
}

bool settings_init() {
    if (nvs_flash_init() == ESP_ERR_NVS_NO_FREE_PAGES ||
        nvs_flash_init() == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        return false;
    }
    load_from(kFactoryNamespace, factory_defaults);
    load_from(kNamespace, current);
    if (current.player_name.empty()) current = factory_defaults;
    return true;
}

const PlayerSettings& settings_current() { return current; }
const PlayerSettings& settings_factory_defaults() { return factory_defaults; }

bool settings_set(const PlayerSettings& value) {
    current = value;
    if (current.volume < 0) current.volume = 0;
    if (current.volume > 100) current.volume = 100;
    return save_to(kNamespace, current);
}

bool settings_save_factory_defaults() {
    factory_defaults = current;
    return save_to(kFactoryNamespace, factory_defaults);
}

bool settings_factory_reset() {
    current = factory_defaults;
    return save_to(kNamespace, current);
}
