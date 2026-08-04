#include "Model/Boss1.h"
#include <cmath>

Boss1::Boss1(Vector2 position, Vector2 size) 
    : Boss(position, size, 1) 
{
    m_damage = 20; // Phase 1 default
    m_cooldownTimer = 2.0f;
}

void Boss1::TransitionToNextPhase() {
    if (m_currentPhase == BossPhase::Phase1) {
        SetPhase(BossPhase::Phase2);
        ChangeState(BossState::Transition);
        m_damage = 30; // Stronger phase 2
        m_cooldownTimer = 1.5f;
        m_activeTimer = 1.0f; // 1s transition animation
    } else {
        SetPhase(BossPhase::Enraged);
    }
}

void Boss1::UpdateState(float deltaTime, Vector2 playerPos) {
    if (m_currentState == BossState::Die) return;

    // Check health threshold for phase 2
    if (m_currentPhase == BossPhase::Phase1 && (float)m_health / m_maxHealth <= 0.5f) {
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
        m_activeTimer -= deltaTime; // Need to set this in TakeDamage or here? Wait, TakeDamage doesn't set it.
        // Actually, let's just let animation finish or set a fixed stun time
        if (m_activeTimer <= 0.0f) {
            // Let's rely on standard Character animation or just a 0.5s stun
            ChangeState(BossState::Idle);
        }
        return;
    }

    if (m_currentState == BossState::Skill1) {
        m_chargeTimer -= deltaTime;
        if (m_chargeTimer <= 0.0f && !m_skillFired) {
            m_skillFired = true;
            ExecuteMeleeAttack(playerPos);
        }
        m_activeTimer -= deltaTime;
        if (m_activeTimer <= 0.0f) {
            m_cooldownTimer = (m_currentPhase == BossPhase::Phase1) ? 2.0f : 1.5f;
            ChangeState(BossState::Idle);
        }
        return;
    }
    
    // Idle / Walk logic
    float dx = playerPos.x - m_position.x;
    float dy = playerPos.y - m_position.y;
    float dist = std::sqrt(dx*dx + dy*dy);
    
    if (dist <= m_detectionRange) {
        // Face player
        m_direction = (dx > 0) ? Direction::Right : Direction::Left;
        
        if (dist > m_attackRange) {
            ChangeState(BossState::Walk);
            Vector2 dir = { dx/dist, 0 }; // Ground boss, only move horizontally
            
            // Check ledge / wall? 
            Vector2 nextPos = {m_position.x + dir.x * m_speed * deltaTime, m_position.y};
            if (!IsPointSolid({nextPos.x + (dir.x > 0 ? m_size.x : 0), nextPos.y + m_size.y - 1})) {
                 Move(dir, deltaTime);
            }
        } else if (dist <= m_attackRange && m_cooldownTimer <= 0.0f) {
            if (CheckLineOfSight(GetCenter(), playerPos)) {
                ChangeState(BossState::Skill1);
                m_chargeTimer = (m_currentPhase == BossPhase::Phase1) ? 0.5f : 0.4f;
                m_activeTimer = m_chargeTimer + 0.2f; 
            } else {
                ChangeState(BossState::Idle);
            }
        } else {
            ChangeState(BossState::Idle);
        }
    } else {
        ChangeState(BossState::Idle);
    }
}

void Boss1::ExecuteMeleeAttack(Vector2 playerPos) {
    float dx = playerPos.x - m_position.x;
    float dy = playerPos.y - m_position.y;
    float dist = std::sqrt(dx*dx + dy*dy);
    
    if (dist <= m_attackRange) {
        Attack(); // Sets m_isAttacking = true which GameController reads
    }
}
