#include "Model/FakeWall.h"

FakeWall::FakeWall()
    : Entity(EntityType::FakeWall)
    , m_destroyed(false)
    , m_health(FAKE_WALL_HEALTH)
{
    m_tileX = 0;
    m_tileY = 0;
}

FakeWall::FakeWall(Vector2 position, Vector2 size)
    : Entity(position, size, EntityType::FakeWall)
    , m_destroyed(false)
    , m_health(FAKE_WALL_HEALTH)
{
    m_tileX = static_cast<int>(position.x / TILE_SIZE);
    m_tileY = static_cast<int>(position.y / TILE_SIZE);
}

void FakeWall::Update(float deltaTime) {
}


bool FakeWall::IsDestroyed() const { return m_destroyed; }

void FakeWall::TakeDamage(int damage) {
    if (m_destroyed) return;
    m_health -= damage;
    if (m_health <= 0) {
        m_health = 0;
        m_destroyed = true;
        m_active = false;
    }
}

int FakeWall::GetHealth() const { return m_health; }
