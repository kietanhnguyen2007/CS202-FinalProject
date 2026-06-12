#pragma once

#include "raylib.h"
#include <vector>

namespace View {

// Render order (bottom → top):
//   HUD → SkillBar → InteractPrompt → Menu → Inventory → Result
enum class UILayer {
    HUD,
    SkillBar,
    InteractPrompt,
    Menu,
    Inventory,
    Result
};

// Stack-based UI coordinator.
// Controller pushes modal layers on top; UIStateManager auto-dims lower layers.
// HUD / SkillBar / InteractPrompt are non-modal — they render when visible.
// Inventory / Menu / Result are modal — they only render when manually opened.
class UIStateManager {
public:
    static UIStateManager& GetInstance();

    void Init();
    void Shutdown();

    // Stack operations for modal layers (Inventory, Menu, Result)
    void Push(UILayer layer);
    void Pop();
    void Clear();

    UILayer GetTopLayer() const;
    bool IsOverlayActive() const { return !m_stack.empty(); }

    // Render all active layers in correct order with auto-dim.
    // Must be called after GameView::Render each frame.
    void RenderAll();

private:
    UIStateManager() = default;
    ~UIStateManager() = default;

    static bool IsModal(UILayer layer);
    bool IsLayerVisible(UILayer layer) const;
    void RenderLayer(UILayer layer) const;
    void DrawDimOverlay();

    std::vector<UILayer> m_stack;
};

} // namespace View