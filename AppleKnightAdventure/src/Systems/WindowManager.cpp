#include "Systems/WindowManager.h"
#include "View/Renderer.h"
#include <algorithm>

// ---------------------------------------------------------------------------
// Singleton
// ---------------------------------------------------------------------------
WindowManager& WindowManager::GetInstance() {
    static WindowManager instance;
    return instance;
}

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------
void WindowManager::Init(int baseW, int baseH) {
    m_baseW = baseW;
    m_baseH = baseH;

    // Query DPI from the current monitor
    m_dpiScale = GetWindowScaleDPI();

    // Latch current window dimensions
    m_windowW = GetScreenWidth();
    m_windowH = GetScreenHeight();

    // Compute initial scales
    m_scaleX  = static_cast<float>(m_windowW) / static_cast<float>(m_baseW);
    m_scaleY  = static_cast<float>(m_windowH) / static_cast<float>(m_baseH);
    m_uiScale = std::min(m_scaleX, m_scaleY);

    m_resizedThisFrame = false;

    // Notify the Renderer of the current dimensions
    View::Renderer::GetInstance().ResizeWindow(m_windowW, m_windowH);
}

// ---------------------------------------------------------------------------
// Update  (called every frame — zero runtime allocation)
// ---------------------------------------------------------------------------
void WindowManager::Update() {
    m_resizedThisFrame = false;

    if (IsWindowResized()) {
        // Latch new window dimensions
        m_windowW = GetScreenWidth();
        m_windowH = GetScreenHeight();

        // Re-query DPI (may change if window is dragged to another monitor)
        m_dpiScale = GetWindowScaleDPI();

        // Recompute ratio scales
        m_scaleX  = static_cast<float>(m_windowW) / static_cast<float>(m_baseW);
        m_scaleY  = static_cast<float>(m_windowH) / static_cast<float>(m_baseH);
        m_uiScale = std::min(m_scaleX, m_scaleY);

        m_resizedThisFrame = true;

        // Notify Renderer so it can update its internal viewport / render texture
        View::Renderer::GetInstance().ResizeWindow(m_windowW, m_windowH);
    }
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
Vector2 WindowManager::ScalePoint(float baseX, float baseY) const {
    return { baseX * m_scaleX, baseY * m_scaleY };
}

float WindowManager::ScaleValue(float basePixels) const {
    return basePixels * m_uiScale;
}

int WindowManager::ScaleFontSize(int baseSize) const {
    return static_cast<int>(static_cast<float>(baseSize) * m_uiScale);
}
