#ifndef COLLISIONSYSTEM_H
#define COLLISIONSYSTEM_H

#include "Quadtree.h"
#include "Model/Entity.h"
#include <functional>
#include <vector>

class CollisionSystem {
protected:
    Quadtree m_quadtree;
    Rectangle m_worldBounds;

public:
    CollisionSystem();
    explicit CollisionSystem(Rectangle worldBounds);

    void SetWorldBounds(Rectangle bounds);
    Rectangle GetWorldBounds() const;

    void Rebuild(const std::vector<Entity*>& entities);

    bool CheckCollision(Entity* a, Entity* b) const;
    std::vector<Entity*> GetNearbyEntities(Entity* entity) const;
    std::vector<Entity*> QueryRange(Rectangle range) const;

    using CollisionCallback = std::function<void(Entity*, Entity*)>;
    void ForEachCollision(const std::vector<Entity*>& entities,
                          CollisionCallback callback);
};

#endif
