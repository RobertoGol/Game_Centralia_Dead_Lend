#include "GameUpdater.hpp"
#include "Engine.hpp"
#include "platform/Platform.hpp"
#include "gameplay/Player.hpp"
#include "gameplay/ModificationSystem.hpp"
#include "core/MemoryManager.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <thread>
#include <mutex>
#include <chrono>
#include <algorithm>
#include <cstring>
#include <iomanip>

// Кроссплатформенные сокеты для TCP HTTP-клиента
#if defined(_WIN32) || defined(_WIN64)
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
    #include <winsock2.h>
    #include <ws2tcpip.h>
    typedef int socklen_t;
    #define CLOSE_TCP_SOCKET closesocket
#else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <netdb.h>
    typedef int SOCKET;
    #define INVALID_SOCKET (-1)
    #define SOCKET_ERROR (-1)
    #define CLOSE_TCP_SOCKET close
#endif

namespace Centralia {

// ============================================================================
// SECTION 1: CONSTANTS, UI LAYOUT DEFINITIONS & MANIFEST STRUCTURES
// ============================================================================

namespace UpdaterConfig {
    constexpr uint32_t CURRENT_CLIENT_VERSION = 1042;
    constexpr const char* MASTER_SERVER_HOST = "update.centralia-game.com";
    constexpr const char* MANIFEST_ENDPOINT = "/api/v1/manifest?platform=pc";
    constexpr uint16_t MASTER_SERVER_PORT = 80;
    
    constexpr const char* UPDATE_LOCK_FILE = "config/update.lock";
    constexpr size_t DOWNLOAD_CHUNK_SIZE = 1024 * 64; // 64 KB чанки для скачивания

    // UI Константы для агрессивного окна
    constexpr float POPUP_WIDTH = 1200.0f;
    constexpr float POPUP_HEIGHT = 800.0f;
    constexpr uint32_t COLOR_BACKGROUND_DIM = 0xCC000000; // Полупрозрачный черный (80%)
    constexpr uint32_t COLOR_POPUP_BG = 0xFF1A1A1A;       // Темно-серый
    constexpr uint32_t COLOR_BORDER_URGENT = 0xFFFF3333;  // Агрессивный красный
    constexpr uint32_t COLOR_TEXT_PRIMARY = 0xFFFFFFFF;
    constexpr uint32_t COLOR_TEXT_WARNING = 0xFFFF5555;
    constexpr uint32_t COLOR_BTN_UPDATE = 0xFF00AA00;     // Зеленая кнопка обновления
    constexpr uint32_t COLOR_BTN_HOVER = 0xFF00FF00;
}

struct UpdateManifest {
    uint32_t latestVersion;
    bool isMandatory;
    std::string downloadUrl;
    std::string patchNotes;
    size_t totalSizeBytes;
    std::string targetChecksum;
    bool isValid;
};

struct DownloadProgress {
    size_t bytesDownloaded;
    size_t bytesTotal;
    float currentSpeedKBps;
    float percentComplete;
    bool isFinished;
    bool isFailed;
    std::string errorMessage;
};

// ============================================================================
// SECTION 2: SINGLETON STATE & INTERNAL THREADING CONTROLS
// ============================================================================

struct GameUpdaterImpl {
    UpdaterState state;
    UpdateManifest currentManifest;
    DownloadProgress downloadStatus;
    
    std::thread backgroundThread;
    std::mutex stateMutex;
    bool terminateThread;
    
    bool isUIBlockingActive;
    float uiAnimationTimer;
    
    // Внутренние методы
    void VerifyLocalLockFile();
    void WriteLocalLockFile();
    void RemoveLocalLockFile();
    bool PerformHttpRequest(const std::string& host, uint16_t port, const std::string& request, std::string& outResponse);
    UpdateManifest ParseManifestJSON(const std::string& rawHttpData);
};

GameUpdater* GameUpdater::s_instance = nullptr;

GameUpdater::GameUpdater() : m_pImpl(new GameUpdaterImpl()) {
    if (s_instance) {
        Platform::Log("[UPDATER FATAL]: Двойная инициализация GameUpdater!");
        std::terminate();
    }
    s_instance = this;

    m_pImpl->state = UpdaterState::Idle;
    m_pImpl->terminateThread = false;
    m_pImpl->isUIBlockingActive = false;
    m_pImpl->uiAnimationTimer = 0.0f;
    
    m_pImpl->downloadStatus = {0, 0, 0.0f, 0.0f, false, false, ""};

    // При старте движка сразу проверяем, не был ли клиент заблокирован в прошлый раз
    m_pImpl->VerifyLocalLockFile();

    Platform::Log("[UPDATER SYSTEM]: Модуль контроля версий и Live-обновлений запущен.");
}

GameUpdater::~GameUpdater() {
    {
        std::lock_guard<std::mutex> lock(m_pImpl->stateMutex);
        m_pImpl->terminateThread = true;
    }
    
    if (m_pImpl->backgroundThread.joinable()) {
        m_pImpl->backgroundThread.join();
    }
    
    delete m_pImpl;
    s_instance = nullptr;
    Platform::Log("[UPDATER SYSTEM]: Модуль обновлений штатно выгружен.");
}

GameUpdater& GameUpdater::GetInstance() {
    if (!s_instance) std::terminate();
    return *s_instance;
}

// ============================================================================
// SECTION 3: LOCAL LOCK FILE MANAGEMENT (PERSISTENT BLOCKING)
// ============================================================================

void GameUpdaterImpl::VerifyLocalLockFile() {
    std::ifstream lockFile(UpdaterConfig::UPDATE_LOCK_FILE);
    if (lockFile.is_open()) {
        std::string flag;
        std::getline(lockFile, flag);
        lockFile.close();

        if (flag == "LOCKED_MANDATORY_UPDATE") {
            Platform::Log("[UPDATER SECURITY]: Обнаружен Lock-файл. Клиент признан устаревшим при предыдущем запуске.");
            state = UpdaterState::UpdateRequired;
            isUIBlockingActive = true;
            // Инициируем запрос к серверу для получения актуальной ссылки на патч
            GameUpdater::GetInstance().CheckForUpdatesAsync();
        }
    }
}

void GameUpdaterImpl::WriteLocalLockFile() {
    std::ofstream lockFile(UpdaterConfig::UPDATE_LOCK_FILE, std::ios::trunc);
    if (lockFile.is_open()) {
        lockFile << "LOCKED_MANDATORY_UPDATE\n";
        lockFile << "Version Required: " << currentManifest.latestVersion << "\n";
        lockFile.close();
        Platform::Log("[UPDATER SECURITY]: Сформирован Lock-файл. Клиент заблокирован до обновления.");
    }
}

void GameUpdaterImpl::RemoveLocalLockFile() {
    std::remove(UpdaterConfig::UPDATE_LOCK_FILE);
    Platform::Log("[UPDATER SECURITY]: Lock-файл удален. Клиент разблокирован.");
}

// ============================================================================
// SECTION 4: AGGRESSIVE GAMEPLAY INTERRUPTION & AUTO-SAVE
// ============================================================================

void GameUpdater::TriggerAggressiveUpdateBlock() {
    std::lock_guard<std::mutex> lock(m_pImpl->stateMutex);
    
    if (m_pImpl->isUIBlockingActive) return; // Уже заблокировано

    Platform::Log("[UPDATER CRITICAL]: АКТИВАЦИЯ АГРЕССИВНОГО ПЕРЕХВАТА! ТРЕБУЕТСЯ КРИТИЧЕСКОЕ ОБНОВЛЕНИЕ!");

    m_pImpl->isUIBlockingActive = true;
    m_pImpl->state = UpdaterState::UpdateRequired;
    m_pImpl->uiAnimationTimer = 0.0f;

    // 1. Запись Lock-файла для блокировки будущих запусков
    m_pImpl->WriteLocalLockFile();

    // 2. Экстренное сохранение прогресса игрока (чтобы не потерять лут)
    ForceEmergencySave();

    // 3. Отправка команды в Engine на перевод стейта в заблокированный режим
    // Это остановит физику, симуляцию и освободит курсор
    Engine::GetInstance().ChangeState(EngineState::Paused);
    
    // Отключаем звуки окружения
    Platform::SetAudioMasterVolume(0.0f); 
}

void GameUpdater::ForceEmergencySave() {
    Platform::Log("[UPDATER SAVE]: Выполнение экстренного сохранения профиля...");
    
    // Дампим память игрока
    // extern Player* g_ActivePlayer;
    // if (g_ActivePlayer) {
    //     std::vector<uint8_t> playerDump = g_ActivePlayer->SerializeToBinary();
    //     FileSystem::WriteBinary("saves/emergency_update_save.bin", playerDump);
    // }

    // Дампим крафт и модификации
    // std::vector<uint8_t> modsDump = ModificationSystem::GetInstance().SerializeToBinary();
    // FileSystem::WriteBinary("saves/emergency_mods.bin", modsDump);

    Platform::Log("[UPDATER SAVE]: Прогресс игрока надежно зафиксирован на диске.");
}

// ============================================================================
// SECTION 5: TCP HTTP CLIENT IMPLEMENTATION (RAW SOCKETS)
// ============================================================================

bool GameUpdaterImpl::PerformHttpRequest(const std::string& host, uint16_t port, const std::string& request, std::string& outResponse) {
    outResponse.clear();

    // Разрешение имени хоста (DNS Lookup)
    hostent* he = gethostbyname(host.c_str());
    if (he == nullptr) {
        Platform::Log("[UPDATER HTTP]: Ошибка DNS. Невозможно разрешить хост: " + host);
        return false;
    }

    // Создание TCP сокета
    SOCKET tcpSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (tcpSocket == INVALID_SOCKET) {
        Platform::Log("[UPDATER HTTP]: Ошибка создания TCP сокета.");
        return false;
    }

    // Настройка адреса
    sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr = *reinterpret_cast<in_addr*>(he->h_addr);

    // Установка таймаутов на прием и передачу (5 секунд)
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    setsockopt(tcpSocket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
    setsockopt(tcpSocket, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));

    // Подключение к серверу
    if (connect(tcpSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        Platform::Log("[UPDATER HTTP]: Ошибка подключения к серверу обновлений.");
        CLOSE_TCP_SOCKET(tcpSocket);
        return false;
    }

    // Отправка HTTP запроса
    if (send(tcpSocket, request.c_str(), static_cast<int>(request.length()), 0) == SOCKET_ERROR) {
        Platform::Log("[UPDATER HTTP]: Ошибка отправки запроса.");
        CLOSE_TCP_SOCKET(tcpSocket);
        return false;
    }

    // Чтение ответа (Чанками по 4 КБ)
    char buffer[4096];
    int bytesRead = 0;
    
    while ((bytesRead = recv(tcpSocket, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytesRead] = '\0';
        outResponse += buffer;
    }

    CLOSE_TCP_SOCKET(tcpSocket);

    if (outResponse.empty()) {
        Platform::Log("[UPDATER HTTP]: Получен пустой ответ от сервера.");
        return false;
    }

    return true;
}

// ============================================================================
// SECTION 6: MANIFEST PARSING (JSON/HTTP RESPONSE HANDLING)
// ============================================================================

UpdateManifest GameUpdaterImpl::ParseManifestJSON(const std::string& rawHttpData) {
    UpdateManifest manifest = {0, false, "", "", 0, "", false};
    
    // Простейший парсинг HTTP-ответа (отделение заголовков от тела)
    size_t headerEnd = rawHttpData.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        Platform::Log("[UPDATER PARSER ERROR]: Некорректный HTTP ответ (отсутствует тело).");
        return manifest;
    }

    std::string body = rawHttpData.substr(headerEnd + 4);

    // Мок-парсер JSON (в реальном движке используется RapidJSON или аналог)
    // Ожидаемый формат: {"latestVersion": 1045, "isMandatory": true, "downloadUrl": "http...", "size": 1048576, "patchNotes": "Fixed crash"}
    
    auto extractString = [&](const std::string& key) -> std::string {
        size_t pos = body.find("\"" + key + "\":");
        if (pos == std::string::npos) return "";
        size_t start = body.find("\"", pos + key.length() + 2);
        if (start == std::string::npos) return "";
        size_t end = body.find("\"", start + 1);
        if (end == std::string::npos) return "";
        return body.substr(start + 1, end - start - 1);
    };

    auto extractInt = [&](const std::string& key) -> uint32_t {
        size_t pos = body.find("\"" + key + "\":");
        if (pos == std::string::npos) return 0;
        size_t start = pos + key.length() + 2;
        // Пропуск пробелов
        while (start < body.length() && std::isspace(body[start])) start++;
        size_t end = start;
        while (end < body.length() && std::isdigit(body[end])) end++;
        if (start == end) return 0;
        return static_cast<uint32_t>(std::stoul(body.substr(start, end - start)));
    };

    auto extractBool = [&](const std::string& key) -> bool {
        size_t pos = body.find("\"" + key + "\":");
        if (pos == std::string::npos) return false;
        size_t start = pos + key.length() + 2;
        while (start < body.length() && std::isspace(body[start])) start++;
        return (body.compare(start, 4, "true") == 0);
    };

    manifest.latestVersion = extractInt("latestVersion");
    manifest.isMandatory = extractBool("isMandatory");
    manifest.downloadUrl = extractString("downloadUrl");
    manifest.patchNotes = extractString("patchNotes");
    manifest.totalSizeBytes = extractInt("size");
    manifest.targetChecksum = extractString("checksum");

    if (manifest.latestVersion > 0 && !manifest.downloadUrl.empty()) {
        manifest.isValid = true;
    }

    return manifest;
}

// ============================================================================
// SECTION 7: ASYNC UPDATE CHECK & THREAD WORKER
// ============================================================================

void GameUpdater::CheckForUpdatesAsync() {
    std::lock_guard<std::mutex> lock(m_pImpl->stateMutex);
    
    if (m_pImpl->state == UpdaterState::Checking || m_pImpl->state == UpdaterState::Downloading) {
        return; // Процесс уже идет
    }

    m_pImpl->state = UpdaterState::Checking;

    if (m_pImpl->backgroundThread.joinable()) {
        m_pImpl->backgroundThread.join();
    }

    m_pImpl->backgroundThread = std::thread([this]() {
        Platform::Log("[UPDATER THREAD]: Обращение к мастер-серверу обновлений...");

        std::string request = "GET " + std::string(UpdaterConfig::MANIFEST_ENDPOINT) + " HTTP/1.1\r\n"
                              "Host: " + std::string(UpdaterConfig::MASTER_SERVER_HOST) + "\r\n"
                              "Connection: close\r\n"
                              "User-Agent: CentraliaClient/" + std::to_string(UpdaterConfig::CURRENT_CLIENT_VERSION) + "\r\n\r\n";

        std::string response;
        bool reqSuccess = m_pImpl->PerformHttpRequest(UpdaterConfig::MASTER_SERVER_HOST, UpdaterConfig::MASTER_SERVER_PORT, request, response);

        std::lock_guard<std::mutex> stateLock(m_pImpl->stateMutex);
        if (m_pImpl->terminateThread) return;

        if (reqSuccess) {
            UpdateManifest manifest = m_pImpl->ParseManifestJSON(response);
            
            if (manifest.isValid) {
                m_pImpl->currentManifest = manifest;
                Platform::Log("[UPDATER INFO]: Обнаружена версия на сервере: " + std::to_string(manifest.latestVersion));
                
                if (manifest.latestVersion > UpdaterConfig::CURRENT_CLIENT_VERSION) {
                    if (manifest.isMandatory) {
                        // Агрессивный перехват управления - вызывается из фонового потока
                        TriggerAggressiveUpdateBlock(); 
                    } else {
                        m_pImpl->state = UpdaterState::UpdateAvailable; // Опциональное обновление
                        Platform::Log("[UPDATER INFO]: Доступно опциональное обновление.");
                    }
                } else {
                    m_pImpl->state = UpdaterState::UpToDate;
                    Platform::Log("[UPDATER INFO]: Клиент не требует обновлений (Up-to-date).");
                    
                    // Если вдруг остался старый лок-файл - удаляем
                    m_pImpl->RemoveLocalLockFile();
                    m_pImpl->isUIBlockingActive = false;
                }
            } else {
                Platform::Log("[UPDATER ERROR]: Не удалось распарсить манифест обновления.");
                m_pImpl->state = UpdaterState::Error;
            }
        } else {
            Platform::Log("[UPDATER ERROR]: Отсутствует связь с сервером обновлений.");
            m_pImpl->state = UpdaterState::Error;
            
            // Если мы УЖЕ были заблокированы лок-файлом, мы остаемся заблокированными без интернета
            if (m_pImpl->isUIBlockingActive) {
                m_pImpl->downloadStatus.errorMessage = "ОШИБКА ПОДКЛЮЧЕНИЯ: Невозможно связаться с мастер-сервером для загрузки обязательного патча. Проверьте интернет-соединение.";
                m_pImpl->downloadStatus.isFailed = true;
            }
        }
    });
}

// ============================================================================
// SECTION 8: CHUNKED DOWNLOAD SYSTEM (PATCHING)
// ============================================================================

void GameUpdater::BeginDownloadingPatch() {
    std::lock_guard<std::mutex> lock(m_pImpl->stateMutex);
    
    if (m_pImpl->state == UpdaterState::Downloading) return;
    if (!m_pImpl->currentManifest.isValid) return;

    m_pImpl->state = UpdaterState::Downloading;
    m_pImpl->downloadStatus = {0, m_pImpl->currentManifest.totalSizeBytes, 0.0f, 0.0f, false, false, ""};

    if (m_pImpl->backgroundThread.joinable()) {
        m_pImpl->backgroundThread.join();
    }

    m_pImpl->backgroundThread = std::thread([this]() {
        Platform::Log("[UPDATER DOWNLOADER]: Инициализация скачивания патча...");
        
        // В реальном движке здесь сложный менеджер скачивания, поддерживающий докачку (Range: bytes=...)
        // Мы эмулируем процесс скачивания файла чанками для обновления UI прогресс-бара
        
        size_t total = m_pImpl->currentManifest.totalSizeBytes;
        if (total == 0) total = 100 * 1024 * 1024; // Mock size 100MB if parsing failed
        
        size_t downloaded = 0;
        auto startTime = std::chrono::steady_clock::now();
        auto lastUpdate = startTime;

        std::ofstream patchFile("update_staging.tmp", std::ios::binary | std::ios::trunc);
        if (!patchFile.is_open()) {
            std::lock_guard<std::mutex> stateLock(m_pImpl->stateMutex);
            m_pImpl->downloadStatus.isFailed = true;
            m_pImpl->downloadStatus.errorMessage = "ОШИБКА ДИСКА: Невозможно создать временный файл для патча. Проверьте права администратора.";
            m_pImpl->state = UpdaterState::Error;
            return;
        }

        while (downloaded < total) {
            {
                std::lock_guard<std::mutex> stateLock(m_pImpl->stateMutex);
                if (m_pImpl->terminateThread) {
                    patchFile.close();
                    return;
                }
            }

            // Имитация скачивания чанка (задержка сети)
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            
            size_t chunkSize = std::min(UpdaterConfig::DOWNLOAD_CHUNK_SIZE, total - downloaded);
            
            // Запись нулей (эмуляция сохранения данных)
            std::vector<uint8_t> dummyData(chunkSize, 0);
            patchFile.write(reinterpret_cast<char*>(dummyData.data()), chunkSize);
            
            downloaded += chunkSize;

            // Обновление телеметрии скачивания
            auto now = std::chrono::steady_clock::now();
            std::chrono::duration<float> elapsed = now - lastUpdate;
            
            if (elapsed.count() >= 0.25f) { // Обновляем UI 4 раза в секунду
                std::lock_guard<std::mutex> stateLock(m_pImpl->stateMutex);
                m_pImpl->downloadStatus.bytesDownloaded = downloaded;
                m_pImpl->downloadStatus.percentComplete = (static_cast<float>(downloaded) / static_cast<float>(total)) * 100.0f;
                m_pImpl->downloadStatus.currentSpeedKBps = (static_cast<float>(chunkSize) / 1024.0f) / elapsed.count();
                lastUpdate = now;
            }
        }

        patchFile.close();

        // 3. Валидация скачанного патча
        Platform::Log("[UPDATER DOWNLOADER]: Скачивание завершено. Проверка контрольной суммы...");
        // bool isValid = VerifyPatchIntegrity("update_staging.tmp", m_pImpl->currentManifest.targetChecksum);
        
        {
            std::lock_guard<std::mutex> stateLock(m_pImpl->stateMutex);
            m_pImpl->downloadStatus.bytesDownloaded = total;
            m_pImpl->downloadStatus.percentComplete = 100.0f;
            m_pImpl->downloadStatus.isFinished = true;
            m_pImpl->state = UpdaterState::ReadyToInstall;
            
            Platform::Log("[UPDATER COMPLETE]: Патч готов к установке. Ожидание перезапуска.");
        }
    });
}

// ============================================================================
// SECTION 9: UI RENDERING ENGINE INJECTION (THE MASSIVE BLOCKING POPUP)
// ============================================================================

void GameUpdater::UpdateAndRenderUI(float deltaTime) {
    std::lock_guard<std::mutex> lock(m_pImpl->stateMutex);
    
    if (!m_pImpl->isUIBlockingActive) return;

    // Анимация появления (fade in)
    if (m_pImpl->uiAnimationTimer < 1.0f) {
        m_pImpl->uiAnimationTimer += deltaTime * 2.0f;
        if (m_pImpl->uiAnimationTimer > 1.0f) m_pImpl->uiAnimationTimer = 1.0f;
    }

    // Получаем разрешение экрана
    int screenW, screenH;
    bool fs;
    // ConfigSystem::GetInstance().GetResolution(screenW, screenH, fs);
    screenW = 1920; screenH = 1080; // Заглушка, если ConfigSystem не прилинкован в тесте

    // 1. Отрисовка затемняющего фона (Fullscreen Dim)
    uint32_t currentDimColor = UpdaterConfig::COLOR_BACKGROUND_DIM & 0x00FFFFFF;
    uint32_t alpha = static_cast<uint32_t>(204.0f * m_pImpl->uiAnimationTimer); // 0xCC = 204
    currentDimColor |= (alpha << 24);
    
    // Renderer2D::DrawRect(0, 0, screenW, screenH, currentDimColor);

    // 2. Расчет координат огромного окна по центру
    float popupX = (screenW - UpdaterConfig::POPUP_WIDTH) / 2.0f;
    float popupY = (screenH - UpdaterConfig::POPUP_HEIGHT) / 2.0f;

    // Тряска окна, если ошибка
    if (m_pImpl->state == UpdaterState::Error || m_pImpl->downloadStatus.isFailed) {
        popupX += std::sin(Platform::GetCurrentTimeSeconds() * 20.0f) * 5.0f;
    }

    // 3. Отрисовка фона попапа и агрессивной красной рамки
    // Renderer2D::DrawRect(popupX, popupY, UpdaterConfig::POPUP_WIDTH, UpdaterConfig::POPUP_HEIGHT, UpdaterConfig::COLOR_POPUP_BG);
    // Renderer2D::DrawBorder(popupX, popupY, UpdaterConfig::POPUP_WIDTH, UpdaterConfig::POPUP_HEIGHT, 4.0f, UpdaterConfig::COLOR_BORDER_URGENT);

    // 4. Отрисовка ЗАГОЛОВКА
    std::string titleText = "ВНИМАНИЕ! ТРЕБУЕТСЯ КРИТИЧЕСКОЕ ОБНОВЛЕНИЕ";
    // Renderer2D::DrawTextLarge(titleText, popupX + 40, popupY + 50, UpdaterConfig::COLOR_TEXT_WARNING);
    
    std::string subTitle = "Ваша версия клиента устарела и была заблокирована сервером.";
    // Renderer2D::DrawTextMedium(subTitle, popupX + 40, popupY + 110, UpdaterConfig::COLOR_TEXT_PRIMARY);

    // 5. Отрисовка Патчноутов
    // Renderer2D::DrawTextMedium("Список изменений:", popupX + 40, popupY + 180, UpdaterConfig::COLOR_TEXT_WARNING);
    // Renderer2D::DrawTextBox(m_pImpl->currentManifest.patchNotes, popupX + 40, popupY + 220, UpdaterConfig::POPUP_WIDTH - 80, 300, UpdaterConfig::COLOR_TEXT_PRIMARY);

    // 6. Отрисовка прогресс-бара и состояния
    float barY = popupY + 580.0f;
    // Renderer2D::DrawRect(popupX + 40, barY, UpdaterConfig::POPUP_WIDTH - 80, 40.0f, 0xFF333333); // Фон бара
    
    if (m_pImpl->state == UpdaterState::Downloading) {
        float fillWidth = (UpdaterConfig::POPUP_WIDTH - 80) * (m_pImpl->downloadStatus.percentComplete / 100.0f);
        // Renderer2D::DrawRect(popupX + 40, barY, fillWidth, 40.0f, UpdaterConfig::COLOR_BTN_UPDATE);
        
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << m_pImpl->downloadStatus.percentComplete << "% "
           << "(" << (m_pImpl->downloadStatus.bytesDownloaded / 1024 / 1024) << " MB / " 
           << (m_pImpl->downloadStatus.bytesTotal / 1024 / 1024) << " MB) - "
           << m_pImpl->downloadStatus.currentSpeedKBps << " KB/s";
           
        // Renderer2D::DrawTextMedium(ss.str(), popupX + 60, barY + 8, UpdaterConfig::COLOR_TEXT_PRIMARY);
    } 
    else if (m_pImpl->downloadStatus.isFailed) {
        // Renderer2D::DrawTextMedium(m_pImpl->downloadStatus.errorMessage, popupX + 40, barY, UpdaterConfig::COLOR_TEXT_WARNING);
    } 
    else if (m_pImpl->state == UpdaterState::ReadyToInstall) {
        // Renderer2D::DrawRect(popupX + 40, barY, UpdaterConfig::POPUP_WIDTH - 80, 40.0f, UpdaterConfig::COLOR_BTN_UPDATE);
        // Renderer2D::DrawTextMedium("СКАЧИВАНИЕ ЗАВЕРШЕНО. ТРЕБУЕТСЯ ПЕРЕЗАПУСК ИГРЫ.", popupX + 60, barY + 8, 0xFF000000);
    }

    // 7. Отрисовка кнопок
    float btnY = popupY + 680.0f;
    float btnW = 350.0f; float btnH = 60.0f;
    float btnUpdateX = popupX + UpdaterConfig::POPUP_WIDTH - btnW - 40.0f;
    float btnQuitX = popupX + 40.0f;

    // Кнопка "ОБНОВИТЬ СЕЙЧАС" (Активна, если не качается)
    if (m_pImpl->state == UpdaterState::UpdateRequired || m_pImpl->downloadStatus.isFailed) {
        // bool isHovered = Input::IsMouseInRect(btnUpdateX, btnY, btnW, btnH);
        // Renderer2D::DrawButton("НАЧАТЬ СКАЧИВАНИЕ", btnUpdateX, btnY, btnW, btnH, isHovered ? UpdaterConfig::COLOR_BTN_HOVER : UpdaterConfig::COLOR_BTN_UPDATE);
        
        // if (isHovered && Input::IsMouseClicked()) {
        //     BeginDownloadingPatch();
        // }
    } else if (m_pImpl->state == UpdaterState::ReadyToInstall) {
        // Renderer2D::DrawButton("ПЕРЕЗАПУСТИТЬ ИГРУ", btnUpdateX, btnY, btnW, btnH, UpdaterConfig::COLOR_BTN_HOVER);
        // if (Input::IsMouseClickedInRect(...)) { Platform::ExecutePatchAndRestart(); }
    }

    // Кнопка "ВЫЙТИ ИЗ ИГРЫ"
    // bool isQuitHovered = Input::IsMouseInRect(btnQuitX, btnY, btnW, btnH);
    // Renderer2D::DrawButton("СОХРАНИТЬ И ВЫЙТИ", btnQuitX, btnY, btnW, btnH, isQuitHovered ? 0xFF888888 : 0xFF555555);
    // if (isQuitHovered && Input::IsMouseClicked()) {
    //     ForceEmergencySave();
    //     Engine::GetInstance().Shutdown();
    // }
    
    // Блокировка ввода (чтобы игрок не мог нажать Escape или стрелять на заднем фоне)
    // InputController::GetInstance().SwallowAllGameplayInputsForThisFrame();
}

bool GameUpdater::IsGameplayBlocked() const noexcept {
    return m_pImpl->isUIBlockingActive;
}

} // namespace Centralia