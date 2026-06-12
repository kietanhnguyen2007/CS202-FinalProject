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

void InventoryView::Init() {
    m_loaded = true;
}

bool InventoryView::LoadResources(const std::string& atlasJsonPath) {
    (void)atlasJsonPath;
    m_loaded = true;
    return true;
}

void InventoryView::Shutdown() {
    m_loaded = false;
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
    (void)dt;
}

void InventoryView::Render() {
    if (!m_open || !m_loaded) return;
    Renderer& r = Renderer::GetInstance();
    int w = r.GetWindowWidth();
    int h = r.GetWindowHeight();

    // Draw a centered panel
    Vector2 center = { w*0.5f, h*0.5f };
    r.DrawRectangle({center.x-240, center.y-180}, {480, 360}, {30,30,30,220}, Layer::UI, 0.0f);

    // Grid layout
    const int cols = 6;
    const int rows = 4;
    const float slotW = 64.0f;
    const float slotH = 64.0f;
    const float startX = center.x - (cols*slotW)/2.0f + 8.0f;
    const float startY = center.y - (rows*slotH)/2.0f + 20.0f;

    int idx = 0;
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            float sx = startX + x * slotW;
            float sy = startY + y * slotH;
            r.DrawRectangle({sx, sy}, {slotW-8, slotH-8}, {60,60,60,200}, Layer::UI, 0.0f);

            if (idx < (int)m_items.size()) {
                auto &p = m_items[idx];
                // draw item name and count
                r.DrawText(p.first.c_str(), {sx+8, sy+8}, 12, WHITE);
                char b[16]; snprintf(b, sizeof(b), "x%d", p.second);
                r.DrawText(b, {sx+8, sy+32}, 12, YELLOW);
            }

            if (idx == m_selection) {
                // highlight
                r.DrawRectangle({sx, sy}, {slotW-8, slotH-8}, {255,255,255,48}, Layer::UI, 0.0f);
            }
            ++idx;
        }
    }
}

void InventoryView::SetInventorySnapshot(const Inventory& snapshot) {
    // clear and fill from Inventory
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
    // Subscribe to observable callbacks to trigger re-snapshot
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

void InventoryView::RegisterOnRequestUseItem(std::function<void(int)> cb) { m_onRequestUse = cb; }
