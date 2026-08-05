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

    GameState* m_gameState;

public:
    Boss(Vector2 position, Vector2 size, int bossType);
    virtual ~Boss() = default;

    int GetBossType() const { return m_bossType; }
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
    void ResetMelee() { m_wantsMelee = false; }

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
