#pragma once
#include "raylib.h"
#include "Model/Entity.h"
#include "Systems/Quadtree.h"
#include <vector>

namespace Systems {

class RenderSystem {
public:
    RenderSystem();
    explicit RenderSystem(Rectangle worldBounds);

    void SetWorldBounds(Rectangle bounds);

    void CullEntities(const Camera2D& camera, const std::vector<Entity*>& entities);
    const std::vector<Entity*>& GetVisible() const { return m_visible; }

    size_t GetVisibleCount() const { return m_visibleCount; }
    size_t GetTotalCount() const { return m_totalCount; }

private:
    Quadtree m_quadtree;
    Rectangle m_worldBounds;
    std::vector<Entity*> m_visible;
    size_t m_visibleCount = 0;
    size_t m_totalCount = 0;
};

} // namespace Systems
