#include "Model/Enemy.h"
#include <cmath>

Enemy::Enemy()
    : Character(EntityType::Enemy)
    , m_enemyType(EnemyType::Melee)
    , m_state(EnemyState::Idle)
    , m_damage(10)
    , m_detectionRange(200.0f)
    , m_attackRange(ENEMY_MELEE_RANGE)
    , m_patrolRange(100.0f)
    , m_spawnPosition({0, 0})
    , m_stateTimer(0.0f)
{
}

Enemy::Enemy(Vector2 position, EnemyType type)
    : Character(position, {TILE_SIZE * 0.7f, TILE_SIZE * 0.8f}, EntityType::Enemy)
    , m_enemyType(type)
    , m_state(EnemyState::Idle)
    , m_damage(10)
    , m_detectionRange(200.0f)
    , m_attackRange(ENEMY_MELEE_RANGE)
    , m_patrolRange(100.0f)
    , m_spawnPosition(position)
    , m_stateTimer(0.0f)
{
    switch (type) {
        case EnemyType::Melee:
            m_speed = ENEMY_MELEE_SPEED;
            m_damage = 15;
            m_attackRange = ENEMY_MELEE_RANGE;
            m_health = 50;
            m_maxHealth = 50;
            break;
        case EnemyType::Ranged:
            m_speed = ENEMY_RANGED_SPEED;
            m_damage = 10;
            m_attackRange = ENEMY_RANGED_RANGE;
            m_health = 30;
            m_maxHealth = 30;
            break;
        case EnemyType::Flying:
            m_speed = ENEMY_FLYING_SPEED;
            m_damage = 12;
            m_attackRange = ENEMY_MELEE_RANGE;
            m_health = 40;
            m_maxHealth = 40;
            break;
    }
}

void Enemy::Update(float deltaTime) {
    Character::Update(deltaTime);
}


EnemyType Enemy::GetEnemyType() const { return m_enemyType; }

EnemyState Enemy::GetState() const { return m_state; }
void Enemy::SetState(EnemyState state) {
    m_state = state;
    m_stateTimer = 0.0f;
}

int Enemy::GetDamage() const { return m_damage; }
void Enemy::SetDamage(int damage) { m_damage = damage; }

float Enemy::GetDetectionRange() const { return m_detectionRange; }
void Enemy::SetDetectionRange(float range) { m_detectionRange = range; }
float Enemy::GetAttackRange() const { return m_attackRange; }
void Enemy::SetAttackRange(float range) { m_attackRange = range; }
float Enemy::GetPatrolRange() const { return m_patrolRange; }
void Enemy::SetPatrolRange(float range) { m_patrolRange = range; }

Vector2 Enemy::GetSpawnPosition() const { return m_spawnPosition; }

void Enemy::UpdateAI(Vector2 playerPosition, float deltaTime) {
    float dx = playerPosition.x - m_position.x;
    float dy = playerPosition.y - m_position.y;
    float dist = std::sqrt(dx * dx + dy * dy);

    switch (m_state) {
        case EnemyState::Idle:
            if (dist <= m_detectionRange) {
                SetState(EnemyState::Chase);
            } else {
                m_stateTimer += deltaTime;
                if (m_stateTimer > 2.0f) {
                    SetState(EnemyState::Patrol);
                }
            }
            break;

        case EnemyState::Patrol:
            Patrol(deltaTime);
            if (dist <= m_detectionRange) {
                SetState(EnemyState::Chase);
            }
            break;

        case EnemyState::Chase:
            if (dist <= m_attackRange) {
                SetState(EnemyState::Attack);
            } else if (dist > m_detectionRange * 1.5f) {
                SetState(EnemyState::Patrol);
            } else {
                Chase(playerPosition, deltaTime);
            }
            break;

        case EnemyState::Attack:
            Chase(playerPosition, deltaTime);
            if (dist > m_attackRange * 1.2f) {
                SetState(EnemyState::Chase);
            }
            break;

        default:
            break;
    }
}

void Enemy::Patrol(float deltaTime) {
    m_stateTimer += deltaTime;
    float patrolX = m_spawnPosition.x + std::sin(m_stateTimer) * m_patrolRange;
    float dirX = (patrolX > m_position.x) ? 1.0f : -1.0f;
    Move({dirX, 0}, deltaTime);
    m_direction = (dirX > 0) ? Direction::Right : Direction::Left;
}

void Enemy::Chase(Vector2 playerPosition, float deltaTime) {
    float dx = playerPosition.x - m_position.x;
    float dy = playerPosition.y - m_position.y;
    float dist = std::sqrt(dx * dx + dy * dy);
    if (dist > 0) {
        Vector2 dir = {dx / dist, dy / dist};
        Move(dir, deltaTime);
        m_direction = (dx > 0) ? Direction::Right : Direction::Left;
    }
}

void Enemy::Attack() {
    Character::Attack();
}

void Enemy::TakeDamage(int damage) {
    m_health -= damage;
    if (m_health <= 0) {
        m_health = 0;
        m_active = false;
    }
}
