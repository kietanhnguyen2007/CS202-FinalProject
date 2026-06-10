#ifndef PROJECTILE_H
#define PROJECTILE_H

#include "Entity.h"
#include "../Utils/Types.h"

class Projectile : public Entity {
protected:
    ProjectileType m_projectileType;
    int m_damage;
    Direction m_direction;
    float m_lifetime;
    float m_lifeTimer;
    int m_ownerId;

public:
    Projectile();
    Projectile(Vector2 position, Vector2 size, ProjectileType type,
               Direction direction, int damage, int ownerId);

    void Update(float deltaTime) override;
    void Render() override;

    ProjectileType GetProjectileType() const;
    int GetDamage() const;
    Direction GetDirection() const;
    int GetOwnerId() const;

    float GetLifetime() const;
    bool HasExpired() const;

    void OnHit();
};

#endif
