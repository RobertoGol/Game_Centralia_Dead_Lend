#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace Centralia {

// Типы магазинов оружия и тяжелых пушек Титанов/Танков
enum class MagazineType : uint8_t {
    Magazine_Clip,         // Классический магазинный (коробка/диск)
    Single_Shot,           // Однопатронный (каждый раз ручной досыл)
    Powder_Single_Shot,    // Порошково-однопатронный (требует порох + снаряд)
    Revolver_Cylinder      // Револьверный барабан (попатронная зарядка)
};

struct WeaponSpecs {
    uint32_t id;
    std::string name;
    MagazineType magType;
    
    uint32_t requiredAmmoId;     // ID типа патронов из инвентаря
    uint16_t currentAmmoInClip;  // Сколько патронов в пушке прямо сейчас
    uint16_t clipMaxCapacity;    // Емкость барабана/магазина
    
    float baseDamage;            // Базовый урон снаряда
    float fireRateRpm;           // Скорострельность в минуту
    float verticalRecoil;        // Сила отдачи (увод 3D-камеры вверх)
    float reloadTimeSec;         // Время полной перезарядки
    
    bool isReloading = false;
    float reloadProgressTimer = 0.0f;
};

class WeaponSystem {
private:
    WeaponSpecs m_activeWeapon;
    float m_recoilAccumulator = 0.0f; // Текущий сдвиг камеры от отдачи

public:
    WeaponSystem();
    ~WeaponSystem() = default;

    // Загрузка ТТХ выбранной пушки гуманоида или Титана
    void EquipWeapon(uint32_t weaponId);

    // Логика выстрела (вызывается на ЛКМ или триггер геймпада)
    // Возвращает true, если выстрел произошел
    bool Fire(float deltaTime, float playerMovementSpeed, float& outCameraRecoilY);

    // Старт перезарядки на кнопку 'R' или кнопку Х геймпада Xbox
    void StartReload(class Player& player);

    // Обновление таймеров автоматики оружия на CPU (вызывается в Engine::Update)
    void UpdateWeaponTick(float deltaTime, class Player& player);

    const WeaponSpecs& GetActiveWeaponSpecs() const { return m_activeWeapon; }
};

} // namespace Centralia
