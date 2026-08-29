#include "Model/Boss.h"
#include "Model/GameState.h"
#include <cmath>
#include <algorithm>
#include <unordered_set>
#include <vector>

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

    if (m_jumpCooldown > 0.0f) {
        m_jumpCooldown -= deltaTime;
    }

    if (m_meleeWindow > 0.0f) {
        m_meleeWindow -= deltaTime;
        if (m_meleeWindow <= 0.0f) {
            m_meleeWindow = 0.0f;
            m_wantsMelee = false;
            m_meleeHitIds.clear();
        }
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
    
    // Stop horizontal movement when not walking -- but never mid-air, or a boss
    // that changes state during a leap drops straight into the gap it is crossing.
    if (newState != BossState::Walk && m_isOnGround) {
        m_velocity.x = 0.0f;
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

    // Reachability, stuck detection and the retreat timer are refreshed before
    // the state machine runs, so UpdateState and NavigateToPlayer both act on
    // this frame's answer.
    UpdateNavigation(playerPosition, deltaTime);

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
    return m_gameState->IsSolidAt(static_cast<int>(std::floor(point.x / TILE_SIZE)),
                                  static_cast<int>(std::floor(point.y / TILE_SIZE)));
}

// ---------------------------------------------------------------------------
// Navigation brain
// ---------------------------------------------------------------------------

int Boss::FootprintWidth() const {
    return std::max(1, static_cast<int>(std::ceil(m_size.x / TILE_SIZE)));
}

int Boss::FootprintHeight() const {
    return std::max(1, static_cast<int>(std::ceil(m_size.y / TILE_SIZE)));
}

bool Boss::BodyFitsAt(int tx, int ty) const {
    if (!m_gameState) return false;
    const int w = FootprintWidth();
    const int h = FootprintHeight();
    if (tx < 0 || ty < 0) return false;
    if (ty >= m_gameState->GetMapHeight() + 2) return false;
    for (int y = ty - h + 1; y <= ty; ++y) {
        if (y < 0) return false;
        for (int x = tx; x < tx + w; ++x) {
            if (m_gameState->IsSolidAt(x, y)) return false;
        }
    }
    return true;
}

bool Boss::IsStandable(int tx, int ty) const {
    if (!BodyFitsAt(tx, ty)) return false;
    // Something solid under at least part of the footprint to stand on.
    const int w = FootprintWidth();
    for (int x = tx; x < tx + w; ++x) {
        if (m_gameState->IsSolidAt(x, ty + 1)) return true;
    }
    return false;
}

bool Boss::CanReachPlayer(Vector2 playerPos) const {
    // With no map knowledge, assume the player is reachable: a boss that gives
    // up because it cannot see the tiles is far worse than one that walks into
    // a wall.
    if (!m_gameState) return true;

    const int w = FootprintWidth();
    // A jump clears v^2 / 2g -- about three tiles at the boss's jump strength.
    // Kept conservative so the boss never commits to a climb it cannot make.
    constexpr int JUMP_TILES = 3;
    constexpr int FALL_TILES = 10;
    constexpr int MAX_NODES  = 2400;   // bounds the fill on a large arena

    const int startX = static_cast<int>(std::floor(m_position.x / TILE_SIZE));
    const int startY = static_cast<int>(
        std::floor((m_position.y + m_size.y - 1.0f) / TILE_SIZE));

    const int playerTileX = static_cast<int>(std::floor(playerPos.x / TILE_SIZE));
    const int playerTileY = static_cast<int>(std::floor(playerPos.y / TILE_SIZE));

    // A cell counts as "engaged" when the boss standing there would have the
    // player inside its swing and an unobstructed line to them. Standing three
    // tiles away through a wall is not reaching them.
    const int reachTilesX = std::max(
        1, static_cast<int>((m_attackRange * BOSS_MELEE_REACH_RATIO) / TILE_SIZE));
    auto engagesFrom = [&](int tx, int ty) {
        if (std::abs(tx - playerTileX) > reachTilesX + w) return false;
        if (std::abs(ty - playerTileY) > 2) return false;
        const Vector2 from = { (tx + w * 0.5f) * TILE_SIZE,
                               (ty + 0.5f) * TILE_SIZE - m_size.y * 0.5f };
        return CheckLineOfSight(from, playerPos);
    };

    if (engagesFrom(startX, startY)) return true;

    // Breadth-first over standable cells, with the same movement rules the boss
    // actually has: walk sideways, jump up to JUMP_TILES, drop up to FALL_TILES.
    auto key = [](int x, int y) { return static_cast<long long>(x) * 100000LL + y; };
    std::unordered_set<long long> visited;
    visited.reserve(MAX_NODES);

    std::vector<std::pair<int, int>> queue;
    queue.reserve(MAX_NODES);
    queue.push_back({startX, startY});
    visited.insert(key(startX, startY));

    for (size_t head = 0; head < queue.size(); ++head) {
        // Ran out of budget before proving anything. Assume the player is
        // reachable: a boss that keeps chasing on an inconclusive answer is far
        // better than one that turns and flees for no reason the player can see.
        if (queue.size() >= static_cast<size_t>(MAX_NODES)) return true;

        const int cx = queue[head].first;
        const int cy = queue[head].second;

        auto push = [&](int nx, int ny) -> bool {
            if (!visited.insert(key(nx, ny)).second) return false;
            if (!IsStandable(nx, ny)) return false;
            if (engagesFrom(nx, ny)) return true;   // done
            queue.push_back({nx, ny});
            return false;
        };

        for (int dir = -1; dir <= 1; dir += 2) {
            const int nx = cx + dir;

            // Step or walk across at the same height.
            if (push(nx, cy)) return true;

            // Climb: the body has to clear the column above it as it rises.
            for (int up = 1; up <= JUMP_TILES; ++up) {
                if (!BodyFitsAt(cx, cy - up)) break;   // ceiling, stop climbing
                if (push(nx, cy - up)) return true;
            }

            // Drop: the column below the destination has to be open to fall through.
            for (int down = 1; down <= FALL_TILES; ++down) {
                if (!BodyFitsAt(nx, cy + down)) break;
                if (push(nx, cy + down)) return true;
            }
        }
    }

    return false;
}

void Boss::RetreatFromPlayer(Vector2 playerPos, float deltaTime) {
    // Move away from the player, but never off a ledge or into a wall -- a boss
    // that kills itself running away is worse than one that stands there.
    const float dx = playerPos.x - (m_position.x + m_size.x * 0.5f);
    float dirX = (dx > 0.0f) ? -1.0f : 1.0f;

    if (HasWallAhead(dirX) || !HasGroundAhead(dirX)) {
        dirX = -dirX;
        if (HasWallAhead(dirX) || !HasGroundAhead(dirX)) {
            // Cornered with nowhere to run. Face the player and hold; the
            // reachability check will release the retreat once they come out.
            m_velocity.x = 0.0f;
            m_direction = (dx > 0.0f) ? Direction::Right : Direction::Left;
            return;
        }
    }

    // Keep facing the player while backing off, so a ranged boss can still
    // answer the moment it gets a line on them.
    m_direction = (dx > 0.0f) ? Direction::Right : Direction::Left;
    m_velocity.x = dirX * EffectiveSpeed();
}

void Boss::UpdateNavigation(Vector2 playerPos, float deltaTime) {
    if (m_unstickTimer > 0.0f) m_unstickTimer -= deltaTime;
    if (m_retreatTimer > 0.0f) m_retreatTimer -= deltaTime;

    // Stuck detection. Only meaningful on the ground while the boss is asking
    // to move: mid-air and mid-attack it is standing still on purpose.
    const bool wantsToMove = (m_currentState == BossState::Walk);
    if (wantsToMove && m_isOnGround) {
        const float moved = Distance(m_position, m_lastNavPos);
        if (moved < BOSS_STUCK_DISTANCE) {
            m_stuckTimer += deltaTime;
        } else {
            m_stuckTimer = 0.0f;
        }
    } else {
        m_stuckTimer = 0.0f;
    }
    m_lastNavPos = m_position;

    if (m_stuckTimer >= BOSS_STUCK_TIME && m_unstickTimer <= 0.0f) {
        // Wedged. Commit to a shove away from whatever is blocking us, for long
        // enough to actually clear it -- without the commitment the boss turns
        // back toward the player next frame and re-wedges immediately.
        const float towardPlayer =
            (playerPos.x > m_position.x + m_size.x * 0.5f) ? 1.0f : -1.0f;
        float away = -towardPlayer;
        if (HasWallAhead(away) || !HasGroundAhead(away)) away = towardPlayer;
        m_unstickDirX = away;
        m_unstickTimer = BOSS_UNSTICK_TIME;
        m_stuckTimer = 0.0f;
        TryJump(away);   // most wedges are a ledge lip a hop clears
    }

    // Reachability is the expensive part, so it is answered a few times a
    // second rather than every frame.
    // Mid-air the boss's own cell is not standable and the fill would read as
    // "nowhere to go", so the last answer is held until it lands.
    m_navRecheckTimer -= deltaTime;
    if (m_navRecheckTimer <= 0.0f && m_isOnGround) {
        m_navRecheckTimer = BOSS_NAV_RECHECK_INTERVAL;

        const bool canShoot = HasRangedAttack()
            && Distance(GetCenter(), playerPos) <= m_detectionRange
            && CheckLineOfSight(GetCenter(), playerPos);
        m_playerReachable = canShoot || CanReachPlayer(playerPos);

        if (!m_playerReachable) {
            // Player is holed up somewhere the boss cannot follow or hit. Stop
            // feeding them free damage and back out of their range.
            m_retreatTimer = BOSS_RETREAT_TIME;
        } else if (m_retreatTimer > 0.0f) {
            // They came back out -- drop the retreat and re-engage at once.
            m_retreatTimer = 0.0f;
        }
    }
}

void Boss::ResetToPhase1() {
    m_active = true;
    m_currentPhase = BossPhase::Phase1;
    m_currentState = BossState::Idle;
    m_health = m_maxHealth;
    m_phaseTimer = 0.0f;
    m_chargeTimer = 0.0f;
    m_activeTimer = 0.0f;
    m_cooldownTimer = 2.0f;
    m_skillFired = false;
    m_superArmor = false;
    m_comboStep = 0;
    m_wantsMelee = false;
    m_meleeWindow = 0.0f;
    m_meleeHitIds.clear();
    m_jumpCooldown = 0.0f;
    m_recentDamage = 0;
    m_damageTimer = 0.0f;
    m_velocity = {0.0f, 0.0f};
    // Navigation brain back to a clean slate, or a boss respawned into a fresh
    // arena inherits a retreat decision made about the old one.
    m_playerReachable = true;
    m_navRecheckTimer = 0.0f;
    m_lastNavPos = m_position;
    m_stuckTimer = 0.0f;
    m_unstickTimer = 0.0f;
    m_unstickDirX = 0.0f;
    m_retreatTimer = 0.0f;
    // Reset character state
    m_state = Character::State::Idle;
}

void Boss::BeginMeleeSwing(float window) {
    m_wantsMelee = true;
    m_meleeWindow = window;
    m_meleeHitIds.clear();
}

bool Boss::HasMeleeHit(int targetId) const {
    return std::find(m_meleeHitIds.begin(), m_meleeHitIds.end(), targetId) != m_meleeHitIds.end();
}

Rectangle Boss::GetMeleeHitBox() const {
    const Rectangle body = GetBoundingBox();
    // m_attackRange is the range the AI engages at, measured center-to-center.
    // The swing itself covers a shorter, body-relative distance in front.
    float reach = m_attackRange * BOSS_MELEE_REACH_RATIO;
    reach = std::clamp(reach, body.width, body.width * 3.0f);

    // A little vertical slack so a player on a ledge or mid-jump beside the
    // boss is still inside the swing.
    const float pad = body.height * 0.2f;
    Rectangle box;
    box.y = body.y - pad;
    box.height = body.height + pad * 2.0f;
    box.width = body.width + reach;
    box.x = (m_direction == Direction::Left) ? body.x - reach : body.x;
    return box;
}

void Boss::TryJump(float forwardDirX, float strength) {
    if (!m_isOnGround || m_jumpCooldown > 0.0f) return;
    m_velocity.y = PLAYER_JUMP_FORCE * 0.85f * strength;
    // Carry horizontal speed into the leap; without it the boss hops in place
    // and can never cross a gap or reach a ledge beside it.
    if (forwardDirX != 0.0f) {
        m_velocity.x = forwardDirX * EffectiveSpeed() * BOSS_CHASE_SPEED_MULT;
    }
    m_isOnGround = false;
    m_jumpCooldown = BOSS_JUMP_COOLDOWN;
}

bool Boss::HasWallAhead(float dirX) const {
    if (!m_gameState) return false;
    float checkX = m_position.x + (dirX > 0 ? m_size.x + 8.0f : -8.0f);
    // Probe at chest and at shin height: a one-tile step should not read as a
    // wall the boss has to jump over, but a real wall should.
    return IsPointSolid({checkX, m_position.y + m_size.y * 0.35f})
        || IsPointSolid({checkX, m_position.y + m_size.y * 0.75f});
}

bool Boss::HasGroundAhead(float dirX) const {
    if (!m_gameState) return true;
    float checkX = m_position.x + (dirX > 0 ? m_size.x + 16.0f : -16.0f);
    float checkY = m_position.y + m_size.y + 8.0f;
    return IsPointSolid({checkX, checkY});
}

bool Boss::CanLeapGap(float dirX) const {
    if (!m_gameState) return false;
    // Scan forward for a landing surface within the distance a jump covers.
    const float footY = m_position.y + m_size.y + 8.0f;
    const float edgeX = m_position.x + (dirX > 0 ? m_size.x : 0.0f);
    const float maxLeap = TILE_SIZE * 4.0f;
    for (float d = TILE_SIZE * 0.5f; d <= maxLeap; d += TILE_SIZE * 0.5f) {
        if (IsPointSolid({edgeX + dirX * d, footY})) return true;
    }
    return false;
}

void Boss::NavigateToPlayer(Vector2 playerPos, float deltaTime) {
    // Committed to shoving out of a wedge: ignore the player until it clears,
    // otherwise the boss turns straight back into the corner it just left.
    if (m_unstickTimer > 0.0f) {
        m_direction = (m_unstickDirX > 0.0f) ? Direction::Right : Direction::Left;
        if (HasWallAhead(m_unstickDirX)) TryJump(m_unstickDirX);
        MoveX(m_unstickDirX * BOSS_CHASE_SPEED_MULT, deltaTime);
        return;
    }

    // Player is somewhere the boss cannot follow or shoot. Back off instead of
    // grinding against the wall taking free hits.
    if (m_retreatTimer > 0.0f) {
        RetreatFromPlayer(playerPos, deltaTime);
        return;
    }

    const float dx = playerPos.x - (m_position.x + m_size.x * 0.5f);
    const float dy = playerPos.y - (m_position.y + m_size.y * 0.5f);
    const float dirX = (dx > 0) ? 1.0f : -1.0f;

    m_direction = (dx > 0) ? Direction::Right : Direction::Left;

    // Airborne: keep steering toward the player. The ground probes below are
    // meaningless mid-jump, and without air control the boss loses all
    // horizontal speed the moment it leaves the ground.
    if (!m_isOnGround) {
        m_velocity.x = dirX * EffectiveSpeed() * BOSS_CHASE_SPEED_MULT;
        return;
    }

    const bool wallAhead = HasWallAhead(dirX);
    const bool groundAhead = HasGroundAhead(dirX);
    const bool playerAbove = dy < -TILE_SIZE * 0.75f;
    const bool playerBelow = dy > TILE_SIZE * 0.75f;

    // Close the gap faster when far away, so the boss actually pressures the
    // player instead of trailing behind at walking pace.
    const float chase = (std::abs(dx) > TILE_SIZE * 3.0f) ? BOSS_CHASE_SPEED_MULT : 1.0f;

    if (wallAhead) {
        // Blocked. Hop the obstacle rather than standing still: the ledge may
        // be a single step, and the player is often standing on top of it.
        TryJump(dirX);
        // Keep leaning into the wall while the hop is on cooldown. Zeroing the
        // velocity here is what used to leave the boss frozen against a corner
        // it could not clear -- now the stuck timer sees it going nowhere and
        // triggers an unstick shove instead.
        MoveX(dirX * 0.35f, deltaTime);
        return;
    }

    if (!groundAhead) {
        if (playerBelow) {
            MoveX(dirX * chase, deltaTime);   // drop down after the player
        } else if (CanLeapGap(dirX)) {
            TryJump(dirX);                    // leap across to keep chasing
        } else {
            // A real drop with nothing to land on. Do not walk off it, but do
            // not root either: turn around and look for a way around.
            const float around = -dirX;
            if (!HasWallAhead(around) && HasGroundAhead(around)) {
                m_direction = (around > 0.0f) ? Direction::Right : Direction::Left;
                MoveX(around, deltaTime);
            } else {
                m_velocity.x = 0.0f;
            }
        }
        return;
    }

    MoveX(dirX * chase, deltaTime);

    // Player is on higher ground with a clear path: jump up to their level.
    if (playerAbove) TryJump(dirX);
}

