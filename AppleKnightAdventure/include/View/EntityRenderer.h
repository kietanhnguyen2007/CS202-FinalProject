#pragma once

#include "View/Renderer.h"
#include "Model/Entity.h"
#include <unordered_map>
#include <functional>

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

    // Memory safety
    bool IsRegistered(uint32_t entityId) const;
    void SetOnEntityRemovedCallback(uint32_t entityId, std::function<void(uint32_t)> cb);
    void ClearOnEntityRemovedCallback(uint32_t entityId);

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
    std::unordered_map<uint32_t, std::function<void(uint32_t)>> m_removeCallbacks;
};

} // namespace View
