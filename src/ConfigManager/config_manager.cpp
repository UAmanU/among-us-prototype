#include "ConfigManager/ConfigManager.hpp"

#include <charconv>
#include <fstream>
#include <iterator>

bool ConfigManager::load(const std::filesystem::path& path) {
    if (loaded_) return true;
    std::ifstream input(path);
    if (!input) return false;
    const std::string json{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    std::size_t cursor = 0;
    while ((cursor = json.find('"', cursor)) != std::string::npos) {
        const auto key_end = json.find('"', cursor + 1);
        if (key_end == std::string::npos) return false;
        const std::string key = json.substr(cursor + 1, key_end - cursor - 1);
        auto colon = json.find(':', key_end + 1);
        if (colon == std::string::npos) return false;
        auto value_start = json.find_first_not_of(" \t\r\n", colon + 1);
        if (value_start == std::string::npos) return false;
        std::size_t value_end = value_start;
        if (json[value_start] == '"') {
            value_end = json.find('"', value_start + 1);
            if (value_end == std::string::npos) return false;
            config_data_.values[key] = json.substr(value_start + 1, value_end - value_start - 1);
            cursor = value_end + 1;
        } else {
            value_end = json.find_first_of(",}\r\n \t", value_start);
            if (value_end == std::string::npos) value_end = json.size();
            config_data_.values[key] = json.substr(value_start, value_end - value_start);
            cursor = value_end;
        }
    }
    loaded_ = true;
    return true;
}

int ConfigManager::get_int(const std::string& key, int fallback) const {
    const auto it = config_data_.values.find(key);
    if (it == config_data_.values.end()) return fallback;
    int value{};
    const auto result = std::from_chars(it->second.data(), it->second.data() + it->second.size(), value);
    return result.ec == std::errc{} ? value : fallback;
}

std::string ConfigManager::get_string(const std::string& key, std::string fallback) const {
    const auto it = config_data_.values.find(key);
    return it == config_data_.values.end() ? std::move(fallback) : it->second;
}
