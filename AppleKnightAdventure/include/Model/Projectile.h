#ifndef PROJECTILE_H
#define PROJECTILE_H

#include "Entity.h"
#include "Utils/Types.h"
#include <unordered_set>

class Projectile : public Entity {
protected:
    ProjectileType m_projectileType;
    int       m_damage;
    Direction m_direction;
    float     m_lifetime;
    float     m_lifeTimer;
    int       m_ownerId;

    // Homing (dragon fireball)
    DamageType m_element         = DamageType::Physical;
    int       m_remainingPierce  = 0;
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
    float GetLifeTimer() const { return m_lifeTimer; }
    bool HasExpired() const;
    void OnHit();
    void SetLifetime(float lifetime) { m_lifetime = lifetime; }
    void SetDamage(int damage) { m_damage = damage; }
    void SetVelocity(Vector2 vel) { m_velocity = vel; }

    // Hit tracking
    bool      m_hasHit           = false;
    std::unordered_set<int> m_hitEntityIds;

    // Subtype for specific projectile identification (e.g., Boss3 has multiple projectiles)
    int       m_subType          = 0;
    int       GetSubType() const { return m_subType; }
    void      SetSubType(int subType) { m_subType = subType; }

    // Element carried into the hit. Physical never reacts with anything, so
    // every projectile that is not explicitly elemental behaves as before.
    DamageType GetElement() const { return m_element; }
    void SetElement(DamageType element) { m_element = element; }

    // How many extra targets this projectile passes through before it dies.
    // Granted by the Gravity Lens core; zero restores the old behaviour of
    // despawning on the first thing it touches.
    int  GetRemainingPierce() const { return m_remainingPierce; }
    void SetRemainingPierce(int count) { m_remainingPierce = count; }
    // Spends one pierce. False when there is none left and the projectile
    // should despawn.
    bool ConsumePierce() {
        if (m_remainingPierce <= 0) return false;
        --m_remainingPierce;
        return true;
    }

    // Homing
    bool IsHoming() const { return m_isHoming; }
    void SetHoming(bool homing) { m_isHoming = homing; }
    void SetHomingTargetPos(Vector2 pos) { m_homingTargetPos = pos; }

    bool HasHit() const { return m_hasHit; }
    void SetHasHit(bool hit) { m_hasHit = hit; }
    bool HasHitEntity(int entityId) const { return m_hitEntityIds.count(entityId) != 0; }
    void MarkHitEntity(int entityId) { m_hitEntityIds.insert(entityId); }
};

#endif
