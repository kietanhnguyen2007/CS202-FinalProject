// =============================================================================
// MenuView.cpp — Apple Knight Adventure
// Full implementation: Parallax BG, 2-pass Gaussian Blur, Animated Buttons
// with Scale-Lerp hover, glow rings, title bobbing, header HUD.
// =============================================================================
#include "View/MenuView.h"
#include "View/Renderer.h"
#include "View/UIHelpers.h"
#include "View/UIResourceManager.h"
#include "Systems/SoundManager.h"
#include "Systems/TweenSystem.h"
#include "Systems/AchievementManager.h"
#include "Model/SaveManager.h"
#include <cmath>
#include <cstdio>
#include <algorithm>
#include <cctype>

// Needed for M_PI on MSVC
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace View;

// ─────────────────────────────────────────────────────────────────────────────
// Inline easing helpers (local to this TU)
// ─────────────────────────────────────────────────────────────────────────────
static inline float EaseOutCubic(float t) {
    float f = 1.0f - t;
    return 1.0f - f * f * f;
}

static inline float Lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

namespace {
struct PauseLayout {
    Rectangle panel{};
    Rectangle menuPane{};
    Rectangle infoPane{};
    Rectangle buttons[3]{};
};

struct CustomMapLayout {
    Rectangle panel{};
    Rectangle list{};
    Rectangle actions[3]{};
    Rectangle confirmActions[2]{};
    float rowHeight{};
};

struct RecordsLayout {
    Rectangle panel{};
    Rectangle levelTabs[6]{};
    Rectangle modeTabs[2]{};
    Rectangle table{};
};

RecordsLayout BuildRecordsLayout(float w, float h) {
    RecordsLayout l;
    const float panelW = std::min(std::max(w * 0.84f, 620.0f), w - 30.0f);
    const float panelH = std::min(std::max(h * 0.82f, 410.0f), h - 26.0f);
    l.panel = {(w - panelW) * 0.5f, (h - panelH) * 0.5f, panelW, panelH};
    const float inset = std::clamp(panelW * 0.035f, 18.0f, 32.0f);
    const float gap = std::clamp(panelW * 0.012f, 6.0f, 12.0f);
    const float tabW = (panelW - inset * 2.0f - gap * 5.0f) / 6.0f;
    const float tabY = l.panel.y + std::clamp(panelH * 0.16f, 68.0f, 92.0f);
    const float tabH = std::clamp(panelH * 0.075f, 34.0f, 46.0f);
    for (int i = 0; i < 6; ++i)
        l.levelTabs[i] = {l.panel.x + inset + i * (tabW + gap), tabY, tabW, tabH};
    const float modeW = std::min(190.0f, (panelW - inset * 2.0f - gap) * 0.5f);
    const float modeY = tabY + tabH + std::clamp(panelH * 0.025f, 10.0f, 16.0f);
    l.modeTabs[0] = {l.panel.x + panelW * 0.5f - modeW - gap * 0.5f, modeY, modeW, tabH};
    l.modeTabs[1] = {l.panel.x + panelW * 0.5f + gap * 0.5f, modeY, modeW, tabH};
    const float tableY = modeY + tabH + std::clamp(panelH * 0.025f, 10.0f, 16.0f);
    l.table = {l.panel.x + inset, tableY, panelW - inset * 2.0f,
               l.panel.y + panelH - inset - tableY};
    return l;
}

Rectangle AchievementNodeRect(const AchievementDefinition& definition, Rectangle content) {
    const float node = std::clamp(std::min(content.width / 8.0f, content.height / 5.2f), 30.0f, 82.0f);
    const float stepX = (content.width - node) / 4.0f;
    const float stepY = (content.height - node) / 3.0f;
    return {content.x + definition.column * stepX,
            content.y + definition.row * stepY, node, node};
}

PauseLayout BuildPauseLayout(float w, float h) {
    PauseLayout l;
    const float panelW = std::min(std::max(w * 0.68f, 520.0f), w - 32.0f);
    const float panelH = std::min(std::max(h * 0.64f, 300.0f), h - 28.0f);
    l.panel = {(w - panelW) * 0.5f, (h - panelH) * 0.5f, panelW, panelH};
    const float inset = std::clamp(panelW * 0.035f, 18.0f, 30.0f);
    const float contentY = l.panel.y + std::clamp(panelH * 0.27f, 78.0f, 108.0f);
    const float contentH = l.panel.y + panelH - inset - contentY;
    const float gap = std::clamp(panelW * 0.028f, 14.0f, 24.0f);
    const float leftW = (panelW - inset * 2.0f - gap) * 0.48f;
    l.menuPane = {l.panel.x + inset, contentY, leftW, contentH};
    l.infoPane = {l.menuPane.x + leftW + gap, contentY,
                  panelW - inset * 2.0f - gap - leftW, contentH};

    const float buttonGap = std::clamp(contentH * 0.055f, 8.0f, 14.0f);
    const float buttonH = std::min(58.0f,
        std::max(38.0f, (contentH - buttonGap * 2.0f) / 3.0f));
    for (int i = 0; i < 3; ++i) {
        l.buttons[i] = {l.menuPane.x, l.menuPane.y + i * (buttonH + buttonGap),
                        l.menuPane.width, buttonH};
    }
    return l;
}

CustomMapLayout BuildCustomMapLayout(float w, float h) {
    CustomMapLayout l;
    const float panelW = std::min(std::max(w * 0.70f, 560.0f), w - 34.0f);
    const float panelH = std::min(std::max(h * 0.72f, 330.0f), h - 30.0f);
    l.panel = {(w - panelW) * 0.5f, (h - panelH) * 0.5f, panelW, panelH};
    const float inset = std::clamp(panelW * 0.045f, 20.0f, 34.0f);
    const float actionH = std::clamp(panelH * 0.105f, 38.0f, 54.0f);
    l.list = {l.panel.x + inset, l.panel.y + std::clamp(panelH * 0.24f, 80.0f, 112.0f),
              panelW - inset * 2.0f,
              panelH - std::clamp(panelH * 0.24f, 80.0f, 112.0f) - actionH - inset * 1.35f};
    l.rowHeight = std::clamp(l.list.height / 5.3f, 38.0f, 54.0f);
    const float gap = std::clamp(panelW * 0.02f, 10.0f, 17.0f);
    const float buttonW = (l.list.width - gap * 2.0f) / 3.0f;
    const float actionY = l.panel.y + panelH - inset - actionH;
    for (int i = 0; i < 3; ++i) {
        l.actions[i] = {l.list.x + i * (buttonW + gap), actionY, buttonW, actionH};
    }
    const float confirmW = std::min(150.0f, l.list.width * 0.30f);
    l.confirmActions[0] = {w * 0.5f - confirmW - 8.0f, h * 0.5f + 32.0f,
                           confirmW, 44.0f};
    l.confirmActions[1] = {w * 0.5f + 8.0f, h * 0.5f + 32.0f,
                           confirmW, 44.0f};
    return l;
}
} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Singleton
// ─────────────────────────────────────────────────────────────────────────────
MenuView& MenuView::GetInstance() {
    static MenuView inst;
    return inst;
}

// ─────────────────────────────────────────────────────────────────────────────
// Init
// ─────────────────────────────────────────────────────────────────────────────
bool MenuView::Init() {
    m_loaded = true;
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// LoadResources
// ─────────────────────────────────────────────────────────────────────────────
bool MenuView::LoadResources(const std::string& atlasJsonPath) {
    (void)atlasJsonPath;

    // ── Parallax layers ──────────────────────────────────────────────────
    LoadParallaxLayers();

    // ── Blur shader (2-pass Gaussian) ────────────────────────────────────
    InitBlurShader();

    // ── Fonts ─────────────────────────────────────────────────────────────
    // Try custom fonts first, fall back to raylib default
    m_fontTitle = ::LoadFont("assets/fonts/game_font.ttf");
    m_fontBody  = ::LoadFont("assets/fonts/game_font.ttf");
    m_levelStarIcon = ::LoadTexture("assets/ui/victory/icon_star.png");
    const char* achievementPaths[7] = {
        "assets/ui/victory/icon_trophy.png",
        "assets/ui/victory/icon_star.png",
        "assets/ui/victory/icon_target.png",
        "assets/textures/items/coin.png",
        "assets/textures/items/potion_red.png",
        "assets/textures/player/magic_caster_v2/idle_v2.png",
        "assets/textures/player/ninja_v2/idle_v2.png"
    };
    for (int i = 0; i < 7; ++i) {
        m_achievementIcons[i] = ::LoadTexture(achievementPaths[i]);
        m_achievementIconSources[i] = {0, 0,
            (float)m_achievementIcons[i].width, (float)m_achievementIcons[i].height};
    }
    if (m_achievementIcons[3].id != 0)
        m_achievementIconSources[3].width = (float)m_achievementIcons[3].height;
    if (m_achievementIcons[5].id != 0)
        m_achievementIconSources[5].width = std::min(128.0f, (float)m_achievementIcons[5].width);
    if (m_achievementIcons[6].id != 0)
        m_achievementIconSources[6].width = std::min(128.0f, (float)m_achievementIcons[6].width);
    if (m_fontTitle.texture.id != 0 && m_fontBody.texture.id != 0) {
        m_fontsLoaded = true;
    } else {
        // Unload any partially-loaded fonts and mark as not loaded
        if (m_fontTitle.texture.id != 0) ::UnloadFont(m_fontTitle);
        if (m_fontBody.texture.id  != 0) ::UnloadFont(m_fontBody);
        m_fontsLoaded = false;
    }

    m_loaded = true;
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Shutdown
// ─────────────────────────────────────────────────────────────────────────────
void MenuView::Shutdown() {
    UnloadParallaxLayers();
    ShutdownBlurShader();
    if (m_fontsLoaded) {
        ::UnloadFont(m_fontTitle);
        ::UnloadFont(m_fontBody);
        m_fontsLoaded = false;
    }
    if (m_levelStarIcon.id != 0) {
        ::UnloadTexture(m_levelStarIcon);
        m_levelStarIcon = {};
    }
    for (auto& texture : m_achievementIcons) {
        if (texture.id != 0) ::UnloadTexture(texture);
        texture = {};
    }
    m_loaded = false;
}

// ─────────────────────────────────────────────────────────────────────────────
// SetHeaderData — fed from MenuController after reading SaveManager
// ─────────────────────────────────────────────────────────────────────────────
void MenuView::SetHeaderData(const std::string& playerName, int coins) {
    m_playerName = playerName;
    m_coins      = coins;
}

void MenuView::ShowMainNotice(const std::string& message) {
    m_mainNotice = message;
    m_mainNoticeTimer = 3.5f;
}

Rectangle MenuView::GetMainButtonRect(int index, int screenW, int screenH) const {
    constexpr int columns = 2;
    const int column = index % columns;
    const int row = index / columns;
    const float panelW = screenW * 0.62f;
    const float gapX = screenW * 0.022f;
    const float gapY = screenH * 0.016f;
    const float buttonW = (panelW-gapX)/2.0f;
    const float buttonH = screenH * 0.068f;
    const float startX = (screenW-panelW)*0.5f;
    const float startY = screenH*0.47f;
    // The ninth item (Quit) occupies the final row by itself; centering it
    // keeps the expanded menu balanced instead of leaving a visual hole.
    const float x = (index == 8)
        ? (screenW-buttonW)*0.5f
        : startX+column*(buttonW+gapX);
    return {x,startY+row*(buttonH+gapY),buttonW,buttonH};
}

// ─────────────────────────────────────────────────────────────────────────────
// Update
// ─────────────────────────────────────────────────────────────────────────────
void MenuView::Update(float dt, int selectedIndex) {
    if (!m_loaded || !m_visible) return;

    // ── Sound feedback on selection change ───────────────────────────────
    if (selectedIndex != m_selected) {
        auto& sm = SoundManager::GetInstance();
        if (sm.IsAudioInitialized()) sm.PlaySound("ui_hover");
    }
    m_selected = selectedIndex;

    // ── Scroll time ───────────────────────────────────────────────────────
    m_scrollTime    += dt;
    m_titleBobTime  += dt;
    m_spotlightPulse += dt;
    if (m_mainNoticeTimer > 0.0f) {
        m_mainNoticeTimer = std::max(0.0f, m_mainNoticeTimer - dt);
    }

    // ── Parallax ──────────────────────────────────────────────────────────
    UpdateParallax(dt);

    // ── Smooth shop scroll ────────────────────────────────────────────────
    m_shopScrollDisp = Lerp(m_shopScrollDisp, (float)m_shopScrollYF, 12.0f * dt);

    // ── Animated button hover update ─────────────────────────────────────
    if (m_mode == MenuMode::Main) {
        Vector2 mouse = ::GetMousePosition();
        int w = Renderer::GetInstance().GetWindowWidth();
        int h = Renderer::GetInstance().GetWindowHeight();

        for (int i = 0; i < (int)m_mainItems.size() && i < kMaxMainButtons; ++i) {
            m_mainButtons[i].baseRect = GetMainButtonRect(i, w, h);
            UpdateButtonHover(m_mainButtons[i], mouse, dt);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Mode setters
// ─────────────────────────────────────────────────────────────────────────────
void MenuView::ShowMainMenu() {
    m_mode     = MenuMode::Main;
    m_selected = 0;
    m_visible  = true;
}
void MenuView::ShowPauseOverlay() {
    m_mode     = MenuMode::Pause;
    m_selected = 0;
    m_visible  = true;
}
void MenuView::ShowErrorDialog(const std::string& message) {
    m_mode     = MenuMode::Error;
    m_errorMsg = message;
    m_selected = 0;
}
void MenuView::ShowConnectionStatus(const std::string& ip, bool connected) {
    m_mode          = MenuMode::Connection;
    m_connectionIp  = ip;
    m_connected     = connected;
    m_selected      = 0;
}
void MenuView::ShowRoleSelect(const std::vector<std::string>& roles) {
    m_mode      = MenuMode::RoleSelect;
    m_roleItems = roles;
    m_selected  = 0;
}
void MenuView::ShowShop() {
    m_mode          = MenuMode::Shop;
    m_selected      = 0;
    m_shopScrollYF  = 0;
    m_shopScrollDisp = 0;
}
void MenuView::ShowOptions() {
    m_mode     = MenuMode::Options;
    m_selected = 0;
    m_visible  = true;
}
void MenuView::ShowLevelSelect(int totalLevels, int currentUnlocked) {
    const bool entering = m_mode != MenuMode::LevelSelect;
    m_mode           = MenuMode::LevelSelect;
    m_totalLevels    = totalLevels;
    m_unlockedLevels = currentUnlocked;
    if (entering) m_selected = 0;
}

void MenuView::ShowCustomMaps(const std::vector<std::string>& mapNames,
                              const std::string& deleteConfirmation) {
    const bool entering = m_mode != MenuMode::CustomMaps;
    m_mode = MenuMode::CustomMaps;
    m_customMapNames = mapNames;
    m_customMapDeleteConfirmation = deleteConfirmation;
    if (entering) m_selected = 0;
    if (m_customMapNames.empty()) m_selected = 0;
    else m_selected = std::clamp(m_selected, 0, (int)m_customMapNames.size() - 1);
    m_visible = true;
}

void MenuView::ShowLeaderboard(int level, bool fastestTime) {
    m_mode = MenuMode::Leaderboard;
    SetLeaderboardSelection(level, fastestTime);
    m_visible = true;
}

void MenuView::ShowAchievements(int selectedAchievement) {
    m_mode = MenuMode::Achievements;
    SetAchievementSelection(selectedAchievement);
    m_visible = true;
}

void MenuView::SetLeaderboardSelection(int level, bool fastestTime) {
    m_leaderboardLevel = std::clamp(level, 1, 6);
    m_leaderboardFastest = fastestTime;
    m_selected = m_leaderboardLevel - 1;
}

void MenuView::SetAchievementSelection(int index) {
    const int count = (int)AchievementManager::GetInstance().GetDefinitions().size();
    m_achievementSelected = count > 0 ? std::clamp(index, 0, count - 1) : 0;
    m_selected = m_achievementSelected;
}

void MenuView::SetLevelStars(const std::vector<int>& bestStars) {
    m_levelBestStars = bestStars;
}

// ─────────────────────────────────────────────────────────────────────────────
// Render dispatcher
// ─────────────────────────────────────────────────────────────────────────────
void MenuView::Render() {
    if (!m_loaded || !m_visible) return;

    switch (m_mode) {
        case MenuMode::Main:        RenderMain();        break;
        case MenuMode::Pause:       RenderPause();       break;
        case MenuMode::Error:       RenderError();       break;
        case MenuMode::Connection:  RenderConnection();  break;
        case MenuMode::RoleSelect:  RenderRoleSelect();  break;
        case MenuMode::Shop:        RenderShop();        break;
        case MenuMode::LevelSelect: RenderLevelSelect(); break;
        case MenuMode::CustomMaps:  RenderCustomMaps();  break;
        case MenuMode::Leaderboard: RenderLeaderboard(); break;
        case MenuMode::Achievements: RenderAchievements(); break;
        case MenuMode::Options:     /* handled externally */    break;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// GetHoveredItem
// ─────────────────────────────────────────────────────────────────────────────
int MenuView::GetHoveredItem(Vector2 mousePos) const {
    if (!m_loaded || !m_visible) return -1;

    int w = Renderer::GetInstance().GetWindowWidth();
    int h = Renderer::GetInstance().GetWindowHeight();

    if (m_mode == MenuMode::Main) {
        for (int i = 0; i < (int)m_mainItems.size(); ++i) {
            Rectangle rect = GetMainButtonRect(i,w,h);
            const float expand = rect.width*0.04f;
            rect.x -= expand*0.5f;
            rect.width += expand;
            if (::CheckCollisionPointRec(mousePos, rect)) return i;
        }
    } else if (m_mode == MenuMode::Pause) {
        const PauseLayout layout = BuildPauseLayout((float)w, (float)h);
        for (int i = 0; i < (int)m_pauseItems.size(); ++i) {
            if (::CheckCollisionPointRec(mousePos, layout.buttons[i])) return i;
        }
    } else if (m_mode == MenuMode::LevelSelect) {
        constexpr int columns = 3;
        const int rows = std::max(1, (m_totalLevels + columns - 1) / columns);
        const float gridW = w * 0.76f;
        const float gapX = w * 0.035f;
        const float gapY = h * 0.045f;
        const float cardW = (gridW - gapX * (columns - 1)) / columns;
        const float cardH = std::min(h * 0.255f,
            (h * 0.64f - gapY * (rows - 1)) / rows);
        const float startX = (w - gridW) * 0.5f;
        const float startY = h * 0.19f;
        for (int i = 0; i < m_totalLevels; ++i) {
            Rectangle card = {
                startX + (i % columns) * (cardW + gapX),
                startY + (i / columns) * (cardH + gapY),
                cardW, cardH
            };
            if (CheckCollisionPointRec(mousePos, card)) return i;
        }
    } else if (m_mode == MenuMode::CustomMaps) {
        const CustomMapLayout layout = BuildCustomMapLayout((float)w, (float)h);
        const int visibleRows = std::max(1, (int)(layout.list.height / layout.rowHeight));
        const int start = m_selected >= visibleRows ? m_selected - visibleRows + 1 : 0;
        for (int row = 0; row < visibleRows; ++row) {
            const int index = start + row;
            if (index >= (int)m_customMapNames.size()) break;
            Rectangle rect = {layout.list.x, layout.list.y + row * layout.rowHeight,
                              layout.list.width, layout.rowHeight - 6.0f};
            if (::CheckCollisionPointRec(mousePos, rect)) return index;
        }
    } else if (m_mode == MenuMode::Leaderboard) {
        const RecordsLayout layout = BuildRecordsLayout((float)w, (float)h);
        for (int i = 0; i < 6; ++i)
            if (::CheckCollisionPointRec(mousePos, layout.levelTabs[i])) return i;
    } else if (m_mode == MenuMode::Achievements) {
        const RecordsLayout layout = BuildRecordsLayout((float)w, (float)h);
        Rectangle content{layout.panel.x + 54.0f, layout.panel.y + 112.0f,
                          layout.panel.width - 108.0f, layout.panel.height - 205.0f};
        const auto& definitions = AchievementManager::GetInstance().GetDefinitions();
        for (int i = 0; i < (int)definitions.size(); ++i)
            if (::CheckCollisionPointRec(mousePos, AchievementNodeRect(definitions[i], content))) return i;
    }
    return -1;
}

int MenuView::GetLeaderboardModeHovered(Vector2 mousePos) const {
    if (m_mode != MenuMode::Leaderboard) return -1;
    const RecordsLayout layout = BuildRecordsLayout(
        (float)Renderer::GetInstance().GetWindowWidth(),
        (float)Renderer::GetInstance().GetWindowHeight());
    for (int i = 0; i < 2; ++i)
        if (::CheckCollisionPointRec(mousePos, layout.modeTabs[i])) return i;
    return -1;
}

int MenuView::GetCustomMapActionHovered(Vector2 mousePos) const {
    if (m_mode != MenuMode::CustomMaps) return -1;
    const CustomMapLayout layout = BuildCustomMapLayout(
        (float)Renderer::GetInstance().GetWindowWidth(),
        (float)Renderer::GetInstance().GetWindowHeight());
    if (!m_customMapDeleteConfirmation.empty()) {
        for (int i = 0; i < 2; ++i)
            if (::CheckCollisionPointRec(mousePos, layout.confirmActions[i])) return i + 3;
        return -1;
    }
    for (int i = 0; i < 3; ++i)
        if (::CheckCollisionPointRec(mousePos, layout.actions[i])) return i;
    return -1;
}

// =============================================================================
// ── PARALLAX ────────────────────────────────────────────────────────────────
// =============================================================================
void MenuView::LoadParallaxLayers() {
    // Layer 0 = sky (slowest), Layer 1 = midground, Layer 2 = foreground (fastest)
    const char* paths[kParallaxLayers] = {
        "assets/textures/backgrounds/forest/back.png",
        "assets/textures/backgrounds/forest/middle.png",
        "assets/textures/backgrounds/forest/front.png"
    };
    const float speeds[kParallaxLayers] = { 0.10f, 0.25f, 0.55f };

    for (int i = 0; i < kParallaxLayers; ++i) {
        if (::FileExists(paths[i])) {
            m_parallaxLayers[i].tex    = ::LoadTexture(paths[i]);
            m_parallaxLayers[i].loaded = (m_parallaxLayers[i].tex.id != 0);
        }
        m_parallaxLayers[i].speedMultiplier = speeds[i];
        m_parallaxLayers[i].offsetX         = 0.0f;
    }
}

void MenuView::UnloadParallaxLayers() {
    for (int i = 0; i < kParallaxLayers; ++i) {
        if (m_parallaxLayers[i].loaded) {
            ::UnloadTexture(m_parallaxLayers[i].tex);
            m_parallaxLayers[i] = {};
        }
    }
}

void MenuView::UpdateParallax(float dt) {
    int w = Renderer::GetInstance().GetWindowWidth();
    for (int i = 0; i < kParallaxLayers; ++i) {
        auto& layer = m_parallaxLayers[i];
        layer.offsetX += m_parallaxBaseSpeed * layer.speedMultiplier * dt;
        // Wrap around when we've scrolled one texture width
        if (layer.loaded && layer.tex.width > 0) {
            float tw = (float)layer.tex.width;
            float scale = (float)w / tw;  // scale to fill screen width
            float scaledW = tw * scale;
            if (layer.offsetX >= scaledW) {
                layer.offsetX -= scaledW;
            }
        }
    }
}

void MenuView::RenderParallax() {
    int w = Renderer::GetInstance().GetWindowWidth();
    int h = Renderer::GetInstance().GetWindowHeight();

    // Dark base fill in case layers don't cover
    ::DrawRectangle(0, 0, w, h, Color{8, 6, 18, 255});

    for (int i = 0; i < kParallaxLayers; ++i) {
        auto& layer = m_parallaxLayers[i];
        if (!layer.loaded) continue;

        float tw    = (float)layer.tex.width;
        float th    = (float)layer.tex.height;
        float scale = (float)h / th;           // fit to screen height
        float sw    = tw * scale;               // scaled width
        float sh    = (float)h;

        // Draw 2 copies side-by-side for seamless loop
        float x0 = -layer.offsetX * scale / 1.0f;  // adjust for scale
        // Actually, simpler: offset in world coords then scale
        // Draw copies until screen is covered
        float xDraw = fmodf(-layer.offsetX, sw);
        if (xDraw > 0) xDraw -= sw;

        while (xDraw < (float)w) {
            Rectangle src = { 0.0f, 0.0f, tw, th };
            Rectangle dst = { xDraw, 0.0f, sw, sh };
            ::DrawTexturePro(layer.tex, src, dst, {0, 0}, 0.0f, WHITE);
            xDraw += sw;
        }
    }
}

// =============================================================================
// ── BLUR SHADER (2-pass Gaussian) ───────────────────────────────────────────
// =============================================================================
void MenuView::InitBlurShader() {
    if (!::FileExists("assets/shaders/blur.fs")) {
        m_blurReady = false;
        return;
    }

    m_blurShader = ::LoadShader(nullptr, "assets/shaders/blur.fs");
    if (m_blurShader.id == 0) {
        m_blurReady = false;
        return;
    }

    m_locDirection  = ::GetShaderLocation(m_blurShader, "direction");
    m_locResolution = ::GetShaderLocation(m_blurShader, "resolution");

    int w = Renderer::GetInstance().GetWindowWidth();
    int h = Renderer::GetInstance().GetWindowHeight();

    m_blurPassA = ::LoadRenderTexture(w, h);
    m_blurPassB = ::LoadRenderTexture(w, h);
    m_blurReady = (m_blurPassA.id != 0 && m_blurPassB.id != 0);
}

void MenuView::ShutdownBlurShader() {
    if (m_blurReady) {
        ::UnloadRenderTexture(m_blurPassA);
        ::UnloadRenderTexture(m_blurPassB);
        ::UnloadShader(m_blurShader);
        m_blurReady = false;
    }
}

// Apply blur to whatever is in m_blurPassA.texture → output in m_blurPassB
// (Caller must have rendered scene into m_blurPassA before calling this)
void MenuView::ApplyBlurToScreen() {
    if (!m_blurReady) return;

    int w = Renderer::GetInstance().GetWindowWidth();
    int h = Renderer::GetInstance().GetWindowHeight();

    float resolution[2] = { (float)w, (float)h };
    ::SetShaderValue(m_blurShader, m_locResolution, resolution, SHADER_UNIFORM_VEC2);

    // ── Pass 1: Horizontal (blurPassA → blurPassB) ───────────────────────
    ::BeginTextureMode(m_blurPassB);
        ::ClearBackground(BLACK);
        ::BeginShaderMode(m_blurShader);
            float dirH[2] = { 1.0f, 0.0f };
            ::SetShaderValue(m_blurShader, m_locDirection, dirH, SHADER_UNIFORM_VEC2);
            // Flip Y: raylib RenderTexture is flipped
            Rectangle srcA = { 0.f, 0.f, (float)w, -(float)h };
            Rectangle dstA = { 0.f, 0.f, (float)w,  (float)h };
            ::DrawTexturePro(m_blurPassA.texture, srcA, dstA, {0,0}, 0.f, WHITE);
        ::EndShaderMode();
    ::EndTextureMode();

    // ── Pass 2: Vertical (blurPassB → blurPassA) ─────────────────────────
    ::BeginTextureMode(m_blurPassA);
        ::ClearBackground(BLACK);
        ::BeginShaderMode(m_blurShader);
            float dirV[2] = { 0.0f, 1.0f };
            ::SetShaderValue(m_blurShader, m_locDirection, dirV, SHADER_UNIFORM_VEC2);
            Rectangle srcB = { 0.f, 0.f, (float)w, -(float)h };
            Rectangle dstB = { 0.f, 0.f, (float)w,  (float)h };
            ::DrawTexturePro(m_blurPassB.texture, srcB, dstB, {0,0}, 0.f, WHITE);
        ::EndShaderMode();
    ::EndTextureMode();
}

// =============================================================================
// ── BUTTON HELPERS ───────────────────────────────────────────────────────────
// =============================================================================

void MenuView::UpdateButtonHover(AnimatedButton& btn, Vector2 mouse, float dt) {
    btn.hovered     = ::CheckCollisionPointRec(mouse, btn.baseRect);
    btn.targetScale = btn.hovered ? 1.12f : 1.0f;

    // Smooth scale lerp (speed: 10 units/s)
    float lerpSpeed = 10.0f;
    btn.scale     = Lerp(btn.scale,     btn.targetScale,       std::min(1.0f, lerpSpeed * dt));
    btn.glowAlpha = Lerp(btn.glowAlpha, btn.hovered ? 0.55f : 0.0f, std::min(1.0f, lerpSpeed * dt));
}

void MenuView::DrawGlowRect(Rectangle r, Color col, float alpha, float thickness) {
    if (alpha <= 0.01f) return;
    unsigned char a = (unsigned char)(alpha * 255.f);
    Color glowCol = { col.r, col.g, col.b, a };
    for (float t = 0; t < thickness; t += 1.0f) {
        float fade = (1.0f - t / thickness) * alpha;
        unsigned char fa = (unsigned char)(fade * 255.f);
        Color layerCol = { col.r, col.g, col.b, fa };
        ::DrawRectangleLinesEx(
            { r.x - t, r.y - t, r.width + 2*t, r.height + 2*t },
            1.5f, layerCol);
    }
    (void)glowCol;
}

void MenuView::DrawAnimatedButton(const char* label, AnimatedButton& btn, bool selected) {
    {
        const float scale = btn.scale;
        const float centerX = btn.baseRect.x+btn.baseRect.width*0.5f;
        const float centerY = btn.baseRect.y+btn.baseRect.height*0.5f;
        const float width = btn.baseRect.width*scale;
        const float height = btn.baseRect.height*scale;
        const Rectangle rect{centerX-width*0.5f,centerY-height*0.5f,width,height};
        const bool active = selected || btn.hovered;
        DrawGlowRect(rect,Color{255,205,83,255},active?0.30f+btn.glowAlpha*0.35f:0.0f,9.0f);
        ::DrawRectangleRounded({rect.x+4,rect.y+6,rect.width,rect.height},0.22f,12,
                               Color{5,2,13,150});
        ::DrawRectangleRounded(rect,0.22f,12,
                               active?Color{73,48,112,248}:Color{31,22,52,242});
        ::DrawRectangleRounded({rect.x+4,rect.y+4,rect.width-8,rect.height*0.44f},
                               0.25f,10,active?Color{121,82,167,110}:Color{78,57,108,70});
        ::DrawRectangleRoundedLinesEx(rect,0.22f,12,active?3.0f:1.8f,
                                     active?Color{255,216,100,255}:Color{142,112,180,220});
        const float ornamentX=rect.x+22.0f;
        ::DrawPoly({ornamentX,centerY},4,std::max(5.0f,height*0.10f),45.0f,
                   active?Color{255,219,105,255}:Color{115,91,151,220});
        ::DrawLineEx({rect.x+34,centerY},{rect.x+52,centerY},1.5f,
                     active?Color{255,219,105,210}:Color{105,82,140,170});
        const float fontSize=std::max(13.0f,height*0.39f);
        const Font font=m_fontsLoaded ? m_fontBody : ::GetFontDefault();
        const Vector2 measured=::MeasureTextEx(font,label,fontSize,1.0f);
        ::DrawTextEx(font,label,{centerX-measured.x*0.5f,centerY-measured.y*0.5f},
                     fontSize,1.0f,active?Color{255,238,174,255}:Color{226,215,242,245});
    }
    return;

#if 0 // Replaced by the layered fantasy button above.
    float scale = btn.scale;
    float cxBase = btn.baseRect.x + btn.baseRect.width  * 0.5f;
    float cyBase = btn.baseRect.y + btn.baseRect.height * 0.5f;

    float scaledW = btn.baseRect.width  * scale;
    float scaledH = btn.baseRect.height * scale;
    float scaledX = cxBase - scaledW * 0.5f;
    float scaledY = cyBase - scaledH * 0.5f;

    Rectangle scaledRect = { scaledX, scaledY, scaledW, scaledH };

    // ── Glow ring ────────────────────────────────────────────────────────
    DrawGlowRect(scaledRect, Color{220, 200, 100, 255}, btn.glowAlpha, 8.0f);

    // ── Button body ──────────────────────────────────────────────────────
    auto& res = UIResourceManager::GetInstance();
    Texture2D* texBtn = res.GetButton();
    float btnFrameW   = res.GetButtonFrameWidth();

    if (texBtn && texBtn->id != 0 && btnFrameW > 0) {
        int frame = (selected || btn.hovered) ? 1 : 0;
        Rectangle src = { (float)(frame * btnFrameW), 0.0f,
                          btnFrameW, (float)texBtn->height };
        ::DrawTexturePro(*texBtn, src, scaledRect, {0, 0}, 0.0f, WHITE);
    } else {
        // Fallback: gradient-style rectangle
        Color bg  = (selected || btn.hovered) ? Color{90, 65, 140, 230} : Color{45, 35, 65, 200};
        Color bg2 = (selected || btn.hovered) ? Color{130, 90, 190, 230} : Color{70, 55, 100, 200};
        ::DrawRectangleGradientV((int)scaledX, (int)scaledY,
                                  (int)scaledW, (int)scaledH, bg, bg2);
        ::DrawRectangleLinesEx(scaledRect, 1.5f,
                               (selected || btn.hovered) ? Color{255, 220, 80, 200}
                                                         : Color{120, 100, 160, 150});
    }

    // ── Label ────────────────────────────────────────────────────────────
    int fontSize = (int)(scaledH * 0.50f);
    if (fontSize < 10) fontSize = 10;
    Color textCol = (selected || btn.hovered) ? Color{255, 235, 80, 255} : WHITE;

    if (m_fontsLoaded) {
        Vector2 measured = ::MeasureTextEx(m_fontBody, label, (float)fontSize, 1.0f);
        float tx = scaledX + (scaledW - measured.x) * 0.5f;
        float ty = scaledY + (scaledH - measured.y) * 0.5f;
        ::DrawTextEx(m_fontBody, label, {tx, ty}, (float)fontSize, 1.0f, textCol);
    } else {
        int tw = ::MeasureText(label, fontSize);
        float tx = scaledX + (scaledW - (float)tw) * 0.5f;
        float ty = scaledY + (scaledH - (float)fontSize) * 0.5f;
        ::DrawText(label, (int)tx, (int)ty, fontSize, textCol);
    }
}
#endif
}

// ─────────────────────────────────────────────────────────────────────────────
// DrawButton (legacy — used by Pause / Error / Connection / RoleSelect)
// ─────────────────────────────────────────────────────────────────────────────
void MenuView::DrawButton(const char* label, float x, float y, float w, float h, bool selected) {
    Renderer& r = Renderer::GetInstance();
    auto& res   = UIResourceManager::GetInstance();
    Texture2D* texBtn = res.GetButton();
    float btnFrameW   = res.GetButtonFrameWidth();

    if (texBtn && texBtn->id != 0 && btnFrameW > 0) {
        int frame = selected ? 1 : 0;
        Rectangle src = { (float)(frame * btnFrameW), 0.0f,
                          btnFrameW, (float)texBtn->height };
        float scaleX = w / btnFrameW;
        float scaleY = h / (float)texBtn->height;
        r.SubmitSprite(texBtn, src, {x, y}, {scaleX, scaleY},
                       0.0f, {0, 0}, WHITE, Layer::UI, 1.0f, false, 0);
    } else {
        Color bg = selected ? Color{80, 60, 120, 220} : Color{50, 40, 70, 200};
        r.DrawRectangle({x, y}, {w, h}, bg, Layer::UI, 1.0f);
    }

    int fontSize = (int)(h * 0.55f);
    if (fontSize < 10) fontSize = 10;
    int textW = ::MeasureText(label, fontSize);
    float tx = x + (w - (float)textW) * 0.5f;
    float ty = y + (h - (float)fontSize) * 0.5f;
    Color textCol = selected ? YELLOW : WHITE;
    r.DrawText(label, {tx, ty}, fontSize, textCol);
}

// ─────────────────────────────────────────────────────────────────────────────
// DrawPanel
// ─────────────────────────────────────────────────────────────────────────────
void MenuView::DrawPanel(float x, float y, float w, float h) {
    Renderer& r        = Renderer::GetInstance();
    Texture2D* texPanel = UIResourceManager::GetInstance().GetPanelBg();

    if (texPanel && texPanel->id != 0) {
        int corner = texPanel->width / 3;
        NPatchInfo npi;
        npi.source = {0.0f, 0.0f, (float)texPanel->width, (float)texPanel->height};
        npi.left = npi.top = npi.right = npi.bottom = corner;
        npi.layout = 0;
        r.SubmitNPatch(texPanel, npi, {x, y, w, h}, WHITE, Layer::UI, 0.5f);
    } else {
        // Stylized fallback: dark purple panel with border
        ::DrawRectangle((int)x, (int)y, (int)w, (int)h, Color{18, 12, 30, 225});
        ::DrawRectangleLinesEx({ x, y, w, h }, 2.0f, Color{120, 90, 170, 200});
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// DrawHeaderHUD — top bar showing player name + coin count
// ─────────────────────────────────────────────────────────────────────────────
void MenuView::DrawHeaderHUD() {
    int sw = Renderer::GetInstance().GetWindowWidth();
    int sh = Renderer::GetInstance().GetWindowHeight();

    // Semi-transparent top bar
    float barH = sh * 0.055f;
    ::DrawRectangle(0, 0, sw, (int)barH, Color{0, 0, 0, 130});

    int fontSize = (int)(barH * 0.60f);
    if (fontSize < 10) fontSize = 10;

    // Player name (left)
    char buf[128];
    snprintf(buf, sizeof(buf), "  %s", m_playerName.c_str());
    ::DrawText(buf, 8, (int)((barH - fontSize) * 0.5f), fontSize, Color{220, 200, 255, 255});

    // Coin count (right)
    snprintf(buf, sizeof(buf), "%d coins  ", m_coins);
    int coinW = ::MeasureText(buf, fontSize);
    ::DrawText(buf, sw - coinW - 8, (int)((barH - fontSize) * 0.5f), fontSize, Color{255, 220, 60, 255});
}

// =============================================================================
// ── RENDER MAIN ─────────────────────────────────────────────────────────────
// =============================================================================
void MenuView::RenderMain() {
    int w = Renderer::GetInstance().GetWindowWidth();
    int h = Renderer::GetInstance().GetWindowHeight();

    // ── Step 1: Render parallax into blurPassA ────────────────────────────
    if (m_blurReady) {
        ::BeginTextureMode(m_blurPassA);
            ::ClearBackground(BLACK);
            RenderParallax();
        ::EndTextureMode();

        // ── Step 2: Two-pass Gaussian blur ────────────────────────────────
        ApplyBlurToScreen();

        // ── Step 3: Draw blurred background ──────────────────────────────
        Rectangle src = { 0.f, 0.f, (float)w, -(float)h };
        Rectangle dst = { 0.f, 0.f, (float)w,  (float)h };
        ::DrawTexturePro(m_blurPassA.texture, src, dst, {0,0}, 0.f, WHITE);
    } else {
        // Fallback: draw parallax directly (no blur)
        RenderParallax();
    }

    // ── Step 4: Dark vignette overlay ────────────────────────────────────
    ::DrawRectangle(0, 0, w, h, Color{8, 4, 18, 90});

    const Rectangle commandPanel{w*0.16f,h*0.415f,w*0.68f,h*0.47f};
    ::DrawRectangleRounded({commandPanel.x+7,commandPanel.y+9,
                            commandPanel.width,commandPanel.height},
                           0.08f,12,Color{3,1,10,145});
    ::DrawRectangleRounded(commandPanel,0.08f,12,Color{18,11,34,185});
    ::DrawRectangleRoundedLinesEx(commandPanel,0.08f,12,2.0f,Color{126,95,171,200});
    ::DrawLineEx({commandPanel.x+35,commandPanel.y+22},
                 {commandPanel.x+commandPanel.width-35,commandPanel.y+22},
                 1.5f,Color{210,166,76,150});
    ::DrawPoly({commandPanel.x+22,commandPanel.y+22},4,6.0f,45.0f,Color{230,188,88,210});
    ::DrawPoly({commandPanel.x+commandPanel.width-22,commandPanel.y+22},
               4,6.0f,45.0f,Color{230,188,88,210});

    // ── Step 5: Header HUD (player name only — no coins on main menu) ─────────────
    {
        int sw2 = Renderer::GetInstance().GetWindowWidth();
        int sh2 = Renderer::GetInstance().GetWindowHeight();
        float barH = sh2 * 0.048f;
        // Only player name, no coin display
        ::DrawRectangle(0, 0, sw2, (int)barH, Color{0, 0, 0, 90});
        int fontSize = (int)(barH * 0.60f); if (fontSize < 10) fontSize = 10;
        char buf[64];
        snprintf(buf, sizeof(buf), "  %s", m_playerName.c_str());
        ::DrawText(buf, 8, (int)((barH - fontSize) * 0.5f), fontSize,
                   Color{220, 200, 255, 200});
    }

    // ── Step 6: Title text with bobbing effect ────────────────────────────
    {
        const char* title = "Apple Knight";
        // Bob up/down using sine
        float bob = sinf(m_titleBobTime * 1.5f) * 6.0f;

        int titleFontSize = (int)(h * 0.075f);
        if (titleFontSize < 20) titleFontSize = 20;

        if (m_fontsLoaded) {
            Vector2 measured = ::MeasureTextEx(m_fontTitle, title, (float)titleFontSize, 2.0f);
            float tx = ((float)w - measured.x) * 0.5f;
            float ty = h * 0.22f + bob;
            // Shadow
            ::DrawTextEx(m_fontTitle, title, {tx + 3, ty + 3},
                         (float)titleFontSize, 2.0f, Color{0, 0, 0, 120});
            // Main title (gold gradient emulated with a warm white)
            ::DrawTextEx(m_fontTitle, title, {tx, ty},
                         (float)titleFontSize, 2.0f, Color{255, 230, 80, 255});
        } else {
            int tw = ::MeasureText(title, titleFontSize);
            float tx = ((float)w - tw) * 0.5f;
            float ty = h * 0.22f + bob;
            ::DrawText(title, (int)(tx+3), (int)(ty+3), titleFontSize, Color{0,0,0,120});
            ::DrawText(title, (int)tx, (int)ty, titleFontSize, Color{255, 230, 80, 255});
        }

        // Subtitle
        const char* sub = "Adventure";
        int subSize = (int)(h * 0.038f);
        if (subSize < 12) subSize = 12;
        float bobSub = sinf(m_titleBobTime * 1.5f + 0.4f) * 4.0f;
        if (m_fontsLoaded) {
            Vector2 sm = ::MeasureTextEx(m_fontTitle, sub, (float)subSize, 1.5f);
            float sx = ((float)w - sm.x) * 0.5f;
            ::DrawTextEx(m_fontTitle, sub, {sx, h * 0.22f + titleFontSize + 4.0f + bobSub},
                         (float)subSize, 1.5f, Color{200, 180, 255, 220});
        } else {
            int sw2 = ::MeasureText(sub, subSize);
            ::DrawText(sub, (int)(((float)w - sw2)*0.5f),
                       (int)(h * 0.22f + titleFontSize + 4.0f + bobSub),
                       subSize, Color{200, 180, 255, 220});
        }
    }

    // ── Step 7: Animated buttons ──────────────────────────────────────────
    for (int i = 0; i < (int)m_mainItems.size() && i < kMaxMainButtons; ++i) {
        DrawAnimatedButton(m_mainItems[i].c_str(), m_mainButtons[i], (i == m_selected));
    }

    if (m_mainNoticeTimer > 0.0f && !m_mainNotice.empty()) {
        const Font font=m_fontsLoaded ? m_fontBody : ::GetFontDefault();
        const float size=std::max(14.0f,h*0.024f);
        const Vector2 measured=::MeasureTextEx(font,m_mainNotice.c_str(),size,1.0f);
        const Rectangle notice{(w-measured.x)*0.5f-22,h*0.865f,
                               measured.x+44,measured.y+18};
        ::DrawRectangleRounded(notice,0.35f,10,Color{48,20,34,235});
        ::DrawRectangleRoundedLinesEx(notice,0.35f,10,2.0f,Color{255,157,115,230});
        ::DrawTextEx(font,m_mainNotice.c_str(),{notice.x+22,notice.y+9},size,1.0f,
                     Color{255,225,196,255});
    }
}

// =============================================================================
// ── RENDER PAUSE ─────────────────────────────────────────────────────────────
// =============================================================================
void MenuView::RenderCustomMaps() {
    const float w = (float)Renderer::GetInstance().GetWindowWidth();
    const float h = (float)Renderer::GetInstance().GetWindowHeight();
    const CustomMapLayout l = BuildCustomMapLayout(w, h);
    const Font font = m_fontsLoaded ? m_fontBody : ::GetFontDefault();
    const Color gold{232, 186, 83, 255};
    const Color mint{105, 235, 169, 255};
    const Color coral{235, 105, 118, 255};

    RenderParallax();
    ::DrawRectangle(0, 0, (int)w, (int)h, Color{7, 4, 16, 165});
    ::DrawRectangleRounded({l.panel.x + 9, l.panel.y + 11, l.panel.width, l.panel.height},
                           0.045f, 10, Color{0, 0, 0, 130});
    ::DrawRectangleRounded(l.panel, 0.045f, 10, Color{20, 15, 35, 248});
    ::DrawRectangleGradientV((int)l.panel.x + 3, (int)l.panel.y + 3,
                             (int)l.panel.width - 6, (int)(l.panel.height * 0.23f),
                             Color{62, 42, 86, 255}, Color{20, 15, 35, 248});
    ::DrawRectangleRoundedLinesEx(l.panel, 0.045f, 10, 3.0f, gold);

    const float titleSize = std::clamp(l.panel.height * 0.09f, 25.0f, 40.0f);
    const char* title = "CUSTOM MAPS";
    const Vector2 titleV = ::MeasureTextEx(font, title, titleSize, 1.3f);
    ::DrawTextEx(font, title,
                 {l.panel.x + (l.panel.width - titleV.x) * 0.5f,
                  l.panel.y + l.panel.height * 0.065f},
                 titleSize, 1.3f, gold);
    const char* subtitle = "Choose a map created in Map Builder";
    const float subSize = std::clamp(l.panel.height * 0.035f, 10.0f, 15.0f);
    const Vector2 subV = ::MeasureTextEx(font, subtitle, subSize, 1.0f);
    ::DrawTextEx(font, subtitle,
                 {l.panel.x + (l.panel.width - subV.x) * 0.5f,
                  l.panel.y + l.panel.height * 0.165f},
                 subSize, 1.0f, Color{194, 174, 225, 255});

    ::DrawRectangleRounded(l.list, 0.035f, 7, Color{11, 9, 23, 220});
    ::DrawRectangleRoundedLinesEx(l.list, 0.035f, 7, 1.5f,
                                  Color{107, 81, 139, 190});
    if (m_customMapNames.empty()) {
        const char* empty = "NO CUSTOM MAPS YET";
        const char* hint = "Create and save one in Map Builder first.";
        const float emptySize = std::clamp(l.list.height * 0.11f, 15.0f, 22.0f);
        const Vector2 emptyV = ::MeasureTextEx(font, empty, emptySize, 1.0f);
        ::DrawTextEx(font, empty,
                     {l.list.x + (l.list.width - emptyV.x) * 0.5f,
                      l.list.y + l.list.height * 0.36f},
                     emptySize, 1.0f, Color{205, 190, 219, 255});
        const float hintSize = std::clamp(emptySize * 0.65f, 10.0f, 14.0f);
        const Vector2 hintV = ::MeasureTextEx(font, hint, hintSize, 1.0f);
        ::DrawTextEx(font, hint,
                     {l.list.x + (l.list.width - hintV.x) * 0.5f,
                      l.list.y + l.list.height * 0.53f},
                     hintSize, 1.0f, Color{137, 123, 153, 255});
    } else {
        const int visibleRows = std::max(1, (int)(l.list.height / l.rowHeight));
        const int start = m_selected >= visibleRows ? m_selected - visibleRows + 1 : 0;
        for (int row = 0; row < visibleRows; ++row) {
            const int index = start + row;
            if (index >= (int)m_customMapNames.size()) break;
            const Rectangle item{l.list.x + 8.0f,
                                 l.list.y + 7.0f + row * l.rowHeight,
                                 l.list.width - 16.0f, l.rowHeight - 8.0f};
            const bool selected = index == m_selected;
            ::DrawRectangleRounded(item, 0.16f, 6,
                selected ? Color{79, 54, 101, 255} : Color{29, 24, 46, 245});
            ::DrawRectangleRoundedLinesEx(item, 0.16f, 6, selected ? 2.0f : 1.0f,
                selected ? gold : Color{99, 77, 124, 155});
            const float rowFont = std::clamp(item.height * 0.37f, 11.0f, 17.0f);
            ::DrawTextEx(font, TextFormat("%02d", index + 1),
                         {item.x + 14.0f, item.y + (item.height - rowFont) * 0.5f},
                         rowFont, 1.0f, selected ? gold : Color{133, 119, 150, 255});
            std::string displayName = m_customMapNames[index];
            std::replace(displayName.begin(), displayName.end(), '_', ' ');
            ::DrawTextEx(font, displayName.c_str(),
                         {item.x + 58.0f, item.y + (item.height - rowFont) * 0.5f},
                         rowFont, 1.0f, selected ? RAYWHITE : Color{205, 195, 216, 255});
        }
    }

    const char* labels[] = {"PLAY SELECTED", "DELETE", "BACK"};
    const Color accents[] = {mint, coral, Color{170, 153, 191, 255}};
    const Vector2 mouse = ::GetMousePosition();
    for (int i = 0; i < 3; ++i) {
        const Rectangle b = l.actions[i];
        const bool disabled = m_customMapNames.empty() && i != 2;
        const bool hover = !disabled && ::CheckCollisionPointRec(mouse, b);
        ::DrawRectangleRounded(b, 0.18f, 7,
            disabled ? Color{24, 21, 35, 220}
                     : (hover ? Color{69, 49, 88, 255} : Color{34, 28, 52, 250}));
        ::DrawRectangleRoundedLinesEx(b, 0.18f, 7, hover ? 2.0f : 1.2f,
            disabled ? Color{70, 63, 79, 130} : accents[i]);
        float fs = std::clamp(b.height * 0.34f, 10.0f, 16.0f);
        while (fs > 9.0f && ::MeasureTextEx(font, labels[i], fs, 1.0f).x > b.width - 16.0f) fs -= 1.0f;
        const Vector2 mv = ::MeasureTextEx(font, labels[i], fs, 1.0f);
        ::DrawTextEx(font, labels[i], {b.x + (b.width - mv.x) * 0.5f,
                     b.y + (b.height - mv.y) * 0.5f}, fs, 1.0f,
                     disabled ? Color{91, 84, 101, 255} : RAYWHITE);
    }

    if (!m_customMapDeleteConfirmation.empty()) {
        ::DrawRectangle(0, 0, (int)w, (int)h, Color{0, 0, 0, 185});
        const Rectangle box{w * 0.5f - std::min(230.0f, w * 0.36f), h * 0.5f - 100.0f,
                            std::min(460.0f, w * 0.72f), 200.0f};
        ::DrawRectangleRounded(box, 0.07f, 8, Color{29, 20, 39, 255});
        ::DrawRectangleRoundedLinesEx(box, 0.07f, 8, 2.5f, coral);
        const char* warning = "DELETE THIS MAP?";
        const float warningSize = std::clamp(box.height * 0.12f, 16.0f, 23.0f);
        const Vector2 warningV = ::MeasureTextEx(font, warning, warningSize, 1.0f);
        ::DrawTextEx(font, warning, {box.x + (box.width - warningV.x) * 0.5f,
                     box.y + 29.0f}, warningSize, 1.0f, coral);
        const float nameSize = std::clamp(warningSize * 0.72f, 12.0f, 16.0f);
        const Vector2 nameV = ::MeasureTextEx(font, m_customMapDeleteConfirmation.c_str(),
                                              nameSize, 1.0f);
        ::DrawTextEx(font, m_customMapDeleteConfirmation.c_str(),
                     {box.x + (box.width - nameV.x) * 0.5f, box.y + 70.0f},
                     nameSize, 1.0f, RAYWHITE);
        const char* confirmLabels[] = {"DELETE", "CANCEL"};
        for (int i = 0; i < 2; ++i) {
            const Rectangle b = l.confirmActions[i];
            const bool hover = ::CheckCollisionPointRec(mouse, b);
            ::DrawRectangleRounded(b, 0.18f, 7,
                                   hover ? Color{72, 45, 62, 255} : Color{37, 28, 48, 255});
            ::DrawRectangleRoundedLinesEx(b, 0.18f, 7, hover ? 2.0f : 1.2f,
                                          i == 0 ? coral : Color{175, 158, 195, 255});
            const Vector2 tv = ::MeasureTextEx(font, confirmLabels[i], 14.0f, 1.0f);
            ::DrawTextEx(font, confirmLabels[i],
                         {b.x + (b.width - tv.x) * 0.5f, b.y + (b.height - tv.y) * 0.5f},
                         14.0f, 1.0f, RAYWHITE);
        }
    }
}

void MenuView::RenderPause() {
    Renderer& r = Renderer::GetInstance();
    int w = r.GetWindowWidth();
    int h = r.GetWindowHeight();

    // Flush the game scene and dim layer before drawing crisp font-based UI.
    r.DrawRectangle({0, 0}, {(float)w, (float)h}, {3, 2, 10, 190}, Layer::UI, 0.0f);
    r.EndFrameAndFlush();

    const PauseLayout l = BuildPauseLayout((float)w, (float)h);
    const Font font = m_fontsLoaded ? m_fontBody : ::GetFontDefault();
    const Color gold{232, 186, 83, 255};
    const Color lavender{194, 174, 225, 255};
    const Color panel{20, 15, 35, 248};

    ::DrawRectangleRounded({l.panel.x + 9, l.panel.y + 11, l.panel.width, l.panel.height},
                           0.045f, 10, Color{0, 0, 0, 135});
    ::DrawRectangleRounded(l.panel, 0.045f, 10, panel);
    ::DrawRectangleGradientV((int)l.panel.x + 3, (int)l.panel.y + 3,
                             (int)l.panel.width - 6, (int)(l.panel.height * 0.28f),
                             Color{64, 43, 88, 255}, panel);
    ::DrawRectangleRoundedLinesEx(l.panel, 0.045f, 10, 3.0f, gold);
    ::DrawRectangleRoundedLinesEx({l.panel.x + 8, l.panel.y + 8,
                                   l.panel.width - 16, l.panel.height - 16},
                                  0.035f, 10, 1.0f, Color{122, 92, 155, 190});

    const float titleSize = std::clamp(l.panel.height * 0.105f, 26.0f, 43.0f);
    const char* title = "GAME PAUSED";
    const Vector2 titleMeasure = ::MeasureTextEx(font, title, titleSize, 1.4f);
    const float titleX = l.panel.x + (l.panel.width - titleMeasure.x) * 0.5f;
    const float titleY = l.panel.y + l.panel.height * 0.075f;
    ::DrawTextEx(font, title, {titleX + 3, titleY + 3}, titleSize, 1.4f,
                 Color{0, 0, 0, 130});
    ::DrawTextEx(font, title, {titleX, titleY}, titleSize, 1.4f, gold);
    const char* subtitle = "The adventure is waiting for you";
    const float subSize = std::clamp(l.panel.height * 0.042f, 11.0f, 16.0f);
    const Vector2 subMeasure = ::MeasureTextEx(font, subtitle, subSize, 1.0f);
    ::DrawTextEx(font, subtitle,
                 {l.panel.x + (l.panel.width - subMeasure.x) * 0.5f,
                  titleY + titleMeasure.y + 5.0f},
                 subSize, 1.0f, lavender);

    const char* labels[] = {"RESUME", "OPTIONS", "QUIT TO MENU"};
    const char* numbers[] = {"01", "02", "03"};
    for (int i = 0; i < 3; ++i) {
        const Rectangle b = l.buttons[i];
        const bool active = i == m_selected;
        const bool hover = ::CheckCollisionPointRec(::GetMousePosition(), b);
        const Color fill = active ? Color{83, 56, 105, 255}
                                  : (hover ? Color{48, 38, 70, 255}
                                           : Color{31, 26, 48, 248});
        ::DrawRectangleRounded({b.x + 3, b.y + 4, b.width, b.height}, 0.16f, 8,
                               Color{0, 0, 0, 90});
        ::DrawRectangleRounded(b, 0.16f, 8, fill);
        ::DrawRectangleRoundedLinesEx(b, 0.16f, 8, active ? 2.5f : 1.2f,
                                      active ? gold : Color{118, 92, 145, 170});
        const float numSize = std::clamp(b.height * 0.30f, 10.0f, 15.0f);
        ::DrawTextEx(font, numbers[i], {b.x + 14, b.y + (b.height - numSize) * 0.5f},
                     numSize, 1.0f, active ? gold : Color{140, 125, 158, 255});
        const float labelSize = std::clamp(b.height * 0.37f, 12.0f, 19.0f);
        const Vector2 labelMeasure = ::MeasureTextEx(font, labels[i], labelSize, 1.0f);
        ::DrawTextEx(font, labels[i],
                     {b.x + 51, b.y + (b.height - labelMeasure.y) * 0.5f},
                     labelSize, 1.0f, active ? RAYWHITE : Color{207, 195, 220, 255});
        if (active) {
            ::DrawRectangle((int)b.x, (int)(b.y + 8), 4, (int)(b.height - 16), gold);
        }
    }

    ::DrawRectangleRounded(l.infoPane, 0.055f, 8, Color{12, 10, 25, 205});
    ::DrawRectangleRoundedLinesEx(l.infoPane, 0.055f, 8, 1.5f,
                                  Color{112, 84, 143, 190});
    const float infoX = l.infoPane.x + 18.0f;
    float infoY = l.infoPane.y + 18.0f;
    const float infoTitleSize = std::clamp(l.infoPane.height * 0.09f, 12.0f, 18.0f);
    ::DrawTextEx(font, "TAKE A BREATH", {infoX, infoY}, infoTitleSize, 1.0f, gold);
    infoY += infoTitleSize + 16.0f;
    const float bodySize = std::clamp(l.infoPane.height * 0.065f, 10.0f, 14.0f);
    ::DrawTextEx(font, "Progress is saved at", {infoX, infoY}, bodySize, 1.0f, lavender);
    infoY += bodySize + 5.0f;
    ::DrawTextEx(font, "activated checkpoints.", {infoX, infoY}, bodySize, 1.0f, lavender);
    infoY += bodySize + 18.0f;
    ::DrawLineEx({infoX, infoY}, {l.infoPane.x + l.infoPane.width - 18.0f, infoY},
                 1.0f, Color{120, 94, 148, 150});
    infoY += 15.0f;
    const char* hints[] = {"ESC   RESUME", "W / S   NAVIGATE", "ENTER   SELECT"};
    for (const char* hint : hints) {
        if (infoY + bodySize > l.infoPane.y + l.infoPane.height - 10.0f) break;
        ::DrawTextEx(font, hint, {infoX, infoY}, bodySize, 1.0f,
                     Color{178, 162, 197, 235});
        infoY += bodySize + 8.0f;
    }
}

// =============================================================================
// ── RENDER ERROR ─────────────────────────────────────────────────────────────
// =============================================================================
void MenuView::RenderError() {
    Renderer& r = Renderer::GetInstance();
    int w = r.GetWindowWidth();
    int h = r.GetWindowHeight();

    r.DrawRectangle({0, 0}, {(float)w, (float)h}, {0, 0, 0, 140}, Layer::UI, 0.0f);

    float panelW = w * 0.35f;
    float panelH = h * 0.30f;
    float panelX = ((float)w - panelW) * 0.5f;
    float panelY = ((float)h - panelH) * 0.5f;
    DrawPanel(panelX, panelY, panelW, panelH);

    { // ERROR label
        const char* title = "ERROR";
        int fs = (int)(h * 0.04f); if (fs < 12) fs = 12;
        int tw = ::MeasureText(title, fs);
        r.DrawText(title, {panelX + (panelW-(float)tw)*0.5f, panelY+panelH*0.10f}, fs, RED);
    }
    { // Error message
        int fs = (int)(h * 0.025f); if (fs < 10) fs = 10;
        int mw = ::MeasureText(m_errorMsg.c_str(), fs);
        r.DrawText(m_errorMsg.c_str(), {panelX+(panelW-(float)mw)*0.5f, panelY+panelH*0.35f}, fs, WHITE);
    }
    DrawButton(m_errorItems[0].c_str(),
               panelX + (panelW - panelW*0.35f)*0.5f, panelY + panelH*0.70f,
               panelW*0.35f, panelH*0.16f, true);
}

// =============================================================================
// ── RENDER ROLE SELECT ───────────────────────────────────────────────────────
// =============================================================================
void MenuView::RenderRoleSelect() {
    Renderer& r = Renderer::GetInstance();
    int w = r.GetWindowWidth();
    int h = r.GetWindowHeight();

    r.DrawRectangle({0, 0}, {(float)w, (float)h}, {10, 5, 20, 200}, Layer::UI, 0.0f);

    float panelW = w * 0.40f;
    float panelH = h * 0.50f;
    float panelX = ((float)w - panelW) * 0.5f;
    float panelY = ((float)h - panelH) * 0.5f;
    DrawPanel(panelX, panelY, panelW, panelH);

    {
        const char* title = "Select Role";
        int fs = (int)(h * 0.045f); if (fs < 14) fs = 14;
        int tw = ::MeasureText(title, fs);
        r.DrawText(title, {panelX+(panelW-(float)tw)*0.5f, panelY+panelH*0.08f}, fs, WHITE);
    }

    float btnW    = panelW * 0.55f;
    float btnH    = panelH * 0.14f;
    float btnX    = panelX + (panelW - btnW) * 0.5f;
    float startY  = panelY + panelH * 0.30f;
    float spacing = panelH * 0.20f;

    for (int i = 0; i < (int)m_roleItems.size(); ++i) {
        float by = startY + (float)i * spacing;
        DrawButton(m_roleItems[i].c_str(), btnX, by, btnW, btnH, (i == m_selected));
    }
}

// =============================================================================
// ── RENDER CONNECTION ────────────────────────────────────────────────────────
// =============================================================================
void MenuView::RenderConnection() {
    Renderer& r = Renderer::GetInstance();
    int w = r.GetWindowWidth();
    int h = r.GetWindowHeight();

    r.DrawRectangle({0,0}, {(float)w,(float)h}, {0,0,0,140}, Layer::UI, 0.0f);

    float panelW = w * 0.35f;
    float panelH = h * 0.35f;
    float panelX = ((float)w - panelW) * 0.5f;
    float panelY = ((float)h - panelH) * 0.5f;
    DrawPanel(panelX, panelY, panelW, panelH);

    {
        const char* status = m_connected ? "Connected" : "Connecting...";
        Color statusCol = m_connected ? GREEN : GRAY;
        int fs = (int)(h * 0.035f); if (fs < 12) fs = 12;
        int tw = ::MeasureText(status, fs);
        r.DrawText(status, {panelX+(panelW-(float)tw)*0.5f, panelY+panelH*0.15f}, fs, statusCol);
    }
    {
        int fs = (int)(h * 0.025f); if (fs < 10) fs = 10;
        int iw = ::MeasureText(m_connectionIp.c_str(), fs);
        r.DrawText(m_connectionIp.c_str(),
                   {panelX+(panelW-(float)iw)*0.5f, panelY+panelH*0.35f}, fs, WHITE);
    }

    DrawButton(m_connectionItems[0].c_str(),
               panelX + (panelW - panelW*0.35f)*0.5f, panelY + panelH*0.70f,
               panelW*0.35f, panelH*0.14f, (m_selected == 0));
}

// =============================================================================
// ── RENDER SHOP ──────────────────────────────────────────────────────────────
// =============================================================================
void MenuView::RenderShop() {
    int sw = Renderer::GetInstance().GetWindowWidth();
    int sh = Renderer::GetInstance().GetWindowHeight();

    // Background
    ::DrawRectangle(0, 0, sw, sh, Color{10, 8, 22, 255});

    // Header bar
    DrawHeaderHUD();

    // Title
    {
        const char* title = "SHOP";
        int fs = (int)(sh * 0.06f); if (fs < 20) fs = 20;
        int tw = ::MeasureText(title, fs);
        ::DrawText(title, (sw - tw)/2, (int)(sh * 0.07f), fs, Color{255, 220, 80, 255});
    }

    // Grid of character cards (3 columns)
    const char* chars[] = {"Knight", "Fighter", "Magic Caster", "Ninja"};
    const int   prices[] = {0, 10, 20, 30};
    constexpr int kCols = 3;
    constexpr int kCount = 4;

    float cardW   = sw * 0.22f;
    float cardH   = sh * 0.30f;
    float padX    = (sw - kCols * cardW) / (kCols + 1);
    float startY  = sh * 0.16f - m_shopScrollDisp;

    for (int i = 0; i < kCount; ++i) {
        int col = i % kCols;
        int row = i / kCols;
        float cx = padX + col * (cardW + padX);
        float cy = startY + row * (cardH + sh * 0.04f);

        bool hovered  = (m_shopSelected == i);
        Color cardBg  = hovered ? Color{60, 45, 90, 230} : Color{25, 18, 45, 215};
        Color cardBrd = hovered ? Color{220, 180, 80, 200} : Color{80, 60, 110, 180};

        // Card body
        ::DrawRectangle((int)cx, (int)cy, (int)cardW, (int)cardH, cardBg);
        ::DrawRectangleLinesEx({cx, cy, cardW, cardH}, 2.0f, cardBrd);

        // Character name
        int fs = (int)(sh * 0.030f); if (fs < 10) fs = 10;
        int cw = ::MeasureText(chars[i], fs);
        ::DrawText(chars[i], (int)(cx + (cardW - cw)*0.5f), (int)(cy + cardH*0.72f),
                   fs, WHITE);

        // Price or Unlocked tag
        if (prices[i] == 0) {
            const char* tag = "[ Unlocked ]";
            int tw = ::MeasureText(tag, fs);
            ::DrawText(tag, (int)(cx+(cardW-tw)*0.5f), (int)(cy+cardH*0.85f), fs,
                       Color{80, 220, 100, 255});
        } else {
            char priceBuf[32];
            snprintf(priceBuf, sizeof(priceBuf), "%d coins", prices[i]);
            int tw = ::MeasureText(priceBuf, fs);
            ::DrawText(priceBuf, (int)(cx+(cardW-tw)*0.5f), (int)(cy+cardH*0.85f),
                       fs, Color{255, 220, 60, 255});
        }
    }

    // Back button
    {
        float bw = sw * 0.15f;
        float bh = sh * 0.055f;
        float bx = sw * 0.04f;
        float by = sh * 0.92f;
        bool sel = false;
        ::DrawRectangle((int)bx, (int)by, (int)bw, (int)bh, Color{50,40,70,220});
        ::DrawRectangleLinesEx({bx,by,bw,bh}, 1.5f, Color{150,120,200,200});
        int fs = (int)(bh * 0.55f); if (fs < 10) fs = 10;
        int tw = ::MeasureText("Back", fs);
        ::DrawText("Back", (int)(bx+(bw-tw)*0.5f), (int)(by+(bh-fs)*0.5f),
                   fs, sel ? YELLOW : WHITE);
    }
}

// =============================================================================
// ── RENDER LEVEL SELECT ──────────────────────────────────────────────────────
// =============================================================================
void MenuView::RenderLevelSelect() {
    {
        const int screenW = Renderer::GetInstance().GetWindowWidth();
        const int screenH = Renderer::GetInstance().GetWindowHeight();
        const Font font = m_fontsLoaded ? m_fontBody : ::GetFontDefault();
        auto drawCentered = [&](const char* text, float centerX, float y,
                                float size, Color color, float spacing = 1.0f) {
            const Vector2 measured = ::MeasureTextEx(font, text, size, spacing);
            ::DrawTextEx(font, text, {centerX-measured.x*0.5f,y}, size, spacing, color);
        };

        ::DrawRectangle(0,0,screenW,screenH,Color{8,6,18,255});
        const float pulse = sinf(m_spotlightPulse*2.2f)*0.5f+0.5f;
        const float headerH = screenH*0.052f;
        ::DrawRectangle(0,0,screenW,(int)headerH,Color{0,0,0,100});
        ::DrawTextEx(font,m_playerName.c_str(),{18.0f,headerH*0.20f},
                     std::max(14.0f,headerH*0.55f),1.0f,Color{220,200,255,220});
        drawCentered("SELECT LEVEL",screenW*0.5f,screenH*0.075f,
                     std::max(26.0f,screenH*0.058f),Color{255,230,80,255},2.0f);

        constexpr int columns = 3;
        const int rows = std::max(1,(m_totalLevels+columns-1)/columns);
        const float gridW = screenW*0.76f;
        const float gapX = screenW*0.035f;
        const float gapY = screenH*0.045f;
        const float cardW = (gridW-gapX*(columns-1))/columns;
        const float cardH = std::min(screenH*0.255f,
            (screenH*0.64f-gapY*(rows-1))/rows);
        const float startX = (screenW-gridW)*0.5f;
        const float startY = screenH*0.19f;

        for (int i=0;i<m_totalLevels;++i) {
            const int col=i%columns;
            const int row=i/columns;
            const float x=startX+col*(cardW+gapX);
            const float y=startY+row*(cardH+gapY);
            const bool unlocked=i<m_unlockedLevels;
            const bool selected=m_selected==i;
            if (selected) {
                const float glow=6.0f+pulse*7.0f;
                ::DrawRectangleRounded({x-glow,y-glow,cardW+glow*2,cardH+glow*2},
                    0.12f,10,unlocked?Color{240,190,55,70}:Color{180,55,70,65});
            }
            const Color bg=unlocked
                ?(selected?Color{74,56,116,245}:Color{40,30,65,235})
                :Color{20,18,30,225};
            const Color border=unlocked
                ?(selected?Color{255,218,75,255}:Color{130,100,180,210})
                :Color{65,55,78,190};
            ::DrawRectangleRounded({x,y,cardW,cardH},0.10f,10,bg);
            ::DrawRectangleRoundedLinesEx({x,y,cardW,cardH},0.10f,10,
                                           selected?4.0f:2.0f,border);

            char label[24];
            std::snprintf(label,sizeof(label),"LEVEL %d",i+1);
            drawCentered(label,x+cardW*0.5f,y+cardH*0.13f,
                         std::max(18.0f,cardH*0.16f),
                         unlocked?Color{255,240,190,255}:Color{105,98,120,220});
            if (unlocked) {
                const int earned=i<(int)m_levelBestStars.size()
                    ?std::clamp(m_levelBestStars[i],0,3):0;
                const float starSize=std::min(cardW*0.17f,cardH*0.25f);
                const float starGap=starSize*0.22f;
                const float starRowW=starSize*3.0f+starGap*2.0f;
                const float starX=x+(cardW-starRowW)*0.5f;
                const float starY=y+cardH*0.49f;
                for (int star=0;star<3;++star) {
                    const Color tint=star<earned?Color{255,220,75,255}:Color{82,74,103,210};
                    if (m_levelStarIcon.id!=0) {
                        ::DrawTexturePro(m_levelStarIcon,
                            {0,0,(float)m_levelStarIcon.width,(float)m_levelStarIcon.height},
                            {starX+star*(starSize+starGap),starY,starSize,starSize},
                            {0,0},0.0f,tint);
                    } else {
                        drawCentered("*",starX+star*(starSize+starGap)+starSize*0.5f,
                                     starY,starSize,tint);
                    }
                }
                drawCentered(earned>0?"BEST RESULT":"NOT CLEARED",x+cardW*0.5f,
                             y+cardH*0.80f,std::max(12.0f,cardH*0.09f),
                             earned>0?Color{255,222,125,230}:Color{155,145,180,210});
            } else {
                drawCentered("LOCKED",x+cardW*0.5f,y+cardH*0.56f,
                             std::max(14.0f,cardH*0.12f),Color{145,125,165,210});
            }
        }
        const bool selectedUnlocked = m_selected >= 0 && m_selected < m_unlockedLevels;
        const char* action = selectedUnlocked
            ? "READY - CLICK OR PRESS ENTER TO PREPARE"
            : "COMPLETE THE PREVIOUS LEVEL TO UNLOCK";
        float actionSize = std::max(11.0f,screenH*0.018f);
        const float maxActionW = std::min(screenW-32.0f,760.0f);
        while (actionSize > 9.0f &&
               ::MeasureTextEx(font,action,actionSize,1.0f).x > maxActionW-36.0f)
            actionSize -= 1.0f;
        const Vector2 actionMeasure = ::MeasureTextEx(font,action,actionSize,1.0f);
        const float actionW = std::clamp(actionMeasure.x+44.0f,
                                         std::min(screenW*0.42f,420.0f),maxActionW);
        const float actionH = std::max(screenH*0.060f,actionMeasure.y+20.0f);
        Rectangle actionPlate = {(screenW-actionW)*0.5f,screenH*0.815f,actionW,actionH};
        ::DrawRectangleRounded(actionPlate,0.35f,10,
            selectedUnlocked?Color{50,48,79,235}:Color{53,28,42,235});
        ::DrawRectangleRoundedLinesEx(actionPlate,0.35f,10,2.0f,
            selectedUnlocked?Color{244,196,72,225}:Color{175,77,94,210});
        drawCentered(action,screenW*0.5f,
                     actionPlate.y+(actionPlate.height-actionMeasure.y)*0.5f,
                     actionSize,
                     selectedUnlocked?Color{255,232,145,255}:Color{225,153,164,240});
        drawCentered("ARROWS / WASD  NAVIGATE     ENTER / CLICK  SELECT     ESC  BACK",
                     screenW*0.5f,screenH*0.925f,std::max(13.0f,screenH*0.021f),
                     Color{175,162,205,220});
    }
    return;

#if 0 // Replaced by the responsive saved-star layout above.

    int sw = Renderer::GetInstance().GetWindowWidth();
    int sh = Renderer::GetInstance().GetWindowHeight();

    // Dark background
    ::DrawRectangle(0, 0, sw, sh, Color{8, 6, 18, 255});
    
    // Header HUD (player name only — no coins)
    {
        float barH = sh * 0.048f;
        ::DrawRectangle(0, 0, sw, (int)barH, Color{0, 0, 0, 90});
        int fontSize = (int)(barH * 0.60f); if (fontSize < 10) fontSize = 10;
        char buf[64];
        snprintf(buf, sizeof(buf), "  %s", m_playerName.c_str());
        ::DrawText(buf, 8, (int)((barH - fontSize) * 0.5f), fontSize, Color{220, 200, 255, 200});
    }

    // Title
    {
        const char* title = "SELECT LEVEL";
        int fs = (int)(sh * 0.055f); if (fs < 18) fs = 18;
        int tw = ::MeasureText(title, fs);
        ::DrawText(title, (sw-tw)/2, (int)(sh*0.08f), fs, Color{255, 230, 80, 255});
    }

    // Spotlight pulse on unlocked levels (pulse using sine wave)
    float pulse = (sinf(m_spotlightPulse * 2.2f) * 0.5f + 0.5f); // 0..1

    float cardW   = sw * 0.18f;
    float cardH   = sh * 0.22f;
    float spacingX = (sw - m_totalLevels * cardW) / (m_totalLevels + 1);
    float cardY   = (sh - cardH) * 0.5f;

    for (int i = 0; i < m_totalLevels; ++i) {
        float cx = spacingX + i * (cardW + spacingX);
        bool unlocked = (i < m_unlockedLevels);
        bool sel      = (m_selected == i);

        // Spotlight circle behind selected card
        if (sel) {
            float radius = cardW * (0.75f + pulse * 0.12f);
            unsigned char spotA = (unsigned char)(40 + (int)(pulse * 35));
            Color spotColor = unlocked ? Color{200, 180, 80, spotA} : Color{180, 50, 60, spotA};
            ::DrawCircle((int)(cx + cardW*0.5f), (int)(cardY + cardH*0.5f),
                         radius, spotColor);
        }

        // Card body
        Color bg  = unlocked ? (sel ? Color{70,55,110,235} : Color{40,30,65,220})
                             : (sel ? Color{45,25,35,230} : Color{20,18,30,200});
        Color brd = unlocked ? (sel ? Color{255,210,50,230} : Color{130,100,180,180})
                             : (sel ? Color{180,60,70,220} : Color{50,45,65,150});
        ::DrawRectangle((int)cx, (int)cardY, (int)cardW, (int)cardH, bg);
        ::DrawRectangleLinesEx({cx, cardY, cardW, cardH}, 2.5f, brd);

        // Level number
        char buf[8];
        snprintf(buf, sizeof(buf), "%d", i+1);
        int fs = (int)(cardH * 0.40f); if (fs < 14) fs = 14;
        int tw = ::MeasureText(buf, fs);
        Color numCol = unlocked ? (sel ? Color{255,230,60,255} : WHITE)
                                : Color{80,75,90,200};
        ::DrawText(buf, (int)(cx+(cardW-tw)*0.5f), (int)(cardY+cardH*0.20f), fs, numCol);

        // Lock icon if locked
        if (!unlocked) {
            const char* lock = "[LOCKED]";
            int lfs = (int)(cardH * 0.15f); if (lfs < 10) lfs = 10;
            int lw = ::MeasureText(lock, lfs);
            ::DrawText(lock, (int)(cx+(cardW-lw)*0.5f), (int)(cardY+cardH*0.65f),
                       lfs, Color{150,130,180,200});
        }
    }

    // Navigation hint
    {
        const char* hint = "[<- ->] Navigate    [ENTER] Select    [ESC] Back";
        int hfs = (int)(sh * 0.022f); if (hfs < 10) hfs = 10;
        int hw = ::MeasureText(hint, hfs);
        ::DrawText(hint, (sw-hw)/2, (int)(sh*0.90f), hfs, Color{160,150,190,200});
    }
#endif
}

void MenuView::DrawAchievementIcon(int icon, Rectangle destination, Color tint) const {
    if (icon < 0 || icon >= (int)m_achievementIcons.size()) return;
    const Texture2D& texture = m_achievementIcons[icon];
    if (texture.id == 0) {
        DrawPoly({destination.x + destination.width * 0.5f,
                  destination.y + destination.height * 0.5f}, 4,
                 std::min(destination.width, destination.height) * 0.30f, 45.0f, tint);
        return;
    }
    if (icon == (int)AchievementIcon::Coop) {
        Rectangle left{destination.x, destination.y + destination.height * 0.12f,
                       destination.width * 0.62f, destination.height * 0.82f};
        Rectangle right{destination.x + destination.width * 0.38f, destination.y,
                        destination.width * 0.62f, destination.height * 0.82f};
        DrawTexturePro(texture, m_achievementIconSources[icon], left, {0, 0}, 0.0f, tint);
        DrawTexturePro(texture, m_achievementIconSources[icon], right, {0, 0}, 0.0f, tint);
        return;
    }
    DrawTexturePro(texture, m_achievementIconSources[icon], destination, {0, 0}, 0.0f, tint);
}

void MenuView::RenderLeaderboard() {
    const float w = (float)Renderer::GetInstance().GetWindowWidth();
    const float h = (float)Renderer::GetInstance().GetWindowHeight();
    const RecordsLayout l = BuildRecordsLayout(w, h);
    const Font font = m_fontsLoaded ? m_fontBody : GetFontDefault();
    const Color gold{241, 193, 78, 255};
    const Color violet{151, 116, 206, 255};

    RenderParallax();
    DrawRectangle(0, 0, (int)w, (int)h, Color{7, 4, 17, 188});
    DrawRectangleRounded({l.panel.x + 9, l.panel.y + 11, l.panel.width, l.panel.height},
                         0.045f, 12, Color{0, 0, 0, 135});
    DrawRectangleRounded(l.panel, 0.045f, 12, Color{19, 14, 34, 250});
    DrawRectangleGradientV((int)l.panel.x + 3, (int)l.panel.y + 3,
                           (int)l.panel.width - 6, (int)(l.panel.height * 0.15f),
                           Color{65, 44, 91, 255}, Color{19, 14, 34, 250});
    DrawRectangleRoundedLinesEx(l.panel, 0.045f, 12, 3.0f, gold);

    const float titleSize = std::clamp(l.panel.height * 0.067f, 25.0f, 39.0f);
    const char* title = "LEADERBOARD";
    const Vector2 titleMeasure = MeasureTextEx(font, title, titleSize, 1.4f);
    DrawTextEx(font, title, {l.panel.x + (l.panel.width - titleMeasure.x) * 0.5f,
                            l.panel.y + 18.0f}, titleSize, 1.4f, gold);

    for (int i = 0; i < 6; ++i) {
        const bool selected = m_leaderboardLevel == i + 1;
        DrawRectangleRounded(l.levelTabs[i], 0.18f, 8,
            selected ? Color{89, 61, 124, 255} : Color{31, 24, 51, 245});
        DrawRectangleRoundedLinesEx(l.levelTabs[i], 0.18f, 8, selected ? 2.5f : 1.3f,
            selected ? gold : Color{91, 73, 125, 220});
        char label[24];
        std::snprintf(label, sizeof(label), "LEVEL %d", i + 1);
        float fs = std::clamp(l.levelTabs[i].height * 0.38f, 12.0f, 17.0f);
        Vector2 size = MeasureTextEx(font, label, fs, 0.6f);
        DrawTextEx(font, label,
            {l.levelTabs[i].x + (l.levelTabs[i].width - size.x) * 0.5f,
             l.levelTabs[i].y + (l.levelTabs[i].height - size.y) * 0.5f},
            fs, 0.6f, selected ? Color{255, 244, 204, 255} : Color{188, 178, 207, 255});
    }

    const char* modeLabels[2] = {"TOP SCORE", "FASTEST TIME"};
    for (int i = 0; i < 2; ++i) {
        const bool selected = m_leaderboardFastest == (i == 1);
        DrawRectangleRounded(l.modeTabs[i], 0.20f, 8,
            selected ? Color{70, 104, 117, 255} : Color{28, 23, 47, 245});
        DrawRectangleRoundedLinesEx(l.modeTabs[i], 0.20f, 8, 2.0f,
            selected ? Color{112, 230, 207, 255} : Color{83, 67, 115, 210});
        const float fs = std::clamp(l.modeTabs[i].height * 0.38f, 12.0f, 17.0f);
        const Vector2 size = MeasureTextEx(font, modeLabels[i], fs, 0.7f);
        DrawTextEx(font, modeLabels[i],
            {l.modeTabs[i].x + (l.modeTabs[i].width - size.x) * 0.5f,
             l.modeTabs[i].y + (l.modeTabs[i].height - size.y) * 0.5f},
            fs, 0.7f, selected ? Color{210, 255, 239, 255} : Color{178, 169, 198, 255});
    }

    DrawRectangleRounded(l.table, 0.025f, 8, Color{11, 9, 22, 235});
    DrawRectangleRoundedLinesEx(l.table, 0.025f, 8, 1.5f, Color{91, 72, 125, 210});
    const float headerH = std::clamp(l.table.height * 0.13f, 34.0f, 46.0f);
    DrawRectangleGradientH((int)l.table.x + 2, (int)l.table.y + 2,
                           (int)l.table.width - 4, (int)headerH,
                           Color{53, 38, 76, 245}, Color{32, 28, 59, 245});
    const float columns[6] = {0.035f, 0.12f, 0.39f, 0.60f, 0.75f, 0.89f};
    const char* headers[6] = {"#", "PLAYER", "HERO", "SCORE", "TIME", "STARS"};
    const float headerFs = std::clamp(headerH * 0.38f, 12.0f, 16.0f);
    for (int i = 0; i < 6; ++i)
        DrawTextEx(font, headers[i], {l.table.x + l.table.width * columns[i],
                   l.table.y + (headerH - headerFs) * 0.5f}, headerFs, 0.6f, gold);

    const auto& entries = m_leaderboardFastest
        ? SaveManager::GetInstance().GetTopTimes(m_leaderboardLevel)
        : SaveManager::GetInstance().GetTopScores(m_leaderboardLevel);
    if (entries.empty()) {
        const char* empty = "NO RECORDS YET - COMPLETE THIS LEVEL TO CLAIM THE BOARD";
        float fs = std::clamp(l.table.height * 0.06f, 14.0f, 20.0f);
        Vector2 size = MeasureTextEx(font, empty, fs, 0.7f);
        DrawTextEx(font, empty, {l.table.x + (l.table.width - size.x) * 0.5f,
                   l.table.y + headerH + (l.table.height - headerH - size.y) * 0.5f},
                   fs, 0.7f, Color{155, 145, 178, 235});
    } else {
        const float rowH = (l.table.height - headerH - 4.0f) / 5.0f;
        for (int row = 0; row < (int)entries.size() && row < 5; ++row) {
            const auto& entry = entries[row];
            const float y = l.table.y + headerH + row * rowH;
            if (row % 2 == 0) DrawRectangle((int)l.table.x + 2, (int)y,
                (int)l.table.width - 4, (int)rowH, Color{36, 28, 54, 145});
            if (row == 0) DrawRectangle((int)l.table.x + 2, (int)y, 5, (int)rowH, gold);
            const Color textColor = row == 0 ? Color{255, 238, 180, 255} : Color{225, 217, 237, 255};
            const float fs = std::clamp(rowH * 0.34f, 12.0f, 18.0f);
            char rank[8], score[24], time[24], stars[8];
            std::snprintf(rank, sizeof(rank), "%d", row + 1);
            std::snprintf(score, sizeof(score), "%d", entry.score);
            const int seconds = entry.timeMs / 1000;
            std::snprintf(time, sizeof(time), "%02d:%02d.%03d", seconds / 60,
                          seconds % 60, entry.timeMs % 1000);
            std::snprintf(stars, sizeof(stars), "%d/3", entry.stars);
            std::string heroes;
            for (size_t i = 0; i < entry.characterIds.size(); ++i) {
                std::string hero;
                if (entry.characterIds[i] == "magic_caster") hero = "MAGE";
                else if (entry.characterIds[i] == "fighter") hero = "FIGHTER";
                else if (entry.characterIds[i] == "ninja") hero = "NINJA";
                else hero = "KNIGHT";
                if (i) heroes += " + ";
                heroes += hero;
            }
            const float textY = y + (rowH - fs) * 0.5f;
            DrawTextEx(font, rank, {l.table.x + l.table.width * columns[0], textY}, fs, 0.5f, textColor);
            DrawTextEx(font, entry.playerName.c_str(), {l.table.x + l.table.width * columns[1], textY}, fs, 0.5f, textColor);
            DrawTextEx(font, heroes.c_str(), {l.table.x + l.table.width * columns[2], textY}, fs * 0.86f, 0.3f,
                       entry.localCoop ? Color{121, 235, 210, 255} : textColor);
            DrawTextEx(font, score, {l.table.x + l.table.width * columns[3], textY}, fs, 0.5f, textColor);
            DrawTextEx(font, time, {l.table.x + l.table.width * columns[4], textY}, fs * 0.90f, 0.3f, textColor);
            DrawTextEx(font, stars, {l.table.x + l.table.width * columns[5], textY}, fs, 0.5f, textColor);
        }
    }

    const char* hint = "[LEFT / RIGHT] LEVEL     [UP / DOWN] RANKING     [ESC] BACK";
    const float hintFs = std::clamp(h * 0.018f, 11.0f, 15.0f);
    const Vector2 hintSize = MeasureTextEx(font, hint, hintFs, 0.5f);
    DrawTextEx(font, hint, {(w - hintSize.x) * 0.5f, l.panel.y + l.panel.height - 21.0f},
               hintFs, 0.5f, Color{170, 158, 194, 230});
}

void MenuView::RenderAchievements() {
    const float w = (float)Renderer::GetInstance().GetWindowWidth();
    const float h = (float)Renderer::GetInstance().GetWindowHeight();
    const RecordsLayout base = BuildRecordsLayout(w, h);
    const Rectangle panel = base.panel;
    const Rectangle content{panel.x + 54.0f, panel.y + 112.0f,
                            panel.width - 108.0f, panel.height - 205.0f};
    const Font font = m_fontsLoaded ? m_fontBody : GetFontDefault();
    const auto& manager = AchievementManager::GetInstance();
    const auto& definitions = manager.GetDefinitions();
    const Color gold{242, 194, 77, 255};

    RenderParallax();
    DrawRectangle(0, 0, (int)w, (int)h, Color{7, 4, 17, 192});
    DrawRectangleRounded({panel.x + 9, panel.y + 11, panel.width, panel.height},
                         0.045f, 12, Color{0, 0, 0, 135});
    DrawRectangleRounded(panel, 0.045f, 12, Color{18, 14, 32, 250});
    DrawRectangleGradientV((int)panel.x + 3, (int)panel.y + 3,
                           (int)panel.width - 6, 92,
                           Color{65, 44, 91, 255}, Color{18, 14, 32, 250});
    DrawRectangleRoundedLinesEx(panel, 0.045f, 12, 3.0f, gold);

    const float titleSize = std::clamp(panel.height * 0.062f, 24.0f, 38.0f);
    const char* title = "ACHIEVEMENTS";
    const Vector2 titleSizeV = MeasureTextEx(font, title, titleSize, 1.3f);
    DrawTextEx(font, title, {panel.x + (panel.width - titleSizeV.x) * 0.5f,
               panel.y + 16.0f}, titleSize, 1.3f, gold);
    char countText[48];
    std::snprintf(countText, sizeof(countText), "ADVANCEMENTS  %d / %d",
                  manager.GetUnlockedCount(), (int)definitions.size());
    const float countFs = std::clamp(panel.height * 0.026f, 12.0f, 16.0f);
    const Vector2 countSize = MeasureTextEx(font, countText, countFs, 0.6f);
    DrawTextEx(font, countText, {panel.x + (panel.width - countSize.x) * 0.5f,
               panel.y + 60.0f}, countFs, 0.6f, Color{195, 181, 216, 255});

    const int links[][2] = {{0,1},{1,2},{0,3},{3,4},{2,5},{5,6},{7,8},{7,9},{10,11},{11,12},{12,13},{13,14}};
    for (const auto& link : links) {
        if (link[0] >= (int)definitions.size() || link[1] >= (int)definitions.size()) continue;
        Rectangle a = AchievementNodeRect(definitions[link[0]], content);
        Rectangle b = AchievementNodeRect(definitions[link[1]], content);
        const bool lit = manager.IsUnlocked(definitions[link[0]].id)
                      && manager.IsUnlocked(definitions[link[1]].id);
        DrawLineEx({a.x + a.width * 0.5f, a.y + a.height * 0.5f},
                   {b.x + b.width * 0.5f, b.y + b.height * 0.5f},
                   lit ? 5.0f : 3.0f, lit ? Color{242, 194, 77, 220} : Color{70, 61, 88, 210});
    }

    for (int i = 0; i < (int)definitions.size(); ++i) {
        const auto& definition = definitions[i];
        const bool unlocked = manager.IsUnlocked(definition.id);
        const bool selected = i == m_achievementSelected;
        Rectangle node = AchievementNodeRect(definition, content);
        if (selected) {
            const float pulse = 5.0f + std::sin(m_spotlightPulse * 4.0f) * 2.0f;
            DrawCircleGradient({node.x + node.width * 0.5f,
                node.y + node.height * 0.5f}, node.width * 0.75f + pulse,
                Color{247, 196, 70, 80}, Color{247, 196, 70, 0});
        }
        DrawRectangleRounded({node.x + 5.0f, node.y + 7.0f, node.width, node.height},
                             0.18f, 9, Color{0, 0, 0, 130});
        DrawRectangleRounded(node, 0.18f, 9,
            unlocked ? Color{79, 58, 104, 255} : Color{31, 29, 40, 255});
        DrawRectangleRoundedLinesEx(node, 0.18f, 9, selected ? 4.0f : 2.5f,
            unlocked ? gold : (selected ? Color{167, 151, 186, 255} : Color{76, 69, 88, 255}));
        Rectangle iconDest{node.x + node.width * 0.16f, node.y + node.height * 0.13f,
                           node.width * 0.68f, node.height * 0.68f};
        DrawAchievementIcon((int)definition.icon, iconDest,
            unlocked ? WHITE : Color{75, 75, 83, 230});
        const int progress = manager.GetProgress(definition.id);
        if (!unlocked && definition.target > 1) {
            const float ratio = std::clamp((float)progress / definition.target, 0.0f, 1.0f);
            Rectangle bar{node.x + 7.0f, node.y + node.height - 9.0f, node.width - 14.0f, 4.0f};
            DrawRectangleRec(bar, Color{16, 14, 23, 255});
            DrawRectangle((int)bar.x, (int)bar.y, (int)(bar.width * ratio), (int)bar.height,
                          Color{119, 211, 179, 255});
        }
    }

    if (!definitions.empty()) {
        const auto& selected = definitions[std::clamp(m_achievementSelected, 0, (int)definitions.size() - 1)];
        const bool unlocked = manager.IsUnlocked(selected.id);
        const int progress = manager.GetProgress(selected.id);
        Rectangle tip{panel.x + 34.0f, panel.y + panel.height - 79.0f,
                      panel.width - 68.0f, 56.0f};
        DrawRectangleRounded(tip, 0.12f, 8, Color{34, 26, 51, 250});
        DrawRectangleRoundedLinesEx(tip, 0.12f, 8, 2.0f,
                                    unlocked ? gold : Color{93, 80, 116, 255});
        const float nameFs = std::clamp(tip.height * 0.34f, 15.0f, 20.0f);
        DrawTextEx(font, selected.title.c_str(), {tip.x + 16.0f, tip.y + 8.0f},
                   nameFs, 0.6f, unlocked ? gold : Color{205, 197, 218, 255});
        DrawTextEx(font, selected.description.c_str(), {tip.x + 16.0f, tip.y + 31.0f},
                   nameFs * 0.70f, 0.4f, Color{184, 174, 202, 255});
        char progressText[32];
        std::snprintf(progressText, sizeof(progressText), unlocked ? "UNLOCKED" : "%d / %d",
                      progress, selected.target);
        const Vector2 psize = MeasureTextEx(font, progressText, nameFs * 0.78f, 0.5f);
        DrawTextEx(font, progressText, {tip.x + tip.width - psize.x - 16.0f,
                   tip.y + (tip.height - psize.y) * 0.5f}, nameFs * 0.78f, 0.5f,
                   unlocked ? Color{119, 232, 183, 255} : Color{190, 174, 211, 255});
    }

    const char* hint = "[ARROWS] EXPLORE     [ESC] BACK";
    const float hintFs = std::clamp(h * 0.016f, 10.0f, 13.0f);
    const Vector2 hintSize = MeasureTextEx(font, hint, hintFs, 0.4f);
    DrawTextEx(font, hint, {(w - hintSize.x) * 0.5f, panel.y + panel.height - 18.0f},
               hintFs, 0.4f, Color{155, 143, 178, 225});
}
