#include "View/InteractPrompt.h"
#include "View/Renderer.h"
#include "View/UIResourceManager.h"
#include <algorithm>

namespace View {

InteractPrompt& InteractPrompt::GetInstance() {
    static InteractPrompt inst;
    return inst;
}

bool InteractPrompt::LoadResources(const std::string& atlasJsonPath) {
    (void)atlasJsonPath;
    m_texIcon = ::LoadTexture("assets/ui/darkDwellers/20251125helpButton1-Sheet.png");
    if (m_texIcon.id != 0) {
        m_iconFrameW = m_texIcon.width / 4;
    }
    return true;
}

void InteractPrompt::Shutdown() {
    ::UnloadTexture(m_texIcon);
    m_texIcon = {};
    m_iconFrameW = 0;
}

void InteractPrompt::Show(const std::string& text) {
    m_text = text;
    m_visible = true;
}

void InteractPrompt::Hide() {
    m_visible = false;
    m_text.clear();
}

bool InteractPrompt::IsVisible() const {
    return m_visible;
}

void InteractPrompt::Render() {
    if (!m_visible) return;
    Renderer& r = Renderer::GetInstance();
    r.EndFrameAndFlush();
    const int w = ::GetScreenWidth();
    const int h = ::GetScreenHeight();
    const float scale = std::clamp(std::min(w / 1280.0f, h / 720.0f), 0.65f, 1.5f);
    const int fontSize = std::max(11, static_cast<int>(16 * scale));
    const float textWidth = static_cast<float>(::MeasureText(m_text.c_str(), fontSize));
    const float panelW = std::clamp(textWidth + 76 * scale, 220 * scale, 470 * scale);
    const float panelH = 44 * scale;
    Rectangle panel{(w - panelW) * 0.5f, h - 164 * scale, panelW, panelH};

    ::DrawRectangleRounded(panel, 0.45f, 10, Color{10,7,20,232});
    ::DrawRectangleRoundedLinesEx(panel, 0.45f, 10, 1.5f * scale, Color{111,90,145,245});
    Vector2 keyCenter{panel.x + 27 * scale, panel.y + panel.height * 0.5f};
    ::DrawCircleV(keyCenter, 15 * scale, Color{37,28,54,255});
    ::DrawCircleLinesV(keyCenter, 15 * scale, Color{239,198,96,255});
    ::DrawText("F", static_cast<int>(keyCenter.x - ::MeasureText("F", fontSize) * 0.5f),
               static_cast<int>(keyCenter.y - fontSize * 0.5f), fontSize, WHITE);
    ::DrawText(m_text.c_str(), static_cast<int>(panel.x + 52 * scale),
               static_cast<int>(panel.y + (panel.height - fontSize) * 0.5f), fontSize, WHITE);
}

} // namespace View
