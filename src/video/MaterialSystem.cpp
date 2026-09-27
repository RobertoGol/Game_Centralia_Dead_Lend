#include "video/MaterialSystem.hpp"
#include "video/Shader.hpp"
#include "platform/Platform.hpp"

namespace Centralia {

MaterialSystem::MaterialSystem() {}

void MaterialSystem::InitializeMaterialLibrary() {
    m_materials.clear();

    // Profile 1: Factory paint coatings for traditional chassis structures (Gloss Polish)
    MaterialProperties classicCarPaint;
    classicCarPaint.materialId = 801;
    classicCarPaint.materialName = "Заводской Глянец ВАЗ";
    classicCarPaint.baseRoughness = 0.2f;       // Reflective sheen profile (Low roughness scale)
    classicCarPaint.factoryPaintAlpha = 1.0f;   // Solid original paint coat thickness
    classicCarPaint.rustIntensity = 0.05f;      // Minor target oxidation trace variables
    classicCarPaint.radGlowIntensity = 0.0f;    // Clean environment layer
    m_materials[classicCarPaint.materialId] = classicCarPaint;

    // Profile 2: Heavily degraded rust textures for wasteland mechs and scrap frames
    MaterialProperties wastelandWreck;
    wastelandWreck.materialId = 802;
    wastelandWreck.materialName = "Ржавый Индустриальный Корпус";
    wastelandWreck.baseRoughness = 0.8f;       // Matte diffuse scatter profile
    wastelandWreck.factoryPaintAlpha = 0.4f;   // Obleached base coat properties (40% remaining)
    wastelandWreck.rustIntensity = 0.75f;      // Extended corrosion overlay
    wastelandWreck.radGlowIntensity = 0.0f;
    m_materials[wastelandWreck.materialId] = wastelandWreck;

    // Profile 3: Radioactive biological target textures (Self-illuminating neon phosphors)
    MaterialProperties radGlowBio;
    radGlowBio.materialId = 803; 
    radGlowBio.materialName = "Облученная Зараженная Плоть";
    radGlowBio.baseRoughness = 0.9f;
    radGlowBio.factoryPaintAlpha = 0.0f;       // Zero paint layer attributes
    radGlowBio.rustIntensity = 0.0f;
    radGlowBio.radGlowIntensity = 0.85f;       // Emits strong green luminescence over fragment buffers
    
    // FIXED: Swapped out broken primitive shadows to index via valid struct data members
    m_materials[radGlowBio.materialId] = radGlowBio;

    Platform::Log("MaterialSystem: Многоуровневые профили красок и слоев износа (ВАЗ, Титаны, Рад-Био) успешно загружены.");
}

void MaterialSystem::ApplyMaterialToShader(uint32_t materialId, Shader& activeShader) {
    auto it = m_materials.find(materialId);
    if (it == m_materials.end()) return;
    const MaterialProperties& mat = it->second;

    // Pipe multi-layered parameters straight to programmable GPU shader uniform slots
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
