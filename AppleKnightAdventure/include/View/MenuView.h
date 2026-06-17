#pragma once

#include "raylib.h"
#include <vector>
#include <string>

namespace View {

enum class MenuMode {
    Main,
    Pause,
    Error,
    Connection
};

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

    void ShowMainMenu();
    void ShowPauseOverlay();
    void ShowErrorDialog(const std::string& message);
    void ShowConnectionStatus(const std::string& ip, bool connected);

private:
    MenuView() = default;

    void RenderMain();
    void RenderPause();
    void RenderError();
    void RenderConnection();

    // Helpers — draw Dark Dwellers themed UI elements
    void DrawButton(const char* label, float x, float y, float w, float h, bool selected);
    void DrawPanel(float x, float y, float w, float h);

    bool m_visible = true;
    bool m_loaded = false;
    int m_selected = 0;

    MenuMode m_mode = MenuMode::Main;

    std::vector<std::string> m_mainItems = { "Start", "Options", "Quit" };
    std::vector<std::string> m_pauseItems = { "Resume", "Quit to Menu" };

    std::string m_errorMsg;
    std::vector<std::string> m_errorItems = { "OK" };

    std::string m_connectionIp;
    bool m_connected = false;
    std::vector<std::string> m_connectionItems = { "Back" };

    // Dark Dwellers textures
    Texture2D m_texBtn{};       // Button sprite sheet (4 frames: Normal, Hover, Pressed, Disabled)
    Texture2D m_texPanel{};     // 9-slice panel background
    Texture2D m_texHeader{};    // Decorative header bar
    Texture2D m_texClose{};     // Close button sprite sheet (4 frames)

    int m_btnFrameW = 0;       // Single button frame width
    int m_btnFrameH = 0;       // Single button frame height
    int m_closeFrameW = 0;     // Single close button frame width
    int m_closeFrameH = 0;     // Single close button frame height
};

} // namespace View
