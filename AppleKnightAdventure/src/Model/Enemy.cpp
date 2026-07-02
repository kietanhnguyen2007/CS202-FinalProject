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
    : Character(position, {TILE_SIZE * 0.8f * 1.7f, TILE_SIZE * 0.9f * 1.7f}, EntityType::Enemy)
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

    switch (m_state) {
        case EnemyState::Idle:
            Character::SetState(State::Idle);
            break;
        case EnemyState::Patrol:
        case EnemyState::Chase:
            Character::SetState(State::Walk);
            break;
        case EnemyState::Attack:
            Character::SetState(State::Attack);
            break;
        case EnemyState::Hurt:
            Character::SetState(State::Hurt);
            break;
        case EnemyState::Dead:
            Character::SetState(State::Dead);
            break;
    }
}


EnemyType Enemy::GetEnemyType() const { return m_enemyType; }

EnemyState Enemy::GetState() const { return m_state; }
void Enemy::SetState(EnemyState state) {
    m_state = state;
    m_stateTimer = 0.0f;
}

float Enemy::GetStateTimer() const { return m_stateTimer; }
void Enemy::SetStateTimer(float timer) { m_stateTimer = timer; }

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
        case EnemyState::Dead:
            m_stateTimer += deltaTime;
            if (m_stateTimer >= 0.5f) {
                m_active = false;
            }
            break;

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

        case EnemyState::Chase: {
            float distFromSpawn = std::abs(m_position.x - m_spawnPosition.x);
            if (distFromSpawn > m_patrolRange * 1.5f) {
                SetState(EnemyState::Patrol);
            } else if (dist <= m_attackRange) {
                SetState(EnemyState::Attack);
            } else if (dist > m_detectionRange * 1.5f) {
                SetState(EnemyState::Patrol);
            } else {
                Chase(playerPosition, deltaTime);
            }
            break;
        }

        case EnemyState::Attack: {
            if (m_enemyType != EnemyType::Flying) {
                MoveX(0, deltaTime); // Stop moving while attacking for ground units
            }
            m_stateTimer += deltaTime;
            if (m_stateTimer >= 0.6f) { // Attack duration
                if (m_enemyType == EnemyType::Flying) {
                    SetState(EnemyState::Patrol); // Force retreat up
                } else {
                    float distFromSpawn = std::abs(m_position.x - m_spawnPosition.x);
                    if (distFromSpawn > m_patrolRange * 1.5f) {
                        SetState(EnemyState::Patrol);
                    } else if (dist > m_attackRange * 1.2f) {
                        SetState(EnemyState::Chase);
                    } else {
                        SetState(EnemyState::Attack); // Restart attack
                    }
                }
            }
            break;
        }

        case EnemyState::Hurt:
            MoveX(0, deltaTime); // Stop moving while hurt
            m_stateTimer += deltaTime;
            if (m_stateTimer >= 0.4f) {
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
    float diff = patrolX - m_position.x;
    float dirX = (diff > 0) ? 1.0f : -1.0f;
    MoveX(dirX, deltaTime); // Use MoveX to preserve gravity (vel.y) for ground units
    
    if (m_enemyType == EnemyType::Flying) {
        float dy = m_spawnPosition.y - m_position.y;
        Vector2 vel = GetVelocity();
        vel.y = dy * 2.0f; // Smoothly float back to spawn height
        SetVelocity(vel);
    }
    
    if (std::abs(diff) > 1.0f) {
        m_direction = (dirX > 0) ? Direction::Right : Direction::Left;
    }
}

void Enemy::Chase(Vector2 playerPosition, float deltaTime) {
    float dx = playerPosition.x - m_position.x;
    float dirX = (dx > 0) ? 1.0f : -1.0f;
    
    if (m_enemyType == EnemyType::Flying) {
        float dy = playerPosition.y - m_position.y;
        float dist = std::sqrt(dx * dx + dy * dy);
        if (dist > 0) Move({dx / dist, dy / dist}, deltaTime); // Flying: full 2D movement
    } else {
        MoveX(dirX, deltaTime); // Ground: only horizontal, preserve gravity vel.y
    }
    
    if (std::abs(dx) > 1.0f) {
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
        if (m_state != EnemyState::Dead) {
            SetState(EnemyState::Dead);
            SetVelocity({0.0f, 0.0f}); // Ensure it stands still
        }
    } else {
        SetState(EnemyState::Hurt);
    }
}
