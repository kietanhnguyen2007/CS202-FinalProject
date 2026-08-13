#ifndef MENUCONTROLLER_H
#define MENUCONTROLLER_H

#include <string>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// MenuController — drives the Main Menu loop.
// Reads SaveManager, feeds MenuStateData to MenuView, handles all input
// (keyboard arrows + mouse hover + mouse click).
// ─────────────────────────────────────────────────────────────────────────────
class MenuController {
public:
    static MenuController& GetInstance();

    // Lifecycle
    bool Init();
    void Shutdown();

    // Called every frame from main loop
    void Update(float dt);

    // ── Transitions out of the menu loop ─────────────────────────────────
    bool ShouldStartGame()       const { return m_startGame; }
    bool ShouldOpenMapBuilder()  const { return m_openMapBuilder; }
    bool ShouldQuit()            const { return m_quit; }
    bool ShouldOpenShop()        const { return m_openShop; }
    bool ShouldOpenOptions()     const { return m_openOptions; }

    // Which level was selected on the Level-Select screen
    int  GetSelectedLevel()      const { return m_selectedLevel; }

    // Which character was selected on the Shop/Role screen
    int  GetSelectedCharIndex()  const { return m_selectedCharIndex; }

    // Reset transition flags (call after consuming them)
    void ResetFlags();

    // Re-enter the main menu (e.g., coming back from game)
    void ShowMainMenu();

    // ── Internal menu state query (used by View helpers) ──────────────────
    int  GetSelected() const { return m_selected; }

private:
    MenuController() = default;
    MenuController(const MenuController&) = delete;
    MenuController& operator=(const MenuController&) = delete;

    // ── Per-mode input handlers ───────────────────────────────────────────
    void HandleMainMenuInput(float dt);
    void HandleLevelSelectInput(float dt);
    void HandleShopInput(float dt);
    void HandlePauseInput(float dt);
    void HandleCustomMapsInput(float dt);
    void RefreshCustomMapLibrary();

    // ── Helper: sync save data to View header every N frames ─────────────
    void RefreshHeaderData();

    // ── State ─────────────────────────────────────────────────────────────
    int  m_selected          = 0;
    bool m_startGame         = false;
    bool m_openMapBuilder    = false;
    bool m_quit              = false;
    bool m_openShop          = false;
    bool m_openOptions       = false;
    int  m_selectedLevel     = 1;
    int  m_selectedCharIndex = 0;   // 0=Knight

    // Sub-screen flags
    bool m_inShop        = false;
    bool m_inLevelSelect = false;
    bool m_inCustomMaps  = false;

    std::vector<std::string> m_customMapFiles;
    std::vector<std::string> m_customMapNames;
    std::string m_pendingMapDelete;

    // Cooldown to prevent double-presses
    float m_inputCooldown = 0.0f;
    static constexpr float kInputCooldown = 0.18f;

    // Header refresh timer
    float m_headerRefreshTimer = 0.0f;
};

#endif // MENUCONTROLLER_H
