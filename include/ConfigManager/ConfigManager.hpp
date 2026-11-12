#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>

struct ConfigData {
    std::unordered_map<std::string, std::string> values;
};

class ConfigManager {
public:
    bool load(const std::filesystem::path& path);
    [[nodiscard]] int get_int(const std::string& key, int fallback) const;
    [[nodiscard]] std::string get_string(const std::string& key, std::string fallback) const;
    [[nodiscard]] const ConfigData& data() const { return config_data_; }
    [[nodiscard]] bool is_loaded() const { return loaded_; }

private:
    ConfigData config_data_;
    bool loaded_{};
};
