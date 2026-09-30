#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <unordered_map>
#include "platform/Platform.hpp"

namespace Centralia {

enum class OpCode : uint8_t {
    OP_NOP      = 0x00,
    OP_SET_REG  = 0x01,
    OP_GET_HWID = 0x02,
    OP_IF_RAD   = 0x03,
    OP_IF_ZONE  = 0x04,
    OP_EXEC_CH  = 0x05,
    OP_HALT     = 0xFF
};

struct Instruction {
    OpCode   opcode;
    uint32_t reg_target;
    int32_t  immediate_value;
};

class MemoryManager {
private:
    std::vector<uint8_t> m_ghostRamBuffer;
    std::unordered_map<std::string, int32_t> m_runtimeRegistry;
    std::string m_encryptionKey;

    void ApplyCipher(std::vector<uint8_t>& data);
    MemoryManager(size_t poolSize = 1024 * 1024);

public:
    ~MemoryManager();

    MemoryManager(const MemoryManager&) = delete;
    MemoryManager& operator=(const MemoryManager&) = delete;

    static MemoryManager& GetInstance() {
        static MemoryManager instance;
        return instance;
    }

    bool Initialize(const std::string& deviceHwid);
    bool SaveEncryptedFile(const std::string& filepath, const std::vector<uint8_t>& rawData);
    bool LoadDecryptedFile(const std::string& filepath, std::vector<uint8_t>& outData);
    void SetRegistryValue(const std::string& key, int32_t value);
    [[nodiscard]] int32_t GetRegistryValue(const std::string& key) const;
    void ExecuteDecisionScript(const std::vector<Instruction>& bytecode);
};

} // namespace Centralia