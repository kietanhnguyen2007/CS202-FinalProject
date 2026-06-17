#include "View/InventoryView.h"
#include "View/Renderer.h"
#include "View/UIHelpers.h"
#include "Model/Inventory.h"
#include "Model/Item.h"
#include "Systems/SoundManager.h"
#include <cstdio>

using namespace View;

InventoryView& InventoryView::GetInstance() {
    static InventoryView inst;
    return inst;
}

bool InventoryView::Init() {
    m_loaded = true;
    return true;
}

void InventoryView::LoadItemAtlases() {
    auto loadOne = [&](const std::string& itemName, const std::string& jsonPath,
                       bool animated, const std::string& clipName) {
        auto atlas = Animations::TextureAtlas::LoadFromJSON(jsonPath);
        if (!atlas) return;
        atlas->LoadTexture();
        ItemIconInfo info;
        info.atlas = std::move(atlas);
        info.animated = animated;
        if (animated) {
            info.anim.SetTexture(info.atlas->GetTexture());
            if (info.atlas->HasClip(clipName)) {
                info.anim.AddClip(info.atlas->GetClip(clipName));
                info.anim.Play(clipName);
            }
        }
        m_itemIcons.emplace(itemName, std::move(info));
    };

    loadOne("Apple",     "assets/textures/items/apple.json",       false, "");
    loadOne("Coin",      "assets/textures/items/coin.json",        true,  "spin");
    loadOne("Key",       "assets/textures/items/key.json",         false, "");
    loadOne("BagCoins",  "assets/textures/items/bag_coins.json",   false, "");
    loadOne("Equipment", "assets/textures/items/equipment.json",   false, "");
    loadOne("PotionRed", "assets/textures/items/potion_red.json",  false, "");
}

bool InventoryView::LoadResources(const std::string& atlasJsonPath) {
    (void)atlasJsonPath;
    m_itemIcons.clear();
    LoadItemAtlases();

    // Load Dark Dwellers UI textures
    m_texPanelBg  = ::LoadTexture("assets/ui/darkDwellers/20251029darkDwellers9SlicesC.png");
    m_texSlot     = ::LoadTexture("assets/ui/darkDwellers/20251124emptyFrameA1-Sheet.png");
    m_texCloseBtn = ::LoadTexture("assets/ui/darkDwellers/20251125closeButton1-Sheet.png");

    // Calculate per-frame widths from horizontal sprite sheets
    if (m_texSlot.id != 0) {
        m_slotFrameW = m_texSlot.width / 5;   // 5 frames in the sheet
    }
    if (m_texCloseBtn.id != 0) {
        m_closeBtnFrameW = m_texCloseBtn.width / 4; // 4 frames in the sheet
    }

    m_loaded = true;
    return true;
}

void InventoryView::Shutdown() {
    for (auto& kv : m_itemIcons) {
        kv.second.anim.Stop();
        kv.second.atlas.reset();
    }
    m_itemIcons.clear();

    // Unload Dark Dwellers textures
    if (m_texPanelBg.id != 0)  ::UnloadTexture(m_texPanelBg);
    if (m_texSlot.id != 0)     ::UnloadTexture(m_texSlot);
    if (m_texCloseBtn.id != 0) ::UnloadTexture(m_texCloseBtn);
    m_texPanelBg  = {};
    m_texSlot     = {};
    m_texCloseBtn = {};
    m_slotFrameW     = 0;
    m_closeBtnFrameW = 0;

    m_loaded = false;
    DetachObservable();
}

void InventoryView::Open() {
    m_open = true;
    SoundManager::GetInstance().PlaySound("ui_inventory_open");
}

void InventoryView::Close() {
    m_open = false;
    SoundManager::GetInstance().PlaySound("ui_inventory_close");
}
bool InventoryView::IsOpen() const { return m_open; }

void InventoryView::Update(float dt) {
    for (auto& kv : m_itemIcons) {
        if (kv.second.animated) {
            kv.second.anim.Update(dt);
        }
    }
}

// ---------------------------------------------------------------------------
// DrawSlot  — draws one inventory slot using the Dark Dwellers sprite sheet
// ---------------------------------------------------------------------------
void InventoryView::DrawSlot(float x, float y, float size, bool highlighted) {
    if (m_texSlot.id == 0 || m_slotFrameW == 0) return;
    Renderer& r = Renderer::GetInstance();

    int frame = highlighted ? 1 : 0;
    float frameW = (float)m_slotFrameW;
    float frameH = (float)m_texSlot.height;

    Rectangle src = {
        frame * frameW,
        0.0f,
        frameW,
        frameH
    };

    float scaleX = size / frameW;
    float scaleY = size / frameH;

    r.SubmitSprite(&m_texSlot, src,
                   {x, y},
                   {scaleX, scaleY},
                   0.0f, {0, 0}, WHITE,
                   Layer::UI, 0.1f, false, 0);
}

// ---------------------------------------------------------------------------
// Render
// ---------------------------------------------------------------------------
void InventoryView::Render() {
    if (!m_open || !m_loaded) return;
    Renderer& r = Renderer::GetInstance();
    int w = r.GetWindowWidth();
    int h = r.GetWindowHeight();

    // ---- Panel dimensions (37.5% × 50% of screen, centered) ----
    float panelW = w * 0.375f;
    float panelH = h * 0.50f;
    float panelX = (w - panelW) * 0.5f;
    float panelY = (h - panelH) * 0.5f;

    // Draw panel background texture stretched to panel size
    if (m_texPanelBg.id != 0) {
        Rectangle panelSrc = { 0.0f, 0.0f, (float)m_texPanelBg.width, (float)m_texPanelBg.height };
        float bgScaleX = panelW / (float)m_texPanelBg.width;
        float bgScaleY = panelH / (float)m_texPanelBg.height;
        r.SubmitSprite(&m_texPanelBg, panelSrc,
                       {panelX, panelY},
                       {bgScaleX, bgScaleY},
                       0.0f, {0, 0}, WHITE,
                       Layer::UI, 0.0f, false, 0);
    } else {
        // Fallback: solid rectangle if texture failed to load
        r.DrawRectangle({panelX, panelY}, {panelW, panelH},
                        {30, 30, 30, 220}, Layer::UI, 0.0f);
    }

    // ---- Close button (top-right corner of panel) ----
    if (m_texCloseBtn.id != 0 && m_closeBtnFrameW > 0) {
        float closeBtnSize = panelW * 0.06f;
        float closeBtnX = panelX + panelW - closeBtnSize - panelW * 0.03f;
        float closeBtnY = panelY + panelH * 0.03f;

        float cbFrameW = (float)m_closeBtnFrameW;
        float cbFrameH = (float)m_texCloseBtn.height;
        // Frame 0 = normal state
        Rectangle cbSrc = { 0.0f, 0.0f, cbFrameW, cbFrameH };
        float cbScaleX = closeBtnSize / cbFrameW;
        float cbScaleY = closeBtnSize / cbFrameH;

        r.SubmitSprite(&m_texCloseBtn, cbSrc,
                       {closeBtnX, closeBtnY},
                       {cbScaleX, cbScaleY},
                       0.0f, {0, 0}, WHITE,
                       Layer::UI, 0.2f, false, 0);
    }

    // ---- Title "INVENTORY" ----
    float titleFontSize = panelH * 0.07f;
    float titleX = panelX + panelW * 0.05f;
    float titleY = panelY + panelH * 0.04f;
    r.DrawText("INVENTORY", {titleX, titleY}, (int)titleFontSize, WHITE);

    // ---- Grid: 6 cols × 4 rows ----
    const int cols = 6;
    const int rows = 4;

    // Slot area: inset from panel edges
    float gridPadX  = panelW * 0.06f;
    float gridTopY  = panelY + panelH * 0.16f;   // below title
    float gridBotY  = panelY + panelH * 0.95f;   // near bottom
    float gridLeftX = panelX + gridPadX;
    float gridW     = panelW - gridPadX * 2.0f;
    float gridH     = gridBotY - gridTopY;

    float slotSize  = gridW / (float)cols;        // width drives size (square slots)
    if (slotSize * rows > gridH) {
        slotSize = gridH / (float)rows;           // clamp if vertical space is tight
    }

    // Center the grid horizontally within panel
    float totalGridW = slotSize * cols;
    float startX = gridLeftX + (gridW - totalGridW) * 0.5f;

    // Center the grid vertically within available space
    float totalGridH = slotSize * rows;
    float startY = gridTopY + (gridH - totalGridH) * 0.5f;

    float iconPadding = slotSize * 0.15f;   // padding inside slot for the item icon
    float iconSize    = slotSize - iconPadding * 2.0f;

    int idx = 0;
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            float sx = startX + col * slotSize;
            float sy = startY + row * slotSize;

            bool highlighted = (idx == m_selection);
            DrawSlot(sx, sy, slotSize, highlighted);

            // Draw item icon if this slot has an item
            if (idx < (int)m_items.size()) {
                auto& p = m_items[idx];

                auto iconIt = m_itemIcons.find(p.first);
                if (iconIt != m_itemIcons.end()) {
                    const auto& icon = iconIt->second;
                    if (icon.atlas && icon.atlas->GetTexture() && icon.atlas->GetTexture()->id != 0) {
                        Rectangle src{};
                        if (icon.animated && icon.anim.IsPlaying()) {
                            src = icon.anim.GetCurrentSrcRect();
                        } else if (icon.atlas->HasFrame("default")) {
                            src = icon.atlas->GetFrameRect("default");
                        }
                        if (src.width > 0 && src.height > 0) {
                            float ix = sx + iconPadding;
                            float iy = sy + iconPadding * 0.6f;  // slightly above center
                            r.SubmitSprite(icon.atlas->GetTexture(), src,
                                           {ix, iy},
                                           {iconSize / src.width, iconSize / src.height},
                                           0.0f, {0, 0}, WHITE,
                                           Layer::UI, 0.3f, false, 0);
                        }
                    }
                }

                // Item count text "xN" in bottom-left of slot
                char b[16];
                snprintf(b, sizeof(b), "x%d", p.second);
                float countFontSize = slotSize * 0.22f;
                float countX = sx + slotSize * 0.08f;
                float countY = sy + slotSize * 0.72f;
                r.DrawText(b, {countX, countY}, (int)countFontSize, YELLOW);
            }

            ++idx;
        }
    }
}

void InventoryView::SetInventorySnapshot(const Inventory& snapshot) {
    m_items.clear();
    int count = snapshot.GetItemCount();
    for (int i = 0; i < count; ++i) {
        Item* it = snapshot.GetItem(i);
        if (!it) continue;
        m_items.push_back({it->GetItemName(), it->GetAmount()});
    }
}

void InventoryView::SetSelectionIndex(int index) { m_selection = index; }

void InventoryView::RegisterInventoryChangedCallback(std::function<void()> cb) { m_onInventoryChanged = cb; }
void InventoryView::UnregisterInventoryChangedCallback() { m_onInventoryChanged = nullptr; }

void InventoryView::AttachObservable(ObservableList<const Item*>* observable) {
    m_attachedObservable = observable;
    if (!observable) return;
    observable->OnItemAddedCallback = [this](const Item* const&) {
        if (m_onInventoryChanged) m_onInventoryChanged();
    };
    observable->OnItemRemovedCallback = [this](const Item* const&) {
        if (m_onInventoryChanged) m_onInventoryChanged();
    };
    observable->OnClearedCallback = [this]() {
        if (m_onInventoryChanged) m_onInventoryChanged();
    };
}

void InventoryView::DetachObservable() {
    if (m_attachedObservable) {
        m_attachedObservable->OnItemAddedCallback = nullptr;
        m_attachedObservable->OnItemRemovedCallback = nullptr;
        m_attachedObservable->OnClearedCallback = nullptr;
        m_attachedObservable = nullptr;
    }
}

void InventoryView::RegisterOnRequestUseItem(std::function<void(int)> cb) { m_onRequestUse = std::move(cb); }