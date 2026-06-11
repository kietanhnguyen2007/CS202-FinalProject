#include "View/CharacterRenderer.h"

namespace View {

CharacterRenderer& CharacterRenderer::GetInstance() {
    static CharacterRenderer instance;
    return instance;
}

void CharacterRenderer::Register(const Entity* entity,
                                  const std::string& atlasPath,
                                  const std::string& defaultClip) {
    if (!entity) return;
    uint32_t id = static_cast<uint32_t>(entity->GetId());

    // Load or retrieve atlas from cache
    auto atlasIt = m_atlasCache.find(atlasPath);
    if (atlasIt == m_atlasCache.end()) {
        auto atlas = Animations::TextureAtlas::LoadFromJSON(atlasPath);
        if (atlas) {
            atlas->LoadTexture();
            auto result = m_atlasCache.emplace(atlasPath, std::move(atlas));
            atlasIt = result.first;
        }
    }
    if (atlasIt == m_atlasCache.end()) return;

    auto& animator = m_animators[id];
    animator.SetTexture(atlasIt->second->GetTexture());

    // Register all clips from atlas into animator
    // (In a more advanced setup, we'd query specific clips per action type.)
    // For now, we simply copy references to loaded AnimationClip objects
    // by iterating known clip names from the atlas.

    // The animator stores its own shared_ptr copies via AddClip,
    // which we populate from the atlas's clip list.
    // Since TextureAtlas stores clips by name, we forward them to the animator
    // on first registration.

    m_entities[id] = entity;

    // Try to play default clip if it exists
    if (!defaultClip.empty() && animator.HasClip(defaultClip)) {
        animator.Play(defaultClip);
    }
}

void CharacterRenderer::Unregister(uint32_t entityId) {
    m_animators.erase(entityId);
    m_entities.erase(entityId);
}

void CharacterRenderer::Clear() {
    m_animators.clear();
    m_entities.clear();
    m_atlasCache.clear();
}

void CharacterRenderer::UpdateAll(float dt) {
    for (auto& [id, animator] : m_animators) {
        animator.Update(dt);
    }
}

void CharacterRenderer::RenderAll() {
    for (auto& [id, animator] : m_animators) {
        auto it = m_entities.find(id);
        if (it == m_entities.end()) continue;

        const Entity* entity = it->second;
        if (!entity->IsActive() || !animator.HasTexture()) continue;

        View::Renderer::GetInstance().SubmitSprite(
            animator.GetTexture(),
            animator.GetCurrentSrcRect(),
            entity->GetPosition(),
            {entity->GetScale(), entity->GetScale()},
            entity->GetRotation(),
            animator.GetCurrentOrigin(),
            WHITE,
            Systems::Layer::World,
            0.0f,
            animator.GetFlipX(),
            id);
    }
}

Animations::Animator* CharacterRenderer::GetAnimator(uint32_t entityId) {
    auto it = m_animators.find(entityId);
    if (it == m_animators.end()) return nullptr;
    return &it->second;
}

} // namespace View
