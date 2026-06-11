#pragma once

#include "raylib.h"
#include "Systems/RenderTypes.h"
#include <cstddef>

namespace Systems {

class Renderer {
public:
    static Renderer& GetInstance();

    bool Init(size_t cmdCapacityPerLayer = 4096, size_t textureSlotsPerLayer = 64);
    void Shutdown();

    void BeginFrame();

    bool SubmitSprite(Texture2D* texture,
                      const Rectangle& src,
                      Vector2 pos,
                      Vector2 scale = {1.0f,1.0f},
                      float rotation = 0.0f,
                      Vector2 origin = {0,0},
                      Color tint = WHITE,
                      Layer layer = Layer::World,
                      float z = 0.0f,
                      bool flipX = false,
                      uint32_t entityId = 0);

    void EndFrameAndFlush();

    void ResizeWindow(int width, int height);

    bool IsInitialized() const { return m_initialized; }

    // debug
    size_t GetDrawCallCount() const { return m_drawCalls; }
    size_t GetSubmittedCount() const { return m_totalSubmitted; }
    size_t GetFrameSubmittedCount() const { return m_frameSubmitted; }
    size_t GetDroppedSubmissionCount() const { return m_droppedSubmissions; }

    int GetWindowWidth() const { return m_windowWidth; }
    int GetWindowHeight() const { return m_windowHeight; }

private:
    Renderer() = default;
    ~Renderer() = default;

    bool m_initialized = false;
    size_t m_cmdCapacityPerLayer = 0;

    size_t m_drawCalls = 0;
    size_t m_totalSubmitted = 0;
    size_t m_frameSubmitted = 0;
    size_t m_droppedSubmissions = 0;
    int m_windowWidth = 0;
    int m_windowHeight = 0;
};

} // namespace Systems
