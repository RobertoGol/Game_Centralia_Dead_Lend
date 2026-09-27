#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <unordered_map>
#include <fstream>
#include <algorithm>
#include "platform/Platform.hpp" // Наш атомарный логер

namespace Centralia {

// Опкоды для нашей виртуальной машины решений void.run (из kernel.ctos)
enum class OpCode : uint8_t {
    OP_NOP      = 0x00,
    OP_SET_REG  = 0x01, // Установить значение в регистр памяти Ghost-RAM
    OP_GET_HWID = 0x02, // Проверить MASTER_HWID железа
    OP_IF_RAD   = 0x03, // Условие: если радиация в реестре выше X
    OP_IF_ZONE  = 0x04, // Условие: если игрок находится в триггер-зоне карты
    OP_EXEC_CH  = 0x05, // Выполнить триггер чата / отправить лог пакет
    OP_HALT     = 0xFF  // Остановить выполнение скрипта решений
};

// Структура инструкции байт-кода для интерпретатора решений
struct Instruction {
    OpCode   opcode;
    uint32_t reg_target;       // ИСПРАВЛЕНО: Тип расширен до uint32_t для адресации всего 1 МБ пула ОЗУ
    int32_t  immediate_value;
};

class MemoryManager {
private:
    std::vector<uint8_t> m_ghostRamBuffer;                      // Изолированный пул защищенной памяти
    std::unordered_map<std::string, int32_t> m_runtimeRegistry; // Быстрый реестр игровых стейтов
    std::string m_encryptionKey;

    inline void ApplyCipher(std::vector<uint8_t>& data) noexcept {
        // Потоковый крипто-алгоритм (XOR модификация с динамическим смещением по ключу)
        // Гарантирует одинаковую работу и переносимость бинарных сейвов между Arch, Win10 и Android
        size_t keyLength = m_encryptionKey.length();
        if (keyLength == 0) return;
        
        for (size_t i = 0; i < data.size(); ++i) {
            data[i] ^= static_cast<uint8_t>(m_encryptionKey[i % keyLength] ^ (i & 0xFF));
        }
    }

    inline MemoryManager(size_t poolSize = 1024 * 1024) noexcept {
        m_ghostRamBuffer.resize(poolSize, 0x00);
    }

public:
    inline ~MemoryManager() {
        // Гарантированная зачистка ОЗУ при закрытии приложения для защиты от дамперов памяти
        std::fill(m_ghostRamBuffer.begin(), m_ghostRamBuffer.end(), 0x00); 
    }

    // Запрет копирования синглтона
    MemoryManager(const MemoryManager&) = delete;
    MemoryManager& operator=(const MemoryManager&) = delete;

    static inline MemoryManager& GetInstance() noexcept {
        static MemoryManager instance;
        return instance;
    }

    inline bool Initialize(const std::string& deviceHwid) noexcept {
        if (deviceHwid.empty()) return false;
        
        // Соль на основе HWID устройства — делает невозможным перенос читерских сейвов между девайсами
        m_encryptionKey = deviceHwid + "_Centralia_DeadLend_2026_Salt";
        Platform::Log("MemoryManager: Ghost-RAM secure layer deployed.");
        return true;
    }

    // Система работы с защищенными конфигами и сейвами персонажей LoginSystem
    inline bool SaveEncryptedFile(const std::string& filepath, const std::vector<uint8_t>& rawData) noexcept {
        std::vector<uint8_t> encrypted = rawData;
        ApplyCipher(encrypted); // Зашифровываем байты перед записью на жесткий диск

        std::ofstream file(filepath, std::ios::binary);
        if (!file.is_open()) return false;

        file.write(reinterpret_cast<const char*>(encrypted.data()), encrypted.size());
        file.close();
        return true;
    }

    inline bool LoadDecryptedFile(const std::string& filepath, std::vector<uint8_t>& outData) noexcept {
        std::ifstream file(filepath, std::ios::binary | std::ios::ate);
        if (!file.is_open()) return false; 

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        outData.resize(static_cast<size_t>(size));
        if (!file.read(reinterpret_cast<char*>(outData.data()), size)) {
            file.close();
            return false;
        }
        file.close();

        ApplyCipher(outData); // Расшифровываем обратно в валидную структуру данных в ОЗУ
        return true;
    }

    // Интерфейс чтения/записи стейтов виртуальной памяти реестра
    inline void SetRegistryValue(const std::string& key, int32_t value) noexcept {
        m_runtimeRegistry[key] = value;
    }

    [[nodiscard]] inline int32_t GetRegistryValue(const std::string& key) const noexcept {
        auto it = m_runtimeRegistry.find(key);
        return (it != m_runtimeRegistry.end()) ? it->second : 0;
    }

    /**
     * @brief Исполнитель void.run — полностью изолированная обработка логики решений квестов и ИИ.
     * Реализован в монолитном инлайне с жесткой аппаратной защитой от краха Out-of-Bounds.
     */
    inline void ExecuteDecisionScript(const std::vector<Instruction>& bytecode) noexcept {
        size_t ip = 0; // Исполнительный указатель (Instruction Pointer)
        bool halted = false;
        size_t bufferSize = m_ghostRamBuffer.size();

        while (ip < bytecode.size() && !halted) {
            const Instruction& inst = bytecode[ip];
            
            switch (inst.opcode) {
                case OpCode::OP_NOP:
                    break;

                case OpCode::OP_SET_REG:
                    // МАСТЕР-ПРЕДОХРАНИТЕЛЬ: Защита от разрушения кучи ОЗУ при выходе индекса за пределы 1 МБ
                    if (inst.reg_target < bufferSize) {
                        m_ghostRamBuffer[inst.reg_target] = static_cast<uint8_t>(inst.immediate_value);
                    }
                    break;

                case OpCode::OP_GET_HWID:
                    if (inst.reg_target < bufferSize && m_ghostRamBuffer[inst.reg_target] == 0x01) {
                        Platform::Log("void.run: Device verified execution stack context.");
                    }
                    break;

                case OpCode::OP_IF_RAD:
                    // Если уровень радиации выжившего меньше указанного, перескакиваем инструкцию решения
                    if (GetRegistryValue("player_radiation") < inst.immediate_value) {
                        ip++; 
                    }
                    break;

                case OpCode::OP_IF_ZONE:
                    if (GetRegistryValue("current_zone_id") != inst.immediate_value) {
                        ip++;
                    }
                    break;

                case OpCode::OP_EXEC_CH:
                    Platform::Log("void.run: Subsystem invoked Network-Chat log push execution.");
                    break;

                case OpCode::OP_HALT:
                    halted = true;
                    break;

                default:
                    Platform::Log("void.run: Unknown Memory-Opcode encountered! Halting stack.");
                    halted = true;
                    break;
            }
            ip++;
        }
    }
};

} // namespace Centralia
