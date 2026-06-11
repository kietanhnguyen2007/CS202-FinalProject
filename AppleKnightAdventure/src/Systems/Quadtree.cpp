#include "Systems/Quadtree.h"

QuadtreeNode::QuadtreeNode(Rectangle bounds, int capacity)
    : bounds(bounds)
    , capacity(capacity)
    , subdivided(false)
{
    entities.reserve(capacity);
}

void QuadtreeNode::Subdivide() {
    float halfW = bounds.width / 2.0f;
    float halfH = bounds.height / 2.0f;

    children[0] = std::make_unique<QuadtreeNode>(
        Rectangle{bounds.x, bounds.y, halfW, halfH}, capacity);
    children[1] = std::make_unique<QuadtreeNode>(
        Rectangle{bounds.x + halfW, bounds.y, halfW, halfH}, capacity);
    children[2] = std::make_unique<QuadtreeNode>(
        Rectangle{bounds.x, bounds.y + halfH, halfW, halfH}, capacity);
    children[3] = std::make_unique<QuadtreeNode>(
        Rectangle{bounds.x + halfW, bounds.y + halfH, halfW, halfH}, capacity);

    subdivided = true;

    for (Entity* entity : entities) {
        for (int i = 0; i < 4; ++i) {
            Rectangle childBounds = children[i]->bounds;
            Rectangle entityBox = entity->GetBoundingBox();
            if (CheckCollisionRecs(entityBox, childBounds)) {
                children[i]->Insert(entity);
            }
        }
    }
    entities.clear();
}

bool QuadtreeNode::Insert(Entity* entity) {
    Rectangle entityBox = entity->GetBoundingBox();
    if (!CheckCollisionRecs(entityBox, bounds)) {
        return false;
    }

    if (!subdivided) {
        if (static_cast<int>(entities.size()) < capacity) {
            entities.push_back(entity);
            return true;
        }
        Subdivide();
    }

    for (int i = 0; i < 4; ++i) {
        if (children[i]->Insert(entity)) {
            return true;
        }
    }
    return false;
}

void QuadtreeNode::Query(Rectangle range, std::vector<Entity*>& result) const {
    if (!CheckCollisionRecs(range, bounds)) {
        return;
    }

    if (subdivided) {
        for (int i = 0; i < 4; ++i) {
            children[i]->Query(range, result);
        }
    } else {
        for (Entity* entity : entities) {
            if (CheckCollisionRecs(range, entity->GetBoundingBox())) {
                result.push_back(entity);
            }
        }
    }
}

void QuadtreeNode::Clear() {
    entities.clear();
    for (int i = 0; i < 4; ++i) {
        if (children[i]) {
            children[i]->Clear();
            children[i].reset();
        }
    }
    subdivided = false;
}

Quadtree::Quadtree()
    : m_capacity(4)
{
    m_root = std::make_unique<QuadtreeNode>(Rectangle{0, 0, 800, 600}, m_capacity);
}

Quadtree::Quadtree(Rectangle bounds, int capacity)
    : m_capacity(capacity)
{
    m_root = std::make_unique<QuadtreeNode>(bounds, capacity);
}

void Quadtree::SetBounds(Rectangle bounds) {
    m_root = std::make_unique<QuadtreeNode>(bounds, m_capacity);
}

void Quadtree::Insert(Entity* entity) {
    if (entity && entity->IsActive()) {
        m_root->Insert(entity);
    }
}

void Quadtree::InsertBulk(const std::vector<Entity*>& entities) {
    for (Entity* entity : entities) {
        Insert(entity);
    }
}

std::vector<Entity*> Quadtree::Query(Rectangle range) const {
    std::vector<Entity*> result;
    m_root->Query(range, result);
    return result;
}

void Quadtree::Query(Rectangle range, std::vector<Entity*>& result) const {
    m_root->Query(range, result);
}

void Quadtree::Clear() {
    m_root->Clear();
}
