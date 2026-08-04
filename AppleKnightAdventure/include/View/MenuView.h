#pragma once

#include "raylib.h"
#include <vector>
#include <string>

namespace View {

enum class MenuMode {
    Main,
    Pause,
    Error,
    Connection,
    RoleSelect
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
    
    int GetHoveredItem(Vector2 mousePos) const;

    void ShowMainMenu();
    void ShowPauseOverlay();
    void ShowErrorDialog(const std::string& message);
    void ShowConnectionStatus(const std::string& ip, bool connected);
    void ShowRoleSelect(const std::vector<std::string>& roles);

private:
    MenuView() = default;

    void RenderMain();
    void RenderPause();
    void RenderError();
    void RenderConnection();
    void RenderRoleSelect();

    // Helpers — draw Dark Dwellers themed UI elements
    void DrawButton(const char* label, float x, float y, float w, float h, bool selected);
    void DrawPanel(float x, float y, float w, float h);

    bool m_visible = true;
    bool m_loaded = false;
    int m_selected = 0;

    MenuMode m_mode = MenuMode::Main;

    std::vector<std::string> m_mainItems = { "Start", "Map Builder", "Options", "Quit" };
    std::vector<std::string> m_pauseItems = { "Resume", "Quit to Menu" };

    std::string m_errorMsg;
    std::vector<std::string> m_errorItems = { "OK" };

    std::string m_connectionIp;
    bool m_connected = false;
    std::vector<std::string> m_connectionItems = { "Back" };

    std::vector<std::string> m_roleItems;
    int m_selectedRole = 0;

};

} // namespace View
