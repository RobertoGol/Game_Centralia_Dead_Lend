#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include "core/Math3D.hpp"
#include "platform/Platform.hpp"

namespace Centralia {

struct MapSector {
    uint32_t sectorX;
    uint32_t sectorY;
    std::string sectorName;
    bool isRadiationZone;
    float dangerLevel;
};

class MapSystem {
private:
    std::unordered_map<uint64_t, MapSector> m_loadedSectors;
    uint32_t m_currentWorldSeed;

    [[nodiscard]] uint64_t CalculateSectorKey(uint32_t x, uint32_t y) const noexcept;

public:
    MapSystem();
    ~MapSystem() = default;

    MapSystem(const MapSystem&) = delete;
    MapSystem& operator=(const MapSystem&) = delete;

    void InitializeWorld(uint32_t seed);
    void LoadSector(uint32_t x, uint32_t y);
    void UnloadSector(uint32_t x, uint32_t y);

    [[nodiscard]] bool GetSectorData(uint32_t x, uint32_t y, MapSector& outSector) const noexcept;
    [[nodiscard]] bool IsPositionRadioactive(const Vector3D& worldPosition) const noexcept;
};

} // namespace Centralia