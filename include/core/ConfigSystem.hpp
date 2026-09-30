#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include "core/MemoryManager.hpp"
#include "core/NetworkProtocol.hpp"
#include "platform/Platform.hpp"

namespace Centralia {

class ConfigSystem {
private:
    std::unordered_map<std::string, std::string> m_settings;
    
    std::vector<uint8_t> PackToBinaryStream() const;
    bool UnpackFromBinaryStream(const std::vector<uint8_t>& stream);

public:
    ConfigSystem() = default;
    ~ConfigSystem() = default;

    ConfigSystem(const ConfigSystem&) = delete;
    ConfigSystem& operator=(const ConfigSystem&) = delete;

    void SetString(const std::string& key, const std::string& value);
    void SetInt(const std::string& key, int value);
    std::string GetString(const std::string& key, const std::string& defaultVal = "");
    int GetInt(const std::string& key, int defaultVal = 0);
    
    bool SaveToFile(const std::string& filename);
    bool LoadFromFile(const std::string& filename);
};

} // namespace Centralia