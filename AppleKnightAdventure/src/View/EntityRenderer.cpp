#include "View/EntityRenderer.h"
#include "View/Renderer.h"

namespace View {

EntityRenderer& EntityRenderer::GetInstance() {
    static EntityRenderer instance;
    return instance;
}

void EntityRenderer::Register(const Entity* entity, Texture2D* tex,
                               Rectangle src, Vector2 origin, bool flipX) {
    if (!entity || !tex) return;
    uint32_t id = static_cast<uint32_t>(entity->GetId());

    if (src.width == 0) {
        src = {0, 0, (float)tex->width, (float)tex->height};
    }

    // Lưu ý: Cần thêm trường 'const Entity* entity' vào cấu trúc struct/tuple của m_entities trong file header
    // Ví dụ: struct EntityRenderData { const Entity* entity; Texture2D* texture; Rectangle src; Vector2 origin; bool flipX; };
    m_entities[id] = {entity, tex, src, origin, flipX};
}

void EntityRenderer::Unregister(uint32_t entityId) {
    m_entities.erase(entityId);
}

void EntityRenderer::Clear() {
    m_entities.clear();
}

void EntityRenderer::RenderAll() {
    for (const auto& [id, data] : m_entities) {
        const Entity* entity = data.entity;
        if (!entity || !entity->IsActive() || !data.texture) continue;

        View::Renderer::GetInstance().SubmitSprite(
            data.texture,
            data.src,
            entity->GetPosition(),
            {entity->GetScale(), entity->GetScale()},
            entity->GetRotation(),
            data.origin,
            WHITE,             // Màu tint mặc định
            View::Layer::World, // Hoặc Backgound/Foreground tùy loại Entity
            0.0f,
            data.flipX,
            id);
    }
}

Texture2D* EntityRenderer::GetTexture(uint32_t entityId) const {
    auto it = m_entities.find(entityId);
    if (it == m_entities.end()) return nullptr;
    return it->second.texture;
}

} // namespace View
