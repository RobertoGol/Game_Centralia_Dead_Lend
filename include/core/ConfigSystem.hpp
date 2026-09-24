#pragma once
#include <string>
#include <unordered_map>
#include <vector>

namespace Centralia {

class ConfigSystem {
private:
    std::unordered_map<std::string, std::string> m_settings;
    
    // Внутренние методы упаковки а-ля OGG-пакеты (сжатие заголовков строк)
    std::vector<uint8_t> PackToBinaryStream() const;
    bool UnpackFromBinaryStream(const std::vector<uint8_t>& stream);

public:
    ConfigSystem() = default;

    // Установка параметров
    void SetString(const std::string& key, const std::string& value);
    void SetInt(const std::string& key, int value);
    
    std::string GetString(const std::string& key, const std::string& defaultVal = "");
    int GetInt(const std::string& key, int defaultVal = 0);

    // Сохранение конфига в нашем зашифрованном бинарном формате
    bool SaveToFile(const std::string& filename);
    
    // Загрузка и расшифровка с диска
    bool LoadFromFile(const std::string& filename);
};

} // namespace Centralia
