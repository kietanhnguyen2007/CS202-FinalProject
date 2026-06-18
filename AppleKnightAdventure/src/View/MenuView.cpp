#include "View/MenuView.h"
#include "View/Renderer.h"
#include "View/UIHelpers.h"
#include "View/UIResourceManager.h"
#include "Systems/SoundManager.h"

using namespace View;

// ---------------------------------------------------------------------------
// Singleton
// ---------------------------------------------------------------------------
MenuView& MenuView::GetInstance() {
    static MenuView inst;
    return inst;
}

// ---------------------------------------------------------------------------
// Init — called once before resources
// ---------------------------------------------------------------------------
bool MenuView::Init() {
    m_loaded = true;
    return true;
}

// ---------------------------------------------------------------------------
// LoadResources — load Dark Dwellers textures from disk
// ---------------------------------------------------------------------------
bool MenuView::LoadResources(const std::string& atlasJsonPath) {
    (void)atlasJsonPath;
    m_loaded = true;
    return true;
}

// ---------------------------------------------------------------------------
// Shutdown — unload all textures
// ---------------------------------------------------------------------------
void MenuView::Shutdown() {
    m_loaded = false;
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------
void MenuView::Update(float dt, int selectedIndex) {
    (void)dt;
    if (selectedIndex != m_selected) {
        auto& sm = SoundManager::GetInstance();
        if (sm.IsAudioInitialized()) sm.PlaySound("ui_hover");
    }
    m_selected = selectedIndex;
}

// ---------------------------------------------------------------------------
// Mode setters
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// Render dispatcher
// ---------------------------------------------------------------------------
void MenuView::Render() {
    if (!m_loaded || !m_visible) return;
    switch (m_mode) {
        case MenuMode::Main:       RenderMain();       break;
        case MenuMode::Pause:      RenderPause();      break;
        case MenuMode::Error:      RenderError();      break;
        case MenuMode::Connection: RenderConnection(); break;
    }
}

// ===========================================================================
// HELPERS
// ===========================================================================

// ---------------------------------------------------------------------------
// DrawButton — renders a single Dark Dwellers button with centred text label
// (x, y) = top-left corner;  (w, h) = desired size on screen
// selected: true → hover frame (1), false → normal frame (0)
// ---------------------------------------------------------------------------
void MenuView::DrawButton(const char* label, float x, float y,
                           float w, float h, bool selected) {
    Renderer& r = Renderer::GetInstance();
    auto& res = UIResourceManager::GetInstance();
    Texture2D* texBtn = res.GetButton();
    float btnFrameW = res.GetButtonFrameWidth();

    if (texBtn && texBtn->id != 0 && btnFrameW > 0) {
        int frame = selected ? 1 : 0; // 0=Normal, 1=Hover
        Rectangle src = {
            (float)(frame * btnFrameW), 0.0f,
            btnFrameW, (float)texBtn->height
        };
        float scaleX = w / btnFrameW;
        float scaleY = h / (float)texBtn->height;
        r.SubmitSprite(texBtn, src, {x, y}, {scaleX, scaleY},
                       0.0f, {0, 0}, WHITE, Layer::UI, 1.0f, false, 0);
    } else {
        // Fallback: solid rectangle
        Color bg = selected ? Color{80, 60, 120, 220} : Color{50, 40, 70, 200};
        r.DrawRectangle({x, y}, {w, h}, bg, Layer::UI, 1.0f);
    }

    // Centre text inside button
    int fontSize = (int)(h * 0.55f);
    if (fontSize < 10) fontSize = 10;
    int textW = ::MeasureText(label, fontSize);
    float tx = x + (w - (float)textW) * 0.5f;
    float ty = y + (h - (float)fontSize) * 0.5f;
    Color textCol = selected ? YELLOW : WHITE;
    r.DrawText(label, {tx, ty}, fontSize, textCol);
}

// ---------------------------------------------------------------------------
// DrawPanel — stretch the 9-slice panel texture to fill (x, y, w, h)
// ---------------------------------------------------------------------------
void MenuView::DrawPanel(float x, float y, float w, float h) {
    Renderer& r = Renderer::GetInstance();
    Texture2D* texPanel = UIResourceManager::GetInstance().GetPanelBg();

    if (texPanel && texPanel->id != 0) {
        int corner = texPanel->width / 3;
        NPatchInfo npi;
        npi.source = {0.0f, 0.0f, (float)texPanel->width, (float)texPanel->height};
        npi.left = corner;
        npi.top = corner;
        npi.right = corner;
        npi.bottom = corner;
        npi.layout = 0; // NPATCH_NINE_PATCH

        r.SubmitNPatch(texPanel, npi, {x, y, w, h}, WHITE, Layer::UI, 0.5f);
    } else {
        // Fallback: dark semi-transparent box
        r.DrawRectangle({x, y}, {w, h}, {20, 15, 30, 220}, Layer::UI, 0.5f);
    }
}

// ===========================================================================
// RENDER MODES
// ===========================================================================

// ---------------------------------------------------------------------------
// RenderMain — full-screen main menu
// ---------------------------------------------------------------------------
void MenuView::RenderMain() {
    Renderer& r = Renderer::GetInstance();
    int w = r.GetWindowWidth();
    int h = r.GetWindowHeight();

    // ---- Dark background overlay ----
    r.DrawRectangle({0, 0}, {(float)w, (float)h}, {10, 5, 20, 200}, Layer::UI, 0.0f);

    // ---- Header decoration behind title at ~25% height ----
    float titleY = h * 0.25f;
    Texture2D* texHeader = UIResourceManager::GetInstance().GetHeader();
    if (texHeader && texHeader->id != 0) {
        float headerW = w * 0.35f;  // 35% of screen width
        float headerH = headerW * ((float)texHeader->height / (float)texHeader->width);
        float headerX = (w - headerW) * 0.5f;
        float headerY = titleY - headerH * 0.3f;
        Rectangle hdrSrc = { 0.0f, 0.0f, (float)texHeader->width, (float)texHeader->height };
        float hScaleX = headerW / (float)texHeader->width;
        float hScaleY = headerH / (float)texHeader->height;
        r.SubmitSprite(texHeader, hdrSrc, {headerX, headerY}, {hScaleX, hScaleY},
                       0.0f, {0, 0}, WHITE, Layer::UI, 0.2f, false, 0);
    }

    // ---- Title text centred at ~30% height ----
    {
        const char* title = "Apple Knight Adventure";
        int titleFontSize = (int)(h * 0.055f);
        if (titleFontSize < 16) titleFontSize = 16;
        int titleTextW = ::MeasureText(title, titleFontSize);
        float tx = ((float)w - (float)titleTextW) * 0.5f;
        float ty = h * 0.28f;
        r.DrawText(title, {tx, ty}, titleFontSize, WHITE);
    }

    // ---- Menu buttons centred, starting at ~45% height ----
    float btnW = w * 0.20f;   // 20% screen width
    float btnH = h * 0.055f;  // 5.5% screen height
    float spacing = h * 0.07f; // 7% gap between button tops
    float startY = h * 0.45f;
    float btnX = ((float)w - btnW) * 0.5f;

    for (size_t i = 0; i < m_mainItems.size(); ++i) {
        float by = startY + (float)i * spacing;
        bool sel = ((int)i == m_selected);
        DrawButton(m_mainItems[i].c_str(), btnX, by, btnW, btnH, sel);
    }
}

// ---------------------------------------------------------------------------
// RenderPause — semi-transparent overlay with panel
// ---------------------------------------------------------------------------
void MenuView::RenderPause() {
    Renderer& r = Renderer::GetInstance();
    int w = r.GetWindowWidth();
    int h = r.GetWindowHeight();

    // ---- Dim overlay ----
    r.DrawRectangle({0, 0}, {(float)w, (float)h}, {0, 0, 0, 160}, Layer::UI, 0.0f);

    // ---- Panel centred, ~35% width × ~40% height ----
    float panelW = w * 0.35f;
    float panelH = h * 0.40f;
    float panelX = ((float)w - panelW) * 0.5f;
    float panelY = ((float)h - panelH) * 0.5f;
    DrawPanel(panelX, panelY, panelW, panelH);

    // ---- Header decoration at top of panel ----
    Texture2D* texHeader = UIResourceManager::GetInstance().GetHeader();
    if (texHeader && texHeader->id != 0) {
        float hdrW = panelW * 0.8f;
        float hdrH = hdrW * ((float)texHeader->height / (float)texHeader->width);
        float hdrX = panelX + (panelW - hdrW) * 0.5f;
        float hdrY = panelY + panelH * 0.04f;
        Rectangle hdrSrc = { 0.0f, 0.0f, (float)texHeader->width, (float)texHeader->height };
        float hsX = hdrW / (float)texHeader->width;
        float hsY = hdrH / (float)texHeader->height;
        r.SubmitSprite(texHeader, hdrSrc, {hdrX, hdrY}, {hsX, hsY},
                       0.0f, {0, 0}, WHITE, Layer::UI, 0.8f, false, 0);
    }

    // ---- "PAUSED" title ----
    {
        const char* title = "PAUSED";
        int fontSize = (int)(h * 0.05f);
        if (fontSize < 14) fontSize = 14;
        int tw = ::MeasureText(title, fontSize);
        float tx = panelX + (panelW - (float)tw) * 0.5f;
        float ty = panelY + panelH * 0.12f;
        r.DrawText(title, {tx, ty}, fontSize, WHITE);
    }

    // ---- Buttons inside panel ----
    float btnW = panelW * 0.65f;
    float btnH = panelH * 0.13f;
    float btnX = panelX + (panelW - btnW) * 0.5f;
    float btnStartY = panelY + panelH * 0.40f;
    float spacing = panelH * 0.18f;

    for (size_t i = 0; i < m_pauseItems.size(); ++i) {
        float by = btnStartY + (float)i * spacing;
        bool sel = ((int)i == m_selected);
        DrawButton(m_pauseItems[i].c_str(), btnX, by, btnW, btnH, sel);
    }
}

// ---------------------------------------------------------------------------
// RenderError — panel with error message + OK button
// ---------------------------------------------------------------------------
void MenuView::RenderError() {
    Renderer& r = Renderer::GetInstance();
    int w = r.GetWindowWidth();
    int h = r.GetWindowHeight();

    // ---- Dim overlay ----
    r.DrawRectangle({0, 0}, {(float)w, (float)h}, {0, 0, 0, 140}, Layer::UI, 0.0f);

    // ---- Panel centred, ~35% width × ~30% height ----
    float panelW = w * 0.35f;
    float panelH = h * 0.30f;
    float panelX = ((float)w - panelW) * 0.5f;
    float panelY = ((float)h - panelH) * 0.5f;
    DrawPanel(panelX, panelY, panelW, panelH);

    // ---- Header decoration ----
    Texture2D* texHeader = UIResourceManager::GetInstance().GetHeader();
    if (texHeader && texHeader->id != 0) {
        float hdrW = panelW * 0.7f;
        float hdrH = hdrW * ((float)texHeader->height / (float)texHeader->width);
        float hdrX = panelX + (panelW - hdrW) * 0.5f;
        float hdrY = panelY + panelH * 0.04f;
        Rectangle hdrSrc = { 0.0f, 0.0f, (float)texHeader->width, (float)texHeader->height };
        r.SubmitSprite(texHeader, hdrSrc, {hdrX, hdrY},
                       {hdrW / (float)texHeader->width, hdrH / (float)texHeader->height},
                       0.0f, {0, 0}, WHITE, Layer::UI, 0.8f, false, 0);
    }

    // ---- "ERROR" label ----
    {
        const char* title = "ERROR";
        int fontSize = (int)(h * 0.04f);
        if (fontSize < 12) fontSize = 12;
        int tw = ::MeasureText(title, fontSize);
        float tx = panelX + (panelW - (float)tw) * 0.5f;
        float ty = panelY + panelH * 0.10f;
        r.DrawText(title, {tx, ty}, fontSize, RED);
    }

    // ---- Error message body ----
    {
        int msgFontSize = (int)(h * 0.025f);
        if (msgFontSize < 10) msgFontSize = 10;
        int msgW = ::MeasureText(m_errorMsg.c_str(), msgFontSize);
        float mx = panelX + (panelW - (float)msgW) * 0.5f;
        float my = panelY + panelH * 0.35f;
        r.DrawText(m_errorMsg.c_str(), {mx, my}, msgFontSize, WHITE);
    }

    // ---- OK button ----
    float btnW = panelW * 0.35f;
    float btnH = panelH * 0.16f;
    float btnX = panelX + (panelW - btnW) * 0.5f;
    float btnY = panelY + panelH * 0.70f;
    DrawButton(m_errorItems[0].c_str(), btnX, btnY, btnW, btnH, (m_selected == 0));
}

// ---------------------------------------------------------------------------
// RenderConnection — panel with IP, status, and Back button
// ---------------------------------------------------------------------------
void MenuView::RenderConnection() {
    Renderer& r = Renderer::GetInstance();
    int w = r.GetWindowWidth();
    int h = r.GetWindowHeight();

    // ---- Dim overlay ----
    r.DrawRectangle({0, 0}, {(float)w, (float)h}, {0, 0, 0, 140}, Layer::UI, 0.0f);

    // ---- Panel centred, ~35% width × ~35% height ----
    float panelW = w * 0.35f;
    float panelH = h * 0.35f;
    float panelX = ((float)w - panelW) * 0.5f;
    float panelY = ((float)h - panelH) * 0.5f;
    DrawPanel(panelX, panelY, panelW, panelH);

    // ---- Header decoration ----
    Texture2D* texHeader = UIResourceManager::GetInstance().GetHeader();
    if (texHeader && texHeader->id != 0) {
        float hdrW = panelW * 0.7f;
        float hdrH = hdrW * ((float)texHeader->height / (float)texHeader->width);
        float hdrX = panelX + (panelW - hdrW) * 0.5f;
        float hdrY = panelY + panelH * 0.04f;
        Rectangle hdrSrc = { 0.0f, 0.0f, (float)texHeader->width, (float)texHeader->height };
        r.SubmitSprite(texHeader, hdrSrc, {hdrX, hdrY},
                       {hdrW / (float)texHeader->width, hdrH / (float)texHeader->height},
                       0.0f, {0, 0}, WHITE, Layer::UI, 0.8f, false, 0);
    }

    // ---- Connection status text ----
    {
        const char* statusLabel = m_connected ? "Connected" : "Connecting...";
        Color statusColor = m_connected ? GREEN : GRAY;
        int fontSize = (int)(h * 0.035f);
        if (fontSize < 12) fontSize = 12;
        int tw = ::MeasureText(statusLabel, fontSize);
        float tx = panelX + (panelW - (float)tw) * 0.5f;
        float ty = panelY + panelH * 0.15f;
        r.DrawText(statusLabel, {tx, ty}, fontSize, statusColor);
    }

    // ---- IP address ----
    {
        int ipFontSize = (int)(h * 0.025f);
        if (ipFontSize < 10) ipFontSize = 10;
        int ipW = ::MeasureText(m_connectionIp.c_str(), ipFontSize);
        float ix = panelX + (panelW - (float)ipW) * 0.5f;
        float iy = panelY + panelH * 0.35f;
        r.DrawText(m_connectionIp.c_str(), {ix, iy}, ipFontSize, WHITE);
    }

    // ---- Back button ----
    float btnW = panelW * 0.35f;
    float btnH = panelH * 0.14f;
    float btnX = panelX + (panelW - btnW) * 0.5f;
    float btnY = panelY + panelH * 0.70f;
    DrawButton(m_connectionItems[0].c_str(), btnX, btnY, btnW, btnH, (m_selected == 0));
}
