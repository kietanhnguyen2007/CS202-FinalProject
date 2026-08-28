#ifndef FIGHTERSKILLSET_H
#define FIGHTERSKILLSET_H

#include "raylib.h"
#include "Utils/Types.h"
#include "Model/CharacterSkillSet.h"
#include "Model/KnightSkillSet.h"  // reuse SkillData struct

// Fighter skill set:
// J = Punch (melee), K = Combo, U = Energy Punch (charged melee),
// H = Ultimate (launches projectile orb), P = Parry
class FighterSkillSet : public CharacterSkillSet {
public:
    // V2 animation lengths. Hit windows and cooldowns remain gameplay-controlled.
    static constexpr float ATTACK1_ANIMATION_DURATION  = 0.64f;
    static constexpr float ATTACK2_ANIMATION_DURATION  = 0.72f;
    static constexpr float ATTACK3_ANIMATION_DURATION  = 0.80f;
    static constexpr float ULTIMATE_ANIMATION_DURATION = 1.00f;

    SkillData attack1 { 20, 0.35f, 0.0f, 0.20f };   // J — Punch
    SkillData attack2 { 35, 0.75f, 0.0f, 0.35f };   // K — Combo (2-hit window)
    SkillData attack3 { 50, 1.5f,  0.40f, 0.25f };  // U — Energy Punch (charged)
    SkillData ultimate { 70, 6.0f, 0.50f, 0.0f };   // H — Ultimate Orb (projectile)
    SkillData parry   { 0,  0.0f,  0.0f,  0.30f };  // P — Block (no cooldown)

    bool m_isParrying   = false;
    bool m_wantsToFire  = false;  // true for 1 frame when ultimate projectile should spawn

    void Update(float deltaTime) override;
    void TickCooldowns(float deltaTime) override;
    void ClearCooldowns() override;

    bool TryAttack1();
    bool TryAttack2();
    bool TryAttack3();
    bool TryUltimate();
    bool TryParry();

    bool IsAttack1Active()  const { return attack1.isActive; }
    bool IsAttack2Active()  const { return attack2.isActive; }
    bool IsAttack3Active()  const { return attack3.isActive; }
    bool IsUltimateCharging() const { return ultimate.isCharging; }
    bool IsParrying()       const { return m_isParrying; }
    bool WantsToFire() const { return m_wantsToFire; }
    void ResetFireFlag()    { m_wantsToFire = false; }

    bool IsAnyAttackActive() const {
        return attack1.isActive || attack2.isActive || attack3.isActive;
    }

    Rectangle GetAttack1HitBox(Vector2 pos, Vector2 size, Direction dir) const;
    Rectangle GetAttack2HitBox(Vector2 pos, Vector2 size, Direction dir) const;
    Rectangle GetAttack3HitBox(Vector2 pos, Vector2 size, Direction dir) const;

    float Attack1CooldownRatio()  const;
    float Attack2CooldownRatio()  const;
    float Attack3CooldownRatio()  const;
    float UltimateCooldownRatio() const;

private:
    void TickSkill(SkillData& s, float dt);
    bool m_ultimateFired = false;
};

#endif
