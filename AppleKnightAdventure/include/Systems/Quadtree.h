#ifndef QUADTREE_H
#define QUADTREE_H

#include "raylib.h"
#include "Model/Entity.h"
#include <vector>
#include <memory>

struct QuadtreeNode {
    Rectangle bounds;
    std::vector<Entity*> entities;
    std::unique_ptr<QuadtreeNode> children[4];
    int capacity;
    bool subdivided;

    QuadtreeNode(Rectangle bounds, int capacity = 4);
    void Subdivide();
    bool Insert(Entity* entity);
    void Query(Rectangle range, std::vector<Entity*>& result) const;
    void Clear();
};

class Quadtree {
protected:
    std::unique_ptr<QuadtreeNode> m_root;
    int m_capacity;

public:
    Quadtree();
    explicit Quadtree(Rectangle bounds, int capacity = 4);

    void SetBounds(Rectangle bounds);
    void Insert(Entity* entity);
    void InsertBulk(const std::vector<Entity*>& entities);
    std::vector<Entity*> Query(Rectangle range) const;
    void Clear();
};

#endif
