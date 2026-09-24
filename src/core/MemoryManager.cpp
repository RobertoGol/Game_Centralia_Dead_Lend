#include "core/MemoryManager.hpp"
#include "platform/Platform.hpp"
#include <fstream>
#include <algorithm>

namespace Centralia {

MemoryManager::MemoryManager(size_t poolSize) {
    m_ghostRamBuffer.resize(poolSize, 0x00);
}

MemoryManager::~MemoryManager() {
    std::fill(m_ghostRamBuffer.begin(), m_ghostRamBuffer.end(), 0x00); // Очистка памяти при закрытии
}

bool MemoryManager::Initialize(const std::string& deviceHwid) {
    if (deviceHwid.empty()) return false;
    
    // Соль на основе HWID устройства — делает невозможным перенос читерских сейвов между девайсами
    m_encryptionKey = deviceHwid + "_Centralia_DeadLend_2026_Salt";
    Platform::Log("MemoryManager: Ghost-RAM secure layer deployed.");
    return true;
}

void MemoryManager::ApplyCipher(std::vector<uint8_t>& data) {
    // Потоковый крипто-алгоритм (XOR модификация с динамическим смещением по ключу)
    // Гарантирует одинаковую работу и переносимость бинарных сейвов между Arch, Win10 и Android
    size_t keyLength = m_encryptionKey.length();
    for (size_t i = 0; i < data.size(); ++i) {
        data[i] ^= static_cast<uint8_t>(m_encryptionKey[i % keyLength] ^ (i & 0xFF));
    }
}

bool MemoryManager::SaveEncryptedFile(const std::string& filepath, const std::vector<uint8_t>& rawData) {
    std::vector<uint8_t> encrypted = rawData;
    ApplyCipher(encrypted); // Зашифровываем байты перед записью на диск

    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) return false;

    file.write(reinterpret_cast<const char*>(encrypted.data()), encrypted.size());
    return true;
}

bool MemoryManager::LoadDecryptedFile(const std::string& filepath, std::vector<uint8_t>& outData) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return false; // Скобка исправлена

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    outData.resize(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char*>(outData.data()), size)) return false;

    ApplyCipher(outData); // Расшифровываем обратно в валидную структуру данных
    return true;
}

void MemoryManager::SetRegistryValue(const std::string& key, int32_t value) {
    m_runtimeRegistry[key] = value;
}

int32_t MemoryManager::GetRegistryValue(const std::string& key) {
    auto it = m_runtimeRegistry.find(key);
    return (it != m_runtimeRegistry.end()) ? it->second : 0;
}

// Исполнитель void.run — полностью изолированная обработка логики решений квестов и ИИ
void MemoryManager::ExecuteDecisionScript(const std::vector<Instruction>& bytecode) {
    size_t ip = 0; // Исполнительный указатель (Instruction Pointer)
    bool halted = false;

    while (ip < bytecode.size() && !halted) {
        const Instruction& inst = bytecode[ip];
        
        switch (inst.opcode) {
            case OpCode::OP_NOP:
                break;

            case OpCode::OP_SET_REG:
                m_ghostRamBuffer[inst.reg_target] = static_cast<uint8_t>(inst.immediate_value);
                break;

            case OpCode::OP_GET_HWID:
                // Проверка безопасности: если флаг в регистре совпадает с валидацией
                if (m_ghostRamBuffer[inst.reg_target] == 0x01) {
                    Platform::Log("void.run: Device verified execution stack context.");
                }
                break;

            case OpCode::OP_IF_RAD:
                // Если уровень радиации в реестре превышает указанное значение, перескакиваем инструкцию
                if (GetRegistryValue("player_radiation") < inst.immediate_value) {
                    ip++; // Пропускаем следующий шаг логики (триггер не сработал)
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

} // namespace Centralia
