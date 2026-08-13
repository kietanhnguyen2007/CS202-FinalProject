#pragma once

#include "Model/Entity.h"

class Player;

class LevelCompleteCup : public Entity {
public:
    explicit LevelCompleteCup(Vector2 position);

    void Update(float deltaTime) override;

    bool CanInteract(const Player* player) const;
    bool IsActivated() const;
    void Activate();

private:
    bool m_activated = false;
};
