#include "gameplay/ModificationSystem.hpp"
#include "platform/Platform.hpp"
#include <algorithm>
#include <string>

namespace Centralia {

bool ModdableEntity::InstallModification(const Modification& mod) {
    // Проверяем, не занят ли уже этот конструкторский слот
    for (const auto& installed : m_installedMods) {
        if (installed.slot == mod.slot) {
            Platform::Log("[MODIFICATION FAIL]: Слот инсталляции уже занят другим модулем обвеса!");
            return false; 
        }
    }
    
    m_installedMods.push_back(mod);
    Platform::Log("[MODIFICATION SUCCESS]: Успешно установлен обвес '" + mod.name + "'. Физика сущности пересчитана.");
    return true;
}

void ModdableEntity::RemoveModification(uint32_t modId) {
    m_installedMods.erase(
        std::remove_if(m_installedMods.begin(), m_installedMods.end(),
            [modId](const Modification& m) { return m.id == modId; }),
        m_installedMods.end()
    );
    Platform::Log("[MODIFICATION]: Модуль ID " + std::to_string(modId) + " демонтирован.");
}

float ModdableEntity::GetModifiedSpeed() const noexcept {
    float speed = m_baseSpeed;
    for (const auto& mod : m_installedMods) {
        speed *= mod.speedMultiplier;
    }
    return speed;
}

} // namespace Centralia
