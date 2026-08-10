#pragma once

#include "raylib.h"
#include <vector>
#include <string>
#include <array>

namespace View {

enum class MenuMode {
    Main,
    Pause,
    Error,
    Connection,
    RoleSelect,
    Shop,
    LevelSelect,
    Options
};

// ─────────────────────────────────────────────────────────────────────────────
// Internal data for one parallax layer
// ─────────────────────────────────────────────────────────────────────────────
struct ParallaxLayer {
    Texture2D tex{};
    float     speedMultiplier = 0.2f;   // relative scroll speed (0=sky, 1=fg)
    float     offsetX         = 0.0f;   // current horizontal offset (pixels)
    bool      loaded          = false;
};

// ─────────────────────────────────────────────────────────────────────────────
// Internal data for one animated menu button
// ─────────────────────────────────────────────────────────────────────────────
struct AnimatedButton {
    Rectangle baseRect{};           // logical rect at scale=1
    float     scale        = 1.0f;  // current animated scale (lerp)
    float     targetScale  = 1.0f;  // 1.0 normal, 1.12 hover
    float     glowAlpha    = 0.0f;  // current glow transparency (lerp)
    bool      hovered      = false;
};

// ─────────────────────────────────────────────────────────────────────────────
// MenuView — singleton
// ─────────────────────────────────────────────────────────────────────────────
class MenuView {
public:
    static MenuView& GetInstance();

    bool Init();
    bool LoadResources(const std::string& atlasJsonPath);
    void Shutdown();

    void Update(float dt, int selectedIndex);
    void Render();
    void SetVisible(bool v) { m_visible = v; }
    MenuMode GetMode() const { return m_mode; }

    int GetHoveredItem(Vector2 mousePos) const;

    void ShowMainMenu();
    void ShowPauseOverlay();
    void ShowErrorDialog(const std::string& message);
    void ShowConnectionStatus(const std::string& ip, bool connected);
    void ShowRoleSelect(const std::vector<std::string>& roles);
    void ShowShop();
    void ShowLevelSelect(int totalLevels, int currentUnlocked);
    void ShowOptions();

    // Feed save data to display in header (no longer shows coins in main menu)
    void SetHeaderData(const std::string& playerName, int coins);

private:
    MenuView() = default;

    // ── Render sub-modes ──────────────────────────────────────────────────
    void RenderMain();
    void RenderPause();
    void RenderError();
    void RenderConnection();
    void RenderRoleSelect();
    void RenderShop();
    void RenderLevelSelect();

    // ── Parallax background ───────────────────────────────────────────────
    void LoadParallaxLayers();
    void UnloadParallaxLayers();
    void UpdateParallax(float dt);
    void RenderParallax();

    // ── Blur (2-pass Gaussian) ────────────────────────────────────────────
    void InitBlurShader();
    void ShutdownBlurShader();
    void ApplyBlurToScreen();   // blurs m_sceneTex → m_blurTex, draws result

    // ── Helpers ───────────────────────────────────────────────────────────
    void DrawButton(const char* label, float x, float y,
                    float w, float h, bool selected);
    void DrawPanel(float x, float y, float w, float h);

    // Animated button helpers
    void UpdateButtonHover(AnimatedButton& btn, Vector2 mouse, float dt);
    void DrawAnimatedButton(const char* label, AnimatedButton& btn, bool selected);

    // Draw glow ring around hovered button
    void DrawGlowRect(Rectangle r, Color col, float alpha, float thickness);

    // Header HUD (player name + coins) top bar
    void DrawHeaderHUD();

    // ── State ─────────────────────────────────────────────────────────────
    bool     m_visible  = true;
    bool     m_loaded   = false;
    int      m_selected = 0;
    MenuMode m_mode     = MenuMode::Main;

    // Header / HUD data (fed from Controller)
    std::string m_playerName = "Player";
    int         m_coins      = 0;

    // Scroll time (drives parallax)
    float m_scrollTime = 0.0f;

    // ── Main menu buttons (animated) ──────────────────────────────────────
    static constexpr int kMaxMainButtons = 6;
    std::array<AnimatedButton, kMaxMainButtons> m_mainButtons{};

    // Label lists
    std::vector<std::string> m_mainItems    = { "Play", "Map Builder", "Shop", "Options", "Quit" };
    std::vector<std::string> m_pauseItems   = { "Resume", "Quit to Menu" };
    std::string              m_errorMsg;
    std::vector<std::string> m_errorItems   = { "OK" };
    std::string              m_connectionIp;
    bool                     m_connected    = false;
    std::vector<std::string> m_connectionItems = { "Back" };
    std::vector<std::string> m_roleItems;
    int                      m_selectedRole  = 0;

    // Level select state
    int m_totalLevels      = 3;
    int m_unlockedLevels   = 1;

    // ── Parallax textures ─────────────────────────────────────────────────
    static constexpr int kParallaxLayers = 3;
    ParallaxLayer m_parallaxLayers[kParallaxLayers];
    float         m_parallaxBaseSpeed = 60.0f;  // pixels/sec at fastest layer

    // ── Blur shader ───────────────────────────────────────────────────────
    Shader          m_blurShader{};
    RenderTexture2D m_blurPassA{};   // horizontal pass output
    RenderTexture2D m_blurPassB{};   // vertical pass output (final)
    int             m_locDirection{};
    int             m_locResolution{};
    bool            m_blurReady = false;

    // ── Fonts ─────────────────────────────────────────────────────────────
    Font m_fontTitle{};
    Font m_fontBody{};
    bool m_fontsLoaded = false;

    // ── Title float animation ─────────────────────────────────────────────
    float m_titleBobTime = 0.0f;

    // ── Shop state ────────────────────────────────────────────────────────
    int              m_shopScrollY    = 0;
    float            m_shopScrollYF   = 0.0f;   // smooth scroll target
    float            m_shopScrollDisp = 0.0f;   // displayed scroll (lerp)
    int              m_shopSelected   = -1;

    // ── Level select spotlight pulse ──────────────────────────────────────
    float m_spotlightPulse = 0.0f;
};

} // namespace View
