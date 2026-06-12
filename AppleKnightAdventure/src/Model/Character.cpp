#include "Model/Character.h"
#include <algorithm>

Character::Character()
    : Entity(EntityType::Effect)
    , m_health(100)
    , m_maxHealth(100)
    , m_speed(0.0f)
    , m_direction(Direction::None)
    , m_attackCooldown(ATTACK_COOLDOWN)
    , m_attackTimer(0.0f)
    , m_state(State::Idle)
{
}

Character::Character(EntityType type)
    : Entity(type)
    , m_health(100)
    , m_maxHealth(100)
    , m_speed(0.0f)
    , m_direction(Direction::None)
    , m_attackCooldown(ATTACK_COOLDOWN)
    , m_attackTimer(0.0f)
    , m_state(State::Idle)
{
}

Character::Character(Vector2 position, Vector2 size, EntityType type)
    : Entity(position, size, type)
    , m_health(100)
    , m_maxHealth(100)
    , m_speed(0.0f)
    , m_direction(Direction::None)
    , m_attackCooldown(ATTACK_COOLDOWN)
    , m_attackTimer(0.0f)
    , m_state(State::Idle)
{
}

void Character::Update(float deltaTime) {
    if (!m_active) return;
    m_position.x += m_velocity.x * deltaTime;
    m_position.y += m_velocity.y * deltaTime;
    if (m_attackTimer > 0) m_attackTimer -= deltaTime;
}


int Character::GetHealth() const { return m_health; }
void Character::SetHealth(int health) { m_health = std::min(health, m_maxHealth); }
int Character::GetMaxHealth() const { return m_maxHealth; }
void Character::SetMaxHealth(int maxHealth) { m_maxHealth = maxHealth; }

void Character::TakeDamage(int damage) {
    m_health -= damage;
    if (m_health <= 0) {
        m_health = 0;
        m_active = false;
    }
}

void Character::Heal(int amount) {
    m_health = std::min(m_health + amount, m_maxHealth);
}

bool Character::IsAlive() const { return m_health > 0; }

float Character::GetSpeed() const { return m_speed; }
void Character::SetSpeed(float speed) { m_speed = speed; }
Direction Character::GetDirection() const { return m_direction; }

void Character::SetDirection(Direction direction) {
    m_direction = direction;
}

Character::State Character::GetState() const { return m_state; }

void Character::SetState(State state) { m_state = state; }

void Character::Move(Vector2 dir, float deltaTime) {
    m_velocity.x = dir.x * m_speed;
    m_velocity.y = dir.y * m_speed;
    m_position.x += m_velocity.x * deltaTime;
    m_position.y += m_velocity.y * deltaTime;
}

float Character::GetAttackCooldown() const { return m_attackCooldown; }
void Character::SetAttackCooldown(float cooldown) { m_attackCooldown = cooldown; }
bool Character::CanAttack() const { return m_attackTimer <= 0.0f; }

void Character::Attack() {
    if (CanAttack()) {
        m_attackTimer = m_attackCooldown;
    }
}

void Character::ResetAttackTimer() { m_attackTimer = 0.0f; }

Rectangle Character::GetAttackBoundingBox() const {
    Rectangle box = GetBoundingBox();
    switch (m_direction) {
        case Direction::Right:
            return {box.x + box.width, box.y, box.width, box.height};
        case Direction::Left:
            return {box.x - box.width, box.y, box.width, box.height};
        case Direction::Up:
            return {box.x, box.y - box.height, box.width, box.height};
        case Direction::Down:
            return {box.x, box.y + box.height, box.width, box.height};
        default:
            return box;
    }
}


