#include "Model/BuffPickup.h"
#include <cmath>

namespace {
constexpr float ORB_SIZE = 26.0f;
}

BuffPickup::BuffPickup(Vector2 position, BuffType type)
    : Entity(position, {ORB_SIZE, ORB_SIZE}, EntityType::BuffOrb)
    , m_buffType(type)
    , m_lifeTimer(LIFETIME)
    , m_anchor(position)
{
}

void BuffPickup::Update(float deltaTime) {
    if (!m_active) return;

    m_bobTimer += deltaTime;
    m_lifeTimer -= deltaTime;
    if (m_lifeTimer <= 0.0f) {
        m_lifeTimer = 0.0f;
        m_active = false;
    }
    // Keep the box anchored; only the drawn sprite bobs.
    m_position = m_anchor;
}

float BuffPickup::GetBobOffset() const {
    return std::sin(m_bobTimer * BOB_SPEED) * BOB_RANGE;
}
