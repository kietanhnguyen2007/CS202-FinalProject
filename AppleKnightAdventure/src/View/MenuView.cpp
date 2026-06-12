#include "View/MenuView.h"
#include "View/Renderer.h"
#include "Systems/SoundManager.h"

using namespace View;

MenuView& MenuView::GetInstance() {
    static MenuView inst;
    return inst;
}

bool MenuView::Init() {
    m_loaded = true;
    return true;
}

bool MenuView::LoadResources(const std::string& atlasJsonPath) {
    (void)atlasJsonPath;
    m_loaded = true;
    return true;
}

void MenuView::Shutdown() { m_loaded = false; }

void MenuView::Update(float dt, int selectedIndex) {
    (void)dt;
    if (selectedIndex != m_selected) {
        auto& sm = SoundManager::GetInstance();
        if (sm.IsAudioInitialized()) sm.PlaySound("ui_hover");
    }
    m_selected = selectedIndex;
}

void MenuView::ShowMainMenu() {
    m_mode = MenuMode::Main;
    m_selected = 0;
}

void MenuView::ShowPauseOverlay() {
    m_mode = MenuMode::Pause;
    m_selected = 0;
}

void MenuView::ShowErrorDialog(const std::string& message) {
    m_mode = MenuMode::Error;
    m_errorMsg = message;
    m_selected = 0;
}

void MenuView::ShowConnectionStatus(const std::string& ip, bool connected) {
    m_mode = MenuMode::Connection;
    m_connectionIp = ip;
    m_connected = connected;
    m_selected = 0;
}

void MenuView::Render() {
    if (!m_loaded || !m_visible) return;
    switch (m_mode) {
        case MenuMode::Main:       RenderMain();       break;
        case MenuMode::Pause:      RenderPause();      break;
        case MenuMode::Error:      RenderError();      break;
        case MenuMode::Connection: RenderConnection(); break;
    }
}

void MenuView::RenderMain() {
    Renderer& r = Renderer::GetInstance();
    int w = r.GetWindowWidth();
    int h = r.GetWindowHeight();
    Vector2 center = { w*0.5f, h*0.35f };
    r.DrawText("Apple Knight Adventure", {center.x - 180, center.y - 60}, 32, WHITE);

    for (size_t i = 0; i < m_mainItems.size(); ++i) {
        float y = center.y + 20 + (float)i * 40.0f;
        Color c = (i == (size_t)m_selected) ? YELLOW : WHITE;
        r.DrawText(m_mainItems[i].c_str(), {center.x - 40, y}, 24, c);
    }
}

void MenuView::RenderPause() {
    Renderer& r = Renderer::GetInstance();
    int w = r.GetWindowWidth();
    int h = r.GetWindowHeight();
    Vector2 center = { w*0.5f, h*0.35f };

    // Semi-transparent backdrop
    r.DrawRectangle({0,0}, {(float)w, (float)h}, {0,0,0,160}, Layer::UI, 0.0f);
    r.DrawText("PAUSED", {center.x - 60, center.y - 80}, 36, WHITE);

    for (size_t i = 0; i < m_pauseItems.size(); ++i) {
        float y = center.y - 20 + (float)i * 40.0f;
        Color c = (i == (size_t)m_selected) ? YELLOW : WHITE;
        r.DrawText(m_pauseItems[i].c_str(), {center.x - 60, y}, 24, c);
    }
}

void MenuView::RenderError() {
    Renderer& r = Renderer::GetInstance();
    int w = r.GetWindowWidth();
    int h = r.GetWindowHeight();
    Vector2 center = { w*0.5f, h*0.4f };

    r.DrawRectangle({center.x - 200, center.y - 80}, {400, 160}, {30,10,10,220}, Layer::UI, 0.0f);
    r.DrawText(m_errorMsg.c_str(), {center.x - 180, center.y - 50}, 18, RED);
    Color c = YELLOW;
    r.DrawText(m_errorItems[0].c_str(), {center.x - 20, center.y + 20}, 22, c);
}

void MenuView::RenderConnection() {
    Renderer& r = Renderer::GetInstance();
    int w = r.GetWindowWidth();
    int h = r.GetWindowHeight();
    Vector2 center = { w*0.5f, h*0.4f };

    r.DrawRectangle({center.x - 200, center.y - 80}, {400, 160}, {20,20,30,220}, Layer::UI, 0.0f);
    std::string status = m_connected ? "Connected" : "Connecting...";
    Color sc = m_connected ? GREEN : GRAY;
    r.DrawText(status.c_str(), {center.x - 60, center.y - 50}, 20, sc);
    r.DrawText(m_connectionIp.c_str(), {center.x - 80, center.y - 20}, 16, WHITE);
    Color c = (m_selected == 0) ? YELLOW : WHITE;
    r.DrawText(m_connectionItems[0].c_str(), {center.x - 20, center.y + 20}, 22, c);
}
