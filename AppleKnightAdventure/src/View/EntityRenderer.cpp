#include "View/EntityRenderer.h"

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

    m_entities[id] = {tex, src, origin, flipX};
}

void EntityRenderer::Unregister(uint32_t entityId) {
    m_entities.erase(entityId);
}

void EntityRenderer::Clear() {
    m_entities.clear();
}

void EntityRenderer::RenderAll() {
    for (const auto& [id, data] : m_entities) {
        // Entity lookup is optional; position/scale/rotation are stored per-register
        // For a full implementation, we'd need a map of entity pointers too.
        // Currently this is a stub for future BE integration.
    }
}

Texture2D* EntityRenderer::GetTexture(uint32_t entityId) const {
    auto it = m_entities.find(entityId);
    if (it == m_entities.end()) return nullptr;
    return it->second.texture;
}

} // namespace View
