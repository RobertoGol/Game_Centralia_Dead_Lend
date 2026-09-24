#pragma once
#include <string>
#include <cstdint>

#if defined(CENTRALIA_PLATFORM_WINDOWS)
    #include <windows.h>
    #include <GL/gl.h>
#else
    #include <GL/gl.h>
#endif

namespace Centralia {

class Shader {
private:
    uint32_t m_programId;

    // Внутренний метод проверки ошибок компиляции драйвером GPU
    bool CheckCompileErrors(uint32_t shaderId, const std::string& type);

public:
    Shader();
    ~Shader();

    // Загрузка текстовых файлов шейдеров с диска и их сборка на GPU
    bool LoadFromFiles(const std::string& vertexPath, const std::string& fragmentPath);
    
    // Сделать эту шейдерную программу активной для видеокарты
    void Use();
    
    // Отключить шейдер
    void Unuse();

    // Передача параметров (позиция камеры, свет, матрицы) в шейдер перед отрисовкой
    void SetVec3(const std::string& name, float x, float y, float z);
    void SetMat4(const std::string& name, const float* matrixData);
    
    uint32_t GetProgramID() const { return m_programId; }
};

} // namespace Centralia
