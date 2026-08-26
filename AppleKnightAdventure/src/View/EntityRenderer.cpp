#include "View/EntityRenderer.h"
#include "View/Renderer.h"
#include "View/AssetManager.h"
#include "Model/Projectile.h"
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
    } else if (!ad.atlas->GetClipNames().empty()) {
        // Fallback: play the first clip available in the JSON (e.g. "projectile_attack2")
        ad.animator.Play(ad.atlas->GetClipNames().front());
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

        if (entity->GetType() == EntityType::Projectile) {
            auto* proj = static_cast<const Projectile*>(entity);
            if (proj->GetSubType() == 3) {
                // This atlas stores nine spatial beam pieces (the last one is
                // the tip), not nine full-beam animation frames. Compose them
                // with 50% overlap so transparent frame padding cannot create
                // gaps, and derive all dimensions from the projectile hitbox.
                auto beamClip = ad.atlas->GetClip("energy_beam");
                if (beamClip && !beamClip->frames.empty()) {
                    const Vector2 size = entity->GetSize();
                    const std::size_t segmentCount = beamClip->frames.size();
                    const float step = size.x / static_cast<float>(segmentCount + 1);
                    const float segmentWidth = step * 2.0f;
                    const bool faceLeft = proj->GetDirection() == Direction::Left;
                    const Vector2 basePos = entity->GetPosition();

                    // The projectile collision is active along the full beam,
                    // so draw every segment immediately after the cast ends.
                    for (std::size_t i = 0; i < segmentCount; ++i) {
                        const Rectangle segmentSrc = beamClip->frames[i].src;
                        if (segmentSrc.width <= 0.0f || segmentSrc.height <= 0.0f) continue;

                        Vector2 segmentPos = basePos;
                        if (faceLeft) {
                            segmentPos.x += size.x - (static_cast<float>(i) * step + segmentWidth);
                        } else {
                            segmentPos.x += static_cast<float>(i) * step;
                        }
                        const Vector2 segmentScale = {
                            segmentWidth / segmentSrc.width,
                            size.y / segmentSrc.height
                        };
                        View::Renderer::GetInstance().SubmitSprite(
                            ad.animator.GetTexture(), segmentSrc, segmentPos, segmentScale,
                            entity->GetRotation(), {0.0f, 0.0f}, WHITE,
                            View::Layer::World, entity->GetZIndex() + 0.20f,
                            faceLeft, id);
                    }
                } else if (src.width > 0.0f && src.height > 0.0f) {
                    // Keep the attack visible if a future atlas rename removes
                    // the expected clip; asset validation should still report it.
                    const Vector2 size = entity->GetSize();
                    const bool faceLeft = proj->GetDirection() == Direction::Left;
                    const Vector2 fallbackScale = {
                        size.x / src.width,
                        size.y / src.height
                    };
                    View::Renderer::GetInstance().SubmitSprite(
                        ad.animator.GetTexture(), src, entity->GetPosition(), fallbackScale,
                        entity->GetRotation(), {0.0f, 0.0f}, WHITE,
                        View::Layer::World, entity->GetZIndex() + 0.20f,
                        faceLeft, id);
                }
                continue;
            }
        }

        Vector2 scale2d = entity->GetScale2D();
        Vector2 renderPosition = entity->GetPosition();

        if (entity->GetType() == EntityType::Projectile) {
            auto* projectile = static_cast<const Projectile*>(entity);
            if (projectile->GetProjectileType() == ProjectileType::BossAttack) {
                const Vector2 size = entity->GetSize();
                Vector2 visualSize = size;
                if (projectile->GetSubType() == 1) {
                    // The phase-3 sphere atlas contains generous transparent
                    // padding. Enlarge only its visual, centered on the safe
                    // 32x32 collision box, instead of inflating its collision
                    // or every other boss projectile.
                    visualSize = {160.0f, 120.0f};
                    renderPosition.x -= (visualSize.x - size.x) * 0.5f;
                    renderPosition.y -= (visualSize.y - size.y) * 0.5f;
                }
                scale2d = {
                    src.width > 0.0f ? visualSize.x / src.width : 1.0f,
                    src.height > 0.0f ? visualSize.y / src.height : 1.0f
                };
            }
        }

        scale2d.x *= entity->GetScale();
        scale2d.y *= entity->GetScale();
        
        View::Renderer::GetInstance().SubmitSprite(
            ad.animator.GetTexture(),
            src,
            renderPosition,
            scale2d,
            entity->GetRotation(),
            ad.origin,
            WHITE,
            View::Layer::World,
            entity->GetZIndex(),
            ad.flipX,
            id);
    }

    // Render static entities
    for (const auto& [id, data] : m_entities) {
        // data may contain entity pointer and texture/src information
        const Entity* entity = data.entity;
        if (!data.visible) continue;
        if (!entity || !entity->IsActive() || !data.texture) continue;

        Vector2 scale2d = entity->GetScale2D();
        scale2d.x *= entity->GetScale();
        scale2d.y *= entity->GetScale();

        View::Renderer::GetInstance().SubmitSprite(
            data.texture,
            data.src,
            entity->GetPosition(),
            scale2d,
            entity->GetRotation(),
            data.origin,
            WHITE,             // Màu tint mặc định
            View::Layer::World, // Hoặc Backgound/Foreground tùy loại Entity
            entity->GetZIndex(),
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
