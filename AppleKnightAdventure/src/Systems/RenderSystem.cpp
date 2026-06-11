#include "Systems/RenderSystem.h"
#include "View/Renderer.h"
#include "raylib.h"

namespace Systems {

RenderSystem::RenderSystem()
    : m_quadtree(Rectangle{0, 0, 800, 600})
    , m_worldBounds({0, 0, 800, 600})
{
}

RenderSystem::RenderSystem(Rectangle worldBounds)
    : m_quadtree(worldBounds)
    , m_worldBounds(worldBounds)
{
}

void RenderSystem::SetWorldBounds(Rectangle bounds) {
    m_worldBounds = bounds;
    m_quadtree.SetBounds(bounds);
}

void RenderSystem::RenderFrame(const Camera2D& camera, std::vector<Entity*>& entities) {
    m_totalCount = entities.size();

    m_quadtree.Clear();
    m_quadtree.InsertBulk(entities);

    float left = camera.target.x - camera.offset.x / camera.zoom;
    float top = camera.target.y - camera.offset.y / camera.zoom;
    float w = (float)GetScreenWidth() / camera.zoom;
    float h = (float)GetScreenHeight() / camera.zoom;
    Rectangle viewRect = { left, top, w, h };

    m_visible.clear();
    m_quadtree.Query(viewRect, m_visible);
    m_visibleCount = m_visible.size();

    View::Renderer& r = View::Renderer::GetInstance();
    BeginMode2D(camera);
    r.BeginFrame();

    for (Entity* e : m_visible) {
        if (!e || !e->IsActive()) continue;
        e->Render();
    }

    r.EndFrameAndFlush();
    EndMode2D();
}

} // namespace Systems
