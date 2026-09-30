#include "ConfigSystem.hpp"
#include "platform/Platform.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <thread>
#include <vector>
#include <iomanip>

namespace Centralia {

// ============================================================================
// SECTION 1: STRING UTILITIES & CONSTANTS
// ============================================================================

namespace StringUtils {
    static inline void LTrim(std::string& s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    }

    static inline void RTrim(std::string& s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
    }

    static inline void Trim(std::string& s) {
        LTrim(s);
        RTrim(s);
    }

    static inline std::string ToLower(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return std::tolower(c); });
        return s;
    }

    static inline bool ParseBoolean(const std::string& str) {
        std::string lower = ToLower(str);
        return (lower == "true" || lower == "1" || lower == "yes" || lower == "on");
    }
}

namespace ConfigDefs {
    constexpr const char* DEFAULT_CONFIG_PATH = "config/centralia_engine.ini";
    constexpr const char* BACKUP_CONFIG_PATH = "config/centralia_engine.bak";
    constexpr uint32_t CONFIG_VERSION = 1042;
}

// ============================================================================
// SECTION 2: CONSOLE VARIABLE (CVAR) SYSTEM STRUCTURES
// ============================================================================

enum class CVarType {
    Int,
    Float,
    Bool,
    String
};

struct CVar {
    std::string name;
    std::string section;
    std::string description;
    CVarType type;
    
    // Текущее и дефолтное значения (хранятся как строки для универсальности)
    std::string stringValue;
    std::string defaultValue;

    // Границы для валидации чисел
    bool hasBounds;
    float minValue;
    float maxValue;

    // Флаги свойств
    bool isReadOnly;
    bool requiresRestart;
    bool isModified;

    // Кэшированные типизированные значения для быстрого доступа
    int cachedInt;
    float cachedFloat;
    bool cachedBool;

    void UpdateCache() {
        if (type == CVarType::Int) cachedInt = std::stoi(stringValue);
        else if (type == CVarType::Float) cachedFloat = std::stof(stringValue);
        else if (type == CVarType::Bool) cachedBool = StringUtils::ParseBoolean(stringValue);
    }

    bool ValidateAndSet(const std::string& newValue) {
        if (isReadOnly) return false;

        try {
            if (type == CVarType::Int || type == CVarType::Float) {
                float val = std::stof(newValue);
                if (hasBounds) {
                    val = std::clamp(val, minValue, maxValue);
                }
                if (type == CVarType::Int) {
                    stringValue = std::to_string(static_cast<int>(val));
                } else {
                    stringValue = std::to_string(val);
                }
            } else if (type == CVarType::Bool) {
                stringValue = StringUtils::ParseBoolean(newValue) ? "true" : "false";
            } else {
                stringValue = newValue;
            }
            
            UpdateCache();
            isModified = true;
            return true;
        } catch (...) {
            Platform::Log("[CVAR ERROR]: Ошибка конвертации значения '" + newValue + "' для CVar: " + name);
            return false;
        }
    }
};

// ============================================================================
// SECTION 3: SYSTEM CLASS DEFINITION & INTERNAL STATE
// ============================================================================

struct ConfigSystemImpl {
    std::unordered_map<std::string, CVar> cvars;
    std::vector<std::string> registeredSections;
    std::string activeConfigPath;
    bool isLoaded;

    void RegisterCVar(const std::string& name, const std::string& section, CVarType type, const std::string& defaultVal, const std::string& desc, bool bounds = false, float minV = 0.0f, float maxV = 0.0f, bool reqRestart = false) {
        std::string lowerName = StringUtils::ToLower(name);
        
        CVar cvar;
        cvar.name = lowerName;
        cvar.section = section;
        cvar.description = desc;
        cvar.type = type;
        cvar.defaultValue = defaultVal;
        cvar.stringValue = defaultVal;
        cvar.hasBounds = bounds;
        cvar.minValue = minV;
        cvar.maxValue = maxV;
        cvar.isReadOnly = false;
        cvar.requiresRestart = reqRestart;
        cvar.isModified = false;
        
        cvar.UpdateCache();

        cvars[lowerName] = cvar;

        // Добавляем секцию в список, если ее еще нет (для красивого сохранения)
        if (std::find(registeredSections.begin(), registeredSections.end(), section) == registeredSections.end()) {
            registeredSections.push_back(section);
        }
    }
};

ConfigSystem* ConfigSystem::s_instance = nullptr;

ConfigSystem::ConfigSystem() : m_pImpl(new ConfigSystemImpl()) {
    if (s_instance) {
        Platform::Log("[CONFIG FATAL]: Двойная инициализация ConfigSystem!");
        std::terminate();
    }
    s_instance = this;
    m_pImpl->isLoaded = false;
    m_pImpl->activeConfigPath = ConfigDefs::DEFAULT_CONFIG_PATH;

    InitializeDefaultCVars();
    Platform::Log("[CONFIG SYSTEM]: Менеджер CVar и конфигураций успешно инициализирован.");
}

ConfigSystem::~ConfigSystem() {
    SaveConfigToFile();
    delete m_pImpl;
    s_instance = nullptr;
    Platform::Log("[CONFIG SYSTEM]: Буферы конфигурации выгружены.");
}

ConfigSystem& ConfigSystem::GetInstance() {
    if (!s_instance) std::terminate();
    return *s_instance;
}

// ============================================================================
// SECTION 4: ENGINE CVAR REGISTRY (DEFAULT SETTINGS)
// ============================================================================

void ConfigSystem::InitializeDefaultCVars() {
    // --- VIDEO & DISPLAY SETTINGS ---
    m_pImpl->RegisterCVar("r_width", "Video", CVarType::Int, "1920", "Разрешение экрана по ширине", true, 800.0f, 7680.0f, true);
    m_pImpl->RegisterCVar("r_height", "Video", CVarType::Int, "1080", "Разрешение экрана по высоте", true, 600.0f, 4320.0f, true);
    m_pImpl->RegisterCVar("r_fullscreen", "Video", CVarType::Bool, "true", "Полноэкранный режим", false, 0.0f, 0.0f, true);
    m_pImpl->RegisterCVar("r_vsync", "Video", CVarType::Bool, "true", "Вертикальная синхронизация", false, 0.0f, 0.0f, false);
    m_pImpl->RegisterCVar("r_maxfps", "Video", CVarType::Int, "144", "Лимит частоты кадров (0 - без лимита)", true, 0.0f, 999.0f, false);
    m_pImpl->RegisterCVar("r_fov", "Video", CVarType::Float, "90.0", "Угол обзора камеры (Field of View)", true, 60.0f, 130.0f, false);

    // --- GRAPHICS QUALITY ---
    m_pImpl->RegisterCVar("g_shadow_quality", "Graphics", CVarType::Int, "3", "Качество теней (0-Low, 1-Med, 2-High, 3-Ultra)", true, 0.0f, 3.0f, false);
    m_pImpl->RegisterCVar("g_texture_lod", "Graphics", CVarType::Int, "0", "Смещение LOD текстур (0 - макс качество)", true, 0.0f, 4.0f, false);
    m_pImpl->RegisterCVar("g_antialiasing", "Graphics", CVarType::Int, "2", "Сглаживание (0-Off, 1-FXAA, 2-TAA, 3-DLSS)", true, 0.0f, 3.0f, false);
    m_pImpl->RegisterCVar("g_raytracing", "Graphics", CVarType::Bool, "false", "Аппаратная трассировка лучей", false, 0.0f, 0.0f, true);
    m_pImpl->RegisterCVar("g_volumetric_fog", "Graphics", CVarType::Bool, "true", "Объемный туман Пустоши", false, 0.0f, 0.0f, false);

    // --- AUDIO SETTINGS ---
    m_pImpl->RegisterCVar("a_master_volume", "Audio", CVarType::Float, "85.0", "Общая громкость", true, 0.0f, 100.0f, false);
    m_pImpl->RegisterCVar("a_music_volume", "Audio", CVarType::Float, "65.0", "Громкость музыки", true, 0.0f, 100.0f, false);
    m_pImpl->RegisterCVar("a_sfx_volume", "Audio", CVarType::Float, "100.0", "Громкость эффектов", true, 0.0f, 100.0f, false);
    m_pImpl->RegisterCVar("a_voice_volume", "Audio", CVarType::Float, "90.0", "Громкость диалогов NPC", true, 0.0f, 100.0f, false);
    m_pImpl->RegisterCVar("a_hrtf_spatial", "Audio", CVarType::Bool, "true", "Бинауральное 3D позиционирование звука", false, 0.0f, 0.0f, true);

    // --- NETWORK SETTINGS ---
    m_pImpl->RegisterCVar("net_port", "Network", CVarType::Int, "7777", "Порт для хостинга сервера", true, 1024.0f, 65535.0f, true);
    m_pImpl->RegisterCVar("net_tickrate", "Network", CVarType::Int, "60", "Частота обновления сервера (Tickrate)", true, 10.0f, 128.0f, true);
    m_pImpl->RegisterCVar("net_max_players", "Network", CVarType::Int, "64", "Максимальное количество игроков", true, 1.0f, 256.0f, true);

    // --- GAMEPLAY & ENGINE SETTINGS ---
    m_pImpl->RegisterCVar("sys_language", "Engine", CVarType::String, "ru_RU", "Язык локализации", false, 0.0f, 0.0f, true);
    m_pImpl->RegisterCVar("g_difficulty", "Gameplay", CVarType::Int, "2", "Сложность (0-Easy, 1-Normal, 2-Hardcore, 3-Survival)", true, 0.0f, 3.0f, false);
    m_pImpl->RegisterCVar("g_headbob", "Gameplay", CVarType::Float, "1.0", "Интенсивность покачивания камеры при ходьбе", true, 0.0f, 2.0f, false);

    Platform::Log("[CVAR REGISTRY]: Системные переменные зарегистрированы (Записей: " + std::to_string(m_pImpl->cvars.size()) + ").");
}

// ============================================================================
// SECTION 5: COMMAND LINE ARGUMENTS PARSING (CLI OVERRIDES)
// ============================================================================

void ConfigSystem::ParseCommandLine(int argc, char** argv) {
    if (argc <= 1) return;

    Platform::Log("[CONFIG CLI]: Парсинг аргументов командной строки...");
    int overrideCount = 0;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        // Поддержка флагов вида: -r_fullscreen 0 или +net_port 8080
        if ((arg[0] == '-' || arg[0] == '+') && arg.length() > 1) {
            std::string cvarName = StringUtils::ToLower(arg.substr(1));
            
            // Если есть следующий аргумент и он не начинается с '-' или '+' - это значение
            if (i + 1 < argc && argv[i + 1][0] != '-' && argv[i + 1][0] != '+') {
                std::string cvarValue = argv[i + 1];
                if (SetCVar(cvarName, cvarValue)) {
                    Platform::Log("[CONFIG CLI]: Переопределение CVar: " + cvarName + " = " + cvarValue);
                    overrideCount++;
                }
                i++; // Пропускаем обработанное значение
            } else {
                // Если значения нет, подразумеваем флаг-тумблер (true)
                if (SetCVar(cvarName, "true")) {
                    Platform::Log("[CONFIG CLI]: Активация флага: " + cvarName);
                    overrideCount++;
                }
            }
        }
    }

    if (overrideCount > 0) {
        Platform::Log("[CONFIG CLI]: Применено аргументов командной строки: " + std::to_string(overrideCount));
    }
}

// ============================================================================
// SECTION 6: HARDWARE AUTODETECTION & PRESET GENERATION
// ============================================================================

void ConfigSystem::AutoDetectHardwareSettings() {
    Platform::Log("[CONFIG AUTODETECT]: Сканирование аппаратного обеспечения для настройки профиля...");

    // Получаем количество логических ядер
    unsigned int hardwareConcurrency = std::thread::hardware_concurrency();
    if (hardwareConcurrency == 0) hardwareConcurrency = 4; // Fallback

    // Заглушки вызовов API ОС для видеопамяти и ОЗУ
    size_t systemRamMB = Platform::GetSystemRAM() / (1024 * 1024);
    size_t vramMB = Platform::GetSystemVRAM() / (1024 * 1024);

    Platform::Log("[HARDWARE INFO]: CPU Ядер: " + std::to_string(hardwareConcurrency) + 
                  " | ОЗУ: " + std::to_string(systemRamMB) + " MB" + 
                  " | VRAM: " + std::to_string(vramMB) + " MB");

    // Алгоритм выбора пресета графики
    if (hardwareConcurrency >= 12 && systemRamMB >= 16000 && vramMB >= 8000) {
        Platform::Log("[CONFIG PRESET]: Обнаружена мощная система. Применен профиль ULTRA.");
        SetCVar("g_shadow_quality", "3");
        SetCVar("g_antialiasing", "2");
        SetCVar("g_texture_lod", "0");
        SetCVar("g_raytracing", "true");
        SetCVar("r_maxfps", "144");
    } 
    else if (hardwareConcurrency >= 6 && systemRamMB >= 8000 && vramMB >= 4000) {
        Platform::Log("[CONFIG PRESET]: Обнаружена средняя система. Применен профиль HIGH/MEDIUM.");
        SetCVar("g_shadow_quality", "2");
        SetCVar("g_antialiasing", "1");
        SetCVar("g_texture_lod", "1");
        SetCVar("g_raytracing", "false");
        SetCVar("r_maxfps", "60");
    } 
    else {
        Platform::Log("[CONFIG PRESET]: Обнаружена слабая система. Применен профиль LOW (Максимальная производительность).");
        SetCVar("g_shadow_quality", "0");
        SetCVar("g_antialiasing", "0");
        SetCVar("g_texture_lod", "3"); // Мыльные текстуры для экономии VRAM
        SetCVar("g_volumetric_fog", "false");
        SetCVar("r_maxfps", "30");
    }

    // Применяем текущее разрешение монитора по умолчанию
    int screenW, screenH;
    Platform::GetDesktopResolution(screenW, screenH);
    SetCVar("r_width", std::to_string(screenW));
    SetCVar("r_height", std::to_string(screenH));
    SetCVar("r_fullscreen", "true");
}

// ============================================================================
// SECTION 7: INI FILE PARSER (READING)
// ============================================================================

bool ConfigSystem::LoadConfigFromFile(const std::string& filePath) {
    if (!filePath.empty()) {
        m_pImpl->activeConfigPath = filePath;
    }

    Platform::Log("[CONFIG LOAD]: Чтение файла настроек: " + m_pImpl->activeConfigPath);
    std::ifstream fileStream(m_pImpl->activeConfigPath);

    if (!fileStream.is_open()) {
        Platform::Log("[CONFIG WARNING]: Файл не найден. Запуск автоопределения железа и генерация нового файла.");
        AutoDetectHardwareSettings();
        SaveConfigToFile();
        m_pImpl->isLoaded = true;
        return true;
    }

    std::string currentLine;
    std::string currentSection = "Global";
    size_t parsedLines = 0;

    while (std::getline(fileStream, currentLine)) {
        StringUtils::Trim(currentLine);

        // Пропуск пустых строк и комментариев
        if (currentLine.empty() || currentLine[0] == '#' || currentLine[0] == ';') {
            continue;
        }

        // Парсинг секций [SectionName]
        if (currentLine.front() == '[' && currentLine.back() == ']') {
            currentSection = currentLine.substr(1, currentLine.length() - 2);
            StringUtils::Trim(currentSection);
            continue;
        }

        // Парсинг пар Ключ=Значение
        size_t delimiterPos = currentLine.find('=');
        if (delimiterPos != std::string::npos) {
            std::string key = currentLine.substr(0, delimiterPos);
            std::string value = currentLine.substr(delimiterPos + 1);

            StringUtils::Trim(key);
            StringUtils::Trim(value);
            
            // Очистка значения от инлайн-комментариев (например: value = 144 # Limit FPS)
            size_t commentPos = value.find_first_of("#;");
            if (commentPos != std::string::npos) {
                value = value.substr(0, commentPos);
                StringUtils::Trim(value);
            }

            // Установка значения в систему CVar
            if (HasCVar(key)) {
                SetCVar(key, value);
            } else {
                // Если переменной нет в реестре, создаем кастомную строковую переменную (для модов)
                m_pImpl->RegisterCVar(key, currentSection, CVarType::String, value, "Custom User Variable");
            }
            parsedLines++;
        }
    }

    fileStream.close();
    m_pImpl->isLoaded = true;
    Platform::Log("[CONFIG LOAD SUCCESS]: Успешно загружено " + std::to_string(parsedLines) + " параметров.");
    return true;
}

// ============================================================================
// SECTION 8: INI FILE WRITER (SAVING)
// ============================================================================

bool ConfigSystem::SaveConfigToFile() const {
    // Создание бэкапа перед перезаписью
    std::ifstream src(m_pImpl->activeConfigPath, std::ios::binary);
    std::ofstream dst(ConfigDefs::BACKUP_CONFIG_PATH, std::ios::binary);
    if (src && dst) {
        dst << src.rdbuf();
    }
    src.close();
    dst.close();

    std::ofstream outFile(m_pImpl->activeConfigPath, std::ios::trunc);
    if (!outFile.is_open()) {
        Platform::Log("[CONFIG SAVE ERROR]: Невозможно открыть файл для записи: " + m_pImpl->activeConfigPath);
        return false;
    }

    outFile << "# ====================================================================\n";
    outFile << "# CENTRALIA ENGINE - CONFIGURATION FILE (Version " << ConfigDefs::CONFIG_VERSION << ")\n";
    outFile << "# ВНИМАНИЕ: Ручное изменение параметров может привести к сбоям движка.\n";
    outFile << "# ====================================================================\n\n";

    // Группировка CVar по секциям для красивого форматирования
    for (const std::string& sectionName : m_pImpl->registeredSections) {
        bool sectionHasVars = false;

        // Предварительная проверка, есть ли переменные в этой секции
        for (const auto& [key, cvar] : m_pImpl->cvars) {
            if (cvar.section == sectionName) {
                sectionHasVars = true;
                break;
            }
        }

        if (!sectionHasVars) continue;

        outFile << "[" << sectionName << "]\n";

        for (auto& [key, cvar] : m_pImpl->cvars) {
            if (cvar.section == sectionName) {
                // Выравнивание ключей для читаемости
                outFile << std::left << std::setw(25) << cvar.name 
                        << "= " << std::setw(15) << cvar.stringValue;
                
                // Добавление комментария-описания
                if (!cvar.description.empty()) {
                    outFile << " ; " << cvar.description;
                    if (cvar.requiresRestart) outFile << " (Requires Restart)";
                }
                outFile << "\n";
            }
        }
        outFile << "\n";
    }

    outFile.close();
    Platform::Log("[CONFIG SAVE]: Конфигурация успешно сохранена на диск.");
    return true;
}

// ============================================================================
// SECTION 9: HOT-RELOAD & CALLBACK MECHANISMS
// ============================================================================

void ConfigSystem::ReloadConfig() {
    Platform::Log("[CONFIG HOT-RELOAD]: Инициация горячей перезагрузки конфигурации...");
    LoadConfigFromFile(m_pImpl->activeConfigPath);

    // В реальном движке здесь вызываются Event Dispatchers, уведомляющие подсистемы об изменениях.
    // Например, если r_width или r_height изменились - вызываем функцию пересоздания SwapChain.
    
    // Проверка, изменилось ли разрешение экрана
    CVar& wVar = m_pImpl->cvars["r_width"];
    CVar& hVar = m_pImpl->cvars["r_height"];
    CVar& fsVar = m_pImpl->cvars["r_fullscreen"];

    if (wVar.isModified || hVar.isModified || fsVar.isModified) {
        Platform::Log("[CONFIG HOT-RELOAD]: Обнаружено изменение настроек дисплея. Применение...");
        Platform::UpdateWindowResolution(wVar.cachedInt, hVar.cachedInt, fsVar.cachedBool);
        
        wVar.isModified = false;
        hVar.isModified = false;
        fsVar.isModified = false;
    }

    // Проверка громкости звука
    CVar& masterVol = m_pImpl->cvars["a_master_volume"];
    if (masterVol.isModified) {
        Platform::SetAudioMasterVolume(masterVol.cachedFloat);
        masterVol.isModified = false;
    }

    Platform::Log("[CONFIG HOT-RELOAD]: Перезагрузка завершена. Изменения применены (кроме требующих рестарта).");
}

// ============================================================================
// SECTION 10: GETTERS, SETTERS & PUBLIC CVAR API
// ============================================================================

bool ConfigSystem::HasCVar(const std::string& name) const noexcept {
    return m_pImpl->cvars.find(StringUtils::ToLower(name)) != m_pImpl->cvars.end();
}

bool ConfigSystem::SetCVar(const std::string& name, const std::string& value) {
    std::string lowerName = StringUtils::ToLower(name);
    auto it = m_pImpl->cvars.find(lowerName);
    
    if (it == m_pImpl->cvars.end()) {
        Platform::Log("[CVAR WARNING]: Попытка записи в неизвестную переменную: " + lowerName);
        return false;
    }

    return it->second.ValidateAndSet(value);
}

int ConfigSystem::GetInt(const std::string& name) const {
    std::string lowerName = StringUtils::ToLower(name);
    auto it = m_pImpl->cvars.find(lowerName);
    if (it != m_pImpl->cvars.end()) {
        return it->second.cachedInt;
    }
    return 0; // Fallback
}

float ConfigSystem::GetFloat(const std::string& name) const {
    std::string lowerName = StringUtils::ToLower(name);
    auto it = m_pImpl->cvars.find(lowerName);
    if (it != m_pImpl->cvars.end()) {
        return it->second.cachedFloat;
    }
    return 0.0f;
}

bool ConfigSystem::GetBool(const std::string& name) const {
    std::string lowerName = StringUtils::ToLower(name);
    auto it = m_pImpl->cvars.find(lowerName);
    if (it != m_pImpl->cvars.end()) {
        return it->second.cachedBool;
    }
    return false;
}

std::string ConfigSystem::GetString(const std::string& name) const {
    std::string lowerName = StringUtils::ToLower(name);
    auto it = m_pImpl->cvars.find(lowerName);
    if (it != m_pImpl->cvars.end()) {
        return it->second.stringValue;
    }
    return "";
}

// ============================================================================
// SECTION 11: SPECIFIC ENGINE SUBSYSTEM HELPERS
// ============================================================================

void ConfigSystem::GetResolution(int& outWidth, int& outHeight, bool& outFullscreen) const noexcept {
    outWidth = GetInt("r_width");
    outHeight = GetInt("r_height");
    outFullscreen = GetBool("r_fullscreen");
}

uint32_t ConfigSystem::GetTargetFramerate() const noexcept {
    return static_cast<uint32_t>(GetInt("r_maxfps"));
}

float ConfigSystem::GetMasterVolume() const noexcept {
    return GetFloat("a_master_volume");
}

// ============================================================================
// SECTION 12: BINARY CACHING (FAST BOOT SERIALIZATION)
// ============================================================================
// Вместо парсинга INI-файла при каждом запуске, движок может сбрасывать 
// хэш-таблицу CVar в бинарный кэш для сверхбыстрой загрузки (Fast Boot).

std::vector<uint8_t> ConfigSystem::SerializeToBinary() const {
    std::vector<uint8_t> buffer;
    buffer.reserve(4096);

    uint32_t cvarCount = static_cast<uint32_t>(m_pImpl->cvars.size());
    const uint8_t* cPtr = reinterpret_cast<const uint8_t*>(&cvarCount);
    buffer.insert(buffer.end(), cPtr, cPtr + sizeof(uint32_t));

    for (const auto& [key, cvar] : m_pImpl->cvars) {
        // Упаковка ключа
        uint32_t keyLen = static_cast<uint32_t>(key.length());
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&keyLen), reinterpret_cast<const uint8_t*>(&keyLen) + 4);
        buffer.insert(buffer.end(), key.begin(), key.end());

        // Упаковка значения
        uint32_t valLen = static_cast<uint32_t>(cvar.stringValue.length());
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&valLen), reinterpret_cast<const uint8_t*>(&valLen) + 4);
        buffer.insert(buffer.end(), cvar.stringValue.begin(), cvar.stringValue.end());
    }

    Platform::Log("[CONFIG CACHE]: Система CVar упакована в бинарный кэш (" + std::to_string(buffer.size()) + " байт).");
    return buffer;
}

} // namespace Centralia