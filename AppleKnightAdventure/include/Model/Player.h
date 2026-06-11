#ifndef PLAYER_H
#define PLAYER_H

#include "Character.h"
#include "Inventory.h"
#include <string>

class Player : public Character {
protected:
    Inventory m_inventory;
    int m_score;
    int m_skillPoints;
    std::string m_name;

public:
    Player();
    explicit Player(Vector2 position);

    void Update(float deltaTime) override;

    Inventory& GetInventory();
    const Inventory& GetInventory() const;

    int GetScore() const;
    void AddScore(int amount);
    void SetScore(int score);

    int GetSkillPoints() const;
    void SetSkillPoints(int points);
    void AddSkillPoints(int amount);

    const std::string& GetName() const;
    void SetName(const std::string& name);
};

#endif
