#include "Model/FakeWall.h"

FakeWall::FakeWall()
    : Entity(EntityType::FakeWall)
    , m_destroyed(false)
    , m_health(FAKE_WALL_HEALTH)
{
}

FakeWall::FakeWall(Vector2 position, Vector2 size)
    : Entity(position, size, EntityType::FakeWall)
    , m_destroyed(false)
    , m_health(FAKE_WALL_HEALTH)
{
}

void FakeWall::Update(float deltaTime) {
}

void FakeWall::Render() {
    SubmitRender();
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
