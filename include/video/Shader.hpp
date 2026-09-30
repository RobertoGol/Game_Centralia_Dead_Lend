#pragma once
#include <string>
#include <cstdint>

namespace Centralia {

class Shader {
private:
    uint32_t m_programId;
    bool CheckCompileErrors(uint32_t shaderId, const std::string& type);

public:
    Shader();
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    bool LoadFromFiles(const std::string& vertexPath, const std::string& fragmentPath);
    void Use();
    void Unuse();
    void SetVec3(const std::string& name, float x, float y, float z);
    void SetMat4(const std::string& name, const float* matrixData);

    [[nodiscard]] uint32_t GetProgramID() const { return m_programId; }
};

} // namespace Centralia