#pragma once

#include "Model/Entity.h"
#include <string>

class Player;

class Signboard : public Entity {
public:
    Signboard(Vector2 position, std::string message);

    void Update(float deltaTime) override;

    const std::string& GetMessage() const;
    bool CanInteract(const Player* player) const;

private:
    std::string m_message;
};
