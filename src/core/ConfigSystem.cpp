#include "core/ConfigSystem.hpp"
#include "core/MemoryManager.hpp"
#include "core/NetworkProtocol.hpp"
#include "platform/Platform.hpp"

namespace Centralia {

void ConfigSystem::SetString(const std::string& key, const std::string& value) {
    m_settings[key] = value;
}

void ConfigSystem::SetInt(const std::string& key, int value) {
    m_settings[key] = std::to_string(value);
}

std::string ConfigSystem::GetString(const std::string& key, const std::string& defaultVal) {
    auto it = m_settings.find(key);
    return (it != m_settings.end()) ? it->second : defaultVal;
}

int ConfigSystem::GetInt(const std::string& key, int defaultVal) {
    auto it = m_settings.find(key);
    return (it != m_settings.end()) ? std::stoi(it->second) : defaultVal;
}

// Побайтовое сжатие и упаковка структуры словаря (как фреймы в ogg формате)
std::vector<uint8_t> ConfigSystem::PackToBinaryStream() const {
    std::vector<uint8_t> stream;
    
    // Записываем заголовок кастомного формата настроек (Сигнатура: 'C','F','G','X')
    stream.push_back('C'); stream.push_back('F'); stream.push_back('G'); stream.push_back('X');
    
    // Пишем количество записей
    NetworkSerializer::WriteUInt32(stream, static_cast<uint32_t>(m_settings.size()));
    
    for (const auto& [key, value] : m_settings) {
        NetworkSerializer::WriteString(stream, key);
        NetworkSerializer::WriteString(stream, value);
    }
    
    return stream;
}

bool ConfigSystem::UnpackFromBinaryStream(const std::vector<uint8_t>& stream) {
    if (stream.size() < 8) return false;
    
    // Проверяем сигнатуру заголовка фрейма
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

bool ConfigSystem::SaveToFile(const std::string& filename) {
    std::vector<uint8_t> rawStream = PackToBinaryStream();
    
    // Используем наш MemoryManager для наложения XOR-шифрования по HWID железа ПК
    // Конфиг, созданный на одной Windows 10, нельзя будет подделать на другом ПК
    MemoryManager crypto;
    crypto.Initialize(Platform::GetDeviceHWID());
    
    std::string fullPath = Platform::GetSaveDirectoryPath() + filename;
    return crypto.SaveEncryptedFile(fullPath, rawStream);
}

bool ConfigSystem::LoadFromFile(const std::string& filename) {
    MemoryManager crypto;
    crypto.Initialize(Platform::GetDeviceHWID());
    
    std::string fullPath = Platform::GetSaveDirectoryPath() + filename;
    std::vector<uint8_t> decryptedStream;
    
    if (!crypto.LoadDecryptedFile(fullPath, decryptedStream)) {
        return false;
    }
    
    return UnpackFromBinaryStream(decryptedStream);
}

} // namespace Centralia
