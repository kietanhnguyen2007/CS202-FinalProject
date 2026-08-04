#include "Model/Boss3.h"
#include "Model/GameState.h"
#include "Model/Projectile.h"
#include <cmath>

Boss3::Boss3(Vector2 position, Vector2 size) 
    : Boss(position, size, 3) 
{
    m_damage = 30; // Phase 1
    m_cooldownTimer = 2.0f;
    m_attackRange = 100.0f;
    m_detectionRange = 800.0f;
    m_maxHealth = 2000;
    m_health = m_maxHealth;
}

void Boss3::TransitionToNextPhase() {
    if (m_currentPhase == BossPhase::Phase1) {
        SetPhase(BossPhase::Phase2);
        ChangeState(BossState::Transition);
        m_damage = 40;
        m_cooldownTimer = 1.5f;
        m_activeTimer = 1.0f;
    } else if (m_currentPhase == BossPhase::Phase2) {
        SetPhase(BossPhase::Phase3);
        ChangeState(BossState::Transition);
        m_activeTimer = 1.0f;
    } else if (m_currentPhase == BossPhase::Phase3) {
        SetPhase(BossPhase::Phase4);
        ChangeState(BossState::Transition);
        m_activeTimer = 1.0f;
    }
}

void Boss3::UpdateState(float deltaTime, Vector2 playerPos) {
    if (m_currentState == BossState::Die) return;

    float hpRatio = (float)m_health / m_maxHealth;
    if (m_currentPhase == BossPhase::Phase1 && hpRatio <= 0.75f) {
        TransitionToNextPhase();
        return;
    }
    if (m_currentPhase == BossPhase::Phase2 && hpRatio <= 0.50f) {
        TransitionToNextPhase();
        return;
    }
    if (m_currentPhase == BossPhase::Phase3 && hpRatio <= 0.25f) {
        TransitionToNextPhase();
        return;
    }
    
    if (m_currentState == BossState::Transition || m_currentState == BossState::Hurt) {
        m_activeTimer -= deltaTime;
        if (m_activeTimer <= 0.0f) {
            ChangeState(BossState::Idle);
        }
        return;
    }

    // Skill 1: Melee
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

    // Skill 2: Energy Sphere / Smash
    if (m_currentState == BossState::Skill2) {
        m_chargeTimer -= deltaTime;
        if (m_chargeTimer <= 0.0f && !m_skillFired) {
            m_skillFired = true;
            if (m_currentPhase == BossPhase::Phase4) ExecuteGroundSmash(playerPos);
            else ExecuteEnergySphere();
        }
        m_activeTimer -= deltaTime;
        if (m_activeTimer <= 0.0f) {
            m_cooldownTimer = 4.0f;
            ChangeState(BossState::Idle);
        }
        return;
    }

    // Skill 3: Energy Blast (AoE)
    if (m_currentState == BossState::Skill3) {
        m_chargeTimer -= deltaTime;
        if (m_chargeTimer <= 0.0f && !m_skillFired) {
            m_skillFired = true;
            ExecuteEnergyBlast(m_aoeTarget);
        }
        m_activeTimer -= deltaTime;
        if (m_activeTimer <= 0.0f) {
            m_cooldownTimer = 6.0f;
            ChangeState(BossState::Idle);
        }
        return;
    }

    // Skill 4: Energy Beam
    if (m_currentState == BossState::Skill4) {
        m_chargeTimer -= deltaTime;
        if (m_chargeTimer <= 0.0f && !m_skillFired) {
            m_skillFired = true;
            // Beam starts firing!
        }
        if (m_chargeTimer <= 0.0f) {
            ExecuteEnergyBeam(playerPos);
        }
        
        m_activeTimer -= deltaTime;
        if (m_activeTimer <= 0.0f) {
            m_cooldownTimer = 8.0f;
            ChangeState(BossState::Idle);
        }
        return;
    }

    // AI Logic
    float dx = playerPos.x - m_position.x;
    float dy = playerPos.y - m_position.y;
    float dist = std::sqrt(dx*dx + dy*dy);
    
    if (dist <= m_detectionRange) {
        m_direction = (dx > 0) ? Direction::Right : Direction::Left;
        
        if (m_cooldownTimer <= 0.0f) {
            if (m_currentPhase == BossPhase::Phase4) {
                int r = rand() % 4;
                if (r == 0 && dist <= 150.0f) {
                    ChangeState(BossState::Skill2); // Smash
                    m_chargeTimer = 1.0f;
                    m_activeTimer = m_chargeTimer + 0.3f;
                } else if (r == 1) {
                    ChangeState(BossState::Skill3); // Blast
                    m_chargeTimer = 1.2f;
                    m_activeTimer = m_chargeTimer + 0.5f;
                    m_aoeTarget = playerPos;
                } else if (r == 2) {
                    ChangeState(BossState::Skill4); // Beam
                    m_chargeTimer = 1.5f;
                    m_activeTimer = m_chargeTimer + 2.0f; // 2s active
                } else {
                    ChangeState(BossState::Skill1); // Melee
                    m_chargeTimer = 0.4f;
                    m_activeTimer = m_chargeTimer + 0.2f;
                }
            } else if (m_currentPhase == BossPhase::Phase3) {
                if (rand() % 2 == 0) {
                    ChangeState(BossState::Skill2); // Sphere
                    m_chargeTimer = 1.0f;
                    m_activeTimer = m_chargeTimer + 0.5f;
                } else {
                    ChangeState(BossState::Skill1); // Melee
                    m_chargeTimer = 0.4f;
                    m_activeTimer = m_chargeTimer + 0.2f;
                }
            } else {
                ChangeState(BossState::Skill1); // Melee
                m_chargeTimer = (m_currentPhase == BossPhase::Phase1) ? 0.5f : 0.4f;
                m_activeTimer = m_chargeTimer + 0.2f;
            }
        } else {
            if (dist > m_attackRange) {
                ChangeState(BossState::Walk);
                Vector2 dir = { dx/dist, 0 };
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

void Boss3::ExecuteMeleeAttack(Vector2 playerPos) {
    if (Distance(GetCenter(), playerPos) <= m_attackRange) {
        Attack();
    }
}

void Boss3::ExecuteEnergySphere() {
    if (!m_gameState) return;
    Attack(); 
    Vector2 pSize = {32.0f, 32.0f};
    Vector2 spawnPos = { m_position.x + (m_direction == Direction::Right ? m_size.x : -pSize.x), m_position.y + m_size.y * 0.5f };
    auto proj = std::make_unique<Projectile>(spawnPos, pSize, ProjectileType::BossAttack, m_direction, 45, m_id);
    proj->SetVelocity({(m_direction == Direction::Right ? 150.0f : -150.0f), 0.0f});
    m_gameState->AddEntity(std::move(proj));
}

void Boss3::ExecuteGroundSmash(Vector2 playerPos) {
    Attack();
    if (m_gameState) {
        auto proj = std::make_unique<Projectile>(m_position, Vector2{200.0f, 100.0f}, ProjectileType::BossAttack, Direction::None, 50, m_id);
        proj->SetVelocity({0.0f, 0.0f});
        m_gameState->AddEntity(std::move(proj));
    }
}

void Boss3::ExecuteEnergyBlast(Vector2 playerPos) {
    if (!m_gameState) return;
    auto proj = std::make_unique<Projectile>(Vector2{m_aoeTarget.x - 64.0f, m_aoeTarget.y - 64.0f}, Vector2{128.0f, 128.0f}, ProjectileType::BossAttack, Direction::None, 60, m_id);
    proj->SetVelocity({0.0f, 0.0f});
    m_gameState->AddEntity(std::move(proj));
}

void Boss3::ExecuteEnergyBeam(Vector2 playerPos) {
    if (!m_gameState) return;
    float beamLength = 800.0f;
    float startX = m_position.x + (m_direction == Direction::Right ? m_size.x : 0.0f);
    float cx = startX;
    
    float step = 16.0f;
    float dirX = (m_direction == Direction::Right) ? 1.0f : -1.0f;
    for (float d = 0; d < beamLength; d += step) {
        if (IsPointSolid({cx, m_position.y + m_size.y * 0.5f})) {
            beamLength = d;
            break;
        }
        cx += dirX * step;
    }
    
    Vector2 pSize = {beamLength, 32.0f};
    Vector2 spawnPos = { (m_direction == Direction::Right) ? startX : startX - beamLength, m_position.y + m_size.y * 0.5f - 16.0f };
    
    auto proj = std::make_unique<Projectile>(spawnPos, pSize, ProjectileType::BossAttack, Direction::None, 2, m_id);
    proj->SetVelocity({0.0f, 0.0f});
    m_gameState->AddEntity(std::move(proj));
}
