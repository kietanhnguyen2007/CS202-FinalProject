#include "Model/MagicCasterSkillSet.h"

void MagicCasterSkillSet::TickSkill(SkillData& s, float dt) {
    if (s.cooldownTimer > 0.0f) {
        s.cooldownTimer -= dt;
        if (s.cooldownTimer < 0.0f) s.cooldownTimer = 0.0f;
    }
    if (s.isCharging) {
        s.chargeTimer -= dt;
        if (s.chargeTimer <= 0.0f) {
            s.isCharging  = false;
            s.isActive    = true;
            s.activeTimer = s.activeDuration;
        }
    }
    if (s.isActive) {
        s.activeTimer -= dt;
        if (s.activeTimer <= 0.0f) {
            s.isActive    = false;
            s.activeTimer = 0.0f;
        }
    }
}

void MagicCasterSkillSet::Update(float dt) {
    // Track transition from charging→active to fire signals
    bool wasCharging1 = attack1.isCharging;
    bool wasCharging2 = attack2.isCharging;
    bool wasCharging3 = attack3.isCharging;
    bool wasChargingU = ultimate.isCharging;

    TickSkill(attack1,  dt);
    TickSkill(attack2,  dt);
    TickSkill(attack3,  dt);
    TickSkill(ultimate, dt);
    TickSkill(parry,    dt);

    // Fire signals: set when charge just finished
    if (wasCharging1 && !attack1.isCharging)   m_wantsLightning    = true;
    if (wasCharging2 && !attack2.isCharging)   m_wantsFireball     = true;
    if (wasCharging3 && !attack3.isCharging)   m_wantsWave         = true;
    if (wasChargingU && !ultimate.isCharging)  m_wantsUltLightning = true;

    if (m_isParrying && !parry.isActive) {
        m_isParrying = false;
    }
}

bool MagicCasterSkillSet::TryAttack1() {
    if (attack1.cooldownTimer > 0.0f || attack1.isActive || attack1.isCharging) return false;
    attack1.isCharging    = true;
    attack1.chargeTimer   = attack1.chargeMax;
    attack1.cooldownTimer = attack1.cooldownMax;
    return true;
}

bool MagicCasterSkillSet::TryAttack2() {
    if (attack2.cooldownTimer > 0.0f || attack2.isActive || attack2.isCharging) return false;
    attack2.isCharging    = true;
    attack2.chargeTimer   = attack2.chargeMax;
    attack2.cooldownTimer = attack2.cooldownMax;
    return true;
}

bool MagicCasterSkillSet::TryAttack3() {
    if (attack3.cooldownTimer > 0.0f || attack3.isActive || attack3.isCharging) return false;
    attack3.isCharging    = true;
    attack3.chargeTimer   = attack3.chargeMax;
    attack3.cooldownTimer = attack3.cooldownMax;
    return true;
}

bool MagicCasterSkillSet::TryUltimate() {
    if (ultimate.cooldownTimer > 0.0f || ultimate.isActive || ultimate.isCharging) return false;
    ultimate.isCharging    = true;
    ultimate.chargeTimer   = ultimate.chargeMax;
    ultimate.cooldownTimer = ultimate.cooldownMax;
    return true;
}

bool MagicCasterSkillSet::TryParry() {
    // Allow re-trigger while P is held
    parry.isActive      = true;
    parry.activeTimer   = parry.activeDuration;
    m_isParrying        = true;
    return true;
}

float MagicCasterSkillSet::Attack1CooldownRatio()  const {
    return (attack1.cooldownMax  > 0.0f) ? (1.0f - attack1.cooldownTimer  / attack1.cooldownMax)  : 1.0f;
}
float MagicCasterSkillSet::Attack2CooldownRatio()  const {
    return (attack2.cooldownMax  > 0.0f) ? (1.0f - attack2.cooldownTimer  / attack2.cooldownMax)  : 1.0f;
}
float MagicCasterSkillSet::Attack3CooldownRatio()  const {
    return (attack3.cooldownMax  > 0.0f) ? (1.0f - attack3.cooldownTimer  / attack3.cooldownMax)  : 1.0f;
}
float MagicCasterSkillSet::UltimateCooldownRatio() const {
    return (ultimate.cooldownMax > 0.0f) ? (1.0f - ultimate.cooldownTimer / ultimate.cooldownMax) : 1.0f;
}
