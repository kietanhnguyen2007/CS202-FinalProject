#ifndef BOSS3_H
#define BOSS3_H

#include "Model/Boss.h"

class Boss3 : public Boss {
public:
    // Melee-only for its first two phases, so line of sight alone is not enough
    // to engage until it picks up the energy sphere and the beam in Phase 3.
    bool HasRangedAttack() const override {
        return m_currentPhase != BossPhase::Phase1 && m_currentPhase != BossPhase::Phase2;
    }

    Boss3(Vector2 position, Vector2 size);
    
    void UpdateState(float deltaTime, Vector2 playerPos) override;
    void TransitionToNextPhase() override;
    void ResetToPhase1() override;
    bool IsFinalPhase() const override { return m_currentPhase == BossPhase::Phase4; }

private:
    void ExecuteMeleeAttack(Vector2 playerPos);
    void ExecuteEnergySphere();
    void ExecuteGroundSmash(Vector2 playerPos);
    void ExecuteEnergyBlast(Vector2 playerPos);
    void ExecuteEnergyBeam(Vector2 playerPos);
    void SpawnEnergyBlastTelegraph(Vector2 playerPos);

    Vector2 m_aoeTarget;
};

#endif
