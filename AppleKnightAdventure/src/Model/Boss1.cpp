#include "Model/Boss1.h"
#include <cmath>

namespace {
constexpr float ATTACK_DURATION = 0.8f;
constexpr float ATTACK_IMPACT_TIME = 0.55f;
constexpr float DASH_WINDUP_TIME = 0.3f;
constexpr float DASH_ACTIVE_TIME = ATTACK_DURATION - DASH_WINDUP_TIME;
constexpr float TRANSITION_DURATION = 1.5f;
}

Boss1::Boss1(Vector2 position, Vector2 size) 
    : Boss(position, size, 1) 
{
    m_damage = 20; // Phase 1 default
    m_cooldownTimer = 2.0f;
    m_attackRange = 210.0f; // scaled up: hitbox x1.75 * 1.7, easier to hit player
    m_maxHealth = 5000;
    m_health = m_maxHealth;
}

void Boss1::ChangeState(BossState newState) {
    Boss::ChangeState(newState);
    if (newState == BossState::Skill3) {
        m_state = Character::State::Walk; // Use walk animation for dodge/backstep
    }
}

void Boss1::TransitionToNextPhase() {
    if (m_currentPhase == BossPhase::Phase1) {
        SetPhase(BossPhase::Phase2);
        ChangeState(BossState::Transition);
        m_damage = 30; // Stronger phase 2
        m_cooldownTimer = 1.5f;
        m_activeTimer = TRANSITION_DURATION;
        m_health = m_maxHealth; // Refill health for Phase 2
    }
}

void Boss1::ResetToPhase1() {
    Boss::ResetToPhase1();
    m_damage = 20;
    m_cooldownTimer = 2.0f;
    m_attackRange = 87.5f;
}

void Boss1::UpdateState(float deltaTime, Vector2 playerPos) {
    if (m_currentState == BossState::Die) return;

    // Transition when health drops to 0 (clamped to 1 by Boss::TakeDamage)
    if (m_currentPhase == BossPhase::Phase1 && m_health <= 1) {
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

    // Removed Backstep logic

    if (m_currentState == BossState::Skill2) { // Dash Attack
        m_chargeTimer -= deltaTime;
        if (m_chargeTimer > 0.0f) {
            // Wind-up: slightly move backward
            Vector2 dir = { (m_direction == Direction::Right) ? -1.0f : 1.0f, 0 };
            MoveX(dir.x * 0.5f, deltaTime);
            return;
        }
        
        m_activeTimer -= deltaTime;
        Vector2 dir = { (m_direction == Direction::Right) ? 1.0f : -1.0f, 0 };
        MoveX(dir.x * 3.0f, deltaTime);
        
        // Execute attack if close during dash
        float curDist = std::abs(playerPos.x - m_position.x);
        if (curDist <= m_attackRange && !m_skillFired) {
            m_skillFired = true;
            ExecuteMeleeAttack(playerPos);
        }
        
        if (m_activeTimer <= 0.0f) {
            ChangeState(BossState::Idle);
            m_cooldownTimer = 1.5f;
        }
        return;
    }

    if (m_currentState == BossState::Skill1) { // Combo Melee
        m_chargeTimer -= deltaTime;
        if (m_chargeTimer <= 0.0f && !m_skillFired) {
            m_skillFired = true;
            ExecuteMeleeAttack(playerPos);
        }
        m_activeTimer -= deltaTime;
        if (m_activeTimer <= 0.0f) {
            if (m_currentPhase == BossPhase::Phase2 && m_comboStep < 2) { // 3-hit combo
                m_comboStep++;
                // Re-track player
                float dx = playerPos.x - m_position.x;
                m_direction = (dx > 0) ? Direction::Right : Direction::Left;
                
                ChangeState(BossState::Idle); // Reset triggers
                ChangeState(BossState::Skill1);
                m_chargeTimer = ATTACK_IMPACT_TIME;
                m_activeTimer = ATTACK_DURATION;
            } else {
                m_comboStep = 0;
                m_cooldownTimer = (m_currentPhase == BossPhase::Phase1) ? 2.0f : 1.5f;
                ChangeState(BossState::Idle);
            }
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
            if (dist > 300.0f && CheckLineOfSight(GetCenter(), playerPos)) {
                // Dash Attack
                ChangeState(BossState::Skill2);
                m_chargeTimer = DASH_WINDUP_TIME;
                m_activeTimer = DASH_ACTIVE_TIME;
                return;
            }
            if (dist <= m_attackRange) {
                if (CheckLineOfSight(GetCenter(), playerPos)) {
                    ChangeState(BossState::Skill1);
                    m_comboStep = 0;
                    m_chargeTimer = ATTACK_IMPACT_TIME;
                    m_activeTimer = ATTACK_DURATION;
                    return;
                }
            }
        }
        
        if (dist > m_attackRange) {
            ChangeState(BossState::Walk);
            NavigateToPlayer(playerPos, deltaTime);
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
        // The swing stays live briefly; GameController tests its hitbox against
        // the player each frame of that window.
        BeginMeleeSwing();
    }
}
