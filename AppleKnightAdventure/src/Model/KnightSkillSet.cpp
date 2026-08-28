#include <initializer_list>
#include "Model/KnightSkillSet.h"

// ---------- private helper ----------
void KnightSkillSet::TickSkill(SkillData& s, float dt) {
    // Cooldown countdown
    if (s.cooldownTimer > 0.0f) {
        s.cooldownTimer -= dt;
        if (s.cooldownTimer < 0.0f) s.cooldownTimer = 0.0f;
    }

    // Charge countdown
    if (s.isCharging) {
        s.chargeTimer -= dt;
        if (s.chargeTimer <= 0.0f) {
            s.isCharging  = false;
            s.isActive    = true;
            s.activeTimer = s.activeDuration;
        }
    }

    // Active window countdown
    if (s.isActive) {
        s.activeTimer -= dt;
        if (s.activeTimer <= 0.0f) {
            s.isActive    = false;
            s.activeTimer = 0.0f;
        }
    }
}

// ---------- public ----------
void KnightSkillSet::Update(float dt) {
    TickSkill(attack1,  dt);
    TickSkill(attack2,  dt);
    TickSkill(attack3,  dt);
    TickSkill(ultimate, dt);
    TickSkill(parry,    dt);

    if (m_isLunging) {
        m_lungeTimer -= dt;
        if (m_lungeTimer <= 0.0f) {
            m_isLunging  = false;
            m_lungeTimer = 0.0f;
        }
    }

    // Parry ends when the parry skill's active window closes
    if (m_isParrying && !parry.isActive) {
        m_isParrying = false;
    }
}

bool KnightSkillSet::TryAttack1() {
    if (attack1.cooldownTimer > 0.0f || attack1.isActive || attack1.isCharging) return false;
    // Instant — no charge
    attack1.isActive    = true;
    attack1.activeTimer = attack1.activeDuration;
    attack1.cooldownTimer = attack1.cooldownMax;
    return true;
}

bool KnightSkillSet::TryAttack2() {
    if (attack2.cooldownTimer > 0.0f || attack2.isActive || attack2.isCharging) return false;
    // Has charge phase
    attack2.isCharging    = true;
    attack2.chargeTimer   = attack2.chargeMax;
    attack2.cooldownTimer = attack2.cooldownMax;
    return true;
}

bool KnightSkillSet::TryAttack3() {
    if (attack3.cooldownTimer > 0.0f || attack3.isActive || attack3.isCharging) return false;
    // Instant + lunge (reduced: 650px/s × 0.15s ≈ 97px)
    attack3.isActive      = true;
    attack3.activeTimer   = attack3.activeDuration;
    attack3.cooldownTimer = attack3.cooldownMax;
    m_isLunging           = true;
    m_lungeTimer          = attack3.activeDuration;
    return true;
}

bool KnightSkillSet::TryUltimate() {
    if (ultimate.cooldownTimer > 0.0f || ultimate.isActive || ultimate.isCharging) return false;
    // Short charge then big active window
    ultimate.isCharging    = true;
    ultimate.chargeTimer   = ultimate.chargeMax;
    ultimate.cooldownTimer = ultimate.cooldownMax;
    return true;
}

bool KnightSkillSet::TryParry() {
    // Allow re-trigger while P is held (no cooldown, no isActive guard)
    parry.isActive      = true;
    parry.activeTimer   = parry.activeDuration;  // refreshed every frame while held
    m_isParrying        = true;
    return true;
}

Rectangle KnightSkillSet::GetAttack2HitBox(Vector2 playerPos, Vector2 playerSize, Direction dir) const {
    // Heavy strike: wider arc in front, slightly below (downward slam feel)
    float w = playerSize.x * 2.2f;
    float h = playerSize.y * 0.9f;
    float x = (dir == Direction::Right)
        ? playerPos.x + playerSize.x * 0.5f
        : playerPos.x - playerSize.x * 1.7f;
    float y = playerPos.y + playerSize.y * 0.15f;
    return { x, y, w, h };
}

Rectangle KnightSkillSet::GetUltimateHitBox(Vector2 playerPos, Vector2 playerSize, Direction dir) const {
    // Ultimate: very wide arc in front — 3× player width, full height
    float w = playerSize.x * 3.0f;
    float h = playerSize.y * 1.1f;
    float x = (dir == Direction::Right)
        ? playerPos.x + playerSize.x * 0.3f
        : playerPos.x - playerSize.x * 2.3f;
    float y = playerPos.y - playerSize.y * 0.05f;
    return { x, y, w, h };
}

float KnightSkillSet::Attack1CooldownRatio() const {
    return (attack1.cooldownMax > 0.0f) ? (1.0f - attack1.cooldownTimer / attack1.cooldownMax) : 1.0f;
}

float KnightSkillSet::Attack2CooldownRatio() const {
    return (attack2.cooldownMax > 0.0f) ? (1.0f - attack2.cooldownTimer / attack2.cooldownMax) : 1.0f;
}

float KnightSkillSet::Attack3CooldownRatio() const {
    return (attack3.cooldownMax > 0.0f) ? (1.0f - attack3.cooldownTimer / attack3.cooldownMax) : 1.0f;
}

float KnightSkillSet::UltimateCooldownRatio() const {
    return (ultimate.cooldownMax > 0.0f) ? (1.0f - ultimate.cooldownTimer / ultimate.cooldownMax) : 1.0f;
}

// Extra cooldown drain from the Adrenaline boon. Charge and active timers are
// left alone so a skill's authored timing never changes.
void KnightSkillSet::TickCooldowns(float dt) {
    for (SkillData* s : {&attack1, &attack2, &attack3, &ultimate, &parry}) {
        if (s->cooldownTimer > 0.0f) {
            s->cooldownTimer -= dt;
            if (s->cooldownTimer < 0.0f) s->cooldownTimer = 0.0f;
        }
    }
}

// Focus boon: everything usable again immediately.
void KnightSkillSet::ClearCooldowns() {
    for (SkillData* s : {&attack1, &attack2, &attack3, &ultimate, &parry}) {
        s->cooldownTimer = 0.0f;
    }
}
