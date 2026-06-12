#include "View/FloatingText.h"
#include "View/Renderer.h"
#include <algorithm>

namespace View {

FloatingTextManager& FloatingTextManager::GetInstance() {
    static FloatingTextManager inst;
    return inst;
}

void FloatingTextManager::Emit(const Vector2& worldPos, const std::string& text, Color color, float lifetime) {
    FloatingTextItem it;
    it.worldPos = worldPos;
    it.text = text;
    it.color = color;
    it.lifetime = lifetime;
    it.timer = 0.0f;
    it.riseSpeed = 30.0f;
    m_items.push_back(std::move(it));
}

void FloatingTextManager::Update(float dt) {
    for (auto& it : m_items) {
        it.timer += dt;
        it.worldPos.y -= it.riseSpeed * dt;
    }
    m_items.erase(std::remove_if(m_items.begin(), m_items.end(), [](const FloatingTextItem& it){
        return it.timer >= it.lifetime;
    }), m_items.end());
}

void FloatingTextManager::Render(const Camera2D& camera) {
    // render world->screen
    for (const auto& it : m_items) {
        Vector2 screenPos = GetWorldToScreen2D(it.worldPos, camera);
        // draw with outline: black shadow then white text
        View::Renderer::GetInstance().DrawText(it.text.c_str(), {screenPos.x+1, screenPos.y+1}, 24, BLACK);
        View::Renderer::GetInstance().DrawText(it.text.c_str(), {screenPos.x, screenPos.y}, 24, it.color);
    }
}

} // namespace View
