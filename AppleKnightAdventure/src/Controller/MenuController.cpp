// =============================================================================
// MenuController.cpp — Apple Knight Adventure
// Drives the Main Menu loop: reads SaveManager, feeds MenuView,
// handles keyboard + mouse input for all menu modes.
// =============================================================================
#include "Controller/MenuController.h"
#include "Controller/InputController.h"
#include "View/MenuView.h"
#include "Model/SaveManager.h"
#include "Model/MenuStateData.h"
#include "Systems/SoundManager.h"
#include "Systems/TweenSystem.h"
#include "raylib.h"
#include <algorithm>
#include <filesystem>

// ─────────────────────────────────────────────────────────────────────────────
// Singleton
// ─────────────────────────────────────────────────────────────────────────────
MenuController& MenuController::GetInstance() {
    static MenuController instance;
    return instance;
}

// ─────────────────────────────────────────────────────────────────────────────
// Init
// ─────────────────────────────────────────────────────────────────────────────
bool MenuController::Init() {
    // ── Load persistent save data ─────────────────────────────────────────
    auto& save = SaveManager::GetInstance();
    save.Load();  // loads from "save.json"; creates defaults if missing

    // ── Init & load View resources ────────────────────────────────────────
    auto& menuView = View::MenuView::GetInstance();
    menuView.Init();
    menuView.LoadResources("assets/ui/ui_atlas.json");

    // Push header data (player name + coins from save)
    RefreshHeaderData();

    menuView.ShowMainMenu();

    // ── Background music ──────────────────────────────────────────────────
    auto& snd = SoundManager::GetInstance();
    if (snd.InitAudio()) {
        snd.LoadMusic("bgm_menu", "assets/sounds/music/bgm_menu.wav");
        snd.PlayMusic("bgm_menu");
    }

    // ── Reset flags ───────────────────────────────────────────────────────
    ResetFlags();
    m_selected    = 0;
    m_inShop      = false;
    m_inLevelSelect = false;
    m_inputCooldown = 0.0f;
    m_headerRefreshTimer = 0.0f;

    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Shutdown
// ─────────────────────────────────────────────────────────────────────────────
void MenuController::Shutdown() {
    SaveManager::GetInstance().Save();
    View::MenuView::GetInstance().Shutdown();
}

// ─────────────────────────────────────────────────────────────────────────────
// ShowMainMenu — re-enter menu (e.g., from game → menu)
// ─────────────────────────────────────────────────────────────────────────────
void MenuController::ShowMainMenu() {
    ResetFlags();
    m_selected       = 0;
    m_inShop         = false;
    m_inLevelSelect  = false;
    m_inputCooldown  = kInputCooldown; // brief lock to avoid accidental input
    RefreshHeaderData();
    View::MenuView::GetInstance().ShowMainMenu();
}

// ─────────────────────────────────────────────────────────────────────────────
// ResetFlags
// ─────────────────────────────────────────────────────────────────────────────
void MenuController::ResetFlags() {
    m_startGame      = false;
    m_openMapBuilder = false;
    m_quit           = false;
    m_openShop       = false;
    m_openOptions    = false;
}

// ─────────────────────────────────────────────────────────────────────────────
// RefreshHeaderData — push save data into MenuView header
// ─────────────────────────────────────────────────────────────────────────────
void MenuController::RefreshHeaderData() {
    auto& save = SaveManager::GetInstance();
    View::MenuView::GetInstance().SetHeaderData(save.GetPlayerName(), save.GetCoins());
}

// =============================================================================
// Update — master per-frame entry point
// =============================================================================
void MenuController::Update(float dt) {
    // Advance TweenSystem so button animations work
    TweenSystem::GetInstance().Update(dt);

    // Cooldown ticker
    if (m_inputCooldown > 0.0f) {
        m_inputCooldown -= dt;
        if (m_inputCooldown < 0.0f) m_inputCooldown = 0.0f;
    }

    // Periodic save-data refresh for coins display
    m_headerRefreshTimer += dt;
    if (m_headerRefreshTimer >= 2.0f) {
        m_headerRefreshTimer = 0.0f;
        RefreshHeaderData();
    }

    // Update music
    auto& snd = SoundManager::GetInstance();
    if (snd.IsAudioInitialized()) snd.UpdateMusicStream("bgm_menu");

    // Dispatch to the correct input handler
    auto& view = View::MenuView::GetInstance();
    View::MenuMode mode = view.GetMode();

    switch (mode) {
        case View::MenuMode::Main:        HandleMainMenuInput(dt); break;
        case View::MenuMode::LevelSelect: HandleLevelSelectInput(dt); break;
        case View::MenuMode::Shop:        HandleShopInput(dt); break;
        case View::MenuMode::Pause:       HandlePauseInput(dt); break;
        default: break;
    }

    // Always update the view (runs animations etc.)
    view.Update(dt, m_selected);
}

// =============================================================================
// HandleMainMenuInput
// Items: 0=Adventure, 1=Custom Map, 2=Builder, 3=Shop, 4=Options, 5=Quit
// =============================================================================
void MenuController::HandleMainMenuInput(float dt) {
    (void)dt;
    auto& view = View::MenuView::GetInstance();
    InputCommand cmd = InputController::GetInstance().Poll();

    const int kItemCount = 6;

    // ── Keyboard navigation ───────────────────────────────────────────────
    if (m_inputCooldown <= 0.0f) {
        if (cmd.menuDelta != 0) {
            // The main menu is a 2 x 3 grid: vertical input keeps the column.
            m_selected = (m_selected + cmd.menuDelta * 2 + kItemCount) % kItemCount;
            m_inputCooldown = kInputCooldown;
        } else if (cmd.menuDeltaX != 0) {
            // Horizontal input toggles between the two buttons in the same row.
            const int row = m_selected / 2;
            const int col = m_selected % 2;
            m_selected = row * 2 + (col + cmd.menuDeltaX + 2) % 2;
            m_inputCooldown = kInputCooldown;
        }
    }

    // ── Mouse hover ───────────────────────────────────────────────────────
    Vector2 mousePos = ::GetMousePosition();
    int hovered = view.GetHoveredItem(mousePos);
    if (hovered >= 0 && hovered < kItemCount) {
        m_selected = hovered;
    }

    // ── Confirm (keyboard Enter or mouse click) ───────────────────────────
    bool confirm = cmd.menuConfirm
                   || (hovered >= 0 && ::IsMouseButtonPressed(MOUSE_BUTTON_LEFT));

    if (confirm && m_inputCooldown <= 0.0f) {
        auto& snd = SoundManager::GetInstance();
        if (snd.IsAudioInitialized()) snd.PlaySound("ui_confirm");

        switch (m_selected) {
            case 0: // Play → go to Level Select
                m_inLevelSelect = true;
                m_selected = 0;
                m_inputCooldown = kInputCooldown;
                view.ShowLevelSelect(6, 1);
                break;

            case 1: // Play the most recently saved Map Builder level
                if (std::filesystem::exists("assets/levels/custom_map.lvl")) {
                    m_selectedLevel = -98;
                    m_startGame = true;
                    m_inputCooldown = kInputCooldown;
                } else {
                    if (snd.IsAudioInitialized()) snd.PlaySound("ui_error");
                    view.ShowMainNotice("SAVE A MAP IN MAP BUILDER FIRST");
                    m_inputCooldown = kInputCooldown;
                }
                break;

            case 2: // Map Builder
                m_openMapBuilder = true;
                m_inputCooldown = kInputCooldown;
                break;

            case 3: // Shop
                m_openShop = true;
                m_inputCooldown = kInputCooldown;
                break;

            case 4: // Options
                m_openOptions = true;
                m_inputCooldown = kInputCooldown;
                view.ShowOptions();
                break;

            case 5: // Quit
                m_quit = true;
                break;
        }
    }
}

// =============================================================================
// HandleLevelSelectInput
// =============================================================================
void MenuController::HandleLevelSelectInput(float dt) {
    (void)dt;
    auto& view = View::MenuView::GetInstance();
    auto& save = SaveManager::GetInstance();
    InputCommand cmd = InputController::GetInstance().Poll();

    const int kLevels = 6;
    std::vector<int> bestStars(kLevels, 0);
    for (int i = 0; i < kLevels; ++i) {
        bestStars[i] = std::clamp(save.GetLevelBestStars(i + 1), 0, 3);
    }
    view.SetLevelStars(bestStars);
    int unlockedLevels = 1;
    for (int i = 1; i < kLevels; ++i) {
        if (save.GetLevelHighScore(i) > 0 || save.GetLevelBestStars(i) > 0) {
            unlockedLevels = i + 1;
        }
    }
    if (unlockedLevels > kLevels) unlockedLevels = kLevels;

    // ── Keyboard navigation ───────────────────────────────────────────────
    if (m_inputCooldown <= 0.0f) {
        if (cmd.menuDeltaX != 0) {
            m_selected = std::clamp(m_selected + cmd.menuDeltaX, 0, kLevels - 1);
            m_inputCooldown = kInputCooldown;
        }
    }

    // ── Confirm ───────────────────────────────────────────────────────────
    if (cmd.menuConfirm && m_inputCooldown <= 0.0f) {
        if (m_selected < unlockedLevels) {
            m_selectedLevel = m_selected + 1;  // levels are 1-indexed
            m_startGame     = true;
            m_inputCooldown = kInputCooldown;
        } else {
            auto& snd = SoundManager::GetInstance();
            if (snd.IsAudioInitialized()) snd.PlaySound("ui_error");
        }
    }

    // ── Back / Escape ─────────────────────────────────────────────────────
    if (::IsKeyPressed(KEY_ESCAPE) || cmd.pause) {
        m_inLevelSelect = false;
        m_selected      = 0;
        m_inputCooldown = kInputCooldown;
        view.ShowMainMenu();
        return;
    }

    view.ShowLevelSelect(kLevels, unlockedLevels);
}

// =============================================================================
// HandleShopInput
// =============================================================================
void MenuController::HandleShopInput(float dt) {
    (void)dt;
    auto& view = View::MenuView::GetInstance();
    InputCommand cmd = InputController::GetInstance().Poll();

    // Character prices (index 0 = Knight = free)
    const int prices[]    = { 0, 200, 350, 500 };
    const char* names[]   = { "Knight", "Fighter", "Magic Caster", "Ninja" };
    constexpr int kCount  = 4;

    // ── Navigation ────────────────────────────────────────────────────────
    if (m_inputCooldown <= 0.0f && cmd.menuDelta != 0) {
        m_selected = std::clamp(m_selected + cmd.menuDelta, 0, kCount - 1);
        m_inputCooldown = kInputCooldown;
    }

    // ── Buy confirm ───────────────────────────────────────────────────────
    if (cmd.menuConfirm && m_inputCooldown <= 0.0f) {
        auto& save = SaveManager::GetInstance();
        int idx = m_selected;

        if (prices[idx] == 0 || save.IsCharUnlocked(names[idx])) {
            // Already unlocked — select this character
            m_selectedCharIndex = idx;
            auto& snd = SoundManager::GetInstance();
            if (snd.IsAudioInitialized()) snd.PlaySound("ui_confirm");
        } else if (save.GetCoins() >= prices[idx]) {
            // Purchase
            save.SpendCoins(prices[idx]);
            save.UnlockChar(names[idx]);
            save.Save();
            RefreshHeaderData();
            m_selectedCharIndex = idx;
            auto& snd = SoundManager::GetInstance();
            if (snd.IsAudioInitialized()) snd.PlaySound("ui_confirm");
        } else {
            // Not enough coins
            auto& snd = SoundManager::GetInstance();
            if (snd.IsAudioInitialized()) snd.PlaySound("ui_error");
        }
        m_inputCooldown = kInputCooldown;
    }

    // ── Back / Escape ─────────────────────────────────────────────────────
    if (::IsKeyPressed(KEY_ESCAPE)) {
        m_inShop    = false;
        m_selected  = 0;
        m_inputCooldown = kInputCooldown;
        view.ShowMainMenu();
    }
}

// =============================================================================
// HandlePauseInput
// =============================================================================
void MenuController::HandlePauseInput(float dt) {
    (void)dt;
    auto& view = View::MenuView::GetInstance();
    InputCommand cmd = InputController::GetInstance().Poll();

    const int kItems = 2; // Resume, Quit to Menu

    Vector2 mousePos = ::GetMousePosition();
    int hovered = view.GetHoveredItem(mousePos);
    if (hovered >= 0) m_selected = hovered;

    if (m_inputCooldown <= 0.0f && cmd.menuDelta != 0) {
        m_selected = (m_selected + cmd.menuDelta + kItems) % kItems;
        m_inputCooldown = kInputCooldown;
    }

    bool confirm = cmd.menuConfirm
                   || (hovered >= 0 && ::IsMouseButtonPressed(MOUSE_BUTTON_LEFT));

    if (confirm && m_inputCooldown <= 0.0f) {
        switch (m_selected) {
            case 0: // Resume — signal caller to unpause (startGame acts as "resume")
                m_startGame = true;
                break;
            case 1: // Quit to Menu
                ShowMainMenu();
                break;
        }
        m_inputCooldown = kInputCooldown;
    }

    view.Update(dt, m_selected);
}
