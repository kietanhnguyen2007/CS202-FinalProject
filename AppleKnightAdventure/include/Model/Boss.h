#ifndef BOSS_H
#define BOSS_H

#include "Character.h"
#include <cmath>
#include "Utils/Types.h"
#include "Utils/Constants.h"
#include <vector>

class GameState;

class Boss : public Character {
protected:
    BossPhase m_currentPhase;
    BossState m_currentState;
    
    int m_damage;
    float m_detectionRange;
    float m_attackRange;
    float m_enrageThreshold;
    float m_phaseTimer;
    int m_bossType;
    
    // Skill properties
    float m_chargeTimer;
    float m_activeTimer;
    float m_cooldownTimer;
    bool m_skillFired;

    int m_comboStep;
    bool m_superArmor;
    int m_recentDamage;
    float m_damageTimer;
    bool m_wantsMelee;
    // A swing stays live for a short window instead of a single frame, so a hit
    // is not lost to the one tick where the boss and the player happen not to
    // overlap. Each target can still only be hit once per swing.
    float m_meleeWindow = 0.0f;
    std::vector<int> m_meleeHitIds;

    GameState* m_gameState;
    bool m_isOnGround = false;
    float m_jumpCooldown = 0.0f;

    // ---- Navigation brain ----
    // The boss re-answers one question a few times a second: can I still get to
    // the player, or hit them from here? If not, it stops feeding itself to a
    // player wedged in a nook and backs off instead.
    bool  m_playerReachable  = true;
    float m_navRecheckTimer  = 0.0f;
    // Stuck detection: how long the boss has been asking to move and not
    // actually moving. Pressed into a corner this used to last forever.
    Vector2 m_lastNavPos     = {0.0f, 0.0f};
    float m_stuckTimer       = 0.0f;
    // While > 0 the boss is committed to an unstick shove in m_unstickDirX,
    // ignoring the player so it does not walk straight back into the corner.
    float m_unstickTimer     = 0.0f;
    float m_unstickDirX      = 0.0f;
    // While > 0 the boss is backing away from an unreachable player.
    float m_retreatTimer     = 0.0f;

public:
    Boss(Vector2 position, Vector2 size, int bossType);
    virtual ~Boss() = default;

    virtual void ResetToPhase1();

    int GetBossType() const { return m_bossType; }
    int GetCurrentPhaseNumber() const { return static_cast<int>(m_currentPhase) + 1; }
    int GetTotalPhases() const {
        return (m_bossType == 1) ? 2 : (m_bossType == 2) ? 3 : 4;
    }
    virtual bool IsFinalPhase() const = 0;
    
    Vector2 GetCenter() const {
        return {m_position.x + m_size.x * 0.5f, m_position.y + m_size.y * 0.5f};
    }
    float Distance(Vector2 a, Vector2 b) const {
        float dx = a.x - b.x;
        float dy = a.y - b.y;
        return std::sqrt(dx*dx + dy*dy);
    }

    virtual void Update(float deltaTime) override;

    BossPhase GetPhase() const { return m_currentPhase; }
    void SetPhase(BossPhase phase);

    BossState GetBossState() const { return m_currentState; }
    virtual void ChangeState(BossState newState);

    int GetDamage() const { return m_damage; }
    void SetDamage(int damage) { m_damage = damage; }

    bool WantsMelee() const { return m_wantsMelee; }
    void ResetMelee() { m_wantsMelee = false; m_meleeWindow = 0.0f; m_meleeHitIds.clear(); }

    // Opens a melee swing that stays live for `window` seconds.
    void BeginMeleeSwing(float window = 0.25f);
    bool IsMeleeActive() const { return m_meleeWindow > 0.0f; }
    bool HasMeleeHit(int targetId) const;
    void MarkMeleeHit(int targetId) { m_meleeHitIds.push_back(targetId); }
    // Damage volume in front of the boss. Covers the body plus its reach, so a
    // player standing against a boss this tall is inside it -- the old
    // center-to-center distance test could not express that.
    Rectangle GetMeleeHitBox() const;

    void SetOnGround(bool onGround) { m_isOnGround = onGround; }
    bool IsOnGround() const { return m_isOnGround; }
    void NavigateToPlayer(Vector2 playerPos, float deltaTime);
    void TryJump(float forwardDirX = 0.0f, float strength = 1.0f);
    bool HasWallAhead(float dirX) const;
    bool HasGroundAhead(float dirX) const;

    // ---- Navigation brain ----
    // True when the boss has a ranged answer, i.e. line of sight is enough to
    // engage and it never needs to physically reach the player. Melee bosses
    // leave this false and must path all the way in.
    virtual bool HasRangedAttack() const { return false; }
    // Can the boss still do anything about the player from where it is -- walk
    // to them, or shoot them? Refreshed a few times a second by UpdateAI.
    bool CanEngagePlayer() const { return m_playerReachable; }
    bool IsRetreating() const { return m_retreatTimer > 0.0f; }
    // Flood fill over the tiles the boss's body actually fits through, asking
    // whether any standing spot in melee range of the player is walkable to.
    bool CanReachPlayer(Vector2 playerPos) const;
    // Walks away from the player and gives up the chase for a few seconds.
    void RetreatFromPlayer(Vector2 playerPos, float deltaTime);
    // Ticks reachability, stuck detection and the retreat timer.
    void UpdateNavigation(Vector2 playerPos, float deltaTime);
    // True when there is a gap ahead that solid ground resumes after, i.e. the
    // boss can clear it with a running jump instead of stopping at the edge.
    bool CanLeapGap(float dirX) const;

    float GetDetectionRange() const { return m_detectionRange; }
    void SetDetectionRange(float range) { m_detectionRange = range; }
    float GetAttackRange() const { return m_attackRange; }
    void SetAttackRange(float range) { m_attackRange = range; }

    virtual void TakeDamage(int damage);
    void Attack() override;

    virtual void UpdateAI(Vector2 playerPosition, float deltaTime, GameState* gameState);
    virtual void TransitionToNextPhase() = 0; 

    // FSM functions
    virtual void UpdateState(float deltaTime, Vector2 playerPos) = 0;

    // Helper functions
    bool CheckLineOfSight(Vector2 start, Vector2 end) const;
    bool IsPointSolid(Vector2 point) const;

private:
    // Tile footprint of the boss body, in whole tiles.
    int FootprintWidth() const;
    int FootprintHeight() const;
    // (tx, ty) is the cell the boss's FEET sit in; the body extends upward.
    bool BodyFitsAt(int tx, int ty) const;
    bool IsStandable(int tx, int ty) const;
};

#endif
