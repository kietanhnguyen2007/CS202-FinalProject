#ifndef BOSS_H
#define BOSS_H

#include "Character.h"
#include "Utils/Types.h"
#include "Utils/Constants.h"
#include <vector>

class Boss : public Character {
protected:
    BossPhase m_phase;
    int m_damage;
    float m_detectionRange;
    float m_attackRange;
    float m_enrageThreshold;
    float m_phaseTimer;
    std::vector<BossPhase> m_phaseOrder;

public:
    Boss();
    Boss(Vector2 position, Vector2 size);

    void Update(float deltaTime) override;

    BossPhase GetPhase() const;
    void SetPhase(BossPhase phase);

    int GetDamage() const;
    void SetDamage(int damage);

    float GetDetectionRange() const;
    void SetDetectionRange(float range);
    float GetAttackRange() const;
    void SetAttackRange(float range);

    void TakeDamage(int damage);
    void Attack() override;

    void UpdateAI(Vector2 playerPosition, float deltaTime);
    void TransitionToNextPhase();
    bool IsEnraged() const;

    virtual void OnPhaseEnter(BossPhase newPhase);
    virtual void ExecutePhaseBehavior(float deltaTime);
};

#endif
