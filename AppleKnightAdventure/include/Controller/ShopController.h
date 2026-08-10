#pragma once
#include <string>

// ─────────────────────────────────────────────────────────────────────────────
// ShopController — drives the Shop screen.
// Owns ShopView, reads/writes SaveManager. Called from main.cpp when
// MenuController signals ShouldOpenShop().
// ─────────────────────────────────────────────────────────────────────────────
class ShopController {
public:
    static ShopController& GetInstance();

    bool Init();          // load UI resources, build item lists from SaveManager
    void Shutdown();

    // Call each frame while shop is active
    void Update(float dt);

    // Signals for main.cpp
    bool ShouldReturnToMenu() const { return m_returnToMenu; }

    // Re-open shop (re-syncs coins from SaveManager)
    void Open();

private:
    ShopController() = default;
    ShopController(const ShopController&) = delete;
    ShopController& operator=(const ShopController&) = delete;

    void BuildItemLists();
    void HandleBuyAttempt();

    bool m_initialized   = false;
    bool m_returnToMenu  = false;
};
