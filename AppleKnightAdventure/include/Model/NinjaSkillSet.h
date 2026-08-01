#ifndef NINJASKILLSET_H
#define NINJASKILLSET_H

#include "raylib.h"
#include "Utils/Types.h"
#include "Model/CharacterSkillSet.h"
#include "Model/KnightSkillSet.h"  // reuse SkillData

// Ninja skill set:
// J = Slash (melee), K = Blade Rush (projectile after anim),
// U = Teleport (near nearest enemy), H = Shadow Clone (projectile), P = Parry
class NinjaSkillSet : public CharacterSkillSet {
public:
    SkillData attack1 { 25, 0.40f, 0.0f,  0.20f };  // J — Slash (melee)
    SkillData attack2 { 40, 1.0f,  0.0f,  0.35f };  // K — Blade Rush (projectile on anim end)
    SkillData attack3 { 0,  2.0f,  0.0f,  0.0f  };  // U — Teleport
    SkillData ultimate{ 60, 7.0f,  0.40f, 0.0f  };  // H — Shadow Clone (projectile)
    SkillData parry   { 0,  0.0f,  0.0f,  0.30f };  // P — Block (no cooldown)

    bool m_isParrying       = false;

    // Teleport (attack3) state
    bool  m_isTeleporting   = false;
    bool  m_teleportDone    = false;  // true once position has been snapped
    float m_teleportTimer   = 0.0f;
    static constexpr float TELEPORT_START_DURATION = 0.30f; // play start anim, then snap
    static constexpr float TELEPORT_END_DURATION   = 0.30f; // play end anim
    static constexpr float TELEPORT_OFFSET         = 80.0f; // pixels in front of enemy
    static constexpr float TELEPORT_MAX_RANGE      = 500.0f;
    static constexpr float TELEPORT_NO_ENEMY_DIST  = 200.0f; // teleport distance if no enemy

    // Projectile fire signals
    bool m_wantsBladeRush   = false;  // attack2 anim finished
    bool m_wantsShadowClone = false;  // ultimate anim finished

    // Projectile speeds/ranges
    static constexpr float BLADE_RUSH_SPEED  = 480.0f;
    static constexpr float BLADE_RUSH_RANGE  = 550.0f;
    static constexpr float CLONE_SPEED       = 420.0f;
    static constexpr float CLONE_RANGE       = 600.0f;

    void Update(float deltaTime) override;

    bool TryAttack1();
    bool TryAttack2();
    bool TryAttack3();
    bool TryUltimate();
    bool TryParry();

    bool IsAttack1Active()    const { return attack1.isActive; }
    bool IsAttack2Active()    const { return attack2.isActive; }
    bool IsTeleporting()      const { return m_isTeleporting; }
    bool IsParrying()         const { return m_isParrying; }
    bool WantsBladeRush()     const { return m_wantsBladeRush; }
    bool WantsShadowClone()   const { return m_wantsShadowClone; }
    void ResetBladeRush()           { m_wantsBladeRush   = false; }
    void ResetShadowClone()         { m_wantsShadowClone = false; }

    Rectangle GetAttack1HitBox(Vector2 pos, Vector2 size, Direction dir) const;

    float Attack1CooldownRatio()  const;
    float Attack2CooldownRatio()  const;
    float Attack3CooldownRatio()  const;
    float UltimateCooldownRatio() const;

private:
    void TickSkill(SkillData& s, float dt);
};

#endif
