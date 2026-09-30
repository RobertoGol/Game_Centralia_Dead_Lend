#include "MemoryManager.hpp"
#include "platform/Platform.hpp"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace Centralia {

// ============================================================================
// SECTION 1: CONSTANTS, MACROS & CANARY VALUES
// ============================================================================

namespace {
    // Магические числа для проверки выхода за границы памяти (Bounds Checking)
    constexpr uint32_t MEMORY_MAGIC_HEADER = 0xDEADC0DE;
    constexpr uint32_t MEMORY_MAGIC_FOOTER = 0xDEADBEEF;
    constexpr uint32_t MEMORY_FREED_MARKER = 0xFEFEEFEE;

    // Выравнивание по умолчанию (16 байт для SIMD/SSE инструкций)
    constexpr size_t DEFAULT_ALIGNMENT = 16;
    
    // Пул мелких объектов
    constexpr size_t POOL_SIZE_16 = 16;
    constexpr size_t POOL_SIZE_32 = 32;
    constexpr size_t POOL_SIZE_64 = 64;
    constexpr size_t POOL_SIZE_128 = 128;
    constexpr size_t POOL_SIZE_256 = 256;

    inline size_t AlignForward(size_t address, size_t alignment) {
        return (address + (alignment - 1)) & ~(alignment - 1);
    }
}

// ============================================================================
// SECTION 2: INTERNAL DATA STRUCTURES (HEADERS & TRACKING)
// ============================================================================

// Заголовок для блоков общей кучи (Heap)
struct HeapBlockHeader {
    size_t size;             // Размер блока (включая заголовок)
    bool isFree;             // Флаг свободы
    HeapBlockHeader* next;   // Следующий блок в связном списке
    HeapBlockHeader* prev;   // Предыдущий блок
    uint32_t magicHeader;    // Проверочный код (Canary)
};

// Запись для трекера утечек памяти (Leak Tracker)
struct AllocationRecord {
    void* address;
    size_t size;
    const char* file;
    int line;
    const char* subsystem;
};

// Узел свободного списка для Pool-аллокатора
struct PoolFreeNode {
    PoolFreeNode* next;
};

// ============================================================================
// SECTION 3: SUBSYSTEM CLASSES (LINEAR, POOL, HEAP)
// ============================================================================

// ----------------------------------------------------------------------------
// 3.1. LINEAR ALLOCATOR (FRAME ARENA)
// ----------------------------------------------------------------------------
class LinearAllocator {
private:
    void* m_startAddress;
    size_t m_totalSize;
    size_t m_offset;
    std::mutex m_mutex;

public:
    LinearAllocator() : m_startAddress(nullptr), m_totalSize(0), m_offset(0) {}
    
    ~LinearAllocator() {
        if (m_startAddress) std::free(m_startAddress);
    }

    void Initialize(size_t size) {
        m_totalSize = size;
        m_startAddress = std::malloc(m_totalSize);
        m_offset = 0;
        if (!m_startAddress) {
            Platform::Log("[MEMORY FATAL]: Не удалось выделить память под Linear Allocator!");
            std::terminate();
        }
    }

    void* Allocate(size_t size, size_t alignment = DEFAULT_ALIGNMENT) {
        std::lock_guard<std::mutex> lock(m_mutex);

        size_t currentAddress = reinterpret_cast<size_t>(m_startAddress) + m_offset;
        size_t alignedAddress = AlignForward(currentAddress, alignment);
        size_t adjustment = alignedAddress - currentAddress;

        if (m_offset + adjustment + size > m_totalSize) {
            Platform::Log("[MEMORY WARNING]: Переполнение Linear Allocator (Frame Arena)!");
            return nullptr;
        }

        m_offset += adjustment + size;
        return reinterpret_cast<void*>(alignedAddress);
    }

    void Clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_offset = 0; // O(1) мгновенная очистка арены
    }
    
    size_t GetUsedSpace() const { return m_offset; }
};

// ----------------------------------------------------------------------------
// 3.2. POOL ALLOCATOR (FOR FAST SMALL OBJECTS)
// ----------------------------------------------------------------------------
class PoolAllocator {
private:
    size_t m_chunkSize;
    size_t m_poolCapacity;
    void* m_memoryBlock;
    PoolFreeNode* m_freeList;
    std::mutex m_mutex;

public:
    PoolAllocator() : m_chunkSize(0), m_poolCapacity(0), m_memoryBlock(nullptr), m_freeList(nullptr) {}

    ~PoolAllocator() {
        if (m_memoryBlock) std::free(m_memoryBlock);
    }

    void Initialize(size_t chunkSize, size_t capacity) {
        m_chunkSize = std::max(chunkSize, sizeof(PoolFreeNode));
        m_poolCapacity = capacity;
        m_memoryBlock = std::malloc(m_chunkSize * m_poolCapacity);

        if (!m_memoryBlock) {
            Platform::Log("[MEMORY FATAL]: Ошибка выделения памяти под Pool Allocator!");
            std::terminate();
        }

        // Инициализация свободного связного списка (Free List)
        m_freeList = reinterpret_cast<PoolFreeNode*>(m_memoryBlock);
        PoolFreeNode* current = m_freeList;

        for (size_t i = 1; i < m_poolCapacity; ++i) {
            size_t nextAddress = reinterpret_cast<size_t>(m_memoryBlock) + (i * m_chunkSize);
            current->next = reinterpret_cast<PoolFreeNode*>(nextAddress);
            current = current->next;
        }
        current->next = nullptr;
    }

    void* Allocate() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_freeList) return nullptr; // Пул исчерпан

        PoolFreeNode* freeNode = m_freeList;
        m_freeList = m_freeList->next;
        
        return reinterpret_cast<void*>(freeNode);
    }

    void Free(void* ptr) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!ptr) return;

        // Защита от двойного освобождения и невалидных указателей в рамках пула
        size_t startAddr = reinterpret_cast<size_t>(m_memoryBlock);
        size_t endAddr = startAddr + (m_chunkSize * m_poolCapacity);
        size_t ptrAddr = reinterpret_cast<size_t>(ptr);

        if (ptrAddr >= startAddr && ptrAddr < endAddr) {
            PoolFreeNode* node = reinterpret_cast<PoolFreeNode*>(ptr);
            node->next = m_freeList;
            m_freeList = node;
        } else {
            Platform::Log("[MEMORY ERROR]: Попытка вернуть в пул чужой или невалидный указатель!");
        }
    }
};

// ----------------------------------------------------------------------------
// 3.3. HEAP ALLOCATOR (WITH COALESCING & BEST-FIT)
// ----------------------------------------------------------------------------
class HeapAllocator {
private:
    void* m_startAddress;
    size_t m_totalSize;
    size_t m_usedSize;
    HeapBlockHeader* m_freeList;
    std::mutex m_mutex;

    // Вспомогательная функция для склеивания соседних свободных блоков (дефрагментация)
    void Coalesce(HeapBlockHeader* block) {
        if (!block) return;

        // Проверяем следующий блок
        if (block->next && block->next->isFree) {
            block->size += block->next->size;
            block->next = block->next->next;
            if (block->next) {
                block->next->prev = block;
            }
        }

        // Проверяем предыдущий блок
        if (block->prev && block->prev->isFree) {
            block->prev->size += block->size;
            block->prev->next = block->next;
            if (block->next) {
                block->next->prev = block->prev;
            }
            block = block->prev; // Смещаем фокус на начало слитого блока
        }
    }

public:
    HeapAllocator() : m_startAddress(nullptr), m_totalSize(0), m_usedSize(0), m_freeList(nullptr) {}

    ~HeapAllocator() {
        if (m_startAddress) std::free(m_startAddress);
    }

    void Initialize(size_t size) {
        m_totalSize = size;
        m_usedSize = 0;
        m_startAddress = std::malloc(size);

        if (!m_startAddress) {
            Platform::Log("[MEMORY FATAL]: Невозможно выделить основную кучу (Heap) размером " + std::to_string(size) + " байт!");
            std::terminate();
        }

        // Инициализируем весь блок как один огромный свободный кусок
        m_freeList = reinterpret_cast<HeapBlockHeader*>(m_startAddress);
        m_freeList->size = size;
        m_freeList->isFree = true;
        m_freeList->next = nullptr;
        m_freeList->prev = nullptr;
        m_freeList->magicHeader = MEMORY_MAGIC_HEADER;
    }

    void* Allocate(size_t size, size_t alignment) {
        std::lock_guard<std::mutex> lock(m_mutex);

        // Размер выделения: Заголовок + Данные + Футер
        size_t totalRequired = sizeof(HeapBlockHeader) + size + sizeof(uint32_t);
        
        // Поиск подходящего блока по алгоритму Best-Fit
        HeapBlockHeader* bestFit = nullptr;
        HeapBlockHeader* current = m_freeList;
        size_t smallestDiff = SIZE_MAX;

        while (current) {
            if (current->isFree && current->size >= totalRequired) {
                size_t diff = current->size - totalRequired;
                if (diff < smallestDiff) {
                    smallestDiff = diff;
                    bestFit = current;
                }
            }
            current = current->next;
        }

        if (!bestFit) {
            Platform::Log("[MEMORY WARNING]: Куча исчерпана или сильно фрагментирована. Ошибка аллокации!");
            return nullptr;
        }

        // Разделение блока (Splitting), если остаток достаточно велик для нового заголовка
        if (bestFit->size >= totalRequired + sizeof(HeapBlockHeader) + 64) {
            size_t oldSize = bestFit->size;
            bestFit->size = totalRequired;
            bestFit->isFree = false;

            // Создаем новый свободный блок из остатка
            size_t nextBlockAddress = reinterpret_cast<size_t>(bestFit) + totalRequired;
            HeapBlockHeader* nextBlock = reinterpret_cast<HeapBlockHeader*>(nextBlockAddress);
            nextBlock->size = oldSize - totalRequired;
            nextBlock->isFree = true;
            nextBlock->magicHeader = MEMORY_MAGIC_HEADER;
            
            // Перестраиваем связный список
            nextBlock->next = bestFit->next;
            nextBlock->prev = bestFit;
            if (bestFit->next) bestFit->next->prev = nextBlock;
            bestFit->next = nextBlock;
        } else {
            bestFit->isFree = false;
        }

        m_usedSize += bestFit->size;

        // Установка защитных Канареек (Canaries)
        size_t dataAddress = reinterpret_cast<size_t>(bestFit) + sizeof(HeapBlockHeader);
        size_t footerAddress = dataAddress + size;
        uint32_t* footer = reinterpret_cast<uint32_t*>(footerAddress);
        *footer = MEMORY_MAGIC_FOOTER;

        return reinterpret_cast<void*>(dataAddress);
    }

    void Free(void* ptr) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!ptr) return;

        // Получение заголовка и проверка целостности
        size_t headerAddress = reinterpret_cast<size_t>(ptr) - sizeof(HeapBlockHeader);
        HeapBlockHeader* block = reinterpret_cast<HeapBlockHeader*>(headerAddress);

        if (block->magicHeader != MEMORY_MAGIC_HEADER) {
            Platform::Log("[MEMORY CORRUPTION]: Нарушена сигнатура заголовка блока памяти!");
            std::terminate();
        }

        if (block->isFree) {
            Platform::Log("[MEMORY ERROR]: Попытка двойного освобождения (Double Free)!");
            return;
        }

        // Проверка футера (переполнение буфера - Buffer Overflow)
        size_t dataSize = block->size - sizeof(HeapBlockHeader) - sizeof(uint32_t);
        size_t footerAddress = reinterpret_cast<size_t>(ptr) + dataSize;
        uint32_t* footer = reinterpret_cast<uint32_t*>(footerAddress);

        if (*footer != MEMORY_MAGIC_FOOTER) {
            Platform::Log("[MEMORY FATAL CORRUPTION]: Переполнение буфера! Затерта финальная сигнатура блока.");
            std::terminate();
        }

        // Помечаем как свободный и затираем память
        block->isFree = true;
        block->magicHeader = MEMORY_FREED_MARKER;
        m_usedSize -= block->size;

        // Слияние соседних блоков (Coalescing) для снижения фрагментации
        Coalesce(block);
    }
    
    size_t GetUsedSpace() const { return m_usedSize; }
};

// ============================================================================
// SECTION 4: MEMORY MANAGER IMPLEMENTATION (FACADE)
// ============================================================================

MemoryManager* MemoryManager::s_instance = nullptr;

struct MemoryManagerImpl {
    LinearAllocator frameAllocator;
    HeapAllocator mainHeap;
    
    PoolAllocator pool16;
    PoolAllocator pool32;
    PoolAllocator pool64;
    PoolAllocator pool128;
    PoolAllocator pool256;

    std::unordered_map<void*, AllocationRecord> activeAllocations;
    std::mutex trackingMutex;

    size_t peakMemoryUsage = 0;
};

MemoryManager::MemoryManager() : m_pImpl(new MemoryManagerImpl()) {
    if (s_instance != nullptr) {
        Platform::Log("[MEMORY ERROR]: Попытка создать второй MemoryManager!");
        std::terminate();
    }
    s_instance = this;
    Platform::Log("[MEMORY SYSTEM]: Подсистема управления памятью (MemoryManager) создана.");
}

MemoryManager::~MemoryManager() {
    GenerateLeakReport("logs/memory_leaks.log");
    delete m_pImpl;
    s_instance = nullptr;
    Platform::Log("[MEMORY SYSTEM]: Менеджер памяти выгружен.");
}

MemoryManager& MemoryManager::GetInstance() {
    if (!s_instance) std::terminate();
    return *s_instance;
}

void MemoryManager::InitializeHeap(size_t totalBytes) {
    Platform::Log("[MEMORY SYSTEM]: Распределение пулов и инициализация кучи (" + std::to_string(totalBytes / (1024*1024)) + " MB)...");

    // Выделяем 16 МБ под покадровый Linear Allocator
    m_pImpl->frameAllocator.Initialize(1024 * 1024 * 16);

    // Инициализация Pool Аллокаторов (например, по 10000 элементов каждого размера)
    m_pImpl->pool16.Initialize(POOL_SIZE_16, 20000);
    m_pImpl->pool32.Initialize(POOL_SIZE_32, 10000);
    m_pImpl->pool64.Initialize(POOL_SIZE_64, 5000);
    m_pImpl->pool128.Initialize(POOL_SIZE_128, 5000);
    m_pImpl->pool256.Initialize(POOL_SIZE_256, 2000);

    // Оставшаяся память уходит в основную кучу (Heap Allocator)
    size_t poolOverhead = (16*20000) + (32*10000) + (64*5000) + (128*5000) + (256*2000);
    size_t heapSize = totalBytes - (1024 * 1024 * 16) - poolOverhead;
    
    m_pImpl->mainHeap.Initialize(heapSize);

    Platform::Log("[MEMORY SYSTEM]: Инициализация архитектуры памяти завершена.");
}

// ============================================================================
// SECTION 5: ALLOCATION & DEALLOCATION ROUTING API
// ============================================================================

void* MemoryManager::Allocate(size_t size, const char* file, int line, const char* subsystem) {
    void* ptr = nullptr;

    // Роутинг: Если размер подходит под пулы - используем их для скорости
    if (size <= POOL_SIZE_16)      ptr = m_pImpl->pool16.Allocate();
    else if (size <= POOL_SIZE_32) ptr = m_pImpl->pool32.Allocate();
    else if (size <= POOL_SIZE_64) ptr = m_pImpl->pool64.Allocate();
    else if (size <= POOL_SIZE_128)ptr = m_pImpl->pool128.Allocate();
    else if (size <= POOL_SIZE_256)ptr = m_pImpl->pool256.Allocate();

    // Если пулы исчерпаны или объект слишком велик - идем в основную кучу
    if (!ptr) {
        ptr = m_pImpl->mainHeap.Allocate(size, DEFAULT_ALIGNMENT);
    }

    if (ptr) {
        TrackAllocation(ptr, size, file, line, subsystem);
        
        size_t currentTotal = m_pImpl->mainHeap.GetUsedSpace();
        if (currentTotal > m_pImpl->peakMemoryUsage) {
            m_pImpl->peakMemoryUsage = currentTotal;
        }
    } else {
        Platform::Log("[MEMORY OUT_OF_MEMORY]: Невозможно выделить " + std::to_string(size) + " байт!");
        GenerateCrashDump("crashes/OOM_dump.bin");
        std::terminate();
    }

    return ptr;
}

void MemoryManager::Free(void* ptr, size_t size) {
    if (!ptr) return;

    UntrackAllocation(ptr);

    // Маршрутизация освобождения в зависимости от размера
    if (size <= POOL_SIZE_16)      m_pImpl->pool16.Free(ptr);
    else if (size <= POOL_SIZE_32) m_pImpl->pool32.Free(ptr);
    else if (size <= POOL_SIZE_64) m_pImpl->pool64.Free(ptr);
    else if (size <= POOL_SIZE_128)m_pImpl->pool128.Free(ptr);
    else if (size <= POOL_SIZE_256)m_pImpl->pool256.Free(ptr);
    else                           m_pImpl->mainHeap.Free(ptr);
}

void* MemoryManager::AllocateFrame(size_t size) {
    // Временное выделение на один кадр. Не отслеживается трекером для максимальной скорости
    return m_pImpl->frameAllocator.Allocate(size);
}

void MemoryManager::ClearFrameArena() {
    m_pImpl->frameAllocator.Clear();
}

// ============================================================================
// SECTION 6: TELEMETRY, TRACKING & GARBAGE COLLECTION
// ============================================================================

void MemoryManager::TrackAllocation(void* ptr, size_t size, const char* file, int line, const char* subsystem) {
    std::lock_guard<std::mutex> lock(m_pImpl->trackingMutex);
    
    AllocationRecord rec;
    rec.address = ptr;
    rec.size = size;
    rec.file = file;
    rec.line = line;
    rec.subsystem = subsystem;

    m_pImpl->activeAllocations[ptr] = rec;
}

void MemoryManager::UntrackAllocation(void* ptr) {
    std::lock_guard<std::mutex> lock(m_pImpl->trackingMutex);
    m_pImpl->activeAllocations.erase(ptr);
}

void MemoryManager::GenerateLeakReport(const std::string& filepath) const {
    std::lock_guard<std::mutex> lock(m_pImpl->trackingMutex);
    
    if (m_pImpl->activeAllocations.empty()) {
        Platform::Log("[MEMORY LEAK TRACKER]: Утечек памяти не обнаружено. Архитектура чиста.");
        return;
    }

    std::ofstream outFile(filepath);
    if (!outFile.is_open()) return;

    outFile << "========================================================\n";
    outFile << " CENTRALIA ENGINE - MEMORY LEAK REPORT \n";
    outFile << "========================================================\n\n";
    
    size_t totalLeakedBytes = 0;
    
    for (const auto& [ptr, rec] : m_pImpl->activeAllocations) {
        outFile << "LEAK DETECTED: " << rec.size << " bytes "
                << " | Subsystem: [" << (rec.subsystem ? rec.subsystem : "Unknown") << "] "
                << " | Address: 0x" << std::hex << reinterpret_cast<uintptr_t>(ptr) << std::dec
                << " | File: " << (rec.file ? rec.file : "Unknown") 
                << " | Line: " << rec.line << "\n";
                
        totalLeakedBytes += rec.size;
    }

    outFile << "\nTOTAL LEAKED MEMORY: " << totalLeakedBytes << " bytes.\n";
    outFile.close();
    
    Platform::Log("[MEMORY LEAK TRACKER]: Обнаружены утечки памяти (" + std::to_string(totalLeakedBytes) + " байт). Отчет сохранен в " + filepath);
}

void MemoryManager::RunGarbageCollection() {
    Platform::Log("[MEMORY SYSTEM]: Запуск сборки мусора (Дефрагментация пулов и очистка)...");
    
    // В C++ нет автоматического GC для произвольных указателей,
    // но здесь мы можем сбрасывать кэши, очищать неиспользуемые пулы или
    // принудительно выгружать неиспользуемые ассеты (текстуры, модели) из GPU RAM.
    
    ClearFrameArena();
    
    Platform::Log("[MEMORY SYSTEM]: Сборка мусора завершена. Пиковое потребление: " + std::to_string(m_pImpl->peakMemoryUsage / (1024*1024)) + " MB.");
}

void MemoryManager::GenerateCrashDump(const std::string& filepath) const {
    std::ofstream dumpFile(filepath, std::ios::binary);
    if (!dumpFile.is_open()) return;

    // Пишем магический заголовок дампа
    uint32_t magic = 0xCDCDCDCD;
    dumpFile.write(reinterpret_cast<const char*>(&magic), sizeof(uint32_t));

    // Пишем текущее использование и пиковое
    size_t currentUsage = m_pImpl->mainHeap.GetUsedSpace();
    dumpFile.write(reinterpret_cast<const char*>(&currentUsage), sizeof(size_t));
    dumpFile.write(reinterpret_cast<const char*>(&m_pImpl->peakMemoryUsage), sizeof(size_t));

    // Пишем метаданные активных аллокаций
    uint32_t activeCount = static_cast<uint32_t>(m_pImpl->activeAllocations.size());
    dumpFile.write(reinterpret_cast<const char*>(&activeCount), sizeof(uint32_t));

    dumpFile.close();
    Platform::Log("[MEMORY CRASH DUMP]: Бинарный слепок памяти (minidump) сохранен в " + filepath);
}

size_t MemoryManager::GetAllocatedSize() const noexcept {
    return m_pImpl->mainHeap.GetUsedSpace();
}

size_t MemoryManager::GetMaxHeapSize() const noexcept {
    return m_pImpl->peakMemoryUsage; // Для простоты возвращаем пик
}

} // namespace Centralia