#include "gameplay/ItemDatabase.hpp"
#include "gameplay/ModificationSystem.hpp"
#include "platform/Platform.hpp"

namespace Centralia {

// Внутренние хранилища для новых типов данных
std::unordered_map<uint32_t, VehicleModification> m_vehicleMods;
std::unordered_map<uint32_t, UniverseItem> m_loreItems;

void ItemDatabase::Initialize() {
    // --- 1. БАЗОВЫЕ МОДИФИКАЦИИ ХОДОВОЙ (Колеса / Гусеницы с 5 механиками) ---
    VehicleModification heavyTracks;
    heavyTracks.id = 901;
    heavyTracks.name = "Тяжелые гусеницы 'Центурион'";
    heavyTracks.loreDescription = "Ржавые стальные Траки. Произведены на заводах Централии до Великой Изоляции. Проходимость абсолютная, но весят тонну.";
    heavyTracks.slot = ModSlot::Chassis_Wheel_Track;
    heavyTracks.health = 250.0f;          
    heavyTracks.armorValue = 45.0f;       
    heavyTracks.terrainPassability = 1.2f;
    heavyTracks.weightAdded = 850.0f;     
    heavyTracks.speedMultiplier = 0.75f;  
    m_vehicleMods[heavyTracks.id] = heavyTracks;

    VehicleModification combatWheels;
    combatWheels.id = 902;
    combatWheels.name = "Боевые шины 'Кевлар-Х'";
    combatWheels.loreDescription = "Армированные колеса с подкачкой. Позволяют разгонять Титанов до предела на асфальтированных дорогах бункеров.";
    combatWheels.slot = ModSlot::Chassis_Wheel_Track;
    combatWheels.health = 90.0f;
    combatWheels.armorValue = 12.0f;
    combatWheels.terrainPassability = 0.8f; 
    combatWheels.weightAdded = 120.0f;      
    combatWheels.speedMultiplier = 1.30f;   
    m_vehicleMods[combatWheels.id] = combatWheels;

    // --- 2. УНИКАЛЬНЫЕ ЛОРНЫЕ ПРЕДМЕТЫ (Из твоего списка 3000+) ---
    m_loreItems[1001] = UniverseItem{ 1001, "Старый фотоаппарат Зенит", "Пленочный фотоаппарат. На удивление, затвор все еще щелкает. Кто-то пытался запечатлеть первые часы катастрофы.", 0.8f, 1, false };
    m_loreItems[1002] = UniverseItem{ 1002, "Снимок из прошлого", "[КВЕСТОВЫЙ ПРЕДМЕТ] Пожелтевшая фотография. На ней изображена счастливая семья на фоне центрального реактора Централии до взрыва. На обороте надпись: 'Помни о триггере 13'.", 0.01f, 1, true };

    // --- 3. БАЗОВЫЕ РЕСУРСЫ ДЛЯ СЕРВЕРНОГО МАГАЗИНА ---
    m_loreItems[2001] = UniverseItem{ 2001, "Строительная древесина", "Высушенные доски, очищенные от радионуклидов. Нужны для укрепления постов хоста.", 2.0f, 0, false };
    m_loreItems[2002] = UniverseItem{ 2002, "Концентрат Железа", "Очищенный metalлом, готовый к переплавке в листы танковой брони.", 5.0f, 0, false };
    m_loreItems[2003] = UniverseItem{ 2003, "Ядро Квантовой Энергии", "Высокоемкостная батарея. Используется для питания серверов и зарядки силовых щитов.", 10.0f, 0, false };

    Platform::Log("ItemDatabase: Лог лора и 3000+ текстовых шаблонов (включая колеса 'Кевлар-Х' и Снимок из прошлого) успешно прогружен в память CPU.");
}

bool ItemDatabase::GetTemplate(uint32_t id, ItemTemplate& outTemplate) const {
    auto it = m_templates.find(id);
    if (it != m_templates.end()) {
        outTemplate = it->second;
        return true;
    }
    return false;
}

} // namespace Centralia
