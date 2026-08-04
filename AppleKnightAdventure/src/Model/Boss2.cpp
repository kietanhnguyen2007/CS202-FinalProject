#include "Model/Boss2.h"
#include "Model/GameState.h"
#include "Model/Projectile.h"
#include "Controller/GameController.h" // For spawning projectiles maybe? Wait, Boss needs to spawn projectiles via GameState
#include <cmath>
#include <algorithm>

Boss2::Boss2(Vector2 position, Vector2 size) 
    : Boss(position, size, 2)
    , m_telegraphSpawned(false)
{
    m_damage = 25; 
    m_cooldownTimer = 3.0f;
    m_attackRange = 600.0f; // Ranged boss
    m_detectionRange = 700.0f;
}

void Boss2::TransitionToNextPhase() {
    if (m_currentPhase == BossPhase::Phase1) {
        SetPhase(BossPhase::Phase2);
        ChangeState(BossState::Transition);
        m_cooldownTimer = 2.5f;
        m_activeTimer = 1.0f;
    } else if (m_currentPhase == BossPhase::Phase2) {
        SetPhase(BossPhase::Phase3);
        ChangeState(BossState::Transition);
        m_damage = 30;
        m_cooldownTimer = 2.0f;
        m_activeTimer = 1.0f;
    }
}

void Boss2::UpdateState(float deltaTime, Vector2 playerPos) {
    if (m_currentState == BossState::Die) return;

    // Check health threshold
    float hpRatio = (float)m_health / m_maxHealth;
    if (m_currentPhase == BossPhase::Phase1 && hpRatio <= 0.66f) {
        TransitionToNextPhase();
        return;
    }
    if (m_currentPhase == BossPhase::Phase2 && hpRatio <= 0.33f) {
        TransitionToNextPhase();
        return;
    }
    
    if (m_currentState == BossState::Transition) {
        m_activeTimer -= deltaTime;
        if (m_activeTimer <= 0.0f) {
            ChangeState(BossState::Idle);
        }
        return;
    }
    
    if (m_currentState == BossState::Hurt) {
        m_activeTimer -= deltaTime;
        if (m_activeTimer <= 0.0f) {
            ChangeState(BossState::Idle);
        }
        return;
    }

    // Skill 1: Projectile
    if (m_currentState == BossState::Skill1) {
        m_chargeTimer -= deltaTime;
        if (m_chargeTimer <= 0.0f && !m_skillFired) {
            m_skillFired = true;
            ExecuteProjectileAttack();
        }
        m_activeTimer -= deltaTime;
        if (m_activeTimer <= 0.0f) {
            m_cooldownTimer = (m_currentPhase == BossPhase::Phase1) ? 3.0f : (m_currentPhase == BossPhase::Phase2 ? 2.5f : 2.0f);
            ChangeState(BossState::Idle);
        }
        return;
    }

    // Skill 2: Healing
    if (m_currentState == BossState::Skill2) {
        m_chargeTimer -= deltaTime;
        if (m_chargeTimer <= 0.0f && !m_skillFired) {
            m_skillFired = true;
            ExecuteHealing();
        }
        m_activeTimer -= deltaTime;
        if (m_activeTimer <= 0.0f) {
            m_cooldownTimer = 10.0f;
            ChangeState(BossState::Idle);
        }
        return;
    }

    // Skill 3: Targeted AoE (Phase 3 only)
    if (m_currentState == BossState::Skill3) {
        m_chargeTimer -= deltaTime;
        if (m_chargeTimer <= 0.0f && !m_skillFired) {
            m_skillFired = true;
            ExecuteTargetedAoE(m_aoeTarget);
        }
        m_activeTimer -= deltaTime;
        if (m_activeTimer <= 0.0f) {
            m_cooldownTimer = 5.0f;
            ChangeState(BossState::Idle);
        }
        return;
    }
    
    // Idle / Walk logic
    float dx = playerPos.x - m_position.x;
    float dy = playerPos.y - m_position.y;
    float dist = std::sqrt(dx*dx + dy*dy);
    
    if (dist <= m_detectionRange) {
        m_direction = (dx > 0) ? Direction::Right : Direction::Left;
        
        if (m_cooldownTimer <= 0.0f) {
            // Decide skill
            if (m_currentPhase == BossPhase::Phase3 && (rand() % 2 == 0)) {
                // Try Targeted AoE
                // Check if player is in camera bounds and not solid
                // We don't have direct access to camera here, but we can assume GameState has map bounds. 
                // As per plan, AoE target must be non-solid.
                if (!IsPointSolid(playerPos)) {
                    ChangeState(BossState::Skill3);
                    m_chargeTimer = 1.0f;
                    m_activeTimer = m_chargeTimer + 0.5f;
                    m_aoeTarget = playerPos;
                    CheckAndSpawnTelegraph(m_aoeTarget);
                } else {
                    ChangeState(BossState::Skill1);
                    m_chargeTimer = 0.4f;
                    m_activeTimer = m_chargeTimer + 0.2f;
                }
            } else if ((m_currentPhase == BossPhase::Phase2 || m_currentPhase == BossPhase::Phase3) && hpRatio < 0.5f && (rand() % 3 == 0)) {
                ChangeState(BossState::Skill2); // Heal
                m_chargeTimer = 1.0f;
                m_activeTimer = m_chargeTimer + 0.5f;
            } else {
                ChangeState(BossState::Skill1); // Projectile
                m_chargeTimer = (m_currentPhase == BossPhase::Phase1) ? 0.6f : (m_currentPhase == BossPhase::Phase2 ? 0.5f : 0.4f);
                m_activeTimer = m_chargeTimer + 0.2f; 
            }
        } else {
            // Ranged boss tries to keep distance
            if (dist < 200.0f) {
                ChangeState(BossState::Walk);
                Vector2 dir = { (dx > 0) ? -1.0f : 1.0f, 0 }; // Run away
                Vector2 nextPos = {m_position.x + dir.x * m_speed * deltaTime, m_position.y};
                if (!IsPointSolid({nextPos.x + (dir.x > 0 ? m_size.x : 0), nextPos.y + m_size.y - 1})) {
                    Move(dir, deltaTime);
                }
            } else {
                ChangeState(BossState::Idle);
            }
        }
    } else {
        ChangeState(BossState::Idle);
    }
}

void Boss2::ExecuteProjectileAttack() {
    if (!m_gameState) return;
    
    Attack(); // Play attack animation
    
    Vector2 pSize = {16.0f, 16.0f};
    Vector2 spawnPos = {
        m_position.x + (m_direction == Direction::Right ? m_size.x : -pSize.x),
        m_position.y + m_size.y * 0.5f
    };
    
    auto proj = std::make_unique<Projectile>(spawnPos, pSize, ProjectileType::BossAttack, m_direction, m_damage, m_id);
    proj->SetVelocity({(m_direction == Direction::Right ? 300.0f : -300.0f), 0.0f});
    // Projectile lifetime logic should be added to Projectile class. Let's just pass it or set a custom type.
    // For now, it travels until solid tile overlap which is handled by GameController::UpdateProjectiles.
    
    m_gameState->AddEntity(std::move(proj));
}

void Boss2::ExecuteHealing() {
    m_health += 50;
    if (m_health > m_maxHealth) m_health = m_maxHealth;
    // We could emit a heal particle here via GameController or just rely on the GameController/View to notice the health change
}

void Boss2::ExecuteTargetedAoE(Vector2 playerPos) {
    if (!m_gameState) return;
    // Spawn an AoE damage volume
    // To simplify, we can just spawn an invisible projectile with 0 speed and large hitbox that lives for 0.5s
    Vector2 pSize = {64.0f, 64.0f};
    Vector2 spawnPos = {
        playerPos.x - pSize.x * 0.5f,
        playerPos.y - pSize.y * 0.5f
    };
    
    auto proj = std::make_unique<Projectile>(spawnPos, pSize, ProjectileType::BossAttack, Direction::None, 40, m_id);
    proj->SetVelocity({0.0f, 0.0f});
    // In Projectile logic, we can add a lifetime field.
    m_gameState->AddEntity(std::move(proj));
}

void Boss2::CheckAndSpawnTelegraph(Vector2 playerPos) {
    m_telegraphSpawned = true;
    // View layer will draw a warning circle at m_aoeTarget if m_currentState == Skill3 and m_chargeTimer > 0
}
