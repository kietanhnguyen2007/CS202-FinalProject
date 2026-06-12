#pragma once

#include "raylib.h"

namespace View {

inline Vector2 ScreenPercent(float px, float py, int screenW, int screenH) {
    return { screenW * px, screenH * py };
}

inline int ScaledFontSize(int baseSize, float scale) {
    return (int)(baseSize * scale);
}

} // namespace View
