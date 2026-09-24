#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <string_view>

namespace Centralia {

// Побайтовое выравнивание структур заголовка для прямой записи памяти на диск
#pragma pack(push, 1)

struct OGGXHeader {
    uint8_t  magic[4];       // "OGGX" - Сигнатура кастомного формата сжатия ресурса
    uint32_t version;        // Версия формата (например, 1)
    uint32_t fileCount;      // Общее количество файлов внутри контейнера
    uint32_t reservedBuffer; // Резервный слой под будущие расширения/физику
};

struct OGGXFileEntry {
    char     filePath[128];  // Относительный путь в Data/Ingame/mods/...
    uint64_t fileOffset;     // Смещение начала данных файла относительно начала архива
    uint64_t fileSize;       // Чистый бинарный вес файла на диске
    uint32_t crc32Checksum;  // Контрольная сумма CRC32 для валидации целостности
};

#pragma pack(pop)

class ResourcePackerOGGX {
private:
    // Классический быстрый алгоритм CRC32 (табличный метод)
    static uint32_t CalculateCRC32(const std::vector<uint8_t>& data) noexcept;
    
public:
    ResourcePackerOGGX() = default;
    ~ResourcePackerOGGX() = default;

    /**
     * @brief Создает плотный ogg-подобный бинарный контейнер .oggx из списка файлов.
     * @param inputFiles - Вектор путей к исходным файлам модов (config.txt, texture.bmp, mesh.obj)
     * @param outputPackPath - Путь к финальному упакованному архиву .oggx
     * @return true, если упаковка прошла успешно без сбоев ввода-вывода
     */
    static bool PackResources(const std::vector<std::string>& inputFiles, const std::string& outputPackPath);

    /**
     * @brief Быстрая проверка целостности контейнера на движке перед парсингом ресурсов.
     * @param packPath - Путь к проверяемому файлу .oggx
     * @return true, если все контрольные суммы CRC32 внутри совпали
     */
    static bool ValidatePackIntegrity(const std::string& packPath);
};

} // namespace Centralia
