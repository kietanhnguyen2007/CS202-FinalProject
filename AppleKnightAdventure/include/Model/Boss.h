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
};

#endif
