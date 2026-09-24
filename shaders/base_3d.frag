#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

// Параметры от игрового движка
uniform vec3 cameraPos;         // Позиция 3D-камеры для расчета угла взгляда
uniform samplerCube environmentMap; // Легкая кубическая текстура мира ПустошиCentralia
uniform float stealthAlpha;     // Коэффициент невидимости (1.0 - виден, 0.2 - Ghost режим приседа)

void main() {
    // Базовый цвет брони (индустриальный стальной цвет по умолчанию)
    vec3 baseArmorColor = vec3(0.32, 0.34, 0.36); 

    // Нормализуем входящие векторы нормалей и направления взгляда
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(FragPos - cameraPos);
    
    // Формула зеркального отражения золотой эры: R = I - 2 * dot(N, I) * N
    vec3 reflectDir = reflect(viewDir, norm);
    
    // Выборка пикселя отражения из кубической карты окружения
    vec3 dynamicReflection = texture(environmentMap, reflectDir).rgb;

    // Смешиваем матовый металл брони с честным отражением мира (интенсивность 25%)
    vec3 finalColor = mix(baseArmorColor, dynamicReflection, 0.25);

    // Добавляем базовое затеняющее освещение (Ambient), чтобы модель имела объем в 3D
    vec3 ambientLight = vec3(0.2, 0.2, 0.2) * finalColor;
    vec3 resultColor = finalColor + ambientLight;

    // Управляем прозрачностью фрагмента в зависимости от режима скрытности (Сtrl)
    FragColor = vec4(resultColor, stealthAlpha);
}
