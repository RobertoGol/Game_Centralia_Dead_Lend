#pragma once
#include "core/MemoryManager.hpp"

namespace Centralia {

class Engine {
private:
    bool m_isRunning;
    MemoryManager m_memoryManager; // Менеджер защищенной памяти и скриптов решений

public:
    Engine();
    ~Engine();

    bool Start();
    void Update();
    void Render();
    void Stop();

    bool IsRunning() const { return m_isRunning; }
};

} // namespace Centralia
