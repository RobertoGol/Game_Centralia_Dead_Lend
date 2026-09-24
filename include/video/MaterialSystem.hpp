#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>

namespace Centralia {

// Идентификаторы слоев краски для многоуровневого смешивания
enum class PaintLayer : uint8_t {
    Base_Primer,      // 1. Грунтовка / Голый матовый металл
    Factory_Paint,    // 2. Заводской цвет кузова/брони (синий, стальной, камуфляж)
    Rust_Decal,       // 3. Слой ржавчины Пустоши ( Fallout-эффект)
    Rad_Glow_Layer    // 4. Слой радиационного свечения (зеленый изотопный налет)
};

struct MaterialProperties {
    uint32_t materialId;
    std::string materialName;

    // Многоуровневые коэффициенты смешивания красок (0.0f - полностью прозрачно, 1.0f - плотный слой)
    float baseRoughness = 0.5f;       // Шероховатость (матовость/глянец)
    float factoryPaintAlpha = 1.0f;   // Плотность основной краски автомобиля/меха
    float rustIntensity = 0.1f;       // Степень коррозии металла (0.0f - новый, 1.0f - гнилой хлам)
    float radGlowIntensity = 0.0f;    // Интенсивность свечения заражения (для Rad_Ghoul и зон реактора)
};

class MaterialSystem {
private:
    std::unordered_map<uint32_t, MaterialProperties> m_materials;
    MaterialSystem(); // Синглтон

public:
    static MaterialSystem& GetInstance() {
        static MaterialSystem instance;
        return instance;
    }

    // Загрузка базовых профилей красок для техники и Титанов
    void InitializeMaterialLibrary();

    // Проброс параметров многоуровневой краски в активный Shader перед отрисовкой модели
    void ApplyMaterialToShader(uint32_t materialId, class Shader& activeShader);

    bool GetMaterialSpecs(uint32_t materialId, MaterialProperties& outProperties) const;
};

} // namespace Centralia
