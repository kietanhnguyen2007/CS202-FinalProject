#pragma once
#include "raylib.h"
#include "Systems/RenderTypes.h"

namespace View {

class Renderer {
public:
    static Renderer& GetInstance();

    void BeginFrame();
    void EndFrameAndFlush();

    bool SubmitSprite(Texture2D* texture, const Rectangle& src, Vector2 pos,
                      Vector2 scale = {1.0f, 1.0f},
                      float rotation = 0.0f,
                      Vector2 origin = {0.0f, 0.0f},
                      Color tint = WHITE,
                      Systems::Layer layer = Systems::Layer::UI,
                      float z = 0.0f,
                      bool flipX = false,
                      uint32_t entityId = 0);

    void DrawRectangle(Vector2 pos, Vector2 wsize, Color color,
                       Systems::Layer layer = Systems::Layer::UI,
                       float z = 0.0f);

    void DrawText(const char* text, Vector2 pos, int fontSize, Color color);

    void Shutdown();

private:
    Renderer() = default;
    ~Renderer() = default;

    void EnsureWhitePixel();

    Texture2D m_whitePixel = {0};
    bool m_whitePixelReady = false;
};

} // namespace View
