#pragma once

#include "View/Renderer.h"
#include "Model/Entity.h"
#include <unordered_map>

namespace View {

class EntityRenderer {
public:
    static EntityRenderer& GetInstance();

    void Register(const Entity* entity, Texture2D* tex,
                  Rectangle src = {}, Vector2 origin = {}, bool flipX = false);
    void Unregister(uint32_t entityId);
    void Clear();

    void RenderAll();

    // Update the source rectangle used for a registered entity (e.g. chest open/close)
    void UpdateSpriteRect(uint32_t entityId, const Rectangle& src);

    // Toggle visibility of a registered entity (e.g. fake wall destroyed)
    void SetEntityVisible(uint32_t entityId, bool visible);

    Texture2D* GetTexture(uint32_t entityId) const;

    // Accessor for other view components to query the registered entity pointer
    const Entity* GetEntityPtr(uint32_t entityId) const {
        auto it = m_entities.find(entityId);
        if (it == m_entities.end()) return nullptr;
        return it->second.entity;
    }

private:
    EntityRenderer() = default;
    ~EntityRenderer() = default;

    struct RenderData {
        const Entity* entity;
        Texture2D* texture;
        Rectangle src;
        Vector2 origin;
        bool flipX;
        bool visible = true;
    };

    std::unordered_map<uint32_t, RenderData> m_entities;
};

} // namespace View
