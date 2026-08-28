#include "Model/Boss3.h"
#include "Model/GameState.h"
#include "Model/Projectile.h"
#include <cmath>
#include <algorithm>

namespace {
constexpr float ATTACK_DURATION = 0.8f;
constexpr float ATTACK_IMPACT_TIME = 0.55f;
constexpr float TRANSITION_DURATION = 1.5f;
constexpr float BLAST_WARNING_DURATION = 0.55f;
constexpr float BLAST_ACTIVE_DURATION = 0.8f;
constexpr float BEAM_CAST_TIME = 0.75f;
constexpr float BEAM_ACTIVE_DURATION = 1.8f;
constexpr float BLAST_ONLY_RANGE = 180.0f;
constexpr float ENERGY_SPHERE_COLLISION_SIZE = 32.0f;
constexpr float BEAM_HEIGHT = 96.0f;
}

Boss3::Boss3(Vector2 position, Vector2 size) 
    : Boss(position, size, 3) 
{
    m_damage = 30; // Base damage
    m_cooldownTimer = 1.5f;
    m_attackRange = 245.0f;
    m_detectionRange = 800.0f;
    m_maxHealth = 5000;
    m_health = m_maxHealth;
}

void Boss3::TransitionToNextPhase() {
    if (m_currentPhase == BossPhase::Phase1) {
        SetPhase(BossPhase::Phase2);
        ChangeState(BossState::Transition);
        m_damage = 40;
        m_cooldownTimer = 1.2f;
        m_activeTimer = TRANSITION_DURATION;
        m_health = m_maxHealth;
        m_superArmor = true;
    } else if (m_currentPhase == BossPhase::Phase2) {
        SetPhase(BossPhase::Phase3);
        ChangeState(BossState::Transition);
        m_damage = 45;
        m_cooldownTimer = 1.2f;
        m_activeTimer = TRANSITION_DURATION;
        m_health = m_maxHealth;
        m_superArmor = true;
    } else if (m_currentPhase == BossPhase::Phase3) {
        SetPhase(BossPhase::Phase4);
        ChangeState(BossState::Transition);
        m_damage = 50;
        m_cooldownTimer = 1.0f;
        m_activeTimer = TRANSITION_DURATION;
        m_health = m_maxHealth;
        m_superArmor = true;
    }
}

void Boss3::ResetToPhase1() {
    Boss::ResetToPhase1();
    m_damage = 30;
    m_cooldownTimer = 1.5f;
    m_attackRange = 122.5f;
    m_detectionRange = 800.0f;
    m_aoeTarget = {0.0f, 0.0f};
}

void Boss3::UpdateState(float deltaTime, Vector2 playerPos) {
    if (m_currentState == BossState::Die) return;

    // Check Phase Transitions when HP drops to 1
    if (m_currentPhase == BossPhase::Phase1 && m_health <= 1) {
        TransitionToNextPhase();
        return;
    }
    if (m_currentPhase == BossPhase::Phase2 && m_health <= 1) {
        TransitionToNextPhase();
        return;
    }
    if (m_currentPhase == BossPhase::Phase3 && m_health <= 1) {
        TransitionToNextPhase();
        return;
    }
    
    // Handling Transition Animation State
    if (m_currentState == BossState::Transition) {
        m_activeTimer -= deltaTime;
        if (m_activeTimer <= 0.0f) {
            ChangeState(BossState::Idle);
            m_superArmor = false;
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

    // Skill 1: Melee Attack (Phases 1, 2, 3, 4)
    if (m_currentState == BossState::Skill1) {
        m_chargeTimer -= deltaTime;
        if (m_chargeTimer <= 0.0f && !m_skillFired) {
            m_skillFired = true;
            ExecuteMeleeAttack(playerPos);
        }
        m_activeTimer -= deltaTime;
        if (m_activeTimer <= 0.0f) {
            m_cooldownTimer = (m_currentPhase == BossPhase::Phase1) ? 1.5f : 1.0f;
            ChangeState(BossState::Idle);
        }
        return;
    }

    // Skill 2: Energy Sphere (Phase 3) or Energy Blast (Phase 4 Attack 2)
    if (m_currentState == BossState::Skill2) {
        m_chargeTimer -= deltaTime;
        if (m_chargeTimer <= 0.0f && !m_skillFired) {
            m_skillFired = true;
            if (m_currentPhase == BossPhase::Phase4) {
                ExecuteEnergyBlast(m_aoeTarget);
            } else {
                ExecuteEnergySphere();
            }
        }
        m_activeTimer -= deltaTime;
        if (m_activeTimer <= 0.0f) {
            m_cooldownTimer = 2.0f;
            ChangeState(BossState::Idle);
        }
        return;
    }

    // Skill 3: Energy Beam (Phase 4 Attack 3)
    if (m_currentState == BossState::Skill3) {
        m_chargeTimer -= deltaTime;
        if (m_chargeTimer <= 0.0f && !m_skillFired) {
            m_skillFired = true;
            ExecuteEnergyBeam(playerPos);
        }
        m_activeTimer -= deltaTime;
        if (m_activeTimer <= 0.0f) {
            m_cooldownTimer = 3.0f;
            ChangeState(BossState::Idle);
        }
        return;
    }

    // AI Decision Logic per Phase
    float dx = playerPos.x - m_position.x;
    float dy = playerPos.y - m_position.y;
    float dist = std::sqrt(dx*dx + dy*dy);
    
    if (dist <= m_detectionRange) {
        m_direction = (dx > 0) ? Direction::Right : Direction::Left;
        
        if (m_cooldownTimer <= 0.0f) {
            if (m_currentPhase == BossPhase::Phase1 || m_currentPhase == BossPhase::Phase2) {
                // Phase 1 & Phase 2: Melee Only
                ChangeState(BossState::Skill1);
                m_chargeTimer = ATTACK_IMPACT_TIME;
                m_activeTimer = ATTACK_DURATION;
            } else if (m_currentPhase == BossPhase::Phase3) {
                // Phase 3: Attack 1 (Melee) or Attack 2 (Energy Sphere)
                if (rand() % 2 == 0 && dist > m_attackRange) {
                    ChangeState(BossState::Skill2); // Energy Sphere (attack_2.json)
                    m_chargeTimer = ATTACK_IMPACT_TIME;
                    m_activeTimer = ATTACK_DURATION;
                } else {
                    ChangeState(BossState::Skill1); // Melee (attack_1.json)
                    m_chargeTimer = ATTACK_IMPACT_TIME;
                    m_activeTimer = ATTACK_DURATION;
                }
            } else if (m_currentPhase == BossPhase::Phase4) {
                // Phase 4: Attack 1 (Melee + Ground Animate), Attack 2 (Energy Blast), Attack 3 (Energy Beam)
                if (dist <= 120.0f) {
                    // Tầm gần (Close range)
                    ChangeState(BossState::Skill1);
                    m_chargeTimer = ATTACK_IMPACT_TIME;
                    m_activeTimer = ATTACK_DURATION;
                } else if (dist <= BLAST_ONLY_RANGE) {
                    // Tầm trung (Mid range) - Chủ yếu dùng Attack 2
                    ChangeState(BossState::Skill2); // Energy Blast
                    m_chargeTimer = BLAST_WARNING_DURATION;
                    m_activeTimer = BLAST_WARNING_DURATION + BLAST_ACTIVE_DURATION;
                    m_aoeTarget = playerPos;
                    SpawnEnergyBlastTelegraph(m_aoeTarget);
                } else {
                    // Tầm xa (Long range) - Tỉ lệ giữa Attack 2 và Attack 3 (Attack 3 mạnh nên ra ít hơn)
                    int choice = rand() % 3; // 66% chance cho Attack 2, 33% cho Attack 3
                    if (choice < 2) {
                        ChangeState(BossState::Skill2);
                        m_chargeTimer = BLAST_WARNING_DURATION;
                        m_activeTimer = BLAST_WARNING_DURATION + BLAST_ACTIVE_DURATION;
                        m_aoeTarget = playerPos;
                        SpawnEnergyBlastTelegraph(m_aoeTarget);
                    } else {
                        ChangeState(BossState::Skill3); // Energy Beam
                        m_chargeTimer = BEAM_CAST_TIME;
                        m_activeTimer = BEAM_CAST_TIME + BEAM_ACTIVE_DURATION;
                    }
                }
            }
        } else {
            if (m_currentPhase == BossPhase::Phase4) {
                // Maintain a fixed distance for ranged attacks (~200px)
                float desiredDist = 200.0f;

                auto canMove = [&](float moveDir) -> bool {
                    if (!m_gameState) return true;
                    float checkX = m_position.x + (moveDir > 0 ? m_size.x : 0.0f) + moveDir * 32.0f;
                    float checkY = m_position.y + m_size.y * 0.5f; // Middle of boss
                    float pitCheckY = m_position.y + m_size.y + 16.0f; // Below feet
                    bool hasWall = false, hasGround = false;
                    float mapW = m_gameState->GetMapWidth() * 32.0f;
                    if (checkX < 0 || checkX > mapW) hasWall = true;

                    for (const auto& tile : m_gameState->GetTiles(MapLayer::Main)) {
                        if (!tile.solid) continue;
                        float tx = tile.x * 32.0f, ty = tile.y * 32.0f;
                        if (checkX >= tx && checkX <= tx + 32.0f && checkY >= ty && checkY <= ty + 32.0f) hasWall = true;
                        if (checkX >= tx && checkX <= tx + 32.0f && pitCheckY >= ty && pitCheckY <= ty + 32.0f) hasGround = true;
                    }
                    return !hasWall && hasGround;
                };

                if (dist > desiredDist + 20.0f) {
                    float moveDir = dx > 0 ? 1.0f : -1.0f;
                    if (!canMove(moveDir)) {
                        ChangeState(BossState::Idle);
                    } else {
                        ChangeState(BossState::Walk);
                        MoveX(moveDir, deltaTime);
                    }
                } else if (dist < desiredDist - 20.0f && dist > m_attackRange + 20.0f) {
                    // Back away to kite player
                    float moveDir = dx > 0 ? -1.0f : 1.0f;
                    if (!canMove(moveDir)) {
                        // Edge case: cannot move backwards (wall or void). Move forwards instead!
                        moveDir = -moveDir;
                        if (canMove(moveDir)) {
                            ChangeState(BossState::Walk);
                            MoveX(moveDir, deltaTime);
                        } else {
                            ChangeState(BossState::Idle);
                        }
                    } else {
                        ChangeState(BossState::Walk);
                        MoveX(moveDir, deltaTime);
                    }
                } else {
                    ChangeState(BossState::Idle);
                }

            } else {
                if (dist > m_attackRange) {
                    ChangeState(BossState::Walk);
                    NavigateToPlayer(playerPos, deltaTime);
                } else {
                    ChangeState(BossState::Idle);
                }
            }
        }
    } else {
        ChangeState(BossState::Idle);
    }
}

void Boss3::ExecuteMeleeAttack(Vector2 playerPos) {
    Attack();
    BeginMeleeSwing();

    // Phase 4 Attack 1: Melee + Ground Animate placed slightly offset towards facing direction
    if (m_currentPhase == BossPhase::Phase4 && m_gameState) {
        Vector2 gSize = {200.0f, 100.0f};
        float scaledWidth = m_size.x * m_scale;
        float scaledHeight = m_size.y * m_scale;
        float offsetX = (m_direction == Direction::Right) ? scaledWidth * 0.8f : -gSize.x * 0.8f;
        Vector2 spawnPos = { m_position.x + offsetX, m_position.y + scaledHeight - gSize.y };
        
        auto proj = std::make_unique<Projectile>(spawnPos, gSize, ProjectileType::BossAttack, Direction::None, m_damage, m_id);
        proj->SetVelocity({0.0f, 0.0f});
        proj->SetLifetime(BLAST_ACTIVE_DURATION);
        proj->SetSubType(4); // Ground Animate
        m_gameState->AddEntity(std::move(proj));
    }
}

void Boss3::ExecuteEnergySphere() {
    if (!m_gameState) return;
    Attack(); // Play attack_2 animation for Phase 3
    // Keep this below the boss body height so the moving projectile cannot
    // overlap the floor and despawn on the frame it is created. Its padded
    // atlas receives a separate, larger visual footprint in EntityRenderer.
    Vector2 pSize = {ENERGY_SPHERE_COLLISION_SIZE, ENERGY_SPHERE_COLLISION_SIZE};
    
    float scaledWidth = m_size.x * m_scale;
    float scaledHeight = m_size.y * m_scale;
    float spawnY = m_position.y + (scaledHeight - pSize.y) * 0.5f;
    
    Vector2 spawnPos = { 
        m_position.x + (m_direction == Direction::Right ? scaledWidth : -pSize.x), 
        spawnY
    };
    auto proj = std::make_unique<Projectile>(spawnPos, pSize, ProjectileType::BossAttack, m_direction, m_damage, m_id);
    proj->SetVelocity({(m_direction == Direction::Right ? 350.0f : -350.0f), 0.0f});
    proj->SetLifetime(2.5f);
    proj->SetSubType(1); // Sphere
    m_gameState->AddEntity(std::move(proj));
}

void Boss3::ExecuteGroundSmash(Vector2 playerPos) {
    ExecuteMeleeAttack(playerPos);
}

void Boss3::ExecuteEnergyBlast(Vector2 playerPos) {
    if (!m_gameState) return;
    Vector2 pSize = {128.0f, 128.0f};
    Vector2 spawnPos = { playerPos.x - pSize.x * 0.5f, playerPos.y - pSize.y * 0.7f };
    
    auto proj = std::make_unique<Projectile>(spawnPos, pSize, ProjectileType::BossAttack, Direction::None, m_damage + 10, m_id);
    proj->SetVelocity({0.0f, 0.0f});
    proj->SetLifetime(BLAST_ACTIVE_DURATION);
    proj->SetSubType(2); // Blast
    m_gameState->AddEntity(std::move(proj));
}

void Boss3::SpawnEnergyBlastTelegraph(Vector2 playerPos) {
    if (!m_gameState) return;
    Vector2 pSize = {128.0f, 128.0f};
    Vector2 spawnPos = {playerPos.x - pSize.x * 0.5f, playerPos.y - pSize.y * 0.7f};
    auto warning = std::make_unique<Projectile>(
        spawnPos, pSize, ProjectileType::BossAttack, Direction::None, 0, m_id);
    warning->SetVelocity({0.0f, 0.0f});
    warning->SetLifetime(BLAST_WARNING_DURATION);
    warning->SetSubType(5);
    m_gameState->AddEntity(std::move(warning));
}

void Boss3::ExecuteEnergyBeam(Vector2 playerPos) {
    if (!m_gameState) return;
    (void)playerPos;
    
    float scaledWidth = m_size.x * m_scale;
    float scaledHeight = m_size.y * m_scale;
    float startX = m_position.x + (m_direction == Direction::Right ? scaledWidth : 0.0f);
    
    // Clip the beam at the first solid tile so it never damages through walls.
    const float beamDirection = (m_direction == Direction::Right) ? 1.0f : -1.0f;
    const float beamCenterY = m_position.y + scaledHeight * 0.45f;
    float beamLength = 1000.0f;
    for (float distance = 16.0f; distance <= beamLength; distance += 16.0f) {
        if (IsPointSolid({startX + beamDirection * distance, beamCenterY})) {
            beamLength = std::max(32.0f, distance - 16.0f);
            break;
        }
    }
    Vector2 pSize = {beamLength, BEAM_HEIGHT};
    
    Vector2 spawnPos;
    if (m_direction == Direction::Left) {
        spawnPos = {startX - beamLength, beamCenterY - pSize.y * 0.5f};
    } else {
        spawnPos = {startX, beamCenterY - pSize.y * 0.5f};
    }
    
    auto proj = std::make_unique<Projectile>(spawnPos, pSize, ProjectileType::BossAttack, m_direction, m_damage + 15, m_id);
    proj->SetVelocity({0.0f, 0.0f}); // Stationary beam
    proj->SetLifetime(BEAM_ACTIVE_DURATION);
    proj->SetSubType(3); // Beam
    m_gameState->AddEntity(std::move(proj));
}
