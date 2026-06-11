#include "View/CharacterRenderer.h"
#include <iostream>
#include <cmath>

namespace View {

CharacterRenderer& CharacterRenderer::GetInstance() {
    static CharacterRenderer instance;
    return instance;
}

bool CharacterRenderer::Register(const Entity* entity,
                                  const std::string& atlasPath,
                                  const std::string& defaultClip) {
    if (!entity) {
        std::cerr << "[CharacterRenderer] Register: null entity\n";
        return false;
    }
    uint32_t id = static_cast<uint32_t>(entity->GetId());

    // Load or retrieve atlas from cache
    auto atlasIt = m_atlasCache.find(atlasPath);
    if (atlasIt == m_atlasCache.end()) {
        auto atlas = Animations::TextureAtlas::LoadFromJSON(atlasPath);
        if (!atlas) {
            std::cerr << "[CharacterRenderer] Failed to load atlas: " << atlasPath << "\n";
            return false;
        }
        atlas->LoadTexture();
        auto result = m_atlasCache.emplace(atlasPath, std::move(atlas));
        atlasIt = result.first;
    }

    auto& animator = m_animators[id];
    animator.SetTexture(atlasIt->second->GetTexture());

    // Forward all clips from atlas to animator
    for (const auto& clipName : atlasIt->second->GetClipNames()) {
        auto clip = atlasIt->second->GetClip(clipName);
        if (clip) animator.AddClip(clip);
    }

    m_entities[id] = entity;

    // Try to play default clip if it exists
    if (!defaultClip.empty()) {
        if (animator.HasClip(defaultClip)) {
            animator.Play(defaultClip);
        } else {
            std::cerr << "[CharacterRenderer] Default clip not found: " << defaultClip
                      << " for entity " << id << "\n";
        }
    }

    return true;
}

void CharacterRenderer::Unregister(uint32_t entityId) {
    m_animators.erase(entityId);
    m_entities.erase(entityId);
    m_lastActions.erase(entityId);
}

void CharacterRenderer::Clear() {
    m_animators.clear();
    m_entities.clear();
    m_atlasCache.clear();
    m_actionConfigs.clear();
    m_lastActions.clear();
}

void CharacterRenderer::SetActionClipMap(EntityType type,
                                          const std::unordered_map<int, std::string>& clips) {
    m_actionConfigs[type].clipMap = clips;
}

void CharacterRenderer::SetInferFunction(EntityType type,
                                          std::function<int(const Entity*)> inferFn) {
    m_actionConfigs[type].inferAction = std::move(inferFn);
}

int CharacterRenderer::DefaultInferAction(const Entity* entity) {
    if (!entity || !entity->IsActive()) return ACTION_DEAD;
    auto vel = entity->GetVelocity();
    if (std::abs(vel.x) > 0.1f || std::abs(vel.y) > 0.1f) return ACTION_WALK;
    return ACTION_IDLE;
}

void CharacterRenderer::UpdateAll(float dt) {
    for (auto& [id, animator] : m_animators) {
        auto entityIt = m_entities.find(id);
        if (entityIt == m_entities.end()) continue;

        const Entity* entity = entityIt->second;
        if (!entity->IsActive()) continue;

        // Auto-switch clip based on inferred action
        EntityType type = entity->GetType();
        auto configIt = m_actionConfigs.find(type);

        if (configIt != m_actionConfigs.end()) {
            // Determine current action
            int action;
            if (configIt->second.inferAction) {
                action = configIt->second.inferAction(entity);
            } else {
                action = DefaultInferAction(entity);
            }

            // Find clip for this action
            auto clipIt = configIt->second.clipMap.find(action);
            if (clipIt != configIt->second.clipMap.end()) {
                auto prevIt = m_lastActions.find(id);
                int prevAction = (prevIt != m_lastActions.end()) ? prevIt->second : -1;

                // Switch clip if action changed or animator stopped
                if (action != prevAction || !animator.IsPlaying()) {
                    animator.Play(clipIt->second);
                    m_lastActions[id] = action;
                }
            }
        }

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
