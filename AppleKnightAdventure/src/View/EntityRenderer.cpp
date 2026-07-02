#include "View/EntityRenderer.h"
#include "View/Renderer.h"
#include "View/AssetManager.h"
#include <functional>
#include <cassert>
#include <iostream>

namespace View {

EntityRenderer& EntityRenderer::GetInstance() {
    static EntityRenderer instance;
    return instance;
}

void EntityRenderer::Register(const Entity* entity, Texture2D* tex,
                               Rectangle src, Vector2 origin, bool flipX) {
    if (!entity || !tex) return;
    int rawId = entity->GetId();
    assert(rawId >= 0);
    uint32_t id = static_cast<uint32_t>(rawId);

    if (src.width == 0) {
        src = {0, 0, (float)tex->width, (float)tex->height};
    }

    auto& data = m_entities[id];
    data.entity = entity;
    data.texture = tex;
    data.src = src;
    data.origin = origin;
    data.flipX = flipX;
    data.visible = true; // Mặc định hiển thị, tránh lỗi khởi tạo thiếu tự động chuyển thành false
}

void EntityRenderer::Unregister(uint32_t entityId) {
    auto cbIt = m_removeCallbacks.find(entityId);
    if (cbIt != m_removeCallbacks.end()) {
        auto cb = std::move(cbIt->second);
        m_removeCallbacks.erase(cbIt);
        cb(entityId);
    }
    m_entities.erase(entityId);
    m_animatedEntities.erase(entityId);
}

void EntityRenderer::Clear() {
    m_entities.clear();
    m_animatedEntities.clear();
    m_removeCallbacks.clear();
}

bool EntityRenderer::IsRegistered(uint32_t entityId) const {
    return m_entities.find(entityId) != m_entities.end()
        || m_animatedEntities.find(entityId) != m_animatedEntities.end();
}

void EntityRenderer::SetOnEntityRemovedCallback(uint32_t entityId, std::function<void(uint32_t)> cb) {
    if (cb) m_removeCallbacks[entityId] = std::move(cb);
}

void EntityRenderer::ClearOnEntityRemovedCallback(uint32_t entityId) {
    m_removeCallbacks.erase(entityId);
}

bool EntityRenderer::RegisterAnimated(const Entity* entity, const std::string& atlasPath,
                                       const std::string& startClip,
                                       Vector2 origin, bool flipX) {
    if (!entity) return false;
    uint32_t id = static_cast<uint32_t>(entity->GetId());

    auto atlas = AssetManager::GetInstance().GetAtlas(atlasPath);
    if (!atlas) {
        std::cerr << "[EntityRenderer] RegisterAnimated failed: " << atlasPath << "\n";
        return false;
    }

    auto& ad = m_animatedEntities[id];
    ad.entity = entity;
    ad.atlas = std::move(atlas);
    ad.origin = origin;
    ad.flipX = flipX;
    ad.visible = true;
    ad.animator.SetTexture(ad.atlas->GetTexture());
    for (const auto& clipName : ad.atlas->GetClipNames()) {
        auto clip = ad.atlas->GetClip(clipName);
        if (clip) ad.animator.AddClip(clip);
    }
    if (!startClip.empty() && ad.animator.HasClip(startClip)) {
        ad.animator.Play(startClip);
    }
    return true;
}

void EntityRenderer::Update(float dt) {
    for (auto& [id, ad] : m_animatedEntities) {
        if (!ad.entity || !ad.entity->IsActive()) continue;
        ad.animator.Update(dt);
    }
}

void EntityRenderer::RenderAll() {
    // Render animated entities
    for (const auto& [id, ad] : m_animatedEntities) {
        const Entity* entity = ad.entity;
        if (!ad.visible) continue;
        if (!entity || !entity->IsActive()) continue;
        if (!ad.animator.HasTexture()) continue;

        Rectangle src = ad.animator.GetCurrentSrcRect();
        View::Renderer::GetInstance().SubmitSprite(
            ad.animator.GetTexture(),
            src,
            entity->GetPosition(),
            {entity->GetScale(), entity->GetScale()},
            entity->GetRotation(),
            ad.origin,
            WHITE,
            View::Layer::World,
            0.0f,
            ad.flipX,
            id);
    }

    // Render static entities
    for (const auto& [id, data] : m_entities) {
        // data may contain entity pointer and texture/src information
        const Entity* entity = data.entity;
        if (!data.visible) continue;
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
    if (it != m_entities.end()) return it->second.texture;
    auto ait = m_animatedEntities.find(entityId);
    if (ait != m_animatedEntities.end()) return ait->second.animator.GetTexture();
    return nullptr;
}

void EntityRenderer::UpdateSpriteRect(uint32_t entityId, const Rectangle& src) {
    auto it = m_entities.find(entityId);
    if (it == m_entities.end()) return;
    it->second.src = src;
}

void EntityRenderer::SetEntityVisible(uint32_t entityId, bool visible) {
    auto it = m_entities.find(entityId);
    if (it != m_entities.end()) { it->second.visible = visible; return; }
    auto ait = m_animatedEntities.find(entityId);
    if (ait != m_animatedEntities.end()) { ait->second.visible = visible; }
}

void EntityRenderer::SetClip(uint32_t entityId, const std::string& clipName) {
    auto ait = m_animatedEntities.find(entityId);
    if (ait == m_animatedEntities.end()) return;
    ait->second.animator.Play(clipName);
}

} // namespace View
