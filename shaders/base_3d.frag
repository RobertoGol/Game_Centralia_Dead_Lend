#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 cameraPos;
uniform samplerCube environmentMap;
uniform float stealthAlpha;

// Параметры многоуровневой краски от MaterialSystem
uniform vec3 materialParams; // X = Roughness, Y = PaintAlpha, Z = RustIntensity
uniform vec3 materialGlow;   // X = RadGlowIntensity

void main() {
    // 1. Базовые цвета уровней (красок)
    vec3 metalPrimerColor = vec3(0.25, 0.25, 0.27);  // Голый металл подложки
    vec3 factoryPaintColor = vec3(0.15, 0.35, 0.65); // Заводской синий цвет кузова ВАЗ/Титана
    vec3 rustColor = vec3(0.45, 0.22, 0.12);         // Рыжая ржавчина Fallout
    vec3 radGlowColor = vec3(0.0, 0.95, 0.1);        // Зеленое свечение изотопов

    // 2. Послойное смешивание многоуровневой краски на видеокарте
    vec3 mixedPaint = mix(metalPrimerColor, factoryPaintColor, materialParams.y); // Слой краски поверх грунта
    vec3 finalSurfaceColor = mix(mixedPaint, rustColor, materialParams.z);        // Накладываем ржавчину сверху

    // 3. Магия отражений старой школы с учетом шероховатости (Roughness)
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(FragPos - cameraPos);
    vec3 reflectDir = reflect(viewDir, norm);
    vec3 dynamicReflection = texture(environmentMap, reflectDir).rgb;

    // Чем меньше шероховатость (Roughness), тем сильнее зеркальное отражение мира
    float reflectionStrength = 0.4 * (1.0 - materialParams.x);
    vec3 shadedColor = mix(finalSurfaceColor, dynamicReflection, reflectionStrength);

    // 4. Добавляем уровень радиационного свечения (Эффект самосвечения Элдер Тейл / Рад-выброса)
    vec3 finalCalculatedColor = shadedColor + (radGlowColor * materialGlow.x);

    FragColor = vec4(finalCalculatedColor, stealthAlpha);
}
