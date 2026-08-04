#ifndef BOSS1_H
#define BOSS1_H

#include "Model/Boss.h"

class Boss1 : public Boss {
public:
    Boss1(Vector2 position, Vector2 size);
    
    void UpdateState(float deltaTime, Vector2 playerPos) override;
    void TransitionToNextPhase() override;

private:
    void ExecuteMeleeAttack(Vector2 playerPos);
};

#endif
