#pragma once
#include <string>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <vector>
#include <glad/glad.h>
#include "platform/Platform.hpp" // Твой кроссплатформенный логер

namespace Centralia {

class Shader {
private:
    uint32_t m_programId;

    inline bool CheckCompileErrors(uint32_t shaderId, const std::string& type) noexcept {
        int success;
        char infoLog[1024];
        
        if (type != "PROGRAM") {
            glGetShaderiv(shaderId, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(shaderId, 1024, NULL, infoLog);
                Platform::Log("GPU COMPILATION ERROR of type: " + type + "\n" + std::string(infoLog));
                return false;
            }
        } else {
            glGetProgramiv(shaderId, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(shaderId, 1024, NULL, infoLog);
                Platform::Log("GPU LINKING ERROR of type: " + type + "\n" + std::string(infoLog));
                return false;
            }
        }
        return true;
    }

public:
    inline Shader() : m_programId(0) {}
    
    inline ~Shader() {
        if (m_programId != 0) {
            glDeleteProgram(m_programId);
        }
    }

class Shader {
private:
    uint32_t m_programId;
    bool CheckCompileErrors(uint32_t shaderId, const std::string& type) noexcept;

public:
    Shader();
    ~Shader();

    bool LoadFromFiles(const std::string& vertexPath, const std::string& fragmentPath) noexcept;
    void Use() noexcept;
    void Unuse() noexcept;
    void SetVec3(const std::string& name, float x, float y, float z) noexcept;
    void SetMat4(const std::string& name, const float* matrixData) noexcept;
    
    [[nodiscard]] uint32_t GetProgramID() const noexcept { return m_programId; }
};

    inline void Use() noexcept {
        if (m_programId != 0) glUseProgram(m_programId);
    }

    inline void Unuse() noexcept {
        glUseProgram(0);
    }

    inline void SetVec3(const std::string& name, float x, float y, float z) noexcept {
        int location = glGetUniformLocation(m_programId, name.c_str());
        if (location != -1) glUniform3f(location, x, y, z);
    }

    inline void SetMat4(const std::string& name, const float* matrixData) noexcept {
        int location = glGetUniformLocation(m_programId, name.c_str());
        if (location != -1) glUniformMatrix4fv(location, 1, GL_FALSE, matrixData);
    }
    
    [[nodiscard]] uint32_t GetProgramID() const noexcept { return m_programId; }
};

} // namespace Centralia
