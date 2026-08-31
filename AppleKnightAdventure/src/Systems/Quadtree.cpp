#include "Systems/Quadtree.h"

QuadtreeNode::QuadtreeNode(Rectangle bounds, int capacity, int depth)
    : bounds(bounds)
    , capacity(capacity)
    , depth(depth)
    , subdivided(false)
{
    entities.reserve(capacity);
}

void QuadtreeNode::Subdivide() {
    float halfW = bounds.width / 2.0f;
    float halfH = bounds.height / 2.0f;

    children[0] = std::make_unique<QuadtreeNode>(
        Rectangle{bounds.x, bounds.y, halfW, halfH}, capacity, depth + 1);
    children[1] = std::make_unique<QuadtreeNode>(
        Rectangle{bounds.x + halfW, bounds.y, halfW, halfH}, capacity, depth + 1);
    children[2] = std::make_unique<QuadtreeNode>(
        Rectangle{bounds.x, bounds.y + halfH, halfW, halfH}, capacity, depth + 1);
    children[3] = std::make_unique<QuadtreeNode>(
        Rectangle{bounds.x + halfW, bounds.y + halfH, halfW, halfH}, capacity, depth + 1);

    subdivided = true;

    // An entity belongs to at most one child. Entities that cross a split line
    // stay in this node instead of being duplicated into several descendants.
    std::vector<Entity*> previousEntities = std::move(entities);
    entities.clear();
    entities.reserve(capacity);
    for (Entity* entity : previousEntities) {
        const int childIndex = GetContainingChildIndex(entity->GetBoundingBox());
        if (childIndex < 0 || !children[childIndex]->Insert(entity)) {
            entities.push_back(entity);
        }
    }
}

int QuadtreeNode::GetContainingChildIndex(Rectangle entityBox) const {
    if (!subdivided) return -1;

    const float midX = bounds.x + bounds.width * 0.5f;
    const float midY = bounds.y + bounds.height * 0.5f;
    const float right = entityBox.x + entityBox.width;
    const float bottom = entityBox.y + entityBox.height;

    const bool fitsLeft = entityBox.x >= bounds.x && right <= midX;
    const bool fitsRight = entityBox.x >= midX && right <= bounds.x + bounds.width;
    const bool fitsTop = entityBox.y >= bounds.y && bottom <= midY;
    const bool fitsBottom = entityBox.y >= midY && bottom <= bounds.y + bounds.height;

    if (fitsTop) {
        if (fitsLeft) return 0;
        if (fitsRight) return 1;
    } else if (fitsBottom) {
        if (fitsLeft) return 2;
        if (fitsRight) return 3;
    }
    return -1;
}

bool QuadtreeNode::Insert(Entity* entity) {
    Rectangle entityBox = entity->GetBoundingBox();
    if (!CheckCollisionRecs(entityBox, bounds)) {
        return false;
    }

    if (subdivided) {
        const int childIndex = GetContainingChildIndex(entityBox);
        if (childIndex >= 0) {
            return children[childIndex]->Insert(entity);
        }
        entities.push_back(entity);
        return true;
    }

    // A hard depth limit guarantees that coincident or nearly identical
    // hitboxes can never cause unbounded recursive subdivision.
    if (static_cast<int>(entities.size()) < capacity || depth >= MAX_DEPTH) {
        entities.push_back(entity);
        return true;
    }

    Subdivide();
    const int childIndex = GetContainingChildIndex(entityBox);
    if (childIndex >= 0) {
        return children[childIndex]->Insert(entity);
    }
    entities.push_back(entity);
    return true;
}

void QuadtreeNode::Query(Rectangle range, std::vector<Entity*>& result) const {
    if (!CheckCollisionRecs(range, bounds)) {
        return;
    }

    // Split-crossing entities are stored at internal nodes, so every visited
    // node must test its own collection before descending.
    for (Entity* entity : entities) {
        if (CheckCollisionRecs(range, entity->GetBoundingBox())) {
            result.push_back(entity);
        }
    }

    if (subdivided) {
        for (int i = 0; i < 4; ++i) {
            children[i]->Query(range, result);
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
