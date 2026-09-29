#pragma once
#include <cstdint>
#include <array>

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
    uint8_t  isBroken;         // 1 байт: Флаг разрушения сегмента для CPU-бранчинга
}; // Итого: 13 байт на элемент

struct PowerArmorStateData {
    std::array<PowerArmorComponent, 6> components; // 78 байт: 6 независимых узлов
    float    fusionCoreCharge;                     // 4 байта: Текущая емкость энергоячейки [0.0f - 100.0f]
    float    coreDrainModifier;                    // 4 байта: Множитель расхода (зависит от класса Elder Tale)
    uint8_t  isCoreDepleted;                       // 1 байт: Флаг аварийного отключения питания
    uint8_t  padding;                              // ИСПРАВЛЕНО: Атомарный байт для идеального выравнивания
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
    PowerArmorEngineContext() noexcept;

    // Инициализация структуры без динамических аллокаций
    void ResetToDefault() noexcept;

    /**
     * @brief Высокоточный обсчет энергосети экзоскелета. Интегрируется напрямую в тики engine.Update().
     * @param deltaTime - Квант времени из аппаратно-адаптивного цикла main.cpp
     * @param isShiftPressed - Флаг удержания клавиши спринта игроком
     * @param outLinearVelocity - Ссылка на текущий вектор скорости перемещения для его урезания
     */
    void ProcessPowerGridTick(float deltaTime, bool isShiftPressed, float& outLinearVelocity) noexcept;

    /**
     * @brief Регистрация мгновенного расхода энергии (силовые атаки, рывки мехов Titanfall)
     */
    void RegisterHeavyCombatAction() noexcept;

    /**
     * @brief Покомпонентный просчет поглощения входящего урона и износа пластин
     * @param targetComp - ID конкретного сегмента, куда прилетел трассировочный луч или снаряд
     * @param ioDamage - Ссылка на переменную входящего урона. Функция модифицирует ее внутри!
     */
    void ComputeComponentDamage(ArmorComponentID targetComp, float& ioDamage) noexcept;

    // Метод для горячей перезарядки ядерного блока из инвентаря игрока
    void HotSwapFusionCore() noexcept;

    // Экспорт указателя на сырую память для NetworkSerializer и P2P-пакетов
    [[nodiscard]] const uint8_t* GetRawBinaryState() const noexcept {
        return reinterpret_cast<const uint8_t*>(&m_State);
    };

    [[nodiscard]] constexpr size_t GetBinarySize() const noexcept {
        return sizeof(PowerArmorStateData);
    };

    // Прямой доступ к параметрам для UI Modern OpenGL
    [[nodiscard]] float GetCurrentCharge() const noexcept { return m_State.fusionCoreCharge; };
    [[nodiscard]] float GetComponentDurability(ArmorComponentID id) const noexcept { 
        return m_State.components[static_cast<uint8_t>(id)].durability; 
    };
};

}; // namespace Centralia_Project_Passport
