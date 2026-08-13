#pragma once

#include "raylib.h"
#include "View/Animator.h"
#include "View/TextureAtlas.h"
#include <string>
#include <vector>
#include <memory>
#include <array>
#include "View/UIHelpers.h"

namespace View {

// ─────────────────────────────────────────────────────────────────────────────
// Data visible to ShopView for rendering one shop item
// ─────────────────────────────────────────────────────────────────────────────
struct ShopItemData {
    std::string id;           // unique key ("knight", "fighter", etc.)
    std::string displayName;  // "Knight", "Fighter", ...
    std::string idleAtlasPath;// path to idle.json for preview
    int  price       = 0;
    bool isUnlocked  = false;
    bool isEquipped  = false;

    // Stats for detail panel
    int statHP    = 0;
    int statATK   = 0;
    int statSpeed = 0;
    std::string description;
};

// ─────────────────────────────────────────────────────────────────────────────
// Shop tab enum
// ─────────────────────────────────────────────────────────────────────────────
enum class ShopTab { Characters = 0, Pets = 1 };

// ─────────────────────────────────────────────────────────────────────────────
// ShopView — singleton
// Renders the full shop screen: tab bar, left grid, right detail panel, buy btn
// ─────────────────────────────────────────────────────────────────────────────
class ShopView {
public:
    static ShopView& GetInstance();

    bool Init();
    void Shutdown();

    void Update(float dt);
    void Render();

    void SetVisible(bool v) { m_visible = v; }
    bool IsVisible() const { return m_visible; }
    void Show(int currentCoins);

    // Feed items from controller
    void SetCharacterItems(const std::vector<ShopItemData>& items);
    void SetPetItems(const std::vector<ShopItemData>& items);

    // Sync coins (controller updates after purchase)
    void SetCurrentCoins(int coins) { m_currentCoins = coins; }

    // Query results for controller
    bool WantsBack()    const { return m_wantsBack; }
    bool WantsBuy()     const { return m_wantsBuy; }
    int  GetSelectedIndex() const { return m_selectedIdx; }
    ShopTab GetActiveTab()  const { return m_activeTab; }

    void ClearWantsBack() { m_wantsBack = false; }
    void ClearWantsBuy()  { m_wantsBuy  = false; }

    // After a purchase: mark item unlocked so grid updates without data reload
    void MarkSelectedUnlocked();

    // Trigger the "not enough coins" shake animation
    void TriggerBuyShake();
    void ShowNotice(const std::string& message, bool success);

private:
    ShopView() = default;
    ShopView(const ShopView&) = delete;
    ShopView& operator=(const ShopView&) = delete;

    // ── Helpers ───────────────────────────────────────────────────────────
    void RenderBackground();
    void RenderCoinBar();
    void RenderTabBar();
    void RenderGrid();
    void RenderDetailPanel();
    void RenderBuyButton();
    void RenderBackButton();

    // Draw the "lock" overlay procedurally (chain + padlock drawn with shapes)
    void DrawLockOverlay(float x, float y, float w, float h);

    // Animated button scale/glow helpers
    void UpdateBtnAnim(BtnAnim& btn, Rectangle rect, float dt);
    void DrawGlowBorder(Rectangle r, float glowAlpha, Color col);

    // Load idle texture for selected item into m_previewAtlas
    void LoadPreview(const std::string& atlasPath);
    void SyncThumbnailAtlases();

    // ── State ─────────────────────────────────────────────────────────────
    bool  m_visible   = false;
    bool  m_wantsBack = false;
    bool  m_wantsBuy  = false;
    float m_animTime  = 0.0f;   // general timer (panel slide-in, shimmer)
    float m_slideT    = 0.0f;   // 0→1 panel slide-in

    ShopTab m_activeTab   = ShopTab::Characters;
    int     m_selectedIdx = 0;
    int     m_currentCoins = 0;

    // Grid scroll
    float m_scrollY     = 0.0f;
    float m_scrollYDisp = 0.0f; // smooth displayed

    // Item lists
    std::vector<ShopItemData> m_charItems;
    std::vector<ShopItemData> m_petItems;
    std::vector<std::shared_ptr<Animations::TextureAtlas>> m_charThumbnails;
    std::vector<std::shared_ptr<Animations::TextureAtlas>> m_petThumbnails;

    // Preview animation
    std::shared_ptr<Animations::TextureAtlas> m_previewAtlas;
    Animations::Animator m_previewAnim;
    std::string m_loadedPreviewPath;

    // Button animations
    BtnAnim m_buyBtnAnim;
    BtnAnim m_backBtnAnim;
    std::array<BtnAnim, 2> m_tabBtnAnim;   // [0]=Chars [1]=Pets

    // "Not enough coins" shake
    float m_shakeTimer   = 0.0f;
    float m_shakeOffset  = 0.0f;

    // Tab sprite sheets
    Texture2D m_texTabNormal{};
    Texture2D m_texTabActive{};
    bool      m_tabTexLoaded = false;

    // Portrait frame
    Texture2D m_texPortrait{};
    bool      m_portraitLoaded = false;

    // Panel 9-slice
    Texture2D m_texPanel{};
    bool      m_panelLoaded = false;
    Font      m_font{};
    bool      m_fontLoaded = false;
    std::string m_notice;
    float       m_noticeTimer = 0.0f;
    bool        m_noticeSuccess = true;
    float       m_inputCooldown = 0.0f;
};

} // namespace View
