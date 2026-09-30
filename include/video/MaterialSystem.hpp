#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include "video/Shader.hpp"
#include "platform/Platform.hpp"

namespace Centralia {

enum class PaintLayer : uint8_t {
    Base_Primer,      
    Factory_Paint,    
    Rust_Decal,       
    Rad_Glow_Layer    
};

struct MaterialProperties {
    uint32_t materialId;
    std::string materialName;
    float baseRoughness = 0.5f;       
    float factoryPaintAlpha = 1.0f;   
    float rustIntensity = 0.1f;       
    float radGlowIntensity = 0.0f;    
};

class MaterialSystem {
private:
    std::unordered_map<uint32_t, MaterialProperties> m_materials;

    MaterialSystem() noexcept {
        InitializeMaterialLibrary();
    }

public:
    ~MaterialSystem() = default;

    MaterialSystem(const MaterialSystem&) = delete;
    MaterialSystem& operator=(const MaterialSystem&) = delete;

    static MaterialSystem& GetInstance() {
        static MaterialSystem instance;
        return instance;
    }

    void InitializeMaterialLibrary() noexcept {
        m_materials.clear();

        MaterialProperties classicCarPaint;
        classicCarPaint.materialId = 801;
        classicCarPaint.materialName = "Заводской Глянец ВАЗ";
        classicCarPaint.baseRoughness = 0.2f;       
        classicCarPaint.factoryPaintAlpha = 1.0f;   
        classicCarPaint.rustIntensity = 0.05f;      
        classicCarPaint.radGlowIntensity = 0.0f;    
        m_materials[classicCarPaint.materialId] = classicCarPaint;

        MaterialProperties wastelandWreck;
        wastelandWreck.materialId = 802;
        wastelandWreck.materialName = "Ржавый Индустриальный Корпус";
        wastelandWreck.baseRoughness = 0.8f;       
        wastelandWreck.factoryPaintAlpha = 0.4f;   
        wastelandWreck.rustIntensity = 0.75f;      
        wastelandWreck.radGlowIntensity = 0.0f;
        m_materials[wastelandWreck.materialId] = wastelandWreck;

        MaterialProperties radGlowBio;
        radGlowBio.materialId = 803; 
        radGlowBio.materialName = "Облученная Зараженная Плоть";
        radGlowBio.baseRoughness = 0.9f;
        radGlowBio.factoryPaintAlpha = 0.0f;       
        radGlowBio.rustIntensity = 0.0f;
        radGlowBio.radGlowIntensity = 0.85f;    
        m_materials[radGlowBio.materialId] = radGlowBio;

        Platform::Log("MaterialSystem: Многоуровневые профили красок запечатаны в RAM.");
    }

    [[nodiscard]] bool GetMaterialSpecs(uint32_t materialId, MaterialProperties& outProperties) const noexcept {
        auto it = m_materials.find(materialId);
        if (it != m_materials.end()) {
            outProperties = it->second;
            return true;
        }
        return false;
    }
};

} // namespace Centralia