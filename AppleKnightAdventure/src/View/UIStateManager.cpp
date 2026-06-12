#include "View/UIStateManager.h"
#include "View/HUDView.h"
#include "View/SkillBarView.h"
#include "View/InteractPrompt.h"
#include "View/MenuView.h"
#include "View/InventoryView.h"
#include "View/ResultView.h"
#include "View/Renderer.h"

namespace View {

// ── Singleton ────────────────────────────────────────────────────

UIStateManager& UIStateManager::GetInstance() {
    static UIStateManager inst;
    return inst;
}

void UIStateManager::Init() {}

void UIStateManager::Shutdown() {
    m_stack.clear();
}

// ── Stack operations ──────────────────────────────────────────────

static const UILayer kRenderOrder[] = {
    UILayer::HUD,
    UILayer::SkillBar,
    UILayer::InteractPrompt,
    UILayer::Menu,
    UILayer::Inventory,
    UILayer::Result,
};

bool UIStateManager::IsModal(UILayer layer) {
    return layer == UILayer::Menu
        || layer == UILayer::Inventory
        || layer == UILayer::Result;
}

bool UIStateManager::IsLayerVisible(UILayer layer) const {
    switch (layer) {
        case UILayer::HUD:           return true; // always visible (dimmed when under modal)
        case UILayer::SkillBar:      return SkillBarView::GetInstance().IsOpen();
        case UILayer::InteractPrompt: return InteractPrompt::GetInstance().IsVisible();
        case UILayer::Menu:          return MenuView::GetInstance().GetMode() != MenuMode::Main
                                            || true; // menu visible at main menu too
        case UILayer::Inventory:     return InventoryView::GetInstance().IsOpen();
        case UILayer::Result:        return ResultView::GetInstance().IsVisible();
    }
    return false;
}

void UIStateManager::RenderLayer(UILayer layer) const {
    switch (layer) {
        case UILayer::HUD:           HUDView::GetInstance().Render();           break;
        case UILayer::SkillBar:      SkillBarView::GetInstance().Render();      break;
        case UILayer::InteractPrompt: InteractPrompt::GetInstance().Render();   break;
        case UILayer::Menu:          MenuView::GetInstance().Render();          break;
        case UILayer::Inventory:     InventoryView::GetInstance().Render();     break;
        case UILayer::Result:        ResultView::GetInstance().Render();        break;
    }
}

void UIStateManager::Push(UILayer layer) {
    m_stack.push_back(layer);
}

void UIStateManager::Pop() {
    if (!m_stack.empty()) m_stack.pop_back();
}

void UIStateManager::Clear() {
    m_stack.clear();
}

UILayer UIStateManager::GetTopLayer() const {
    if (m_stack.empty()) return UILayer::HUD;
    return m_stack.back();
}

// ── Render ─────────────────────────────────────────────────────────

void UIStateManager::DrawDimOverlay() {
    Renderer& r = Renderer::GetInstance();
    int w = r.GetWindowWidth();
    int h = r.GetWindowHeight();
    r.DrawRectangle({0, 0}, {(float)w, (float)h}, {0, 0, 0, 100}, Layer::UI, 0.0f);
}

void UIStateManager::RenderAll() {
    // 1. Render non-modal layers that are always in the background
    for (auto layer : kRenderOrder) {
        if (IsModal(layer)) continue;
        if (!IsLayerVisible(layer)) continue;
        RenderLayer(layer);
    }

    // 2. Render stacked modal layers in order with dimming
    for (size_t i = 0; i < m_stack.size(); ++i) {
        UILayer layer = m_stack[i];
        if (!IsLayerVisible(layer)) continue;

        // Dim the layers underneath before rendering the top layer
        if (i == m_stack.size() - 1 && m_stack.size() > 1) {
            // This is the top layer — draw dim overlay under it
        }

        RenderLayer(layer);

        // Draw dim overlay on top of this layer if it's not the top
        if (i != m_stack.size() - 1) {
            DrawDimOverlay();
        }
    }
}

} // namespace View