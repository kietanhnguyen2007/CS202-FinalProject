#pragma once

#include "View/Renderer.h"
#include "View/Animator.h"
#include "View/TextureAtlas.h"
#include "Model/Entity.h"
#include <unordered_map>
#include <functional>
#include <memory>
#include <string>

namespace View {

class EntityRenderer {
public:
    static EntityRenderer& GetInstance();

    void Register(const Entity* entity, Texture2D* tex,
                  Rectangle src = {}, Vector2 origin = {}, bool flipX = false);
    void Unregister(uint32_t entityId);
    void Clear();

    // Register an entity with a spritesheet atlas for animation (e.g. Checkpoint, Chest)
    bool RegisterAnimated(const Entity* entity, const std::string& atlasPath,
                          const std::string& startClip,
                          Vector2 origin = {}, bool flipX = false);

    // Advance all animated entities' animators
    void Update(float dt);

    void RenderAll();

    // Update the source rectangle used for a registered entity (e.g. chest open/close)
    void UpdateSpriteRect(uint32_t entityId, const Rectangle& src);

    // Toggle visibility of a registered entity (e.g. fake wall destroyed)
    void SetEntityVisible(uint32_t entityId, bool visible);

    Texture2D* GetTexture(uint32_t entityId) const;

    // Accessor for other view components to query the registered entity pointer
    const Entity* GetEntityPtr(uint32_t entityId) const {
        {
            auto it = m_entities.find(entityId);
            if (it != m_entities.end()) return it->second.entity;
        }
        {
            auto it = m_animatedEntities.find(entityId);
            if (it != m_animatedEntities.end()) return it->second.entity;
        }
        return nullptr;
    }

    // Memory safety
    bool IsRegistered(uint32_t entityId) const;
    void SetOnEntityRemovedCallback(uint32_t entityId, std::function<void(uint32_t)> cb);
    void ClearOnEntityRemovedCallback(uint32_t entityId);

    // Switch the current animation clip for an animated entity
    void SetClip(uint32_t entityId, const std::string& clipName);

    struct RenderData {
        const Entity* entity;
        Texture2D* texture;
        Rectangle src;
        Vector2 origin;
        bool flipX;
        bool visible = true;
    };

    // Iteration support for rendering systems
    const std::unordered_map<uint32_t, RenderData>& GetEntities() const { return m_entities; }

private:
    EntityRenderer() = default;
    ~EntityRenderer() = default;

    std::unordered_map<uint32_t, RenderData> m_entities;
    std::unordered_map<uint32_t, std::function<void(uint32_t)>> m_removeCallbacks;

    struct AnimatedEntityData {
        const Entity* entity;
        std::shared_ptr<Animations::TextureAtlas> atlas;
        Animations::Animator animator;
        Vector2 origin;
        bool flipX;
        bool visible = true;
    };
    std::unordered_map<uint32_t, AnimatedEntityData> m_animatedEntities;
};

} // namespace View
