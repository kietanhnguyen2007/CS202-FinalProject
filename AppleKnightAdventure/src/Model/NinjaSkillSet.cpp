#include "Model/NinjaSkillSet.h"
#include <cmath>

void NinjaSkillSet::TickSkill(SkillData& s, float dt) {
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

void NinjaSkillSet::Update(float dt) {
    bool wasActive2 = attack2.isActive;

    TickSkill(attack1, dt);
    TickSkill(attack2, dt);
    TickSkill(attack3, dt);  // Bug fix: attack3 (teleport) cooldown must tick!
    TickSkill(parry,   dt);

    // Blade Rush: fire projectile when active window closes
    if (wasActive2 && !attack2.isActive) {
        m_wantsBladeRush = true;
    }

    // Ultimate charge — tick manually so we can detect the isCharging→false transition
    if (ultimate.isCharging) {
        ultimate.chargeTimer -= dt;
        if (ultimate.chargeTimer <= 0.0f) {
            ultimate.isCharging  = false;
            m_wantsShadowClone   = true;  // fire projectile now that charge finished
        }
        if (ultimate.cooldownTimer > 0.0f) {
            ultimate.cooldownTimer -= dt;
            if (ultimate.cooldownTimer < 0.0f) ultimate.cooldownTimer = 0.0f;
        }
    } else {
        if (ultimate.cooldownTimer > 0.0f) {
            ultimate.cooldownTimer -= dt;
            if (ultimate.cooldownTimer < 0.0f) ultimate.cooldownTimer = 0.0f;
        }
    }

    if (m_isParrying && !parry.isActive) {
        m_isParrying = false;
    }

    // Teleport timer (managed externally by GameController for position snap)
    if (m_isTeleporting) {
        m_teleportTimer -= dt;
        if (m_teleportTimer <= 0.0f) {
            m_isTeleporting = false;
            m_teleportTimer = 0.0f;
        }
    }
}

bool NinjaSkillSet::TryAttack1() {
    if (attack1.cooldownTimer > 0.0f || attack1.isActive || attack1.isCharging) return false;
    attack1.isActive      = true;
    attack1.activeTimer   = attack1.activeDuration;
    attack1.cooldownTimer = attack1.cooldownMax;
    return true;
}

bool NinjaSkillSet::TryAttack2() {
    if (attack2.cooldownTimer > 0.0f || attack2.isActive || attack2.isCharging) return false;
    attack2.isActive      = true;
    attack2.activeTimer   = attack2.activeDuration;
    attack2.cooldownTimer = attack2.cooldownMax;
    return true;
}

bool NinjaSkillSet::TryAttack3() {
    if (attack3.cooldownTimer > 0.0f || m_isTeleporting) return false;
    attack3.cooldownTimer = attack3.cooldownMax;
    m_isTeleporting       = true;
    m_teleportDone        = false;
    m_teleportTimer       = TELEPORT_START_DURATION + TELEPORT_END_DURATION;
    return true;
}

bool NinjaSkillSet::TryUltimate() {
    if (ultimate.cooldownTimer > 0.0f || ultimate.isCharging || m_wantsShadowClone) return false;
    ultimate.isCharging    = true;
    ultimate.chargeTimer   = ultimate.chargeMax;
    ultimate.cooldownTimer = ultimate.cooldownMax;
    return true;
}

bool NinjaSkillSet::TryParry() {
    // Allow re-trigger while P is held
    parry.isActive      = true;
    parry.activeTimer   = parry.activeDuration;
    m_isParrying        = true;
    return true;
}

Rectangle NinjaSkillSet::GetAttack1HitBox(Vector2 pos, Vector2 size, Direction dir) const {
    float w = size.x * 1.5f;
    float h = size.y;
    float x = (dir == Direction::Right) ? pos.x + size.x * 0.5f : pos.x - size.x * 1.0f;
    return { x, pos.y, w, h };
}

float NinjaSkillSet::Attack1CooldownRatio()  const {
    return (attack1.cooldownMax  > 0.0f) ? (1.0f - attack1.cooldownTimer  / attack1.cooldownMax)  : 1.0f;
}
float NinjaSkillSet::Attack2CooldownRatio()  const {
    return (attack2.cooldownMax  > 0.0f) ? (1.0f - attack2.cooldownTimer  / attack2.cooldownMax)  : 1.0f;
}
float NinjaSkillSet::Attack3CooldownRatio()  const {
    return (attack3.cooldownMax  > 0.0f) ? (1.0f - attack3.cooldownTimer  / attack3.cooldownMax)  : 1.0f;
}
float NinjaSkillSet::UltimateCooldownRatio() const {
    return (ultimate.cooldownMax > 0.0f) ? (1.0f - ultimate.cooldownTimer / ultimate.cooldownMax) : 1.0f;
}
