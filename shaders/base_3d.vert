#version 330 core

// Входные буферы из нашего VBO (layout настроен в Renderer3D.cpp)
layout (location = 0) in vec3 aPos;     // Позиция вершины модели
layout (location = 1) in vec3 aNormal;  // Нормаль полигона (для света и отражений)

// Выходные данные для фрагментного шейдера
out vec3 FragPos;  
out vec3 Normal;   

// Матрицы трансформации от процессора (CPU)
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    // Вычисляем позицию вершины в мировых 3D-координатах
    FragPos = vec3(model * vec4(aPos, 1.0));
    
    // Исправляем искажение нормалей при масштабировании/повороте модели на CPU.
    // Используем нормальную матрицу (инвертированная и транспонированная матрица модели)
    Normal = mat3(transpose(inverse(model))) * aNormal;  
    
    // Финальная позиция вершины на экране 1920x1080
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
