#include "Systems/CollisionSystem.h"
#include <algorithm>

CollisionSystem::CollisionSystem()
    : m_quadtree(Rectangle{0, 0, 800, 600})
    , m_worldBounds({0, 0, 800, 600})
{
}

CollisionSystem::CollisionSystem(Rectangle worldBounds)
    : m_quadtree(worldBounds)
    , m_worldBounds(worldBounds)
{
}

void CollisionSystem::SetWorldBounds(Rectangle bounds) {
    m_worldBounds = bounds;
    m_quadtree.SetBounds(bounds);
}

Rectangle CollisionSystem::GetWorldBounds() const {
    return m_worldBounds;
}

void CollisionSystem::Rebuild(const std::vector<Entity*>& entities) {
    m_quadtree.Clear();
    m_quadtree.InsertBulk(entities);
}

bool CollisionSystem::CheckCollision(Entity* a, Entity* b) const {
    if (!a || !b || !a->IsActive() || !b->IsActive() || a == b) {
        return false;
    }
    return CheckCollisionRecs(a->GetBoundingBox(), b->GetBoundingBox());
}

std::vector<Entity*> CollisionSystem::GetNearbyEntities(Entity* entity) const {
    if (!entity || !entity->IsActive()) return {};
    Rectangle expandedBounds = entity->GetBoundingBox();
    expandedBounds.x -= expandedBounds.width;
    expandedBounds.y -= expandedBounds.height;
    expandedBounds.width *= 3.0f;
    expandedBounds.height *= 3.0f;
    return m_quadtree.Query(expandedBounds);
}

std::vector<Entity*> CollisionSystem::QueryRange(Rectangle range) const {
    return m_quadtree.Query(range);
}

void CollisionSystem::ForEachCollision(const std::vector<Entity*>& entities,
                                       CollisionCallback callback) {
    Rebuild(entities);
    std::vector<Entity*> checked;
    for (Entity* entity : entities) {
        if (!entity || !entity->IsActive()) continue;
        std::vector<Entity*> nearby = GetNearbyEntities(entity);
        for (Entity* other : nearby) {
            if (other == entity) continue;
            if (std::find(checked.begin(), checked.end(), other) != checked.end()) {
                continue;
            }
            if (CheckCollision(entity, other)) {
                callback(entity, other);
            }
        }
        checked.push_back(entity);
    }
}
