#ifndef PET_H
#define PET_H

#include "Character.h"
#include "Utils/Types.h"
#include "Utils/Constants.h"
#include <vector>

// Forward declarations
class Player;
class Entity;

enum class PetState {
    Idle,
    Following,
    Healing,      // Ghost: actively healing player
    Charging,     // Dragon: winding up projectile
    Attacking,    // Dragon: projectile just fired
};

class Pet : public Character {
public:
    // Dragon constants
    static constexpr float DRAGON_FIRE_COOLDOWN = 1.2f;   // fire continuously (every 1.2s)
    static constexpr float DRAGON_CHARGE_TIME   = 0.6f;   // animation charge before fire
    static constexpr float DRAGON_DETECT_RANGE  = 320.0f; // look for enemies within this
    static constexpr int   DRAGON_PROJECTILE_DMG = 18;    // ~10-15% bonus damage
    // Ghost constants
    static constexpr int   GHOST_MAX_HEAL_BUDGET = 70;    // max HP healed per level
    static constexpr float GHOST_HEAL_RATE       = 8.0f;  // HP/s
    static constexpr float GHOST_HEAL_TICK       = 0.3f;  // tick every 0.3s

protected:
    PetType  m_petType;
    PetState m_petState    = PetState::Idle;
    float m_actionTimer;
    float m_hoverTime = 0.0f; // Tracks time for circular hovering pattern
    int m_ownerId;
    float    m_followDistance;
    int      m_damage      = 0;

    // Dragon
    float    m_fireCooldown   = 0.0f;
    float    m_chargeTimer    = 0.0f;
    bool     m_wantsToFire    = false;
    int      m_targetId       = -1;   // entity id of chosen target

    // Ghost
    int      m_healBudget     = GHOST_MAX_HEAL_BUDGET;
    float    m_healTickTimer  = 0.0f;

public:
    Pet();
    Pet(Vector2 position, PetType type, int ownerId);

    void Update(float deltaTime) override;

    PetType  GetPetType()  const;
    PetState GetPetState() const;
    int      GetOwnerId()  const;
    void     SetOwnerId(int ownerId);
    float    GetFollowDistance() const;
    void     SetFollowDistance(float distance);
    int      GetDamage() const { return m_damage; }

    // Dragon
    bool WantsToFire() const;
    void ResetFireFlag();
    int  GetTargetId() const;

    // Ghost
    int  GetHealBudget() const;
    void SetHealBudget(int budget);
    bool CanHeal(const Player* player) const;
    void HealPlayer(Player* player, float dt);

    // Core AI — called from GameController each frame
    // enemies: raw pointers to active enemy Entities for Dragon targeting
    void FollowPlayer(Vector2 playerPosition, float deltaTime);
    void UpdateAI(Vector2 playerPosition, float deltaTime,
                  Player* player,
                  const std::vector<Entity*>& enemies,
                  const std::vector<Entity*>& items,
                  bool inCombat);
};

#endif
