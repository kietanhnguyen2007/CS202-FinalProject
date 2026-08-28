#ifndef KNIGHTSKILLSET_H
#define KNIGHTSKILLSET_H

#include "raylib.h"
#include "Utils/Types.h"
#include "Model/CharacterSkillSet.h"

// Per-skill data: cooldown, charge, active window and damage
struct SkillData {
    int   damage;
    float cooldownMax;
    float chargeMax;       // 0 = instant, >0 = must charge before hitbox is active
    float activeDuration;  // how long the hitbox is "live"

    float cooldownTimer = 0.0f;
    float chargeTimer   = 0.0f;
    float activeTimer   = 0.0f;
    bool  isCharging    = false;
    bool  isActive      = false;
};

// Manages all knight attack slots independently.
// The Controller reads IsAttackNActive() to determine when to deal damage.
class KnightSkillSet : public CharacterSkillSet {
public:
    // V2 animation lengths. Gameplay cooldowns and active hit windows stay independent.
    static constexpr float ATTACK1_ANIMATION_DURATION  = 0.64f;
    static constexpr float ATTACK2_ANIMATION_DURATION  = 0.72f;
    static constexpr float ATTACK3_ANIMATION_DURATION  = 0.68f;
    static constexpr float ULTIMATE_ANIMATION_DURATION = 1.00f;

    // Attack 1 — Quick Slash (J): fast, low damage
    SkillData attack1 { 20, 0.35f, 0.0f, 0.20f };
    // Attack 2 — Heavy Strike (K): charge delay, high damage, 0.75s cooldown
    SkillData attack2 { 45, 0.75f, 0.30f, 0.25f };
    // Attack 3 — Lunge Thrust (U): rushes forward (reduced to ~100px), medium damage
    SkillData attack3 { 32, 1.25f, 0.0f, 0.15f };
    // Ultimate — Close-range power slash (H): big hitbox, high damage
    SkillData ultimate { 80, 5.0f, 0.20f, 0.30f };
    // Parry — Block (P): reduces damage taken by 70% while active
    SkillData parry { 0, 0.0f, 0.0f, 0.30f };

    // Lunge state for Attack3 (reduced speed: 650px/s × 0.15s ≈ 97px)
    bool  m_isLunging  = false;
    float m_lungeTimer = 0.0f;
    float m_lungeSpeed = 650.0f;

    // Parry state
    bool m_isParrying = false;

    void Update(float deltaTime) override;
    void TickCooldowns(float deltaTime) override;
    void ClearCooldowns() override;

    // Returns true if the skill was successfully started
    bool TryAttack1();
    bool TryAttack2();
    bool TryAttack3();
    bool TryUltimate();
    bool TryParry();

    bool IsAttack1Active()  const { return attack1.isActive; }
    bool IsAttack2Active()  const { return attack2.isActive; }
    bool IsAttack3Active()  const { return attack3.isActive; }
    bool IsUltimateActive() const { return ultimate.isActive; }
    bool IsParrying()       const { return m_isParrying; }

    // Wider hitbox for Attack2 (heavy slam downward)
    Rectangle GetAttack2HitBox(Vector2 playerPos, Vector2 playerSize, Direction dir) const;
    // Very wide hitbox for Ultimate
    Rectangle GetUltimateHitBox(Vector2 playerPos, Vector2 playerSize, Direction dir) const;

    bool IsAnyAttackActive() const {
        return attack1.isActive || attack2.isActive || attack3.isActive || ultimate.isActive;
    }

    // Total cooldown progress [0,1] for HUD display
    float Attack1CooldownRatio()  const;
    float Attack2CooldownRatio()  const;
    float Attack3CooldownRatio()  const;
    float UltimateCooldownRatio() const;

private:
    void TickSkill(SkillData& s, float dt);
};

#endif
