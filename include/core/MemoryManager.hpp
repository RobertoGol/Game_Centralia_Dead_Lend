#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <unordered_map>

namespace Centralia {

// Опкоды для нашей виртуальной машины решений void.run (из kernel.ctos)
enum class OpCode : uint8_t {
    OP_NOP      = 0x00,
    OP_SET_REG  = 0x01, // Установить значение в регистр памяти
    OP_GET_HWID = 0x02, // Проверить MASTER_HWID
    OP_IF_RAD   = 0x03, // Условие: если радиация выше X
    OP_IF_ZONE  = 0x04, // Условие: если игрок в триггер-зоне
    OP_EXEC_CH  = 0x05, // Выполнить триггер чата / отправить лог пакет
    OP_HALT     = 0xFF  // Остановить выполнение скрипта
};

// Структура инструкции байт-кода для интерпретатора решений
struct Instruction {
    OpCode opcode;
    uint8_t reg_target;
    int32_t immediate_value;
};

class MemoryManager {
private:
    std::vector<uint8_t> m_ghostRamBuffer; // Изолированный пул защищенной памяти
    std::unordered_map<std::string, int32_t> m_runtimeRegistry; // Быстрый регистр игровых стейтов
    std::string m_encryptionKey;

    void ApplyCipher(std::vector<uint8_t>& data);

public:
    MemoryManager(size_t poolSize = 1024 * 1024); // Выделяем фиксированный 1 МБ под стейты мира
    ~MemoryManager();

    bool Initialize(const std::string& deviceHwid);

    // Система работы с защищенными конфигами и сейвами
    bool SaveEncryptedFile(const std::string& filepath, const std::vector<uint8_t>& rawData);
    bool LoadDecryptedFile(const std::string& filepath, std::vector<uint8_t>& outData);

    // Ядро интерпретатора скриптов void.run
    void ExecuteDecisionScript(const std::vector<Instruction>& bytecode);
    
    // Интерфейс чтения/записи стейтов виртуальной памяти
    void SetRegistryValue(const std::string& key, int32_t value);
    int32_t GetRegistryValue(const std::string& key);
};

} // namespace Centralia
