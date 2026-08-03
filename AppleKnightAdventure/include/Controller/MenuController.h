#ifndef MENUCONTROLLER_H
#define MENUCONTROLLER_H

class MenuController {
public:
    static MenuController& GetInstance();

    bool Init();
    void Shutdown();
    void Update(float dt);

    void ShowMainMenu();

    bool ShouldStartGame() const { return m_startGame; }
    bool ShouldOpenMapBuilder() const { return m_openMapBuilder; }
    bool ShouldQuit() const { return m_quit; }
    int GetSelected() const { return m_selected; }

private:
    MenuController() = default;

    void HandleMainMenuInput();

    int m_selected = 0;
    bool m_startGame = false;
    bool m_openMapBuilder = false;
    bool m_quit = false;
};

#endif
