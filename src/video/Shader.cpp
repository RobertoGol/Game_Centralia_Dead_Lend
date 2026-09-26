#include "video/Shader.hpp"
#include "platform/Platform.hpp"
#include <glad/glad.h>
#include <fstream>
#include <sstream>
#include <vector>

namespace Centralia {

Shader::Shader() : m_programId(0) {}

Shader::~Shader() {
    if (m_programId != 0) {
        glDeleteProgram(m_programId);
    }
}

bool Shader::LoadFromFiles(const std::string& vertexPath, const std::string& fragmentPath) {
    std::string vertexCode;
    std::string fragmentCode;
    std::ifstream vShaderFile;
    std::ifstream fShaderFile;

    // Обеспечиваем контролируемое чтение файлов с диска Windows 10
    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try {
        // Открываем файлы исходного кода шейдеров
        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);
        std::stringstream vShaderStream, fShaderStream;

        // Считываем содержимое файлов в потоки
        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();

        // Закрываем файлы
        vShaderFile.close();
        fShaderFile.close();

        // Конвертируем поток в строки
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

    // 1. Компиляция Вершинного Шейдера (Vertex Shader)
    vertexId = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexId, 1, &vShaderCode, NULL);
    glCompileShader(vertexId);
    if (!CheckCompileErrors(vertexId, "VERTEX")) {
        return false;
    }

    // 2. Компиляция Фрагментного Шейдера (Fragment Shader)
    fragmentId = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentId, 1, &fShaderCode, NULL);
    glCompileShader(fragmentId);
    if (!CheckCompileErrors(fragmentId, "FRAGMENT")) {
        return false;
    }

    // 3. Создание Шейдерной Программы и линковка на GPU
    m_programId = glCreateProgram();
    glAttachShader(m_programId, vertexId);
    glAttachShader(m_programId, fragmentId);
    glLinkProgram(m_programId);
    if (!CheckCompileErrors(m_programId, "PROGRAM")) {
        return false;
    }

    // После успешной линковки в бинарник GPU, промежуточные исходники можно удалить из памяти видеокарты
    glDeleteShader(vertexId);
    glDeleteShader(fragmentId);

    Platform::Log("Shader: Dynamic specular environment mappings compiled successfully on GPU.");
    return true;
}

void Shader::Use() {
    if (m_programId != 0) {
        glUseProgram(m_programId);
    }
}

void Shader::Unuse() {
    glUseProgram(0);
}

// Передача вектора позиции (например, координат 3D-камеры) в память GPU
void Shader::SetVec3(const std::string& name, float x, float y, float z) {
    int location = glGetUniformLocation(m_programId, name.c_str());
    if (location != -1) {
        glUniform3f(location, x, y, z);
    }
}

// Передача матриц трансформации (Projection / View / Model) для расчета отражений
void Shader::SetMat4(const std::string& name, const float* matrixData) {
    int location = glGetUniformLocation(m_programId, name.c_str());
    if (location != -1) {
        // Передаем матрицу 4х4 на видеокарту без транспонирования
        glUniformMatrix4fv(location, 1, GL_FALSE, matrixData);
    }
}

bool Shader::CheckCompileErrors(uint32_t shaderId, const std::string& type) {
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

} // namespace Centralia
