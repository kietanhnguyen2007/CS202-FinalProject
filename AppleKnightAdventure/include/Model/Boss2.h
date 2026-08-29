#ifndef BOSS2_H
#define BOSS2_H

#include "Model/Boss.h"

class Boss2 : public Boss {
public:
    // Fights at range: a clear line on the player is enough to engage, so it
    // never needs to path all the way in before it can do something.
    bool HasRangedAttack() const override { return true; }

    Boss2(Vector2 position, Vector2 size);
    
    void UpdateState(float deltaTime, Vector2 playerPos) override;
    void TransitionToNextPhase() override;
    void ResetToPhase1() override;
    bool IsFinalPhase() const override { return m_currentPhase == BossPhase::Phase3; }

private:
    void ExecuteProjectileAttack();
    void ExecuteTargetedAoE(Vector2 playerPos);
    void ExecuteHealing();
    void CheckAndSpawnTelegraph(Vector2 playerPos);

    Vector2 m_aoeTarget;
};

#endif
