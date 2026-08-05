#ifndef BOSS3_H
#define BOSS3_H

#include "Model/Boss.h"

class Boss3 : public Boss {
public:
    Boss3(Vector2 position, Vector2 size);
    
    void UpdateState(float deltaTime, Vector2 playerPos) override;
    void TransitionToNextPhase() override;
    virtual bool IsFinalPhase() const override { return m_currentPhase == BossPhase::Phase4; }

private:
    void ExecuteMeleeAttack(Vector2 playerPos);
    void ExecuteEnergySphere();
    void ExecuteGroundSmash(Vector2 playerPos);
    void ExecuteEnergyBlast(Vector2 playerPos);
    void ExecuteEnergyBeam(Vector2 playerPos);

    Vector2 m_aoeTarget;
};

#endif
