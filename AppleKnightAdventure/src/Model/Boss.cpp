#include "Model/Boss.h"
#include <cmath>

Boss::Boss()
    : Character(EntityType::Boss)
    , m_phase(BossPhase::Phase1)
    , m_damage(20)
    , m_detectionRange(400.0f)
    , m_attackRange(80.0f)
    , m_enrageThreshold(0.2f)
    , m_phaseTimer(0.0f)
{
    m_speed = BOSS_SPEED;
    m_maxHealth = BOSS_MAX_HEALTH;
    m_health = m_maxHealth;
    m_attackCooldown = BOSS_ATTACK_COOLDOWN;
    m_phaseOrder = {BossPhase::Phase1, BossPhase::Phase2, BossPhase::Phase3, BossPhase::Enraged};
}

Boss::Boss(Vector2 position, Vector2 size)
    : Character(position, size, EntityType::Boss)
    , m_phase(BossPhase::Phase1)
    , m_damage(20)
    , m_detectionRange(400.0f)
    , m_attackRange(80.0f)
    , m_enrageThreshold(0.2f)
    , m_phaseTimer(0.0f)
{
    m_speed = BOSS_SPEED;
    m_maxHealth = BOSS_MAX_HEALTH;
    m_health = m_maxHealth;
    m_attackCooldown = BOSS_ATTACK_COOLDOWN;
    m_phaseOrder = {BossPhase::Phase1, BossPhase::Phase2, BossPhase::Phase3, BossPhase::Enraged};
}

void Boss::Update(float deltaTime) {
    Character::Update(deltaTime);
    m_phaseTimer += deltaTime;
}


BossPhase Boss::GetPhase() const { return m_phase; }

void Boss::SetPhase(BossPhase phase) {
    m_phase = phase;
    OnPhaseEnter(phase);
}

int Boss::GetDamage() const { return m_damage; }
void Boss::SetDamage(int damage) { m_damage = damage; }

float Boss::GetDetectionRange() const { return m_detectionRange; }
void Boss::SetDetectionRange(float range) { m_detectionRange = range; }
float Boss::GetAttackRange() const { return m_attackRange; }
void Boss::SetAttackRange(float range) { m_attackRange = range; }

void Boss::TakeDamage(int damage) {
    m_health -= damage;
    if (m_health <= 0) {
        m_health = 0;
        m_active = false;
        return;
    }
    float healthPercent = static_cast<float>(m_health) / m_maxHealth;
    if (healthPercent <= m_enrageThreshold && m_phase != BossPhase::Enraged) {
        TransitionToNextPhase();
    }
}

void Boss::Attack() {
    Character::Attack();
}

void Boss::UpdateAI(Vector2 playerPosition, float deltaTime) {
    float dx = playerPosition.x - m_position.x;
    float dy = playerPosition.y - m_position.y;
    float dist = std::sqrt(dx * dx + dy * dy);

    if (dist <= m_detectionRange) {
        if (dist > m_attackRange) {
            if (dist > 0) {
                Vector2 dir = {dx / dist, dy / dist};
                Move(dir, deltaTime);
            }
        }
        if (dist <= m_attackRange && CanAttack()) {
            Attack();
        }
    }

    ExecutePhaseBehavior(deltaTime);
}

void Boss::TransitionToNextPhase() {
    int currentIndex = 0;
    for (size_t i = 0; i < m_phaseOrder.size(); ++i) {
        if (m_phaseOrder[i] == m_phase) {
            currentIndex = static_cast<int>(i);
            break;
        }
    }
    int nextIndex = std::min(currentIndex + 1, static_cast<int>(m_phaseOrder.size()) - 1);
    SetPhase(m_phaseOrder[nextIndex]);
}

bool Boss::IsEnraged() const { return m_phase == BossPhase::Enraged; }

void Boss::OnPhaseEnter(BossPhase newPhase) {
    m_phaseTimer = 0.0f;
    switch (newPhase) {
        case BossPhase::Phase2:
            m_damage += 10;
            m_speed *= 1.2f;
            break;
        case BossPhase::Phase3:
            m_damage += 15;
            m_attackCooldown *= 0.8f;
            break;
        case BossPhase::Enraged:
            m_damage *= 2;
            m_speed *= 1.5f;
            m_attackCooldown *= 0.6f;
            m_scale = 1.3f;
            break;
        default:
            break;
    }
}

void Boss::ExecutePhaseBehavior(float deltaTime) {
}
