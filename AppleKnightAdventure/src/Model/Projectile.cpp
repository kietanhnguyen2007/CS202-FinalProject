#include "Model/Projectile.h"
#include "Utils/Constants.h"

Projectile::Projectile()
    : Entity(EntityType::Projectile)
    , m_projectileType(ProjectileType::Arrow)
    , m_damage(0)
    , m_direction(Direction::None)
    , m_lifetime(2.0f)
    , m_lifeTimer(0.0f)
    , m_ownerId(-1)
{
}

Projectile::Projectile(Vector2 position, Vector2 size, ProjectileType type,
                       Direction direction, int damage, int ownerId)
    : Entity(position, size, EntityType::Projectile)
    , m_projectileType(type)
    , m_damage(damage)
    , m_direction(direction)
    , m_lifetime(2.0f)
    , m_lifeTimer(0.0f)
    , m_ownerId(ownerId)
{
    switch (direction) {
        case Direction::Right: m_velocity = {PROJECTILE_SPEED, 0}; break;
        case Direction::Left: m_velocity = {-PROJECTILE_SPEED, 0}; break;
        case Direction::Up: m_velocity = {0, -PROJECTILE_SPEED}; break;
        case Direction::Down: m_velocity = {0, PROJECTILE_SPEED}; break;
        default: m_velocity = {0, 0}; break;
    }
    m_rotation = (direction == Direction::Right) ? 0.0f :
                 (direction == Direction::Down) ? 90.0f :
                 (direction == Direction::Left) ? 180.0f :
                 (direction == Direction::Up) ? 270.0f : 0.0f;
}

void Projectile::Update(float deltaTime) {
    if (!m_active) return;
    m_position.x += m_velocity.x * deltaTime;
    m_position.y += m_velocity.y * deltaTime;
    m_lifeTimer += deltaTime;
    if (m_lifeTimer >= m_lifetime) {
        m_active = false;
    }
}


ProjectileType Projectile::GetProjectileType() const { return m_projectileType; }
int Projectile::GetDamage() const { return m_damage; }
Direction Projectile::GetDirection() const { return m_direction; }
int Projectile::GetOwnerId() const { return m_ownerId; }
float Projectile::GetLifetime() const { return m_lifetime; }
bool Projectile::HasExpired() const { return !m_active; }

void Projectile::OnHit() {
    m_active = false;
}
