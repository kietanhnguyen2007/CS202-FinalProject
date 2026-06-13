#pragma once

#include "raylib.h"
#include "View/TextureAtlas.h"
#include <vector>
#include <string>
#include <memory>

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

    std::shared_ptr<Animations::TextureAtlas> m_menuBtnAtlas;
    std::shared_ptr<Animations::TextureAtlas> m_pauseBtnAtlas;
};

} // namespace View
