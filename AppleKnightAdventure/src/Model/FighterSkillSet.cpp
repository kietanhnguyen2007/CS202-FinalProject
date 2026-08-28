#include <initializer_list>
#include "Model/FighterSkillSet.h"

void FighterSkillSet::TickSkill(SkillData& s, float dt) {
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

void FighterSkillSet::Update(float dt) {
    TickSkill(attack1,  dt);
    TickSkill(attack2,  dt);
    TickSkill(attack3,  dt);
    TickSkill(parry,    dt);

    // Handle ultimate: charge → fire projectile → done
    if (ultimate.isCharging) {
        ultimate.chargeTimer -= dt;
        if (ultimate.chargeTimer <= 0.0f) {
            ultimate.isCharging  = false;
            m_wantsToFire        = true;   // signal GameController to spawn projectile
            m_ultimateFired      = false;
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
}

bool FighterSkillSet::TryAttack1() {
    if (attack1.cooldownTimer > 0.0f || attack1.isActive || attack1.isCharging) return false;
    attack1.isActive      = true;
    attack1.activeTimer   = attack1.activeDuration;
    attack1.cooldownTimer = attack1.cooldownMax;
    return true;
}

bool FighterSkillSet::TryAttack2() {
    if (attack2.cooldownTimer > 0.0f || attack2.isActive || attack2.isCharging) return false;
    attack2.isActive      = true;
    attack2.activeTimer   = attack2.activeDuration;
    attack2.cooldownTimer = attack2.cooldownMax;
    return true;
}

bool FighterSkillSet::TryAttack3() {
    if (attack3.cooldownTimer > 0.0f || attack3.isActive || attack3.isCharging) return false;
    attack3.isCharging    = true;
    attack3.chargeTimer   = attack3.chargeMax;
    attack3.cooldownTimer = attack3.cooldownMax;
    return true;
}

bool FighterSkillSet::TryUltimate() {
    if (ultimate.cooldownTimer > 0.0f || ultimate.isCharging || m_wantsToFire) return false;
    ultimate.isCharging    = true;
    ultimate.chargeTimer   = ultimate.chargeMax;
    ultimate.cooldownTimer = ultimate.cooldownMax;
    return true;
}

bool FighterSkillSet::TryParry() {
    // Allow re-trigger while P is held
    parry.isActive      = true;
    parry.activeTimer   = parry.activeDuration;
    m_isParrying        = true;
    return true;
}

Rectangle FighterSkillSet::GetAttack1HitBox(Vector2 pos, Vector2 size, Direction dir) const {
    float w = size.x * 1.5f;
    float h = size.y;
    float x = (dir == Direction::Right) ? pos.x + size.x * 0.5f : pos.x - size.x * 1.0f;
    return { x, pos.y, w, h };
}

Rectangle FighterSkillSet::GetAttack2HitBox(Vector2 pos, Vector2 size, Direction dir) const {
    float w = size.x * 1.8f;
    float h = size.y;
    float x = (dir == Direction::Right) ? pos.x + size.x * 0.4f : pos.x - size.x * 1.4f;
    return { x, pos.y, w, h };
}

Rectangle FighterSkillSet::GetAttack3HitBox(Vector2 pos, Vector2 size, Direction dir) const {
    // Energy Punch: charged, bigger hitbox
    float w = size.x * 2.5f;
    float h = size.y * 1.0f;
    float x = (dir == Direction::Right) ? pos.x + size.x * 0.3f : pos.x - size.x * 1.8f;
    return { x, pos.y, w, h };
}

float FighterSkillSet::Attack1CooldownRatio()  const {
    return (attack1.cooldownMax  > 0.0f) ? (1.0f - attack1.cooldownTimer  / attack1.cooldownMax)  : 1.0f;
}
float FighterSkillSet::Attack2CooldownRatio()  const {
    return (attack2.cooldownMax  > 0.0f) ? (1.0f - attack2.cooldownTimer  / attack2.cooldownMax)  : 1.0f;
}
float FighterSkillSet::Attack3CooldownRatio()  const {
    return (attack3.cooldownMax  > 0.0f) ? (1.0f - attack3.cooldownTimer  / attack3.cooldownMax)  : 1.0f;
}
float FighterSkillSet::UltimateCooldownRatio() const {
    return (ultimate.cooldownMax > 0.0f) ? (1.0f - ultimate.cooldownTimer / ultimate.cooldownMax) : 1.0f;
}

// Extra cooldown drain from the Adrenaline boon. Charge and active timers are
// left alone so a skill's authored timing never changes.
void FighterSkillSet::TickCooldowns(float dt) {
    for (SkillData* s : {&attack1, &attack2, &attack3, &ultimate, &parry}) {
        if (s->cooldownTimer > 0.0f) {
            s->cooldownTimer -= dt;
            if (s->cooldownTimer < 0.0f) s->cooldownTimer = 0.0f;
        }
    }
}

// Focus boon: everything usable again immediately.
void FighterSkillSet::ClearCooldowns() {
    for (SkillData* s : {&attack1, &attack2, &attack3, &ultimate, &parry}) {
        s->cooldownTimer = 0.0f;
    }
}
