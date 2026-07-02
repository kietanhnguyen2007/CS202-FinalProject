#ifndef PLAYER_H
#define PLAYER_H

#include "Character.h"
#include "Inventory.h"
#include "KnightSkillSet.h"
#include <string>
#include <memory>

class Player : public Character {
public:
    static constexpr float SPRINT_MULTIPLIER  = 1.4f;
    static constexpr float DASH_DURATION      = 0.22f;
    static constexpr float DASH_COOLDOWN_MAX  = 1.0f;
    static constexpr float DASH_SPEED         = 520.0f;

protected:
    Inventory    m_inventory;
    int          m_score;
    int          m_skillPoints;
    std::string  m_name;

    // Sprint
    bool         m_isSprinting  = false;

    // Dash
    bool         m_isDashing    = false;
    bool         m_isInvincible = false;
    bool         m_dashMoving   = false; // true = lunge, false = in-place dodge
    float        m_dashTimer    = 0.0f;
    float        m_dashCooldown = 0.0f;
    float        m_dashDirX    = 0.0f;  // direction of lunge

    // Knight combat skills (null for other classes)
    std::unique_ptr<KnightSkillSet> m_knightSkills;

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

    // Sprint
    bool IsSprinting() const;
    void SetSprinting(bool sprinting);

    // Dash
    bool IsDashing() const;
    bool IsInvincible() const;
    bool CanDash() const;
    void StartDash(bool isMoving, float dirX);
    
    // Attacks
    void Attack();
    void Attack2();
    void Attack3();
    bool IsAttacking() const;

    // Knight skills
    KnightSkillSet* GetKnightSkills() const { return m_knightSkills.get(); }
};

#endif
