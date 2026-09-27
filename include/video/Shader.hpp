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

    inline bool LoadFromFiles(const std::string& vertexPath, const std::string& fragmentPath) noexcept {
        std::string vertexCode;
        std::string fragmentCode;
        std::ifstream vShaderFile;
        std::ifstream fShaderFile;

        vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

        try {
            vShaderFile.open(vertexPath);
            fShaderFile.open(fragmentPath);
            std::stringstream vShaderStream, fShaderStream;

            vShaderStream << vShaderFile.rdbuf();
            fShaderStream << fShaderFile.rdbuf();

            vShaderFile.close();
            fShaderFile.close();

            vertexCode = vShaderStream.str();
            fragmentCode = fShaderStream.str();
        }
        catch (std::ifstream::failure& e) {
            Platform::Log("Critical Error: Shader files could not be read successfully! Path error.");
            return false;
        }

        const char* vShaderCode = vertexCode.c_str();
        const char* fShaderCode = fragmentCode.c_str();

        uint32_t vertexId, fragmentId;

        // 1. Компиляция Вершинного Шейдера
        vertexId = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexId, 1, &vShaderCode, NULL);
        glCompileShader(vertexId);
        if (!CheckCompileErrors(vertexId, "VERTEX")) return false;

        // 2. Компиляция Фрагментного Шейдера
        fragmentId = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentId, 1, &fShaderCode, NULL);
        glCompileShader(fragmentId);
        if (!CheckCompileErrors(fragmentId, "FRAGMENT")) return false;

        // 3. Создание Шейдерной Программы и линковка на GPU
        m_programId = glCreateProgram();
        glAttachShader(m_programId, vertexId);
        glAttachShader(m_programId, fragmentId);
        glLinkProgram(m_programId);
        if (!CheckCompileErrors(m_programId, "PROGRAM")) return false;

        glDeleteShader(vertexId);
        glDeleteShader(fragmentId);

        Platform::Log("Shader: Dynamic specular environment mappings compiled successfully on GPU.");
        return true;
    }

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
