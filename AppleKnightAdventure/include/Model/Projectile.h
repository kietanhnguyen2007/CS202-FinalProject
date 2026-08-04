#ifndef PROJECTILE_H
#define PROJECTILE_H

#include "Entity.h"
#include "Utils/Types.h"

class Projectile : public Entity {
protected:
    ProjectileType m_projectileType;
    int       m_damage;
    Direction m_direction;
    float     m_lifetime;
    float     m_lifeTimer;
    int       m_ownerId;

    // Homing (dragon fireball)
    bool      m_isHoming         = false;
    Vector2   m_homingTargetPos  = {0, 0}; // updated each frame by controller
    float     m_homingStrength   = 280.0f; // steering force

public:
    Projectile();
    Projectile(Vector2 position, Vector2 size, ProjectileType type,
               Direction direction, int damage, int ownerId);

    void Update(float deltaTime) override;

    ProjectileType GetProjectileType() const;
    int GetDamage() const;
    Direction GetDirection() const;
    int GetOwnerId() const;
    float GetLifetime() const;
    bool HasExpired() const;
    void OnHit();
    void SetLifetime(float lifetime) { m_lifetime = lifetime; }
    void SetDamage(int damage) { m_damage = damage; }
    void SetVelocity(Vector2 vel) { m_velocity = vel; }

    // Hit tracking
    bool      m_hasHit           = false;

    // Homing
    bool IsHoming() const { return m_isHoming; }
    void SetHoming(bool homing) { m_isHoming = homing; }
    void SetHomingTargetPos(Vector2 pos) { m_homingTargetPos = pos; }

    bool HasHit() const { return m_hasHit; }
    void SetHasHit(bool hit) { m_hasHit = hit; }
};

#endif
