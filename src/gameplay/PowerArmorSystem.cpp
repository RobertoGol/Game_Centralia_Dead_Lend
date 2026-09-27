#include "gameplay/PowerArmorStateData.hpp"
#include <cstring>
#include <algorithm>

namespace Centralia_Project_Passport {

PowerArmorEngineContext::PowerArmorEngineContext() noexcept {
    ResetToDefault();
}

void PowerArmorEngineContext::ResetToDefault() noexcept {
    std::memset(&m_State, 0, sizeof(PowerArmorStateData));
    
    m_State.fusionCoreCharge    = 100.0f;
    m_State.coreDrainModifier   = 1.0f; // Дефолтный множитель, изменяемый системой классов Elder Tale
    m_State.isCoreDepleted      = 0;

    // Инициализируем ТТХ элементов Т-60 / Экзоскелета
    for (size_t i = 0; i < 6; ++i) {
        auto& comp = m_State.components[i];
        comp.maxDurability = 150.0f;
        comp.durability    = 150.0f;
        comp.isBroken      = 0;
        
        // Распределяем дефолтный коэффициент DR (Торс защищен сильнее)
        if (i == static_cast<size_t>(ArmorComponentID::Torso)) {
            comp.damageResistance = 45.0f;
        } else {
            comp.damageResistance = 20.0f;
        }
    }
}

void PowerArmorEngineContext::ProcessPowerGridTick(float deltaTime, bool isShiftPressed, float& outLinearVelocity) noexcept {
    if (m_State.isCoreDepleted) {
        // Штраф к мобильности при нулевом заряде: отключаем сервоприводы, режем базовый разгон
        outLinearVelocity *= 0.35f; 
        return;
    }

    // Вычисляем текущее потребление энергии на этом тике процессора
    float currentDrain = CONST_IDLE_DRAIN;
    
    if (isShiftPressed && outLinearVelocity > 0.1f) {
        currentDrain += CONST_SPRINT_DRAIN;
        // Сервоприводы увеличивают скорость бега в тяжелом металле
        outLinearVelocity *= 1.45f; 
    }

    // Применяем модификатор класса из Log Horizon (например, инженеры/пилоты тратят меньше)
    m_State.fusionCoreCharge -= (currentDrain * m_State.coreDrainModifier) * deltaTime;

    // Предохранитель разряда
    if (m_State.fusionCoreCharge <= 0.001f) {
        m_State.fusionCoreCharge = 0.0f;
        m_State.isCoreDepleted   = 1;
        outLinearVelocity       *= 0.35f;
    }
}

void PowerArmorEngineContext::RegisterHeavyCombatAction() noexcept {
    if (m_State.isCoreDepleted) return;

    m_State.fusionCoreCharge -= CONST_ACTION_DRAIN;
    if (m_State.fusionCoreCharge <= 0.0f) {
        m_State.fusionCoreCharge = 0.0f;
        m_State.isCoreDepleted   = 1;
    }
}

void PowerArmorEngineContext::ComputeComponentDamage(ArmorComponentID targetComp, float& ioDamage) noexcept {
    uint8_t index = static_cast<uint8_t>(targetComp);
    if (index >= 6) return;

    auto& comp = m_State.components[index];

    // Если пластина выбита в 0%, урон полностью игнорирует DR элемента и летит в HP игрока
    if (comp.isBroken) {
        return; 
    }

    // Формула поглощения урона (DR) в стиле Fallout: снижаем урон, но изнашиваем саму пластину
    float absorbed = comp.damageResistance;
    if (absorbed >= ioDamage * 0.75f) {
        absorbed = ioDamage * 0.75f; // Защита не может поглотить более 75% урона за раз
    }

    ioDamage -= absorbed;
    
    // Износ прочности элемента пропорционален заблокированному урону
    comp.durability -= (absorbed * 0.45f);

    // Проверка критического разрушения сегмента экзоскелета
    if (comp.durability <= 0.0f) {
        comp.durability = 0.0f;
        comp.isBroken   = 1;
    }
}

void PowerArmorEngineContext::HotSwapFusionCore() noexcept {
    m_State.fusionCoreCharge = 100.0f;
    m_State.isCoreDepleted   = 0;
}

} // namespace Centralia_Project_Passport
