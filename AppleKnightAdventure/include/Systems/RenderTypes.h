#pragma once

#include "raylib.h"
#include <cstdint>

namespace Systems {

enum class Layer : uint8_t { Background = 0, World = 1, Foreground = 2, UI = 3 };

// Plain POD render command
struct RenderCommand {
    Texture2D* m_texture = nullptr;
    Rectangle m_src{};
    float m_x = 0.0f;
    float m_y = 0.0f;
    float m_scaleX = 1.0f;
    float m_scaleY = 1.0f;
    float m_rotation = 0.0f;
    float m_originX = 0.0f;
    float m_originY = 0.0f;
    Color m_tint{255,255,255,255};
    bool m_flipX = false;
    uint32_t m_entityId = 0;
    float m_z = 0.0f; // depth for ordering within layer
};

} // namespace Systems
