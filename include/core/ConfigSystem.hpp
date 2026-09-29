#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include "core/MemoryManager.hpp"     
#include "core/NetworkProtocol.hpp"  // FIXED: Patched path reference matching rules to target your valid file layout 
#include "platform/Platform.hpp"       

namespace Centralia {

class ConfigSystem {
private:
    std::unordered_map<std::string, std::string> m_settings;
    
class ConfigSystem {
private:
    std::unordered_map<std::string, std::string> m_settings;
    std::vector<uint8_t> PackToBinaryStream() const noexcept;
    bool UnpackFromBinaryStream(const std::vector<uint8_t>& stream) noexcept;

public:
    ConfigSystem() = default;
    ~ConfigSystem() = default;
    ConfigSystem(const ConfigSystem&) = delete;
    ConfigSystem& operator=(const ConfigSystem&) = delete;

    void SetString(const std::string& key, const std::string& value);
    void SetInt(const std::string& key, int value);
    std::string GetString(const std::string& key, const std::string& defaultVal = "");
    int GetInt(const std::string& key, int defaultVal = 0);
    bool SaveToFile(const std::string& filename) noexcept;
    bool LoadFromFile(const std::string& filename) noexcept;
};

    inline bool UnpackFromBinaryStream(const std::vector<uint8_t>& stream) noexcept {
        if (stream.size() < 8) return false;
        if (stream[0] != 'C' || stream[1] != 'F' || stream[2] != 'G' || stream[3] != 'X') {
            return false;
        }
        
        size_t offset = 4;
        uint32_t size = NetworkSerializer::ReadUInt32(stream, offset);
        
        m_settings.clear();
        for (uint32_t i = 0; i < size; ++i) {
            if (offset >= stream.size()) return false;
            std::string key = NetworkSerializer::ReadString(stream, offset);
            std::string value = NetworkSerializer::ReadString(stream, offset);
            m_settings[key] = value;
        }
        return true;
    }

public:
    ConfigSystem() = default;
    ~ConfigSystem() = default;

    ConfigSystem(const ConfigSystem&) = delete;
    ConfigSystem& operator=(const ConfigSystem&) = delete;

    inline void SetString(const std::string& key, const std::string& value) {
        m_settings[key] = value;
    }

    inline void SetInt(const std::string& key, int value) {
        m_settings[key] = std::to_string(value);
    }
    
    inline std::string GetString(const std::string& key, const std::string& defaultVal = "") {
        auto it = m_settings.find(key);
        return (it != m_settings.end()) ? it->second : defaultVal;
    }

    inline int GetInt(const std::string& key, int defaultVal = 0) {
        auto it = m_settings.find(key);
        try {
            return (it != m_settings.end()) ? std::stoi(it->second) : defaultVal;
        } catch (...) {
            return defaultVal; 
        }
    }

    inline bool SaveToFile(const std::string& filename) noexcept {
        std::vector<uint8_t> rawStream = PackToBinaryStream();
        MemoryManager& crypto = MemoryManager::GetInstance();
        crypto.Initialize(Platform::GetDeviceHWID());
        
        std::string fullPath = Platform::GetSaveDirectoryPath() + filename;
        return crypto.SaveEncryptedFile(fullPath, rawStream);
    }
    
    inline bool LoadFromFile(const std::string& filename) noexcept {
        MemoryManager& crypto = MemoryManager::GetInstance();
        crypto.Initialize(Platform::GetDeviceHWID());
        
        std::string fullPath = Platform::GetSaveDirectoryPath() + filename;
        std::vector<uint8_t> decryptedStream;
        
        if (!crypto.LoadDecryptedFile(fullPath, decryptedStream)) {
            return false;
        }
        return UnpackFromBinaryStream(decryptedStream);
    }
};

} // namespace Centralia
