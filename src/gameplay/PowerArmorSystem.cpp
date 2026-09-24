#pragma once
#include <cstdint>
#include <cstring>
#include <cmath>
#include <array>
#include <string_view>

namespace Centralia_Project_Passport {

// Жесткое побайтовое выравнивание для сетевого сериализатора без разрывов в памяти
#pragma pack(push, 1)

enum class ArmorComponentID : uint8_t {
    Helmet    = 0,
    Torso     = 1,
    LeftArm   = 2,
    RightArm  = 3,
    LeftLeg   = 4,
    RightLeg  = 5,
    COUNT     = 6
};

struct PowerArmorComponent {
    float    durability;       // 4 байта: Текущая прочность [0.0f - 100.0f]
    float    maxDurability;    // 4 байта: Максимальная прочность
    float    damageResistance; // 4 байта: Нативный показатель поглощения урона (DR)
    uint8_t  isBroken;         // 1 байт: Булевый флаг разрушения сегмента для CPU-бранчинга
}; // Итого: 13 байт на элемент

struct PowerArmorStateData {
    std::array<PowerArmorComponent, 6> components; // 78 байт: 6 независимых узлов
    float    fusionCoreCharge;                     // 4 байта: Текущая емкость энергоячейки [0.0f - 100.0f]
    float    coreDrainModifier;                    // 4 байта: Множитель расхода (зависит от класса Elder Tale)
    uint8_t  isCoreDepleted;                       // 1 байт: Флаг аварийного отключения питания
    uint8_t  padding[1];                           // 1 байт: Выравнивание структуры под четную границу
}; // Итого: 88 байт чистых бинарных данных, идеальных для P2P-пакетов

#pragma pack(pop)

class PowerArmorEngineContext {
private:
    PowerArmorStateData m_State;

    // Константы энергопотребления (вынесены в статические constexpr для оптимизации компилятора)
    static constexpr float CONST_IDLE_DRAIN   = 0.005f;  // Пассивный тик в секунду
    static constexpr float CONST_SPRINT_DRAIN = 0.550f;  // Жор ядра при беге на Left Shift
    static constexpr float CONST_ACTION_DRAIN = 4.250f;  // Использование тяжелого оружия пушек Титанов

public:
    PowerArmorEngineContext() noexcept {
        ResetToDefault();
    }

    // Инициализация структуры без динамических аллокаций
    void ResetToDefault() noexcept {
        std::memset(&m_State, 0, sizeof(PowerArmorStateData));
        
        m_State.fusionCoreCharge    = 100.0f;
        m_State.coreDrainModifier   = 1.0f; // Дефолтный множитель, будет изменен системой классов Elder Tale
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

    /**
     * @brief Высокоточный обсчет энергосети экзоскелета. Интегрируется напрямую в тики engine.Update().
     * @param deltaTime - Квант времени из аппаратно-адаптивного цикла main.cpp
     * @param isShiftPressed - Флаг удержания клавиши спринта игроком
     * @param outLinearVelocity - Ссылка на текущий вектор скорости перемещения для его урезания
     */
    void ProcessPowerGridTick(float deltaTime, bool isShiftPressed, float& outLinearVelocity) noexcept {
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

    /**
     * @brief Регистрация мгновенного расхода энергии (силовые атаки, рывки мехов Titanfall)
     */
    void RegisterHeavyCombatAction() noexcept {
        if (m_State.isCoreDepleted) return;

        m_State.fusionCoreCharge -= CONST_ACTION_DRAIN;
        if (m_State.fusionCoreCharge <= 0.0f) {
            m_State.fusionCoreCharge = 0.0f;
            m_State.isCoreDepleted   = 1;
        }
    }

    /**
     * @brief Покомпонентный просчет поглощения входящего урона и износа пластин
     * @param targetComp - ID конкретного сегмента, куда прилетел трассировочный луч или снаряд
     * @param ioDamage - Ссылка на переменную входящего урона. Функция модифицирует ее внутри!
     */
    void ComputeComponentDamage(ArmorComponentID targetComp, float& ioDamage) noexcept {
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

    // Метод для горячей перезарядки ядерного блока из инвентаря игрока
    void HotSwapFusionCore() noexcept {
        m_State.fusionCoreCharge = 100.0f;
        m_State.isCoreDepleted   = 0;
    }

    // Экспорт указателя на сырую память для NetworkSerializer и P2P-пакетов
    [[nodiscard]] const uint8_t* GetRawBinaryState() const noexcept {
        return reinterpret_cast<const uint8_t*>(&m_State);
    }

    [[nodiscard]] constexpr size_t GetBinarySize() const noexcept {
        return sizeof(PowerArmorStateData);
    }

    // Прямой доступ к параметрам для UI Modern OpenGL
    [[nodiscard]] float GetCurrentCharge() const noexcept { return m_State.fusionCoreCharge; }
    [[nodiscard]] float GetComponentDurability(ArmorComponentID id) const noexcept { 
        return m_State.components[static_cast<uint8_t>(id)].durability; 
    }
};

} // namespace Centralia_Project_Passport
