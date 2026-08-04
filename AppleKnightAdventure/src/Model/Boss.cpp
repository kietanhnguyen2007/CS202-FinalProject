#include "Model/Boss.h"
#include "Model/GameState.h"
#include <cmath>
#include <algorithm>

Boss::Boss(Vector2 position, Vector2 size, int bossType)
    : Character(position, size, EntityType::Boss)
    , m_currentPhase(BossPhase::Phase1)
    , m_currentState(BossState::Idle)
    , m_damage(20)
    , m_detectionRange(400.0f)
    , m_attackRange(80.0f)
    , m_enrageThreshold(0.2f)
    , m_phaseTimer(0.0f)
    , m_bossType(bossType)
    , m_chargeTimer(0.0f)
    , m_activeTimer(0.0f)
    , m_cooldownTimer(0.0f)
    , m_skillFired(false)
    , m_gameState(nullptr)
{
    m_speed = BOSS_SPEED;
    m_maxHealth = BOSS_MAX_HEALTH;
    m_health = m_maxHealth;
    m_attackCooldown = BOSS_ATTACK_COOLDOWN;
}

void Boss::Update(float deltaTime) {
    Character::Update(deltaTime);
    m_phaseTimer += deltaTime;
    
    if (m_cooldownTimer > 0.0f) {
        m_cooldownTimer -= deltaTime;
    }
}

void Boss::SetPhase(BossPhase phase) {
    m_currentPhase = phase;
}

void Boss::ChangeState(BossState newState) {
    if (m_currentState == newState) return;
    m_currentState = newState;
    
    // Reset skill state when entering a new skill
    m_chargeTimer = 0.0f;
    m_activeTimer = 0.0f;
    m_skillFired = false;
    
    if (newState == BossState::Hurt) {
        m_activeTimer = 0.5f; // Hurt animation time
    }
    
    // Sync with Character renderer state
    switch(newState) {
        case BossState::Idle:       m_state = Character::State::Idle; break;
        case BossState::Walk:       m_state = Character::State::Walk; break;
        case BossState::Hurt:       m_state = Character::State::Hurt; break;
        case BossState::Die:        m_state = Character::State::Dead; break;
        case BossState::Skill1:     m_state = Character::State::Attack; break;
        case BossState::Skill2:     m_state = Character::State::Attack2; break;
        case BossState::Skill3:     m_state = Character::State::Attack3; break;
        case BossState::Skill4:     m_state = Character::State::Ultimate; break;
        case BossState::Transition: m_state = Character::State::Skill; break;
    }
}

void Boss::TakeDamage(int damage) {
    m_health -= damage;
    if (m_health <= 0) {
        m_health = 0;
        ChangeState(BossState::Die);
        return;
    }
    
    // Hurt state interrupts skills unless transitioning
    if (m_currentState != BossState::Transition && m_currentState != BossState::Die) {
        ChangeState(BossState::Hurt);
    }
}

void Boss::Attack() {
    Character::Attack();
}

void Boss::UpdateAI(Vector2 playerPosition, float deltaTime, GameState* gameState) {
    if (!IsAlive()) return;
    
    m_gameState = gameState;
    
    UpdateState(deltaTime, playerPosition);
}

bool Boss::CheckLineOfSight(Vector2 start, Vector2 end) const {
    if (!m_gameState) return false;
    
    // Very simple DDA raycast on tilemap
    float dx = end.x - start.x;
    float dy = end.y - start.y;
    float dist = std::sqrt(dx*dx + dy*dy);
    
    if (dist < 1.0f) return true; // Too close
    
    int steps = static_cast<int>(dist / (TILE_SIZE / 2.0f)); // step every half tile
    if (steps == 0) steps = 1;
    
    float xStep = dx / steps;
    float yStep = dy / steps;
    
    float cx = start.x;
    float cy = start.y;
    
    for (int i = 0; i <= steps; ++i) {
        if (IsPointSolid({cx, cy})) {
            return false; // Hit a solid block
        }
        cx += xStep;
        cy += yStep;
    }
    
    return true; // No obstacles
}

bool Boss::IsPointSolid(Vector2 point) const {
    if (!m_gameState) return true;
    
    int tx = static_cast<int>(point.x / TILE_SIZE);
    int ty = static_cast<int>(point.y / TILE_SIZE);
    
    for (const auto& tile : m_gameState->GetTiles(MapLayer::Main)) {
        if (tile.solid && tile.x == tx && tile.y == ty) {
            return true;
        }
    }
    return false;
}
