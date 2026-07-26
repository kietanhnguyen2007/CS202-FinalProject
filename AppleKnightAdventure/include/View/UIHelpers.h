#pragma once

#include "raylib.h"
#include "Systems/WindowManager.h"

namespace View {

// Original overload: explicit screen size args (backward compatible)
inline Vector2 ScreenPercent(float px, float py, int screenW, int screenH) {
    return { screenW * px, screenH * py };
}

// New overload: uses WindowManager (current window size)
inline Vector2 ScreenPercent(float px, float py) {
    const auto& wm = WindowManager::GetInstance();
    return { wm.GetWidth() * px, wm.GetHeight() * py };
}

// Scale a pixel value by the uniform UI scale
inline float ScaleValue(float basePixels) {
    return basePixels * WindowManager::GetInstance().GetUIScale();
}

// Scale font size by the uniform UI scale
inline int ScaleFontSize(int baseSize) {
    return WindowManager::GetInstance().ScaleFontSize(baseSize);
}

// Original: kept for backward compatibility
inline int ScaledFontSize(int baseSize, float scale) {
    return (int)(baseSize * scale);
}

} // namespace View
