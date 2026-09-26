#include "video/MaterialSystem.hpp"
#include "video/Shader.hpp"
#include "platform/Platform.hpp"

namespace Centralia {

MaterialSystem::MaterialSystem() {}

void MaterialSystem::InitializeMaterialLibrary() {
    m_materials.clear();

    // Профиль 1: Заводская краска для ВАЗ-2101 / 2108 (Чистая советская классика)
    MaterialProperties classicCarPaint;
    classicCarPaint.materialId = 801;
    classicCarPaint.materialName = "Заводской Глянец ВАЗ";
    classicCarPaint.baseRoughness = 0.2f;       // Отражает свет (низкая шероховатость)
    classicCarPaint.factoryPaintAlpha = 1.0f;   // Плотный слой краски
    classicCarPaint.rustIntensity = 0.05f;      // Почти нет ржавчины
    classicCarPaint.radGlowIntensity = 0.0f;    // Чистый металл
    m_materials[classicCarPaint.materialId] = classicCarPaint;

    // Профиль 2: Ржавая сталь Пустоши (Для брони Титанов и машин-развалюх а-ля Fallout)
    MaterialProperties wastelandWreck;
    wastelandWreck.materialId = 802;
    wastelandWreck.materialName = "Ржавый Индустриальный Корпус";
    wastelandWreck.baseRoughness = 0.8f;       // Матовая шершавая поверхность
    wastelandWreck.factoryPaintAlpha = 0.4f;   // Краска сильно облезла (всего 40% осталось)
    wastelandWreck.rustIntensity = 0.75f;      // 75% поверхности покрыто ржавчиной
    wastelandWreck.radGlowIntensity = 0.0f;
    m_materials[wastelandWreck.materialId] = wastelandWreck;

    // Профиль 3: Радиоактивная био-масса (Для Гулей, Бегемотов и зараженных Мехов)
//    Временно комментируем битый отладочный вывод:
    float radGlowGlowBio = 1.0f; // <-- ОШИБКА: Ты объявил локальный float с именем radGlowGlowBio!
//    someVar.materialId = radGlowGlowBio.materialId;
    MaterialProperties radGlowBio;
    radGlowBio.materialId = 803; // <-- ФАТАЛЬНАЯ ОШИБКА: Компилятор пытается взять у float поле .materialId!
    radGlowBio.materialName = "Облученная Зараженная Плоть";
    radGlowBio.baseRoughness = 0.9f;
    radGlowBio.factoryPaintAlpha = 0.0f;       // Нет заводской краски
    radGlowBio.rustIntensity = 0.0f;
    radGlowBio.radGlowIntensity = 0.85f;    // Мощное фосфорное зеленое свечение в темноте
    m_materials[radGlowBio.materialId] = radGlowBio;

    Platform::Log("MaterialSystem: Многоуровневые профили красок и слоев износа (ВАЗ, Титаны, Рад-Био) успешно загружены.");
}

void MaterialSystem::ApplyMaterialToShader(uint32_t materialId, Shader& activeShader) {
    auto it = m_materials.find(materialId);
    if (it == m_materials.end()) return;
    const MaterialProperties& mat = it->second;

    // Пробрасываем коэффициенты многоуровневых слоев краски напрямую в память GPU
    // Фрагментный шейдер использует эти веса для попиксельного смешивания текстур
    activeShader.SetVec3("materialParams", mat.baseRoughness, mat.factoryPaintAlpha, mat.rustIntensity);
    activeShader.SetVec3("materialGlow", mat.radGlowIntensity, 0.0f, 0.0f);
}

bool MaterialSystem::GetMaterialSpecs(uint32_t materialId, MaterialProperties& outProperties) const {
    auto it = m_materials.find(materialId);
    if (it != m_materials.end()) {
        outProperties = it->second;
        return true;
    }
    return false;
}

} // namespace Centralia
