#include "Controller/MenuController.h"
#include "Controller/InputController.h"
#include "View/MenuView.h"
#include "Systems/SoundManager.h"

MenuController& MenuController::GetInstance() {
    static MenuController instance;
    return instance;
}

bool MenuController::Init() {
    auto& menuView = View::MenuView::GetInstance();
    menuView.Init();
    menuView.LoadResources("assets/ui/ui_atlas.json");
    menuView.ShowMainMenu();

    auto& snd = SoundManager::GetInstance();
    if (snd.InitAudio()) {
        snd.LoadMusic("bgm_menu", "assets/sounds/music/bgm_menu.wav");
        snd.PlayMusic("bgm_menu");
    }

    m_selected = 0;
    m_startGame = false;
    m_quit = false;
    return true;
}

void MenuController::Shutdown() {
    View::MenuView::GetInstance().Shutdown();
}

void MenuController::ShowMainMenu() {
    m_selected = 0;
    m_startGame = false;
    m_quit = false;
    View::MenuView::GetInstance().ShowMainMenu();
}

void MenuController::HandleMainMenuInput() {
    InputCommand cmd = InputController::GetInstance().Poll();
    auto& menuView = View::MenuView::GetInstance();

    if (cmd.menuDelta != 0) {
        m_selected += cmd.menuDelta;
        if (m_selected < 0) m_selected = 2;
        if (m_selected > 2) m_selected = 0;
    }

    if (cmd.menuConfirm) {
        switch (m_selected) {
            case 0:
                m_startGame = true;
                break;
            case 1:
                break;
            case 2:
                m_quit = true;
                break;
        }
    }

    menuView.Update(0.0f, m_selected);
}

void MenuController::Update(float dt) {
    (void)dt;
    HandleMainMenuInput();
}
