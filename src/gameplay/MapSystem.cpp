#include "gameplay/MapSystem.hpp"
#include "platform/Platform.hpp"
#include "core/MemoryManager.hpp"
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <fstream>
#include <cstring>

namespace Centralia {

// ============================================================================
// SECTION 1: CONSTANTS, CHUNK DEFINITIONS & HASHING
// ============================================================================

namespace MapConfig {
    constexpr int CHUNK_SIZE = 64;           // Размер чанка в метрах (и вершинах, 1 метр = 1 вершина)
    constexpr float VERTEX_SPACING = 1.0f;   // Расстояние между вершинами сетки
    constexpr int RENDER_DISTANCE = 8;       // Радиус загрузки чанков вокруг игрока
    constexpr int UNLOAD_DISTANCE = 10;      // Радиус, при котором чанк удаляется из ОЗУ
    constexpr uint32_t MAP_SAVE_MAGIC = 0x4D415053; // "MAPS"
    
    // Функция сопряжения Кантора (Cantor Pairing) для хэширования 2D координат в 1D ключ
    inline uint64_t GetChunkHash(int x, int z) {
        int64_t a = x >= 0 ? 2 * static_cast<int64_t>(x) : -2 * static_cast<int64_t>(x) - 1;
        int64_t b = z >= 0 ? 2 * static_cast<int64_t>(z) : -2 * static_cast<int64_t>(z) - 1;
        return static_cast<uint64_t>((a >= b ? a * a + a + b : a + b * b) / 2);
    }
}

// Внутренние структуры для геометрии
struct TerrainVertex {
    float x, y, z;
    float nx, ny, nz;
    float tx, ty, tz, tw;
    float u, v;
    float r, g, b, a; // Color blending (Ржавчина, Песок, Радиация)
};

struct PropInstance {
    uint32_t modelId;
    float x, y, z;
    float rotX, rotY, rotZ;
    float scale;
};

struct ChunkData {
    int gridX;
    int gridZ;
    std::vector<TerrainVertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<PropInstance> props;
    
    bool isGenerated;
    bool isMeshReady;
    bool isModified; // Если игрок построил базу или выкопал яму
    
    // Hardware Buffers (VBO, VAO, IBO)
    uint32_t vaoId, vboId, iboId;
};

// ============================================================================
// SECTION 2: HIGH-PERFORMANCE PERLIN NOISE & FBM (FRACTAL BROWNIAN MOTION)
// ============================================================================
// Написано с нуля для генерации рельефа без сторонних библиотек.

class PerlinNoise {
private:
    std::vector<int> p;

    static float Fade(float t) noexcept { return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f); }
    static float Lerp(float t, float a, float b) noexcept { return a + t * (b - a); }
    static float Grad(int hash, float x, float y, float z) noexcept {
        int h = hash & 15;
        float u = h < 8 ? x : y;
        float v = h < 4 ? y : h == 12 || h == 14 ? x : z;
        return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
    }

public:
    PerlinNoise(uint32_t seed = 0) {
        p.resize(256);
        for (int i = 0; i < 256; ++i) p[i] = i;

        // Псевдослучайное перемешивание на основе сида
        std::minstd_rand0 rng(seed);
        std::shuffle(p.begin(), p.end(), rng);
        p.insert(p.end(), p.begin(), p.end());
    }

    float Noise(float x, float y, float z) const {
        int X = static_cast<int>(std::floor(x)) & 255;
        int Y = static_cast<int>(std::floor(y)) & 255;
        int Z = static_cast<int>(std::floor(z)) & 255;

        x -= std::floor(x);
        y -= std::floor(y);
        z -= std::floor(z);

        float u = Fade(x);
        float v = Fade(y);
        float w = Fade(z);

        int A  = p[X] + Y;      int AA = p[A] + Z;      int AB = p[A + 1] + Z;
        int B  = p[X + 1] + Y;  int BA = p[B] + Z;      int BB = p[B + 1] + Z;

        return Lerp(w, Lerp(v, Lerp(u, Grad(p[AA], x, y, z),
                                     Grad(p[BA], x - 1.0f, y, z)),
                             Lerp(u, Grad(p[AB], x, y - 1.0f, z),
                                     Grad(p[BB], x - 1.0f, y - 1.0f, z))),
                     Lerp(v, Lerp(u, Grad(p[AA + 1], x, y, z - 1.0f),
                                     Grad(p[BA + 1], x - 1.0f, y, z - 1.0f)),
                             Lerp(u, Grad(p[AB + 1], x, y - 1.0f, z - 1.0f),
                                     Grad(p[BB + 1], x - 1.0f, y - 1.0f, z - 1.0f))));
    }

    // Многооктавный шум для реалистичного ландшафта
    float FBM(float x, float y, int octaves, float persistence, float lacunarity, float scale) const {
        float total = 0.0f;
        float frequency = scale;
        float amplitude = 1.0f;
        float maxValue = 0.0f;

        for (int i = 0; i < octaves; ++i) {
            total += Noise(x * frequency, y * frequency, 0.0f) * amplitude;
            maxValue += amplitude;
            amplitude *= persistence;
            frequency *= lacunarity;
        }
        return total / maxValue;
    }
};

// ============================================================================
// SECTION 3: BIOME GENERATION SYSTEM
// ============================================================================

enum class BiomeType {
    DeadWasteland,
    RadioactiveCrater,
    ScorchedForest,
    DryCanyon
};

struct BiomeData {
    BiomeType type;
    float heightMultiplier;
    float heightOffset;
    float r, g, b; // Цвет поверхности для блендинга текстур (Vertex Color)
};

class BiomeManager {
private:
    PerlinNoise m_moistureNoise;
    PerlinNoise m_elevationNoise;

public:
    BiomeManager(uint32_t seed) : m_moistureNoise(seed + 123), m_elevationNoise(seed + 456) {}

    BiomeData GetBiomeAt(float worldX, float worldZ) const {
        // Большой скейл для плавного перехода биомов
        float moisture = m_moistureNoise.FBM(worldX, worldZ, 3, 0.5f, 2.0f, 0.0005f);
        float elevation = m_elevationNoise.FBM(worldX, worldZ, 3, 0.5f, 2.0f, 0.0005f);

        BiomeData data;

        if (elevation > 0.4f) {
            data.type = BiomeType::DryCanyon;
            data.heightMultiplier = 40.0f;
            data.heightOffset = 20.0f;
            data.r = 0.8f; data.g = 0.4f; data.b = 0.2f; // Оранжевый песок каньонов
        } else if (moisture < -0.2f && elevation < 0.0f) {
            data.type = BiomeType::RadioactiveCrater;
            data.heightMultiplier = 15.0f;
            data.heightOffset = -10.0f;
            data.r = 0.2f; data.g = 0.9f; data.b = 0.2f; // Ядовито-зеленый оттенок кратеров
        } else if (moisture > 0.3f) {
            data.type = BiomeType::ScorchedForest;
            data.heightMultiplier = 25.0f;
            data.heightOffset = 5.0f;
            data.r = 0.3f; data.g = 0.3f; data.b = 0.3f; // Пепельный цвет горелого леса
        } else {
            data.type = BiomeType::DeadWasteland;
            data.heightMultiplier = 10.0f;
            data.heightOffset = 0.0f;
            data.r = 0.6f; data.g = 0.6f; data.b = 0.5f; // Тусклый серо-желтый
        }

        return data;
    }
};

// ============================================================================
// SECTION 4: MAP SYSTEM IMPLEMENTATION & THREADING LOGIC
// ============================================================================

struct MapSystemImpl {
    uint32_t worldSeed;
    PerlinNoise terrainNoise;
    BiomeManager biomeManager;

    std::unordered_map<uint64_t, ChunkData*> activeChunks;
    std::vector<ChunkData*> chunksToUploadToGPU; // Чанки, ожидающие отправки в VRAM
    
    // Система асинхронной генерации
    std::mutex mapMutex;
    std::condition_variable workerCondVar;
    std::queue<std::pair<int, int>> generationQueue;
    std::vector<std::thread> generatorThreads;
    bool terminateWorkers;

    MapSystemImpl(uint32_t seed) 
        : worldSeed(seed), terrainNoise(seed), biomeManager(seed), terminateWorkers(false) {}
};

MapSystem* MapSystem::s_instance = nullptr;

MapSystem::MapSystem() : m_pImpl(new MapSystemImpl(20261042)) { // Хардкод сида игры Centralia
    if (s_instance) std::terminate();
    s_instance = this;

    // Запуск пула потоков для генерации ландшафта (чтобы не было фризов)
    int numThreads = std::max(2u, std::thread::hardware_concurrency() - 2);
    for (int i = 0; i < numThreads; ++i) {
        m_pImpl->generatorThreads.emplace_back(&MapSystem::WorkerThreadLoop, this);
    }

    Platform::Log("[MAP SYSTEM]: Движок стриминга ландшафта инициализирован (Seed: " + std::to_string(m_pImpl->worldSeed) + "). Воркеров: " + std::to_string(numThreads));
}

MapSystem::~MapSystem() {
    {
        std::lock_guard<std::mutex> lock(m_pImpl->mapMutex);
        m_pImpl->terminateWorkers = true;
    }
    m_pImpl->workerCondVar.notify_all();

    for (auto& t : m_pImpl->generatorThreads) {
        if (t.joinable()) t.join();
    }

    for (auto& [hash, chunk] : m_pImpl->activeChunks) {
        delete chunk;
    }
    m_pImpl->activeChunks.clear();

    delete m_pImpl;
    s_instance = nullptr;
    Platform::Log("[MAP SYSTEM]: Память ландшафта очищена. Воркеры остановлены.");
}

MapSystem& MapSystem::GetInstance() {
    if (!s_instance) std::terminate();
    return *s_instance;
}

// ============================================================================
// SECTION 5: CHUNK STREAMING (LOAD / UNLOAD TICK)
// ============================================================================

void MapSystem::UpdateTick(const Vector3D& playerPosition) {
    // Вычисляем координаты центрального чанка, в котором находится игрок
    int currentGridX = static_cast<int>(std::floor(playerPosition.x / MapConfig::CHUNK_SIZE));
    int currentGridZ = static_cast<int>(std::floor(playerPosition.z / MapConfig::CHUNK_SIZE));

    std::vector<uint64_t> chunksToKeep;
    
    // 1. Определение чанков, которые должны быть загружены
    for (int x = -MapConfig::RENDER_DISTANCE; x <= MapConfig::RENDER_DISTANCE; ++x) {
        for (int z = -MapConfig::RENDER_DISTANCE; z <= MapConfig::RENDER_DISTANCE; ++z) {
            // Загружаем по кругу, а не квадратом
            if (x * x + z * z <= MapConfig::RENDER_DISTANCE * MapConfig::RENDER_DISTANCE) {
                int targetX = currentGridX + x;
                int targetZ = currentGridZ + z;
                uint64_t hash = MapConfig::GetChunkHash(targetX, targetZ);
                chunksToKeep.push_back(hash);

                std::lock_guard<std::mutex> lock(m_pImpl->mapMutex);
                if (m_pImpl->activeChunks.find(hash) == m_pImpl->activeChunks.end()) {
                    // Чанка нет, создаем пустышку и кидаем в очередь на генерацию
                    ChunkData* newChunk = new ChunkData();
                    newChunk->gridX = targetX;
                    newChunk->gridZ = targetZ;
                    newChunk->isGenerated = false;
                    newChunk->isMeshReady = false;
                    newChunk->isModified = false;
                    m_pImpl->activeChunks[hash] = newChunk;
                    
                    m_pImpl->generationQueue.push({targetX, targetZ});
                    m_pImpl->workerCondVar.notify_one();
                }
            }
        }
    }

    // 2. Выгрузка чанков, ушедших за горизонт (UNLOAD_DISTANCE)
    std::vector<uint64_t> chunksToRemove;
    {
        std::lock_guard<std::mutex> lock(m_pImpl->mapMutex);
        for (auto& [hash, chunk] : m_pImpl->activeChunks) {
            int dx = chunk->gridX - currentGridX;
            int dz = chunk->gridZ - currentGridZ;
            
            if (dx * dx + dz * dz > MapConfig::UNLOAD_DISTANCE * MapConfig::UNLOAD_DISTANCE) {
                chunksToRemove.push_back(hash);
            }
        }

        for (uint64_t hash : chunksToRemove) {
            ChunkData* chunk = m_pImpl->activeChunks[hash];
            if (chunk->isModified) {
                SaveChunkToDisk(chunk); // Сохраняем изменения перед выгрузкой
            }
            
            // Заглушка для освобождения VRAM (в реальном рендерере: glDeleteBuffers)
            // Renderer3D::FreeHardwareBuffers(chunk->vaoId, chunk->vboId, chunk->iboId);

            delete chunk;
            m_pImpl->activeChunks.erase(hash);
        }
    }

    // 3. Синхронизация готовых чанков с GPU (выполняется строго в Main Thread)
    {
        std::lock_guard<std::mutex> lock(m_pImpl->mapMutex);
        for (auto it = m_pImpl->chunksToUploadToGPU.begin(); it != m_pImpl->chunksToUploadToGPU.end();) {
            ChunkData* chunk = *it;
            // Renderer3D::UploadMeshToGPU(chunk->vertices, chunk->indices, &chunk->vaoId, &chunk->vboId, &chunk->iboId);
            chunk->isMeshReady = true;
            it = m_pImpl->chunksToUploadToGPU.erase(it);
        }
    }
}

// ============================================================================
// SECTION 6: ASYNCHRONOUS PROCEDURAL GENERATION (WORKER THREADS)
// ============================================================================

void MapSystem::WorkerThreadLoop() {
    while (true) {
        std::pair<int, int> coords;
        {
            std::unique_lock<std::mutex> lock(m_pImpl->mapMutex);
            m_pImpl->workerCondVar.wait(lock, [this]() {
                return m_pImpl->terminateWorkers || !m_pImpl->generationQueue.empty();
            });

            if (m_pImpl->terminateWorkers && m_pImpl->generationQueue.empty()) {
                return;
            }

            coords = m_pImpl->generationQueue.front();
            m_pImpl->generationQueue.pop();
        }

        GenerateChunkData(coords.first, coords.second);
    }
}

void MapSystem::GenerateChunkData(int gridX, int gridZ) {
    uint64_t hash = MapConfig::GetChunkHash(gridX, gridZ);
    
    // Проверяем, есть ли сохраненный на диске файл с измененным чанком
    // if (LoadChunkFromDisk(gridX, gridZ)) return;

    // Временные буферы для сборки чанка
    std::vector<TerrainVertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<PropInstance> props;

    int vertsPerRow = MapConfig::CHUNK_SIZE + 1;
    vertices.reserve(vertsPerRow * vertsPerRow);
    indices.reserve(MapConfig::CHUNK_SIZE * MapConfig::CHUNK_SIZE * 6);

    float worldOffsetX = gridX * MapConfig::CHUNK_SIZE;
    float worldOffsetZ = gridZ * MapConfig::CHUNK_SIZE;

    // 1. ГЕНЕРАЦИЯ ВЕРШИН И КАРТЫ ВЫСОТ
    for (int z = 0; z <= MapConfig::CHUNK_SIZE; ++z) {
        for (int x = 0; x <= MapConfig::CHUNK_SIZE; ++x) {
            float wX = worldOffsetX + (x * MapConfig::VERTEX_SPACING);
            float wZ = worldOffsetZ + (z * MapConfig::VERTEX_SPACING);

            BiomeData biome = m_pImpl->biomeManager.GetBiomeAt(wX, wZ);
            
            // Вычисление высоты через шум Перлина
            float rawNoise = m_pImpl->terrainNoise.FBM(wX, wZ, 4, 0.45f, 2.2f, 0.005f);
            float heightY = biome.heightOffset + (rawNoise * biome.heightMultiplier);

            // Создание вершины
            TerrainVertex v;
            v.x = wX;
            v.y = heightY;
            v.z = wZ;
            v.u = static_cast<float>(x) / MapConfig::CHUNK_SIZE;
            v.v = static_cast<float>(z) / MapConfig::CHUNK_SIZE;
            v.r = biome.r; v.g = biome.g; v.b = biome.b; v.a = 1.0f; // Vertex Color
            
            // Нормали и тангенсы считаем позже
            v.nx = 0.0f; v.ny = 1.0f; v.nz = 0.0f;
            v.tx = 1.0f; v.ty = 0.0f; v.tz = 0.0f; v.tw = 1.0f;

            vertices.push_back(v);
            
            // 2. ПРОЦЕДУРНАЯ РАССТАНОВКА ПРОПОВ (Деревья, камни)
            // Расставляем пропы только не на самых краях чанка (чтобы избежать артефактов на стыках)
            if (x > 2 && x < MapConfig::CHUNK_SIZE - 2 && z > 2 && z < MapConfig::CHUNK_SIZE - 2) {
                // Псевдослучайный ролл на основе координат
                float propRoll = m_pImpl->terrainNoise.Noise(wX * 50.3f, 0.0f, wZ * 50.3f);
                
                if (biome.type == BiomeType::ScorchedForest && propRoll > 0.85f) {
                    // Спавн сгоревшего дерева
                    PropInstance tree;
                    tree.modelId = 9001; // ID модели мертвого дерева
                    tree.x = wX; tree.y = heightY; tree.z = wZ;
                    tree.rotX = 0; tree.rotY = propRoll * 360.0f; tree.rotZ = 0; // Случайный поворот
                    tree.scale = 0.8f + (propRoll * 0.4f);
                    props.push_back(tree);
                } 
                else if (biome.type == BiomeType::DeadWasteland && propRoll > 0.95f) {
                    // Спавн ржавого остова машины
                    PropInstance car;
                    car.modelId = 9005; 
                    car.x = wX; car.y = heightY; car.z = wZ;
                    car.rotX = 0; car.rotY = propRoll * 360.0f; car.rotZ = 0;
                    car.scale = 1.0f;
                    props.push_back(car);
                }
            }
        }
    }

    // 3. ГЕНЕРАЦИЯ ИНДЕКСОВ ДЛЯ ТРИАНГУЛЯЦИИ (Quads -> Triangles)
    for (int z = 0; z < MapConfig::CHUNK_SIZE; ++z) {
        for (int x = 0; x < MapConfig::CHUNK_SIZE; ++x) {
            uint32_t topLeft = z * vertsPerRow + x;
            uint32_t topRight = topLeft + 1;
            uint32_t bottomLeft = (z + 1) * vertsPerRow + x;
            uint32_t bottomRight = bottomLeft + 1;

            // Треугольник 1
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            // Треугольник 2
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }

    // 4. РАСЧЕТ НОРМАЛЕЙ И ТАНГЕНСОВ (Освещение)
    for (size_t i = 0; i < indices.size(); i += 3) {
        uint32_t i0 = indices[i];
        uint32_t i1 = indices[i + 1];
        uint32_t i2 = indices[i + 2];

        TerrainVertex& v0 = vertices[i0];
        TerrainVertex& v1 = vertices[i1];
        TerrainVertex& v2 = vertices[i2];

        Vector3D edge1(v1.x - v0.x, v1.y - v0.y, v1.z - v0.z);
        Vector3D edge2(v2.x - v0.x, v2.y - v0.y, v2.z - v0.z);
        Vector3D normal = edge1.Cross(edge2).Normalized();

        v0.nx += normal.x; v0.ny += normal.y; v0.nz += normal.z;
        v1.nx += normal.x; v1.ny += normal.y; v1.nz += normal.z;
        v2.nx += normal.x; v2.ny += normal.y; v2.nz += normal.z;
    }

    // Нормализация сглаженных нормалей
    for (auto& v : vertices) {
        float len = std::sqrt(v.nx * v.nx + v.ny * v.ny + v.nz * v.nz);
        if (len > 0.0f) { v.nx /= len; v.ny /= len; v.nz /= len; }
    }

    // 5. ПРИМЕНЕНИЕ ДАННЫХ К СТРУКТУРЕ ЧАНКА
    {
        std::lock_guard<std::mutex> lock(m_pImpl->mapMutex);
        auto it = m_pImpl->activeChunks.find(hash);
        if (it != m_pImpl->activeChunks.end()) {
            ChunkData* chunk = it->second;
            chunk->vertices = std::move(vertices);
            chunk->indices = std::move(indices);
            chunk->props = std::move(props);
            chunk->isGenerated = true;
            
            // Ставим в очередь на заливку в VRAM (Main Thread)
            m_pImpl->chunksToUploadToGPU.push_back(chunk);
        }
    }
}

// ============================================================================
// SECTION 7: PHYSICS & TERRAIN QUERY API
// ============================================================================

float MapSystem::GetHeightAt(float worldX, float worldZ) const {
    int gridX = static_cast<int>(std::floor(worldX / MapConfig::CHUNK_SIZE));
    int gridZ = static_cast<int>(std::floor(worldZ / MapConfig::CHUNK_SIZE));
    uint64_t hash = MapConfig::GetChunkHash(gridX, gridZ);

    std::lock_guard<std::mutex> lock(m_pImpl->mapMutex);
    auto it = m_pImpl->activeChunks.find(hash);
    
    if (it != m_pImpl->activeChunks.end() && it->second->isGenerated) {
        ChunkData* chunk = it->second;
        
        // Локальные координаты внутри чанка
        float localX = worldX - (gridX * MapConfig::CHUNK_SIZE);
        float localZ = worldZ - (gridZ * MapConfig::CHUNK_SIZE);
        
        int cellX = static_cast<int>(std::floor(localX));
        int cellZ = static_cast<int>(std::floor(localZ));
        
        if (cellX < 0 || cellX >= MapConfig::CHUNK_SIZE || cellZ < 0 || cellZ >= MapConfig::CHUNK_SIZE) {
            return 0.0f; // Edge case protection
        }

        // Билинейная интерполяция высоты внутри квадрата (Quad) сетки
        int vertsPerRow = MapConfig::CHUNK_SIZE + 1;
        float h00 = chunk->vertices[cellZ * vertsPerRow + cellX].y;
        float h10 = chunk->vertices[cellZ * vertsPerRow + (cellX + 1)].y;
        float h01 = chunk->vertices[(cellZ + 1) * vertsPerRow + cellX].y;
        float h11 = chunk->vertices[(cellZ + 1) * vertsPerRow + (cellX + 1)].y;

        float tx = localX - cellX;
        float tz = localZ - cellZ;

        // Треугольная интерполяция (Барицентрические координаты)
        if (tx <= (1.0f - tz)) {
            // Верхний треугольник
            return h00 + (h10 - h00) * tx + (h01 - h00) * tz;
        } else {
            // Нижний треугольник
            return h11 + (h01 - h11) * (1.0f - tx) + (h10 - h11) * (1.0f - tz);
        }
    }

    // Fallback, если чанк еще не сгенерирован (чтобы игрок не провалился в пустоту)
    BiomeData biome = m_pImpl->biomeManager.GetBiomeAt(worldX, worldZ);
    float rawNoise = m_pImpl->terrainNoise.FBM(worldX, worldZ, 4, 0.45f, 2.2f, 0.005f);
    return biome.heightOffset + (rawNoise * biome.heightMultiplier);
}

// ============================================================================
// SECTION 8: CHUNK SERIALIZATION (PERSISTENT WORLD CHANGES)
// ============================================================================

void MapSystem::SaveChunkToDisk(const ChunkData* chunk) const {
    if (!chunk || !chunk->isModified) return;

    std::string filename = "saves/world/chunk_" + std::to_string(chunk->gridX) + "_" + std::to_string(chunk->gridZ) + ".cdat";
    std::ofstream outFile(filename, std::ios::binary | std::ios::trunc);
    if (!outFile.is_open()) return;

    // Заголовок
    uint32_t magic = MapConfig::MAP_SAVE_MAGIC;
    outFile.write(reinterpret_cast<const char*>(&magic), sizeof(uint32_t));
    outFile.write(reinterpret_cast<const char*>(&chunk->gridX), sizeof(int));
    outFile.write(reinterpret_cast<const char*>(&chunk->gridZ), sizeof(int));

    // Вершины
    uint32_t vCount = static_cast<uint32_t>(chunk->vertices.size());
    outFile.write(reinterpret_cast<const char*>(&vCount), sizeof(uint32_t));
    outFile.write(reinterpret_cast<const char*>(chunk->vertices.data()), vCount * sizeof(TerrainVertex));

    // Пропы
    uint32_t pCount = static_cast<uint32_t>(chunk->props.size());
    outFile.write(reinterpret_cast<const char*>(&pCount), sizeof(uint32_t));
    outFile.write(reinterpret_cast<const char*>(chunk->props.data()), pCount * sizeof(PropInstance));

    outFile.close();
    Platform::Log("[MAP SYSTEM]: Модифицированный чанк [" + std::to_string(chunk->gridX) + ", " + std::to_string(chunk->gridZ) + "] сохранен.");
}

} // namespace Centralia