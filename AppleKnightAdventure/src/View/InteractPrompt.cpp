#include "View/InteractPrompt.h"
#include "View/Renderer.h"

namespace View {

InteractPrompt& InteractPrompt::GetInstance() {
    static InteractPrompt inst;
    return inst;
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
    int w = r.GetWindowWidth();
    int h = r.GetWindowHeight();

    Vector2 size = { 320.0f, 40.0f };
    Vector2 pos = { w * 0.5f - size.x * 0.5f, h * 0.85f };
    r.DrawRectangle(pos, size, { 0, 0, 0, 180 }, Layer::UI, 0.0f);
    r.DrawText(m_text.c_str(), { pos.x + 16, pos.y + 8 }, 18, WHITE);
}

} // namespace View