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
    loadOne("Potion",    "assets/textures/items/potion.json",      false, "");
    loadOne("Key",       "assets/textures/items/key.json",         false, "");
    loadOne("KeySilver", "assets/textures/items/key_silver.json",  false, "");
    loadOne("BagCoins",  "assets/textures/items/bag_coins.json",   false, "");
    loadOne("Equipment", "assets/textures/items/equipment.json",   false, "");
    loadOne("PotionRed", "assets/textures/items/potion_red.json",  false, "");
}

bool InventoryView::LoadResources(const std::string& atlasJsonPath) {
    (void)atlasJsonPath;
    m_itemIcons.clear();
    LoadItemAtlases();
    m_loaded = true;
    return true;
}

void InventoryView::Shutdown() {
    for (auto& kv : m_itemIcons) {
        kv.second.anim.Stop();
        kv.second.atlas.reset();
    }
    m_itemIcons.clear();
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

void InventoryView::Render() {
    if (!m_open || !m_loaded) return;
    Renderer& r = Renderer::GetInstance();
    int w = r.GetWindowWidth();
    int h = r.GetWindowHeight();

    Vector2 center = { w*0.5f, h*0.5f };
    r.DrawRectangle({center.x - 240, center.y - 180}, {480, 360}, {30, 30, 30, 220}, Layer::UI, 0.0f);

    const int cols = 6;
    const int rows = 4;
    const float slotW = 64.0f;
    const float slotH = 64.0f;
    const float startX = center.x - (cols * slotW) / 2.0f + 8.0f;
    const float startY = center.y - (rows * slotH) / 2.0f + 20.0f;
    const float iconSize = 32.0f;

    int idx = 0;
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            float sx = startX + x * slotW;
            float sy = startY + y * slotH;
            r.DrawRectangle({sx, sy}, {slotW - 8, slotH - 8}, {60, 60, 60, 200}, Layer::UI, 0.0f);

            if (idx < (int)m_items.size()) {
                auto& p = m_items[idx];
                // Draw item icon
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
                            float ix = sx + (slotW - 8 - iconSize) * 0.5f;
                            float iy = sy + 4;
                            r.SubmitSprite(icon.atlas->GetTexture(), src,
                                           {ix, iy},
                                           {iconSize / src.width, iconSize / src.height},
                                           0.0f, {0, 0}, WHITE, Layer::UI, 0.0f, false, 0);
                        }
                    }
                }
                // Draw item count
                char b[16];
                snprintf(b, sizeof(b), "x%d", p.second);
                r.DrawText(b, {sx + 8, sy + slotH - 20}, 12, YELLOW);
            }

            if (idx == m_selection) {
                r.DrawRectangle({sx, sy}, {slotW - 8, slotH - 8}, {255, 255, 255, 48}, Layer::UI, 0.0f);
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