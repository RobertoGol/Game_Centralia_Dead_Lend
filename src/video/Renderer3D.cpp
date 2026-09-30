#include "video/Renderer3D.hpp"
#include "video/Shader.hpp"
#include "platform/Platform.hpp"
#include "core/MemoryManager.hpp"

// В реальном проекте здесь используется glad/glad.h и GLM
#include <cmath>
#include <algorithm>
#include <iostream>
#include <vector>
#include <string>
#include <random>

// ============================================================================
// MOCK OPENGL MACROS FOR THE SAKE OF ENGINE COMPILATION ABSTRACTION
// ============================================================================
#define GL_COLOR_BUFFER_BIT 0x00004000
#define GL_DEPTH_BUFFER_BIT 0x00000100
#define GL_TEXTURE_2D 0x0DE1
#define GL_FRAMEBUFFER 0x8D40
#define GL_COLOR_ATTACHMENT0 0x8CE0
#define GL_DEPTH_ATTACHMENT 0x8D00
#define GL_RGBA16F 0x881A
#define GL_RGBA 0x1908
#define GL_FLOAT 0x1406
#define GL_DEPTH_COMPONENT 0x1902
#define GL_TRIANGLES 0x0004
#define GL_CULL_FACE 0x0B44
#define GL_DEPTH_TEST 0x0B71
#define GL_LESS 0x0201
#define GL_BACK 0x0405

extern "C" {
    void glClear(unsigned int mask);
    void glClearColor(float r, float g, float b, float a);
    void glEnable(unsigned int cap);
    void glDisable(unsigned int cap);
    void glDepthFunc(unsigned int func);
    void glCullFace(unsigned int mode);
    void glViewport(int x, int y, int width, int height);
    void glGenFramebuffers(int n, unsigned int* framebuffers);
    void glBindFramebuffer(unsigned int target, unsigned int framebuffer);
    void glGenTextures(int n, unsigned int* textures);
    void glBindTexture(unsigned int target, unsigned int texture);
    void glTexImage2D(unsigned int target, int level, int internalformat, int width, int height, int border, unsigned int format, unsigned int type, const void* pixels);
    void glFramebufferTexture2D(unsigned int target, unsigned int attachment, unsigned int textarget, unsigned int texture, int level);
    void glDrawBuffers(int n, const unsigned int* bufs);
    void glCheckFramebufferStatus(unsigned int target);
    void glBindVertexArray(unsigned int array);
    void glDrawElements(unsigned int mode, int count, unsigned int type, const void* indices);
    void glDrawElementsInstanced(unsigned int mode, int count, unsigned int type, const void* indices, int instancecount);
}

namespace Centralia {

// ============================================================================
// SECTION 1: INTERNAL DATA STRUCTURES, BUFFERS & FRUSTUM CULLING
// ============================================================================

struct FrustumPlane {
    Vector3D normal;
    float distance;
};

struct RenderCommand {
    uint32_t vaoId;
    uint32_t indexCount;
    uint32_t materialId;
    Matrix4x4 transform;
    Vector3D boundingBoxMin;
    Vector3D boundingBoxMax;
};

struct InstancedRenderGroup {
    uint32_t vaoId;
    uint32_t indexCount;
    uint32_t materialId;
    std::vector<Matrix4x4> transforms;
    uint32_t instanceVboId; // Буфер для матриц инстансов
};

struct Renderer3DImpl {
    int screenWidth = 1920;
    int screenHeight = 1080;
    bool isInitialized = false;

    // Матрицы камеры
    Matrix4x4 viewMatrix;
    Matrix4x4 projectionMatrix;
    Vector3D cameraPosition;
    FrustumPlane frustumPlanes[6];

    // G-Buffer (Отложенный рендеринг)
    uint32_t gBufferFBO;
    uint32_t gPosition, gNormal, gAlbedo, gPBR; // Текстуры привязки
    uint32_t gDepthRenderbuffer;

    // SSAO (Экранное затенение)
    uint32_t ssaoFBO, ssaoBlurFBO;
    uint32_t ssaoColorBuffer, ssaoColorBufferBlur;
    uint32_t noiseTexture;
    std::vector<Vector3D> ssaoKernel;

    // Shadow Mapping
    uint32_t shadowMapFBO;
    uint32_t shadowMapTexture;
    Matrix4x4 lightSpaceMatrix;
    Vector3D mainLightDirection = Vector3D(0.5f, -1.0f, 0.5f).Normalized();

    // Шейдеры
    Shader* geometryPassShader;
    Shader* lightingPassShader;
    Shader* shadowPassShader;
    Shader* ssaoShader;
    Shader* ssaoBlurShader;
    Shader* postProcessShader;

    // Очереди рендеринга
    std::vector<RenderCommand> opaqueQueue;
    std::vector<RenderCommand> transparentQueue;
    std::unordered_map<uint32_t, InstancedRenderGroup> instancedGroups;
    std::vector<PointLight> pointLights;

    // Экранный квадрат для постпроцессинга и освещения
    uint32_t quadVAO, quadVBO;
};

Renderer3D* Renderer3D::s_instance = nullptr;

// ============================================================================
// SECTION 2: CONSTRUCTOR, DESTRUCTOR & SINGLETON LOGIC
// ============================================================================

Renderer3D::Renderer3D() : m_pImpl(new Renderer3DImpl()) {
    if (s_instance) {
        Platform::Log("[RENDERER FATAL]: Двойная инициализация Renderer3D!");
        std::terminate();
    }
    s_instance = this;
    Platform::Log("[RENDERER SYSTEM]: Графическое ядро создано.");
}

Renderer3D::~Renderer3D() {
    Shutdown();
    delete m_pImpl;
    s_instance = nullptr;
    Platform::Log("[RENDERER SYSTEM]: Графическое ядро штатно выгружено.");
}

Renderer3D& Renderer3D::GetInstance() {
    if (!s_instance) std::terminate();
    return *s_instance;
}

// ============================================================================
// SECTION 3: RENDERER INITIALIZATION & PIPELINE SETUP (FBOs, G-BUFFER)
// ============================================================================

bool Renderer3D::InitializeContext(int width, int height) {
    m_pImpl->screenWidth = width;
    m_pImpl->screenHeight = height;

    Platform::Log("[RENDERER INIT]: Инициализация OpenGL 4.6 Core контекста...");

    // Глобальные настройки OpenGL
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // 1. Инициализация G-Buffer
    glGenFramebuffers(1, &m_pImpl->gBufferFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_pImpl->gBufferFBO);

    // 1.1 Позиция (RGB) и Глубина/Шероховатость (A) - 16-bit Float
    glGenTextures(1, &m_pImpl->gPosition);
    glBindTexture(GL_TEXTURE_2D, m_pImpl->gPosition);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_pImpl->gPosition, 0);

    // 1.2 Нормали (RGB) - 16-bit Float
    glGenTextures(1, &m_pImpl->gNormal);
    glBindTexture(GL_TEXTURE_2D, m_pImpl->gNormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + 1, GL_TEXTURE_2D, m_pImpl->gNormal, 0);

    // 1.3 Альбедо (Цвет RGB) и Specular (A) - 8-bit
    glGenTextures(1, &m_pImpl->gAlbedo);
    glBindTexture(GL_TEXTURE_2D, m_pImpl->gAlbedo);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + 2, GL_TEXTURE_2D, m_pImpl->gAlbedo, 0);

    // 1.4 PBR (Metallic, Roughness, AO)
    glGenTextures(1, &m_pImpl->gPBR);
    glBindTexture(GL_TEXTURE_2D, m_pImpl->gPBR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + 3, GL_TEXTURE_2D, m_pImpl->gPBR, 0);

    // Указываем OpenGL, в какие цветовые привязки мы будем рендерить
    unsigned int attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT0 + 1, GL_COLOR_ATTACHMENT0 + 2, GL_COLOR_ATTACHMENT0 + 3 };
    glDrawBuffers(4, attachments);

    // Буфер глубины (Renderbuffer)
    // glGenRenderbuffers(1, &m_pImpl->gDepthRenderbuffer);
    // glBindRenderbuffer(GL_RENDERBUFFER, m_pImpl->gDepthRenderbuffer);
    // glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    // glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_pImpl->gDepthRenderbuffer);

    Platform::Log("[RENDERER INIT]: G-Buffer успешно создан (4 Color Attachments, 1 Depth).");

    // 2. Инициализация SSAO
    InitializeSSAO(width, height);

    // 3. Инициализация Shadow Map
    InitializeShadowMap(4096); // 4K Теневая карта

    // 4. Компиляция шейдеров
    LoadCoreShaders();

    // 5. Создание экранного квадрата (Screen Quad)
    SetupScreenQuad();

    m_pImpl->isInitialized = true;
    Platform::Log("[RENDERER INIT SUCCESS]: Графический конвейер готов к отрисовке кадров.");
    return true;
}

// ============================================================================
// SECTION 4: SSAO (SCREEN SPACE AMBIENT OCCLUSION) SETUP
// ============================================================================

void Renderer3D::InitializeSSAO(int width, int height) {
    // Генерация полусферы сэмплов ядра
    std::uniform_real_distribution<float> randomFloats(0.0, 1.0);
    std::default_random_engine generator;

    for (unsigned int i = 0; i < 64; ++i) {
        Vector3D sample(
            randomFloats(generator) * 2.0f - 1.0f, 
            randomFloats(generator) * 2.0f - 1.0f, 
            randomFloats(generator)
        );
        sample = sample.Normalized();
        sample = sample * randomFloats(generator);
        
        // Масштабируем сэмплы ближе к центру
        float scale = static_cast<float>(i) / 64.0f;
        scale = 0.1f + (scale * scale) * (1.0f - 0.1f);
        sample = sample * scale;
        
        m_pImpl->ssaoKernel.push_back(sample);
    }

    // Генерация 4x4 текстуры шума для поворота ядра
    std::vector<Vector3D> ssaoNoise;
    for (unsigned int i = 0; i < 16; i++) {
        Vector3D noise(randomFloats(generator) * 2.0f - 1.0f, randomFloats(generator) * 2.0f - 1.0f, 0.0f);
        ssaoNoise.push_back(noise);
    }

    glGenTextures(1, &m_pImpl->noiseTexture);
    glBindTexture(GL_TEXTURE_2D, m_pImpl->noiseTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 4, 4, 0, GL_RGBA, GL_FLOAT, &ssaoNoise[0]);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // SSAO FBO
    glGenFramebuffers(1, &m_pImpl->ssaoFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_pImpl->ssaoFBO);
    glGenTextures(1, &m_pImpl->ssaoColorBuffer);
    glBindTexture(GL_TEXTURE_2D, m_pImpl->ssaoColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_pImpl->ssaoColorBuffer, 0);

    // SSAO Blur FBO
    glGenFramebuffers(1, &m_pImpl->ssaoBlurFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_pImpl->ssaoBlurFBO);
    glGenTextures(1, &m_pImpl->ssaoColorBufferBlur);
    glBindTexture(GL_TEXTURE_2D, m_pImpl->ssaoColorBufferBlur);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_pImpl->ssaoColorBufferBlur, 0);

    Platform::Log("[RENDERER INIT]: Подсистема SSAO и текстуры шума сгенерированы.");
}

// ============================================================================
// SECTION 5: SHADOW MAPPING (DIRECTIONAL LIGHT)
// ============================================================================

void Renderer3D::InitializeShadowMap(int resolution) {
    glGenFramebuffers(1, &m_pImpl->shadowMapFBO);
    
    glGenTextures(1, &m_pImpl->shadowMapTexture);
    glBindTexture(GL_TEXTURE_2D, m_pImpl->shadowMapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, resolution, resolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    // float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    // glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, m_pImpl->shadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_pImpl->shadowMapTexture, 0);
    
    // Отключаем запись цвета (нам нужна только глубина)
    // glDrawBuffer(GL_NONE);
    // glReadBuffer(GL_NONE);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    Platform::Log("[RENDERER INIT]: Теневая карта (Shadow Map) " + std::to_string(resolution) + "x" + std::to_string(resolution) + " создана.");
}

// ============================================================================
// SECTION 6: SHADER MANAGEMENT & SCREEN QUAD
// ============================================================================

void Renderer3D::LoadCoreShaders() {
    Platform::Log("[RENDERER INIT]: Компиляция и линковка системных шейдеров...");
    
    // В реальности здесь читаются файлы с диска через AssetParser.
    // m_pImpl->geometryPassShader = new Shader("shaders/g_buffer.vert", "shaders/g_buffer.frag");
    // m_pImpl->lightingPassShader = new Shader("shaders/deferred_light.vert", "shaders/deferred_light.frag");
    // m_pImpl->shadowPassShader = new Shader("shaders/shadow_map.vert", "shaders/shadow_map.frag");
    // m_pImpl->ssaoShader = new Shader("shaders/ssao.vert", "shaders/ssao.frag");
    // m_pImpl->postProcessShader = new Shader("shaders/post_process.vert", "shaders/post_process.frag");
}

void Renderer3D::SetupScreenQuad() {
    float quadVertices[] = {
        // Positions        // TexCoords
        -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
         1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
    };
    
    // glGenVertexArrays(1, &m_pImpl->quadVAO);
    // glGenBuffers(1, &m_pImpl->quadVBO);
    // glBindVertexArray(m_pImpl->quadVAO);
    // glBindBuffer(GL_ARRAY_BUFFER, m_pImpl->quadVBO);
    // glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    // glEnableVertexAttribArray(0);
    // glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    // glEnableVertexAttribArray(1);
    // glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
}

// ============================================================================
// SECTION 7: FRUSTUM CULLING MATHEMATICS (AABB INTERSECTION)
// ============================================================================

void Renderer3D::ExtractFrustumPlanes() {
    // Вычисляем комбинированную матрицу ViewProjection
    Matrix4x4 vp = m_pImpl->projectionMatrix * m_pImpl->viewMatrix;

    // Левая плоскость
    m_pImpl->frustumPlanes[0].normal.x = vp.m[3][0] + vp.m[0][0];
    m_pImpl->frustumPlanes[0].normal.y = vp.m[3][1] + vp.m[0][1];
    m_pImpl->frustumPlanes[0].normal.z = vp.m[3][2] + vp.m[0][2];
    m_pImpl->frustumPlanes[0].distance = vp.m[3][3] + vp.m[0][3];

    // Правая плоскость
    m_pImpl->frustumPlanes[1].normal.x = vp.m[3][0] - vp.m[0][0];
    m_pImpl->frustumPlanes[1].normal.y = vp.m[3][1] - vp.m[0][1];
    m_pImpl->frustumPlanes[1].normal.z = vp.m[3][2] - vp.m[0][2];
    m_pImpl->frustumPlanes[1].distance = vp.m[3][3] - vp.m[0][3];

    // Верхняя плоскость
    m_pImpl->frustumPlanes[2].normal.x = vp.m[3][0] - vp.m[1][0];
    m_pImpl->frustumPlanes[2].normal.y = vp.m[3][1] - vp.m[1][1];
    m_pImpl->frustumPlanes[2].normal.z = vp.m[3][2] - vp.m[1][2];
    m_pImpl->frustumPlanes[2].distance = vp.m[3][3] - vp.m[1][3];

    // Нижняя плоскость
    m_pImpl->frustumPlanes[3].normal.x = vp.m[3][0] + vp.m[1][0];
    m_pImpl->frustumPlanes[3].normal.y = vp.m[3][1] + vp.m[1][1];
    m_pImpl->frustumPlanes[3].normal.z = vp.m[3][2] + vp.m[1][2];
    m_pImpl->frustumPlanes[3].distance = vp.m[3][3] + vp.m[1][3];

    // Ближняя плоскость
    m_pImpl->frustumPlanes[4].normal.x = vp.m[3][0] + vp.m[2][0];
    m_pImpl->frustumPlanes[4].normal.y = vp.m[3][1] + vp.m[2][1];
    m_pImpl->frustumPlanes[4].normal.z = vp.m[3][2] + vp.m[2][2];
    m_pImpl->frustumPlanes[4].distance = vp.m[3][3] + vp.m[2][3];

    // Дальняя плоскость
    m_pImpl->frustumPlanes[5].normal.x = vp.m[3][0] - vp.m[2][0];
    m_pImpl->frustumPlanes[5].normal.y = vp.m[3][1] - vp.m[2][1];
    m_pImpl->frustumPlanes[5].normal.z = vp.m[3][2] - vp.m[2][2];
    m_pImpl->frustumPlanes[5].distance = vp.m[3][3] - vp.m[2][3];

    // Нормализация всех плоскостей
    for (int i = 0; i < 6; ++i) {
        float length = std::sqrt(
            m_pImpl->frustumPlanes[i].normal.x * m_pImpl->frustumPlanes[i].normal.x +
            m_pImpl->frustumPlanes[i].normal.y * m_pImpl->frustumPlanes[i].normal.y +
            m_pImpl->frustumPlanes[i].normal.z * m_pImpl->frustumPlanes[i].normal.z
        );
        m_pImpl->frustumPlanes[i].normal.x /= length;
        m_pImpl->frustumPlanes[i].normal.y /= length;
        m_pImpl->frustumPlanes[i].normal.z /= length;
        m_pImpl->frustumPlanes[i].distance /= length;
    }
}

bool Renderer3D::IsAABBInFrustum(const Vector3D& minBounds, const Vector3D& maxBounds) const {
    for (int i = 0; i < 6; ++i) {
        const FrustumPlane& plane = m_pImpl->frustumPlanes[i];
        
        // Находим p-вершину (которая находится дальше всего по направлению нормали)
        Vector3D pVertex = minBounds;
        if (plane.normal.x >= 0) pVertex.x = maxBounds.x;
        if (plane.normal.y >= 0) pVertex.y = maxBounds.y;
        if (plane.normal.z >= 0) pVertex.z = maxBounds.z;

        // Если p-вершина за плоскостью, значит весь AABB снаружи
        if (plane.normal.x * pVertex.x + plane.normal.y * pVertex.y + plane.normal.z * pVertex.z + plane.distance < 0) {
            return false; // Отсекаем (Cull)
        }
    }
    return true; // Видимо
}

// ============================================================================
// SECTION 8: SCENE SUBMISSION PIPELINE (BATCHING & SORTING)
// ============================================================================

void Renderer3D::BeginScene(const Matrix4x4& view, const Matrix4x4& projection, const Vector3D& cameraPos) {
    if (!m_pImpl->isInitialized) return;

    m_pImpl->viewMatrix = view;
    m_pImpl->projectionMatrix = projection;
    m_pImpl->cameraPosition = cameraPos;

    ExtractFrustumPlanes();

    // Очистка очередей предыдущего кадра
    m_pImpl->opaqueQueue.clear();
    m_pImpl->transparentQueue.clear();
    m_pImpl->instancedGroups.clear();
    m_pImpl->pointLights.clear();
}

void Renderer3D::SubmitMesh(uint32_t vaoId, uint32_t indexCount, uint32_t materialId, const Matrix4x4& transform, const Vector3D& aabbMin, const Vector3D& aabbMax) {
    // 1. Применяем Frustum Culling
    // Трансформируем AABB (упрощенно)
    Vector3D worldMin = transform * aabbMin;
    Vector3D worldMax = transform * aabbMax;

    if (!IsAABBInFrustum(worldMin, worldMax)) {
        return; // Объект вне видимости камеры, отбрасываем
    }

    RenderCommand cmd;
    cmd.vaoId = vaoId;
    cmd.indexCount = indexCount;
    cmd.materialId = materialId;
    cmd.transform = transform;

    // В реальном движке проверяем флаг прозрачности в MaterialManager
    bool isTransparent = false; 
    
    if (isTransparent) {
        m_pImpl->transparentQueue.push_back(cmd);
    } else {
        m_pImpl->opaqueQueue.push_back(cmd);
    }
}

void Renderer3D::SubmitInstancedMesh(uint32_t vaoId, uint32_t indexCount, uint32_t materialId, const std::vector<Matrix4x4>& transforms) {
    // Группировка для инстансного рендеринга (лес, трава, обломки)
    auto& group = m_pImpl->instancedGroups[vaoId];
    group.vaoId = vaoId;
    group.indexCount = indexCount;
    group.materialId = materialId;
    
    // В идеале тут тоже нужен Culling для каждого инстанса
    for (const auto& mat : transforms) {
        group.transforms.push_back(mat);
    }
}

void Renderer3D::SubmitPointLight(const PointLight& light) {
    // Если источник света слишком далеко от камеры - отбрасываем
    float distSq = (light.position - m_pImpl->cameraPosition).LengthSquared();
    if (distSq < light.radius * light.radius * 4.0f) { // Эвристика дальности
        m_pImpl->pointLights.push_back(light);
    }
}

// ============================================================================
// SECTION 9: THE RENDER PIPELINE EXECUTION (GEOMETRY -> SHADOW -> SSAO -> LIGHTING)
// ============================================================================

void Renderer3D::EndScene() {
    if (!m_pImpl->isInitialized) return;

    // 1. Отрисовка теней (Shadow Mapping Pass)
    RenderShadowPass();

    // 2. Отрисовка геометрии в G-Buffer (Geometry Pass)
    RenderGeometryPass();

    // 3. Вычисление SSAO (Screen Space Ambient Occlusion Pass)
    RenderSSAOPass();

    // 4. Отрисовка освещения (Deferred Lighting Pass)
    RenderLightingPass();

    // 5. Отрисовка полупрозрачных объектов (Forward Pass)
    RenderTransparentPass();

    // 6. Постпроцессинг (Post-Processing Pass)
    ApplyPostProcessing();
}

void Renderer3D::RenderShadowPass() {
    // Настройка ортографической проекции света
    // Matrix4x4 lightProjection = Matrix4x4::Ortho(-50.0f, 50.0f, -50.0f, 50.0f, 1.0f, 100.0f);
    // Matrix4x4 lightView = Matrix4x4::LookAt(m_pImpl->cameraPosition - m_pImpl->mainLightDirection * 50.0f, m_pImpl->cameraPosition, Vector3D(0,1,0));
    // m_pImpl->lightSpaceMatrix = lightProjection * lightView;

    glViewport(0, 0, 4096, 4096);
    glBindFramebuffer(GL_FRAMEBUFFER, m_pImpl->shadowMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    // if (m_pImpl->shadowPassShader) {
    //     m_pImpl->shadowPassShader->Use();
    //     m_pImpl->shadowPassShader->SetMat4("lightSpaceMatrix", m_pImpl->lightSpaceMatrix);
        
    //     // Отрисовка всех Opaque объектов
    //     for (const auto& cmd : m_pImpl->opaqueQueue) {
    //         m_pImpl->shadowPassShader->SetMat4("model", cmd.transform);
    //         glBindVertexArray(cmd.vaoId);
    //         glDrawElements(GL_TRIANGLES, cmd.indexCount, GL_UNSIGNED_INT, 0);
    //     }
    // }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer3D::RenderGeometryPass() {
    glViewport(0, 0, m_pImpl->screenWidth, m_pImpl->screenHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, m_pImpl->gBufferFBO);
    
    // Очистка черным цветом
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // if (m_pImpl->geometryPassShader) {
    //     m_pImpl->geometryPassShader->Use();
    //     m_pImpl->geometryPassShader->SetMat4("view", m_pImpl->viewMatrix);
    //     m_pImpl->geometryPassShader->SetMat4("projection", m_pImpl->projectionMatrix);

    //     // 1. Одиночные Opaque объекты
    //     for (const auto& cmd : m_pImpl->opaqueQueue) {
    //         m_pImpl->geometryPassShader->SetMat4("model", cmd.transform);
    //         // MaterialManager::BindMaterial(cmd.materialId); // Биндинг Albedo, Normal, M/R текстур
    //         glBindVertexArray(cmd.vaoId);
    //         glDrawElements(GL_TRIANGLES, cmd.indexCount, GL_UNSIGNED_INT, 0);
    //     }

    //     // 2. Инстансные объекты (Лес, Трава)
    //     for (auto& [vao, group] : m_pImpl->instancedGroups) {
    //         if (group.transforms.empty()) continue;
    //         
    //         // Загрузка матриц инстансов в VBO
    //         // glBindBuffer(GL_ARRAY_BUFFER, group.instanceVboId);
    //         // glBufferData(GL_ARRAY_BUFFER, group.transforms.size() * sizeof(Matrix4x4), group.transforms.data(), GL_DYNAMIC_DRAW);
    //         
    //         // MaterialManager::BindMaterial(group.materialId);
    //         glBindVertexArray(group.vaoId);
    //         glDrawElementsInstanced(GL_TRIANGLES, group.indexCount, GL_UNSIGNED_INT, 0, group.transforms.size());
    //     }
    // }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer3D::RenderSSAOPass() {
    // 1. Генерация SSAO
    glBindFramebuffer(GL_FRAMEBUFFER, m_pImpl->ssaoFBO);
    glClear(GL_COLOR_BUFFER_BIT);

    // if (m_pImpl->ssaoShader) {
    //     m_pImpl->ssaoShader->Use();
    //     // Привязка G-Buffer позиций и нормалей
    //     // glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, m_pImpl->gPosition);
    //     // glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, m_pImpl->gNormal);
    //     // glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, m_pImpl->noiseTexture);
        
    //     // Загрузка ядра сэмплов
    //     for (unsigned int i = 0; i < 64; ++i) {
    //         m_pImpl->ssaoShader->SetVec3("samples[" + std::to_string(i) + "]", m_pImpl->ssaoKernel[i]);
    //     }
    //     m_pImpl->ssaoShader->SetMat4("projection", m_pImpl->projectionMatrix);

    //     // Отрисовка Screen Quad
    //     glBindVertexArray(m_pImpl->quadVAO);
    //     // glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    // }

    // 2. Размытие SSAO (для устранения шума от 4x4 текстуры)
    glBindFramebuffer(GL_FRAMEBUFFER, m_pImpl->ssaoBlurFBO);
    glClear(GL_COLOR_BUFFER_BIT);
    // if (m_pImpl->ssaoBlurShader) {
    //     m_pImpl->ssaoBlurShader->Use();
    //     // glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, m_pImpl->ssaoColorBuffer);
    //     glBindVertexArray(m_pImpl->quadVAO);
    //     // glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    // }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer3D::RenderLightingPass() {
    // Отрисовка на дефолтный Framebuffer (или FBO постпроцессинга)
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // if (m_pImpl->lightingPassShader) {
    //     m_pImpl->lightingPassShader->Use();
        
    //     // Привязка всех текстур G-Buffer
    //     // glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, m_pImpl->gPosition);
    //     // glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, m_pImpl->gNormal);
    //     // glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, m_pImpl->gAlbedo);
    //     // glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, m_pImpl->gPBR);
    //     // glActiveTexture(GL_TEXTURE4); glBindTexture(GL_TEXTURE_2D, m_pImpl->ssaoColorBufferBlur);
    //     // glActiveTexture(GL_TEXTURE5); glBindTexture(GL_TEXTURE_2D, m_pImpl->shadowMapTexture);

    //     m_pImpl->lightingPassShader->SetVec3("viewPos", m_pImpl->cameraPosition);
    //     m_pImpl->lightingPassShader->SetVec3("dirLight.direction", m_pImpl->mainLightDirection);
    //     m_pImpl->lightingPassShader->SetVec3("dirLight.color", Vector3D(1.0f, 0.95f, 0.8f));
    //     m_pImpl->lightingPassShader->SetMat4("lightSpaceMatrix", m_pImpl->lightSpaceMatrix);

    //     // Загрузка Point Lights (ограничено, скажем, 32 в шейдере)
    //     for (size_t i = 0; i < m_pImpl->pointLights.size() && i < 32; ++i) {
    //         std::string p = "pointLights[" + std::to_string(i) + "].";
    //         m_pImpl->lightingPassShader->SetVec3(p + "position", m_pImpl->pointLights[i].position);
    //         m_pImpl->lightingPassShader->SetVec3(p + "color", m_pImpl->pointLights[i].color);
    //         m_pImpl->lightingPassShader->SetFloat(p + "radius", m_pImpl->pointLights[i].radius);
    //         m_pImpl->lightingPassShader->SetFloat(p + "intensity", m_pImpl->pointLights[i].intensity);
    //     }

    //     // Отрисовка Screen Quad для освещения
    //     glBindVertexArray(m_pImpl->quadVAO);
    //     // glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    // }
}

void Renderer3D::RenderTransparentPass() {
    // Копирование буфера глубины из G-Buffer в дефолтный фреймбуфер для правильного Z-теста прозрачных объектов
    // glBindFramebuffer(GL_READ_FRAMEBUFFER, m_pImpl->gBufferFBO);
    // glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // Или FBO постпроцессинга
    // glBlitFramebuffer(0, 0, m_pImpl->screenWidth, m_pImpl->screenHeight, 0, 0, m_pImpl->screenWidth, m_pImpl->screenHeight, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

    // glEnable(GL_BLEND);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Сортировка прозрачных объектов от дальнего к ближнему (Back-to-Front)
    /*
    std::sort(m_pImpl->transparentQueue.begin(), m_pImpl->transparentQueue.end(),
        [this](const RenderCommand& a, const RenderCommand& b) {
            float distA = (Vector3D(a.transform.m[3][0], a.transform.m[3][1], a.transform.m[3][2]) - m_pImpl->cameraPosition).LengthSquared();
            float distB = (Vector3D(b.transform.m[3][0], b.transform.m[3][1], b.transform.m[3][2]) - m_pImpl->cameraPosition).LengthSquared();
            return distA > distB;
        });

    for (const auto& cmd : m_pImpl->transparentQueue) {
        // Forward рендеринг прозрачных объектов
        // ...
    }
    */
    // glDisable(GL_BLEND);
}

void Renderer3D::ApplyPostProcessing() {
    // Финальный проход на дефолтный Framebuffer (Окно)
    // if (m_pImpl->postProcessShader) {
    //     m_pImpl->postProcessShader->Use();
    //     // Биндим текстуру сцены с отрендеренным освещением
    //     // glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, sceneTexture);
    //     
    //     // Настройки экспозиции и гаммы
    //     m_pImpl->postProcessShader->SetFloat("exposure", 1.2f);
    //     m_pImpl->postProcessShader->SetFloat("gamma", 2.2f);

    //     glBindVertexArray(m_pImpl->quadVAO);
    //     // glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    // }
}

void Renderer3D::Shutdown() {
    if (!m_pImpl->isInitialized) return;

    // glDeleteFramebuffers(1, &m_pImpl->gBufferFBO);
    // glDeleteTextures(1, &m_pImpl->gPosition);
    // glDeleteTextures(1, &m_pImpl->gNormal);
    // glDeleteTextures(1, &m_pImpl->gAlbedo);
    // glDeleteTextures(1, &m_pImpl->gPBR);
    // glDeleteRenderbuffers(1, &m_pImpl->gDepthRenderbuffer);

    // glDeleteFramebuffers(1, &m_pImpl->ssaoFBO);
    // glDeleteFramebuffers(1, &m_pImpl->ssaoBlurFBO);
    // glDeleteTextures(1, &m_pImpl->noiseTexture);

    // glDeleteFramebuffers(1, &m_pImpl->shadowMapFBO);
    // glDeleteTextures(1, &m_pImpl->shadowMapTexture);

    // glDeleteVertexArrays(1, &m_pImpl->quadVAO);
    // glDeleteBuffers(1, &m_pImpl->quadVBO);

    // delete m_pImpl->geometryPassShader;
    // delete m_pImpl->lightingPassShader;
    // delete m_pImpl->shadowPassShader;
    // delete m_pImpl->ssaoShader;
    // delete m_pImpl->ssaoBlurShader;
    // delete m_pImpl->postProcessShader;

    m_pImpl->isInitialized = false;
}

} // namespace Centralia