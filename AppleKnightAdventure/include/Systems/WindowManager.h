#pragma once

#include "raylib.h"

// Forward-declare Renderer so the header stays free of heavy View includes
namespace View { class Renderer; }

class WindowManager {
public:
    static WindowManager& GetInstance();

    // Sets base resolution, queries initial DPI, syncs Renderer
    void Init(int baseW = 1280, int baseH = 720);

    // Called every frame — detects resize, recomputes scales, notifies Renderer
    void Update();

    // Scale accessors
    float   GetScaleX()            const { return m_scaleX; }
    float   GetScaleY()            const { return m_scaleY; }
    float   GetUIScale()           const { return m_uiScale; }
    float   GetDPIX()              const { return m_dpiScale.x; }
    float   GetDPIY()              const { return m_dpiScale.y; }

    // Window size accessors
    int     GetWidth()             const { return m_windowW; }
    int     GetHeight()            const { return m_windowH; }

    // Frame-level resize flag
    bool    WasResizedThisFrame()  const { return m_resizedThisFrame; }

    // Convenience helpers — scale base-resolution coordinates to current window
    Vector2 ScalePoint(float baseX, float baseY) const;
    float   ScaleValue(float basePixels)          const;
    int     ScaleFontSize(int baseSize)           const;

private:
    WindowManager() = default;

    // Base (design) resolution
    int m_baseW = 1280;
    int m_baseH = 720;

    // Current window size
    int m_windowW = 1280;
    int m_windowH = 720;

    // DPI scale reported by the OS/monitor
    Vector2 m_dpiScale = { 1.0f, 1.0f };

    // Ratio of current window to base resolution
    float m_scaleX = 1.0f;
    float m_scaleY = 1.0f;

    // Uniform scale = min(scaleX, scaleY) — used for UI elements
    float m_uiScale = 1.0f;

    // True only during the frame a resize was detected
    bool m_resizedThisFrame = false;
};
