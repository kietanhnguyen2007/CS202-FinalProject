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
#include <cmath>
#include <cstdio>
#include <algorithm>

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
    m_loaded = false;
}

// ─────────────────────────────────────────────────────────────────────────────
// SetHeaderData — fed from MenuController after reading SaveManager
// ─────────────────────────────────────────────────────────────────────────────
void MenuView::SetHeaderData(const std::string& playerName, int coins) {
    m_playerName = playerName;
    m_coins      = coins;
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

    // ── Parallax ──────────────────────────────────────────────────────────
    UpdateParallax(dt);

    // ── Smooth shop scroll ────────────────────────────────────────────────
    m_shopScrollDisp = Lerp(m_shopScrollDisp, (float)m_shopScrollYF, 12.0f * dt);

    // ── Animated button hover update ─────────────────────────────────────
    if (m_mode == MenuMode::Main) {
        Vector2 mouse = ::GetMousePosition();
        int w = Renderer::GetInstance().GetWindowWidth();
        int h = Renderer::GetInstance().GetWindowHeight();

        float btnW    = w * 0.35f;
        float btnH    = h * 0.080f;
        float spacing = h * 0.100f; // btnH + 0.02f offset
        float startY  = h * 0.46f;
        float btnX    = ((float)w - btnW) * 0.5f;

        for (int i = 0; i < (int)m_mainItems.size() && i < kMaxMainButtons; ++i) {
            float by = startY + (float)i * spacing;
            m_mainButtons[i].baseRect = { btnX, by, btnW, btnH };
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
    m_mode           = MenuMode::LevelSelect;
    m_totalLevels    = totalLevels;
    m_unlockedLevels = currentUnlocked;
    m_selected       = 0;
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
        float btnW    = w * 0.35f;
        float btnH    = h * 0.080f;
        float spacing = h * 0.100f; // btnH + 0.02f offset
        float startY  = h * 0.46f;
        float btnX    = ((float)w - btnW) * 0.5f;

        for (int i = 0; i < (int)m_mainItems.size(); ++i) {
            float by   = startY + (float)i * spacing;
            // Expand hit area slightly for the hover state (scale=1.12)
            float expand = btnW * 0.06f;
            Rectangle rect = { btnX - expand * 0.5f, by, btnW + expand, btnH };
            if (::CheckCollisionPointRec(mousePos, rect)) return i;
        }
    } else if (m_mode == MenuMode::Pause) {
        float panelW = w * 0.35f;
        float panelH = h * 0.40f;
        float panelX = ((float)w - panelW) * 0.5f;
        float panelY = ((float)h - panelH) * 0.5f;
        float btnW   = panelW * 0.65f;
        float btnH   = panelH * 0.13f;
        float btnX   = panelX + (panelW - btnW) * 0.5f;
        float startY = panelY + panelH * 0.40f;
        float spacing = panelH * 0.18f;

        for (int i = 0; i < (int)m_pauseItems.size(); ++i) {
            float by   = startY + (float)i * spacing;
            Rectangle rect = { btnX, by, btnW, btnH };
            if (::CheckCollisionPointRec(mousePos, rect)) return i;
        }
    }
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
}

// =============================================================================
// ── RENDER PAUSE ─────────────────────────────────────────────────────────────
// =============================================================================
void MenuView::RenderPause() {
    Renderer& r = Renderer::GetInstance();
    int w = r.GetWindowWidth();
    int h = r.GetWindowHeight();

    r.DrawRectangle({0, 0}, {(float)w, (float)h}, {0, 0, 0, 160}, Layer::UI, 0.0f);

    float panelW  = w * 0.35f;
    float panelH  = h * 0.40f;
    float panelX  = ((float)w - panelW) * 0.5f;
    float panelY  = ((float)h - panelH) * 0.5f;
    DrawPanel(panelX, panelY, panelW, panelH);

    Texture2D* texHeader = UIResourceManager::GetInstance().GetHeader();
    if (texHeader && texHeader->id != 0) {
        float hdrW = panelW * 0.8f;
        float hdrH = hdrW * ((float)texHeader->height / (float)texHeader->width);
        Rectangle hdrSrc = {0.f, 0.f, (float)texHeader->width, (float)texHeader->height};
        r.SubmitSprite(texHeader, hdrSrc,
                       {panelX + (panelW - hdrW)*0.5f, panelY + panelH*0.04f},
                       {hdrW/(float)texHeader->width, hdrH/(float)texHeader->height},
                       0.f, {0,0}, WHITE, Layer::UI, 0.8f, false, 0);
    }

    {
        const char* title = "PAUSED";
        int fontSize = (int)(h * 0.05f); if (fontSize < 14) fontSize = 14;
        int tw = ::MeasureText(title, fontSize);
        r.DrawText(title, {panelX + (panelW - (float)tw)*0.5f, panelY + panelH*0.12f},
                   fontSize, WHITE);
    }

    float btnW    = panelW * 0.65f;
    float btnH    = panelH * 0.13f;
    float btnX    = panelX + (panelW - btnW) * 0.5f;
    float startY  = panelY + panelH * 0.40f;
    float spacing = panelH * 0.18f;

    for (int i = 0; i < (int)m_pauseItems.size(); ++i) {
        float by = startY + (float)i * spacing;
        DrawButton(m_pauseItems[i].c_str(), btnX, by, btnW, btnH, (i == m_selected));
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
    const int   prices[] = {0, 200, 350, 500};
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
}
