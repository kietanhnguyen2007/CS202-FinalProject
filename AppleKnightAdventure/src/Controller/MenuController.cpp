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
#include <cctype>
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
        snd.LoadManifest("assets/sounds/audio_manifest.json");
        snd.SetMusicVolume(save.GetMusicVolume() / 100.0f);
        snd.SetSFXVolume(save.GetSFXVolume() / 100.0f);
        snd.PlayMusic("bgm_menu");
    }

    // ── Reset flags ───────────────────────────────────────────────────────
    ResetFlags();
    m_selected    = 0;
    m_inShop      = false;
    m_inLevelSelect = false;
    m_inCustomMaps = false;
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
    m_inCustomMaps   = false;
    m_pendingMapDelete.clear();
    m_inputCooldown  = kInputCooldown; // brief lock to avoid accidental input
    RefreshHeaderData();
    View::MenuView::GetInstance().ShowMainMenu();
    SoundManager::GetInstance().PlayMusic("bgm_menu");
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

    // Dispatch to the correct input handler
    auto& view = View::MenuView::GetInstance();
    View::MenuMode mode = view.GetMode();

    switch (mode) {
        case View::MenuMode::Main:        HandleMainMenuInput(dt); break;
        case View::MenuMode::LevelSelect: HandleLevelSelectInput(dt); break;
        case View::MenuMode::Shop:        HandleShopInput(dt); break;
        case View::MenuMode::Pause:       HandlePauseInput(dt); break;
        case View::MenuMode::CustomMaps:  HandleCustomMapsInput(dt); break;
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

            case 1: // Open the custom-map library
                RefreshCustomMapLibrary();
                m_inCustomMaps = true;
                m_pendingMapDelete.clear();
                m_selected = 0;
                view.ShowCustomMaps(m_customMapNames);
                m_inputCooldown = kInputCooldown;
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

void MenuController::RefreshCustomMapLibrary() {
    m_customMapFiles.clear();
    m_customMapNames.clear();

    const std::filesystem::path levelsDir("assets/levels");
    std::error_code ec;
    if (!std::filesystem::exists(levelsDir, ec)) return;

    std::vector<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::directory_iterator(levelsDir, ec)) {
        if (ec || !entry.is_regular_file()) continue;
        const auto path = entry.path();
        if (path.extension() != ".lvl") continue;
        const std::string stem = path.stem().string();
        if (stem == "temp_playtest" || stem == "custom_map") continue;

        bool campaignLevel = stem.size() > 5 && stem.rfind("level", 0) == 0;
        for (size_t i = 5; campaignLevel && i < stem.size(); ++i)
            campaignLevel = std::isdigit(static_cast<unsigned char>(stem[i])) != 0;
        if (!campaignLevel) files.push_back(path);
    }
    std::sort(files.begin(), files.end(), [](const auto& a, const auto& b) {
        return a.stem().string() < b.stem().string();
    });

    // Keep compatibility with maps saved before the named-map library existed.
    if (files.empty()) {
        const std::filesystem::path legacy = levelsDir / "custom_map.lvl";
        if (std::filesystem::exists(legacy, ec)) files.push_back(legacy);
    }

    for (const auto& file : files) {
        m_customMapFiles.push_back(file.string());
        m_customMapNames.push_back(file.stem().string());
    }
}

void MenuController::HandleCustomMapsInput(float dt) {
    (void)dt;
    auto& view = View::MenuView::GetInstance();
    InputCommand cmd = InputController::GetInstance().Poll();
    const Vector2 mouse = ::GetMousePosition();
    const int action = view.GetCustomMapActionHovered(mouse);
    const bool clicked = ::IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    if (!m_pendingMapDelete.empty()) {
        const bool confirmDelete = (clicked && action == 3) || cmd.menuConfirm;
        const bool cancelDelete = (clicked && action == 4) || ::IsKeyPressed(KEY_ESCAPE) || cmd.pause;
        if (confirmDelete) {
            std::error_code ec;
            const std::filesystem::path deletedPath(m_pendingMapDelete);
            const std::filesystem::path alias("assets/levels/custom_map.lvl");
            std::filesystem::remove(deletedPath, ec);
            if (deletedPath != alias) {
                std::error_code aliasEc;
                std::filesystem::remove(alias, aliasEc);
            }
            RefreshCustomMapLibrary();

            if (!m_customMapFiles.empty() &&
                std::filesystem::path(m_customMapFiles.front()) != alias) {
                std::filesystem::copy_file(m_customMapFiles.front(), alias,
                    std::filesystem::copy_options::overwrite_existing, ec);
            } else if (m_customMapFiles.empty()) {
                std::filesystem::remove(alias, ec);
            }
            m_pendingMapDelete.clear();
            m_selected = m_customMapFiles.empty()
                ? 0 : std::clamp(m_selected, 0, (int)m_customMapFiles.size() - 1);
            SoundManager::GetInstance().PlaySound(ec ? "ui_error" : "ui_confirm");
            view.ShowCustomMaps(m_customMapNames);
            m_inputCooldown = kInputCooldown;
            return;
        }
        if (cancelDelete) {
            m_pendingMapDelete.clear();
            view.ShowCustomMaps(m_customMapNames);
            m_inputCooldown = kInputCooldown;
        }
        return;
    }

    const int hovered = view.GetHoveredItem(mouse);
    if (hovered >= 0 && hovered < (int)m_customMapFiles.size()) m_selected = hovered;

    if (!m_customMapFiles.empty() && m_inputCooldown <= 0.0f && cmd.menuDelta != 0) {
        const int count = (int)m_customMapFiles.size();
        m_selected = (m_selected + cmd.menuDelta + count) % count;
        m_inputCooldown = kInputCooldown;
    }

    const bool wantsPlay = !m_customMapFiles.empty() &&
        (cmd.menuConfirm || (clicked && action == 0));
    const bool wantsDelete = !m_customMapFiles.empty() &&
        (::IsKeyPressed(KEY_DELETE) || (clicked && action == 1));
    const bool wantsBack = ::IsKeyPressed(KEY_ESCAPE) || cmd.pause || (clicked && action == 2);

    if (wantsPlay && m_inputCooldown <= 0.0f) {
        std::error_code ec;
        const std::filesystem::path selected(m_customMapFiles[m_selected]);
        const std::filesystem::path alias("assets/levels/custom_map.lvl");
        if (selected != alias) {
            std::filesystem::copy_file(selected, alias,
                std::filesystem::copy_options::overwrite_existing, ec);
        }
        if (ec) {
            SoundManager::GetInstance().PlaySound("ui_error");
        } else {
            m_selectedLevel = -98;
            m_startGame = true;
            SoundManager::GetInstance().PlaySound("start");
        }
        m_inputCooldown = kInputCooldown;
    } else if (wantsDelete && m_inputCooldown <= 0.0f) {
        m_pendingMapDelete = m_customMapFiles[m_selected];
        view.ShowCustomMaps(m_customMapNames, m_customMapNames[m_selected]);
        m_inputCooldown = kInputCooldown;
    } else if (wantsBack) {
        m_inCustomMaps = false;
        m_selected = 1;
        m_inputCooldown = kInputCooldown;
        view.ShowMainMenu();
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
    // Unlock strictly in order: completing level N unlocks level N+1. Stop at
    // the first uncleared level so corrupted/non-sequential save data cannot
    // accidentally expose later levels.
    int unlockedLevels = 1;
    while (unlockedLevels < kLevels) {
        const int prerequisite = unlockedLevels;
        if (save.GetLevelHighScore(prerequisite) <= 0 &&
            save.GetLevelBestStars(prerequisite) <= 0) break;
        ++unlockedLevels;
    }

    const int hovered = view.GetHoveredItem(GetMousePosition());
    if (hovered >= 0 && hovered != m_selected) {
        m_selected = hovered;
    }

    // ── Keyboard navigation ───────────────────────────────────────────────
    if (m_inputCooldown <= 0.0f) {
        if (cmd.menuDeltaX != 0 || cmd.menuDelta != 0) {
            const int delta = cmd.menuDeltaX != 0 ? cmd.menuDeltaX : cmd.menuDelta * 3;
            m_selected = std::clamp(m_selected + delta, 0, kLevels - 1);
            m_inputCooldown = kInputCooldown;
        }
    }

    // ── Confirm ───────────────────────────────────────────────────────────
    const bool clicked = hovered >= 0 && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
    if ((cmd.menuConfirm || clicked) && (clicked || m_inputCooldown <= 0.0f)) {
        if (m_selected < unlockedLevels) {
            m_selectedLevel = m_selected + 1;  // levels are 1-indexed
            m_startGame     = true;
            m_inputCooldown = kInputCooldown;
            SoundManager::GetInstance().PlaySound("start");
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
    const int prices[]    = { 0, 10, 20, 30 };
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

    const int kItems = 3; // Resume, Options, Quit to Menu

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
            case 1: // Options
                m_openOptions = true;
                view.ShowOptions();
                break;
            case 2: // Quit to Menu
                ShowMainMenu();
                break;
        }
        m_inputCooldown = kInputCooldown;
    }

    view.Update(dt, m_selected);
}
