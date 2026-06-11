#include "View/Renderer.h"
#include "Systems/Renderer.h"
#include "raylib.h"

namespace View {

Renderer& Renderer::GetInstance() {
    static Renderer inst;
    return inst;
}

void Renderer::BeginFrame() {
    Systems::Renderer::GetInstance().BeginFrame();
}

void Renderer::EndFrameAndFlush() {
    Systems::Renderer::GetInstance().EndFrameAndFlush();
}

bool Renderer::SubmitSprite(Texture2D* texture, const Rectangle& src, Vector2 pos,
                            Vector2 scale, float rotation,
                            Vector2 origin, Color tint,
                            Systems::Layer layer, float z, bool flipX,
                            uint32_t entityId) {
    return Systems::Renderer::GetInstance().SubmitSprite(
        texture, src, pos, scale, rotation, origin, tint,
        layer, z, flipX, entityId);
}

void Renderer::EnsureWhitePixel() {
    if (m_whitePixelReady) return;
    Image img = GenImageColor(1, 1, WHITE);
    m_whitePixel = LoadTextureFromImage(img);
    UnloadImage(img);
    m_whitePixelReady = true;
}

void Renderer::DrawRectangle(Vector2 pos, Vector2 wsize, Color color,
                              Systems::Layer layer, float z) {
    EnsureWhitePixel();
    Rectangle src = {0, 0, 1, 1};
    Systems::Renderer::GetInstance().SubmitSprite(
        &m_whitePixel, src, pos,
        {wsize.x, wsize.y}, 0.0f, {0, 0}, color,
        layer, z, false, 0);
}

void Renderer::DrawText(const char* text, Vector2 pos, int fontSize, Color color) {
    ::DrawText(text, (int)pos.x, (int)pos.y, fontSize, color);
}

void Renderer::Shutdown() {
    if (m_whitePixelReady) {
        UnloadTexture(m_whitePixel);
        m_whitePixelReady = false;
    }
}

} // namespace View
