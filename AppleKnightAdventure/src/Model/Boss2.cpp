#include "Model/Boss2.h"
#include "Model/GameState.h"
#include "Model/Projectile.h"
#include "Controller/GameController.h" // For spawning projectiles maybe? Wait, Boss needs to spawn projectiles via GameState
#include "View/FloatingText.h"
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
        m_activeTimer = 3.0f;
        m_health = m_maxHealth;
    } else if (m_currentPhase == BossPhase::Phase2) {
        SetPhase(BossPhase::Phase3);
        ChangeState(BossState::Transition);
        m_damage = 30;
        m_cooldownTimer = 2.0f;
        m_activeTimer = 3.0f;
        m_health = m_maxHealth;
    }
}

void Boss2::UpdateState(float deltaTime, Vector2 playerPos) {
    if (m_currentState == BossState::Die) return;

    float hpRatio = (float)m_health / (float)m_maxHealth;

    // Transition when health drops to 0 (clamped to 1 by Boss::TakeDamage)
    if (m_currentPhase == BossPhase::Phase1 && m_health <= 1) {
        TransitionToNextPhase();
        return;
    }
    if (m_currentPhase == BossPhase::Phase2 && m_health <= 1) {
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

    // Skill 4: Teleport (Tactical Retreat)
    if (m_currentState == BossState::Skill4) {
        m_chargeTimer -= deltaTime;
        if (m_chargeTimer <= 0.0f && !m_skillFired) {
            m_skillFired = true;
            // Execute Teleport
            float teleportDist = 300.0f;
            Vector2 dir = { (m_direction == Direction::Right) ? -1.0f : 1.0f, 0 }; // Retreat
            Vector2 targetPos = {m_position.x + dir.x * teleportDist, m_position.y};
            
            // Scan backwards to find a valid teleport destination
            bool foundSpot = false;
            auto scanForSpot = [&](Vector2 scanDir) {
                Vector2 scanPos = {m_position.x + scanDir.x * teleportDist, m_position.y};
                while (std::abs(scanPos.x - m_position.x) > 10.0f) {
                    bool wallClear = CheckLineOfSight(GetCenter(), {scanPos.x + m_size.x/2, scanPos.y + m_size.y/2});
                    float floorCheckY = scanPos.y + m_size.y + 10.0f;
                    bool floorSolid = IsPointSolid({scanPos.x + m_size.x/2, floorCheckY});
                    
                    if (wallClear && floorSolid) {
                        targetPos = scanPos;
                        return true;
                    }
                    scanPos.x -= scanDir.x * 20.0f;
                }
                return false;
            };

            foundSpot = scanForSpot(dir);
            if (!foundSpot) {
                // If cornered, try teleporting to the OTHER side (past the player)
                foundSpot = scanForSpot({-dir.x, dir.y});
            }
            
            if (!foundSpot) {
                targetPos.x = m_position.x; // Don't teleport if no safe spot
            }

            // Keep teleport destination inside the map so the boss never escapes
            if (m_gameState) {
                float mapW = m_gameState->GetMapWidth() * TILE_SIZE;
                float mapH = m_gameState->GetMapHeight() * TILE_SIZE;
                float minX = 0.0f, minY = 0.0f;
                float maxX = mapW - m_size.x, maxY = mapH - m_size.y;
                if (maxX < minX) maxX = minX;
                if (maxY < minY) maxY = minY;
                if (targetPos.x < minX) targetPos.x = minX;
                else if (targetPos.x > maxX) targetPos.x = maxX;
                if (targetPos.y < minY) targetPos.y = minY;
                else if (targetPos.y > maxY) targetPos.y = maxY;
            }
            // Move instantly
            m_position.x = targetPos.x;
            m_position.y = targetPos.y;
        }
        
        m_activeTimer -= deltaTime;
        if (m_activeTimer <= 0.0f) {
            ChangeState(BossState::Idle);
            m_cooldownTimer = 0.0f; // Attack immediately after retreating
        }
        return;
    }

    // Skill 1: Projectile
    if (m_currentState == BossState::Skill1) {
        m_chargeTimer -= deltaTime;
        if (m_chargeTimer <= 0.0f && !m_skillFired) {
            m_skillFired = true;
            ExecuteProjectileAttack(); // Now shoots spread
        }
        m_activeTimer -= deltaTime;
        if (m_activeTimer <= 0.0f) {
            m_cooldownTimer = (m_currentPhase == BossPhase::Phase1) ? 2.5f : 1.5f;
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
            m_cooldownTimer = 10.0f; // Increased cooldown to avoid spam
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
            if (dist < 150.0f) { // Player too close, Teleport
                ChangeState(BossState::Skill4);
                m_chargeTimer = 0.3f; // Wind-up
                m_activeTimer = m_chargeTimer + 0.1f;
                return;
            }
            
            // Decide skill
            if (m_currentPhase == BossPhase::Phase3 && (rand() % 2 == 0)) {
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
            } else if ((m_currentPhase == BossPhase::Phase2 || m_currentPhase == BossPhase::Phase3) && hpRatio < 0.75f && (rand() % 2 == 0)) {
                ChangeState(BossState::Skill2); // Heal + Zoning
                m_chargeTimer = 1.0f;
                m_activeTimer = m_chargeTimer + 0.5f;
            } else {
                ChangeState(BossState::Skill1); // Spread Projectile
                m_chargeTimer = (m_currentPhase == BossPhase::Phase1) ? 0.6f : 0.4f;
                m_activeTimer = m_chargeTimer + 0.2f; 
            }
        } else {
            // Keep distance using walk if teleport is on CD
            if (dist < 200.0f) {
                ChangeState(BossState::Walk);
                float dirX = (dx > 0) ? -1.0f : 1.0f; // Run away
                
                // Cliff detection
                float checkX = m_position.x + (dirX > 0 ? m_size.x + 10.0f : -10.0f);
                float checkY = m_position.y + m_size.y + 10.0f; // 10 pixels below feet
                if (!IsPointSolid({checkX, checkY})) {
                    dirX = 0.0f; // Stop moving if there's no ground
                    ChangeState(BossState::Idle);
                }
                
                MoveX(dirX, deltaTime);
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
    
    Vector2 pSize = {64.0f, 64.0f};
    Vector2 spawnPos = {
        m_position.x + (m_direction == Direction::Right ? m_size.x : -pSize.x),
        m_position.y + (m_size.y - pSize.y) * 0.5f
    };
    
    auto proj1 = std::make_unique<Projectile>(spawnPos, pSize, ProjectileType::BossAttack, m_direction, m_damage, m_id);
    proj1->SetVelocity({(m_direction == Direction::Right ? 300.0f : -300.0f), 0.0f});
    
    // Spread 2 more projectiles for Hard Mode
    auto proj2 = std::make_unique<Projectile>(spawnPos, pSize, ProjectileType::BossAttack, m_direction, m_damage, m_id);
    proj2->SetVelocity({(m_direction == Direction::Right ? 300.0f : -300.0f), -50.0f});

    auto proj3 = std::make_unique<Projectile>(spawnPos, pSize, ProjectileType::BossAttack, m_direction, m_damage, m_id);
    proj3->SetVelocity({(m_direction == Direction::Right ? 300.0f : -300.0f), 50.0f});
    
    m_gameState->AddEntity(std::move(proj1));
    m_gameState->AddEntity(std::move(proj2));
    m_gameState->AddEntity(std::move(proj3));
}

void Boss2::ExecuteHealing() {
    if (!m_gameState) return;
    Attack(); // Play attack2 animation
    m_health += 50;
    if (m_health > m_maxHealth) m_health = m_maxHealth;
    
    // Emit floating text
    View::FloatingTextManager::GetInstance().Emit(
        {m_position.x + m_size.x * 0.5f, m_position.y}, "+50", GREEN, 1.0f);
    
    // Zoning Combo: Spawn AoE on self to prevent player melee interrupts
    Vector2 selfTarget = {m_position.x + m_size.x/2, m_position.y + m_size.y - 1};
    CheckAndSpawnTelegraph(selfTarget); // Spawn telegraph, actual explosion is synced with charge timer?
    // Note: Skill3 normally handles the explosion, but here we can just do a fake telegraph or immediate spawn.
    // For simplicity, we just heal and let telegraph play.
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
    proj->SetLifetime(0.5f); // Limit lifetime so it doesn't stay forever and cause crash
    m_gameState->AddEntity(std::move(proj));
}

void Boss2::CheckAndSpawnTelegraph(Vector2 playerPos) {
    m_telegraphSpawned = true;
    // View layer will draw a warning circle at m_aoeTarget if m_currentState == Skill3 and m_chargeTimer > 0
}
