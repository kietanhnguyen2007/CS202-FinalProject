#include "View/MenuView.h"
#include "View/Renderer.h"

using namespace View;

MenuView& MenuView::GetInstance() {
    static MenuView inst;
    return inst;
}

void MenuView::Init() {
    m_loaded = true;
}

bool MenuView::LoadResources(const std::string& atlasJsonPath) {
    (void)atlasJsonPath;
    m_loaded = true;
    return true;
}

void MenuView::Shutdown() { m_loaded = false; }

void MenuView::Update(float dt, int selectedIndex) {
    (void)dt;
    m_selected = selectedIndex;
}

void MenuView::Render() {
    if (!m_loaded || !m_visible) return;
    Renderer& r = Renderer::GetInstance();
    int w = r.GetWindowWidth();
    int h = r.GetWindowHeight();

    Vector2 center = { w*0.5f, h*0.35f };
    r.DrawText("Apple Knight Adventure", {center.x - 180, center.y - 60}, 32, WHITE);

    for (size_t i = 0; i < m_items.size(); ++i) {
        float y = center.y + 20 + (float)i * 40.0f;
        Color c = (i == (size_t)m_selected) ? YELLOW : WHITE;
        r.DrawText(m_items[i].c_str(), {center.x - 40, y}, 24, c);
    }
}
