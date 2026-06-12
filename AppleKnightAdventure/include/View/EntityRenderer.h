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

    Texture2D* GetTexture(uint32_t entityId) const;

private:
    EntityRenderer() = default;
    ~EntityRenderer() = default;

    struct RenderData {
        const Entity* entity;
        Texture2D* texture;
        Rectangle src;
        Vector2 origin;
        bool flipX;
    };

    std::unordered_map<uint32_t, RenderData> m_entities;
};

} // namespace View
