#pragma once

#include <string>
#include <vector>
#include "raylib.h"

namespace View {

struct FloatingTextItem {
    Vector2 worldPos;
    std::string text;
    Color color;
    float lifetime;
    float timer;
    float riseSpeed;
};

class FloatingTextManager {
public:
    static FloatingTextManager& GetInstance();

    void Emit(const Vector2& worldPos, const std::string& text, Color color = WHITE, float lifetime = 1.0f);
    void Update(float dt);
    void Render(const Camera2D& camera);

private:
    FloatingTextManager() = default;
    ~FloatingTextManager() = default;

    std::vector<FloatingTextItem> m_items;
};

} // namespace View
