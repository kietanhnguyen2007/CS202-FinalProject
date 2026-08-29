#ifndef MAGICCASKERSKILLSET_H
#define MAGICCASKERSKILLSET_H

#include "raylib.h"
#include "Utils/Types.h"
#include "Model/CharacterSkillSet.h"
#include "Model/KnightSkillSet.h"  // reuse SkillData

// Magic Caster skill set:
// J = Lightning (instant-at-target), K = Fireball, U = Wave, H = Ultimate Lightning
// P = Parry
// Ranged attacks spawn projectiles; Lightning teleports projectile to target position.
class MagicCasterSkillSet : public CharacterSkillSet {
public:
    // Element each skill carries. These drive the reaction table in
    // ElementalSystem -- J sets up, K and H detonate hardest.
    static constexpr DamageType ATTACK1_ELEMENT  = DamageType::Thunder; // J - Loi
    static constexpr DamageType ATTACK2_ELEMENT  = DamageType::Fire;    // K - Hoa
    static constexpr DamageType ATTACK3_ELEMENT  = DamageType::Water;   // U - Thuy
    static constexpr DamageType ULTIMATE_ELEMENT = DamageType::Void;    // H - Hu khong

    // V2 animation lengths. Cooldowns and damage remain gameplay-controlled.
    static constexpr float ATTACK1_ANIMATION_DURATION  = 0.72f;
    static constexpr float ATTACK2_ANIMATION_DURATION  = 0.72f;
    static constexpr float ATTACK3_ANIMATION_DURATION  = 0.72f;
    static constexpr float ULTIMATE_ANIMATION_DURATION = 1.00f;

    // J — Lightning Strike: charge → projectile appears AT nearest enemy (range 500px)
    SkillData attack1 { 45, 1.0f,  0.40f, 0.10f };
    // K — Fireball: charge → projectile flies forward (speed 450px/s)
    // Keep K active through the final V2 animation frame.
    static constexpr float ATTACK2_CHARGE_DURATION = 0.35f;
    SkillData attack2 { 35, 0.8f, ATTACK2_CHARGE_DURATION,
                        ATTACK2_ANIMATION_DURATION - ATTACK2_CHARGE_DURATION };
    // U — Wave: charge → projectile flies forward (speed 350px/s)
    SkillData attack3 { 30, 0.9f,  0.35f, 0.0f  };
    // H — Ultimate Lightning: like attack1 but higher damage
    SkillData ultimate { 80, 8.0f, 0.50f, 0.10f };
    // P — Parry / Block
    SkillData parry   { 0,  0.0f,  0.0f,  0.30f };  // P — Block (no cooldown)

    bool m_isParrying = false;

    // Projectile spawn signals — set true for 1 frame when GameController should spawn
    bool m_wantsFireball   = false;  // attack2 finished charging
    bool m_wantsWave       = false;  // attack3 finished charging
    bool m_wantsLightning  = false;  // attack1 finished charging
    bool m_wantsUltLightning = false;// ultimate finished charging

    // Max range for ranged attacks (px)
    static constexpr float LIGHTNING_RANGE  = 500.0f;
    static constexpr float FIREBALL_SPEED   = 450.0f;
    static constexpr float FIREBALL_RANGE   = 600.0f;
    static constexpr float WAVE_SPEED       = 350.0f;
    static constexpr float WAVE_RANGE       = 700.0f;

    void Update(float deltaTime) override;
    void TickCooldowns(float deltaTime) override;
    void ClearCooldowns() override;

    bool TryAttack1();
    bool TryAttack2();
    bool TryAttack3();
    bool TryUltimate();
    bool TryParry();

    bool IsParrying()     const { return m_isParrying; }
    bool IsAttack1Active()const { return attack1.isActive; }
    bool IsAttack2Active()const { return attack2.isActive; }
    bool IsAttack3Active()const { return attack3.isActive; }

    void ResetFireball()     { m_wantsFireball    = false; }
    void ResetWave()         { m_wantsWave         = false; }
    void ResetLightning()    { m_wantsLightning    = false; }
    void ResetUltLightning() { m_wantsUltLightning = false; }

    float Attack1CooldownRatio()  const;
    float Attack2CooldownRatio()  const;
    float Attack3CooldownRatio()  const;
    float UltimateCooldownRatio() const;

private:
    void TickSkill(SkillData& s, float dt);
};

#endif
