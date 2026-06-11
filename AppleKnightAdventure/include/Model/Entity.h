#ifndef ENTITY_H
#define ENTITY_H

#include "raylib.h"
#include <cstdint>

enum class EntityType {
    Player,
    DualWorldPlayer,
    Enemy,
    Boss,
    Projectile,
    Item,
    Checkpoint,
    Chest,
    FakeWall,
    Pet,
    Particle,
    Effect
};

class Entity {
protected:
    int m_id;
    EntityType m_type;
    Vector2 m_position;
    Vector2 m_size;
    Vector2 m_velocity;
    float m_rotation;
    float m_scale;
    bool m_active;

public:
    Entity();
    explicit Entity(EntityType type);
    Entity(Vector2 position, Vector2 size, EntityType type);
    virtual ~Entity() = default;

    virtual void Update(float deltaTime) = 0;

    // Identifiers
    int GetId() const;
    void SetId(int id);
    EntityType GetType() const;

    // Transform
    Vector2 GetPosition() const;
    void SetPosition(Vector2 position);
    Vector2 GetSize() const;
    void SetSize(Vector2 size);
    Vector2 GetVelocity() const;
    void SetVelocity(Vector2 velocity);
    float GetRotation() const;
    void SetRotation(float rotation);
    float GetScale() const;
    void SetScale(float scale);

    // Lifecycle
    bool IsActive() const;
    void SetActive(bool active);

    // Collision
    Rectangle GetBoundingBox() const;

};

#endif
