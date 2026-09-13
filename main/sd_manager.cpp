#include "sd_manager.h"
#include "config.h"

#include "driver/sdmmc_host.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

namespace {
sdmmc_card_t* card = nullptr;
bool mounted = false;
constexpr char TAG[] = "sd";

bool is_directory(const std::string& path) {
    struct stat st{};
    return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

bool move_without_overwrite(const std::string& source, const std::string& backup_dir) {
    const size_t slash = source.find_last_of('/');
    const std::string base = slash == std::string::npos ? source : source.substr(slash + 1);
    std::string destination = backup_dir + "/" + base;
    for (int i = 1; access(destination.c_str(), F_OK) == 0; ++i) {
        destination = backup_dir + "/" + base + "." + std::to_string(i);
    }
    return rename(source.c_str(), destination.c_str()) == 0;
}

std::string read_file(const std::string& path) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return {};
    std::string data;
    char buffer[256];
    while (size_t n = fread(buffer, 1, sizeof(buffer), f)) data.append(buffer, n);
    fclose(f);
    return data;
}

std::string field_after(const std::string& data, const std::string& token) {
    const size_t p = data.find(token);
    if (p == std::string::npos) return {};
    size_t start = p + token.size();
    if (start < data.size() && (data[start] == '|' || data[start] == ':' || data[start] == '=')) ++start;
    size_t end = data.find_first_of("|}\]\r\n", start);
    if (end == std::string::npos) end = data.size();
    return data.substr(start, end - start);
}
}

bool sd_init() {
    if (mounted) return true;
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    esp_vfs_fat_sdmmc_mount_config_t mount_config{};
    mount_config.format_if_mount_failed = false;
    mount_config.max_files = 12;
    mount_config.allocation_unit_size = 16 * 1024;

    esp_err_t e = esp_vfs_fat_sdmmc_mount(SD_MOUNT_POINT, &host, &slot_config, &mount_config, &card);
    if (e != ESP_OK) {
        card = nullptr;
        return false;
    }
    mounted = true;
    sd_ensure_structure();
    ESP_LOGI(TAG, "SD mounted");
    return true;
}

bool sd_present() { return mounted && card != nullptr; }

bool sd_ensure_structure() {
    if (!sd_present()) return false;
    mkdir(SD_BACKUP_DIR, 0775);
    mkdir(SD_SET_DIR, 0775);
    mkdir(SD_SETTINGS_DIR, 0775);
    return is_directory(SD_SETTINGS_DIR);
}

bool sd_unmount() {
    if (!mounted) return true;
    esp_err_t e = esp_vfs_fat_sdcard_unmount(SD_MOUNT_POINT, card);
    card = nullptr;
    mounted = false;
    return e == ESP_OK;
}

std::vector<std::string> sd_list_root() {
    std::vector<std::string> result;
    if (!sd_present()) return result;
    DIR* dir = opendir(SD_MOUNT_POINT);
    if (!dir) return result;
    while (dirent* entry = readdir(dir)) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        result.emplace_back(entry->d_name);
    }
    closedir(dir);
    std::sort(result.begin(), result.end());
    return result;
}

std::vector<std::string> sd_list_settings_files() {
    std::vector<std::string> result;
    if (!sd_present()) return result;
    DIR* dir = opendir(SD_SETTINGS_DIR);
    if (!dir) return result;
    while (dirent* entry = readdir(dir)) {
        std::string name = entry->d_name;
        if (name.size() > 2 && name.substr(name.size() - 2) == ".h") result.push_back(SD_SETTINGS_DIR + "/" + name);
    }
    closedir(dir);
    std::sort(result.begin(), result.end());
    return result;
}

bool sd_is_settings_file(const std::string& path) {
    if (path.size() < 2 || path.substr(path.size() - 2) != ".h") return false;
    const std::string data = read_file(path);
    // Settings verification requires the custom header/signature and RESET.fac marker.
    return data.rfind("ch|", 0) == 0 && data.find("NAME") != std::string::npos && data.find("DATE") != std::string::npos && data.find("RESET.fac") != std::string::npos;
}

std::string sd_settings_name(const std::string& path) {
    return field_after(read_file(path), "NAME");
}

std::string sd_settings_date(const std::string& path) {
    return field_after(read_file(path), "DATE");
}

bool sd_factory_reset_to_flash_defaults() {
    if (!sd_present()) return false;
    if (!sd_ensure_structure()) return false;

    // Only files directly in ROOT are moved. Other root directories are untouched.
    DIR* root = opendir(SD_MOUNT_POINT);
    if (!root) return false;
    std::vector<std::string> root_files;
    while (dirent* entry = readdir(root)) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        std::string path = SD_MOUNT_POINT + std::string("/") + entry->d_name;
        if (!is_directory(path) && strcmp(entry->d_name, "BACKUP") != 0) root_files.push_back(path);
    }
    closedir(root);
    for (const auto& path : root_files) move_without_overwrite(path, SD_BACKUP_DIR);

    // SET itself is moved as one directory so its contents remain intact in BACKUP.
    const std::string set_backup = std::string(SD_BACKUP_DIR) + "/SET";
    if (access(set_backup.c_str(), F_OK) == 0) {
        for (int i = 1; ; ++i) {
            std::string candidate = set_backup + "." + std::to_string(i);
            if (access(candidate.c_str(), F_OK) != 0) {
                if (rename(SD_SET_DIR, candidate.c_str()) == 0) break;
                return false;
            }
        }
    } else if (rename(SD_SET_DIR, set_backup.c_str()) != 0) {
        return false;
    }
    return sd_ensure_structure();
}
