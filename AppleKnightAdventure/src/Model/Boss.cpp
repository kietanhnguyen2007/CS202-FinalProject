#include "Model/Boss.h"
#include "Model/GameState.h"
#include <cmath>
#include <algorithm>

Boss::Boss(Vector2 position, Vector2 size, int bossType)
    : Character(position, size, EntityType::Boss)
    , m_currentPhase(BossPhase::Phase1)
    , m_currentState(BossState::Idle)
    , m_damage(20)
    , m_detectionRange(600.0f)
    , m_attackRange(80.0f)
    , m_enrageThreshold(0.2f)
    , m_phaseTimer(0.0f)
    , m_bossType(bossType)
    , m_chargeTimer(0.0f)
    , m_activeTimer(0.0f)
    , m_cooldownTimer(0.0f)
    , m_skillFired(false)
    , m_comboStep(0)
    , m_superArmor(false)
    , m_recentDamage(0)
    , m_damageTimer(0.0f)
    , m_wantsMelee(false)
    , m_gameState(nullptr)
{
    m_speed = BOSS_SPEED;
    m_maxHealth = BOSS_MAX_HEALTH;
    m_health = m_maxHealth;
    m_attackCooldown = BOSS_ATTACK_COOLDOWN;
}

void Boss::Update(float deltaTime) {
    Character::Update(deltaTime);
    if (!m_active) return;

    if (m_currentState == BossState::Die) {
        m_activeTimer -= deltaTime;
        if (m_activeTimer <= 0.0f) {
            m_active = false;
        }
        return;
    }

    m_phaseTimer += deltaTime;
    
    if (m_cooldownTimer > 0.0f) {
        m_cooldownTimer -= deltaTime;
    }
    
    if (m_damageTimer > 0.0f) {
        m_damageTimer -= deltaTime;
        if (m_damageTimer <= 0.0f) {
            m_recentDamage = 0; // Reset counter
        }
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
    
    if (newState != BossState::Walk) {
        m_velocity.x = 0.0f; // Stop horizontal movement when not walking
    }
    
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
    if (m_currentState == BossState::Transition || m_currentState == BossState::Die) return;

    m_health -= damage;
    
    // Accumulate recent damage for anti-stunlock
    m_recentDamage += damage;
    if (m_damageTimer <= 0.0f) {
        m_damageTimer = 2.0f; // 2 seconds window
    }

    if (m_health <= 0) {
        if (IsFinalPhase()) {
            m_health = 0;
            ChangeState(BossState::Die);
            m_activeTimer = 2.0f;
            return;
        } else {
            m_health = 1; // leave 1 HP so it triggers phase transition in UpdateState
        }
    }
    
    // Boss không bị interrupt khi bị đánh
    // Removed ChangeState(BossState::Hurt) logic
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

void Boss::ResetToPhase1() {
    m_currentPhase = BossPhase::Phase1;
    m_currentState = BossState::Idle;
    m_health = m_maxHealth;
    m_chargeTimer = 0.0f;
    m_activeTimer = 0.0f;
    m_cooldownTimer = 2.0f;
    m_skillFired = false;
    m_superArmor = false;
    m_comboStep = 0;
    m_wantsMelee = false;
    m_velocity = {0.0f, 0.0f};
    // Reset character state
    m_state = Character::State::Idle;
}

void Boss::TryJump() {
    if (!m_isOnGround) return;
    m_velocity.y = PLAYER_JUMP_FORCE * 0.85f; // boss nhảy thấp hơn player 1 chút
    m_isOnGround = false;
}

bool Boss::HasWallAhead(float dirX) const {
    if (!m_gameState) return false;
    float checkX = m_position.x + (dirX > 0 ? m_size.x + 8.0f : -8.0f);
    float checkY = m_position.y + m_size.y * 0.5f;
    return IsPointSolid({checkX, checkY});
}

bool Boss::HasGroundAhead(float dirX) const {
    if (!m_gameState) return true;
    float checkX = m_position.x + (dirX > 0 ? m_size.x + 16.0f : -16.0f);
    float checkY = m_position.y + m_size.y + 8.0f;
    return IsPointSolid({checkX, checkY});
}

void Boss::NavigateToPlayer(Vector2 playerPos, float deltaTime) {
    float dx = playerPos.x - (m_position.x + m_size.x * 0.5f);
    float dy = playerPos.y - (m_position.y + m_size.y * 0.5f);
    float dirX = (dx > 0) ? 1.0f : -1.0f;
    
    m_direction = (dx > 0) ? Direction::Right : Direction::Left;
    
    bool wallAhead = HasWallAhead(dirX);
    bool groundAhead = HasGroundAhead(dirX);
    
    if (wallAhead) {
        // Bị chặn ngang → thử nhảy nếu player ở trên
        if (dy < -TILE_SIZE * 0.5f) TryJump();
        // Nếu không nhảy được thì đứng im chờ
    } else if (!groundAhead) {
        // Không có ground phía trước → drop xuống (bước xuống cliff)
        // Chỉ bước xuống nếu player thực sự ở dưới
        if (dy > TILE_SIZE * 0.5f) MoveX(dirX, deltaTime);
        // Nếu player ngang hoặc trên → không nhảy xuống vực
    } else {
        // Đường thông → đi bình thường
        MoveX(dirX, deltaTime);
        // Nếu player ở platform cao hơn và gần tường → nhảy
        if (dy < -TILE_SIZE * 1.5f && m_isOnGround) TryJump();
    }
}

