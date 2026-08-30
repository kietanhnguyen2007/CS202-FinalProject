#include "Model/Player.h"
#include "raylib.h"
#include <cmath>
#include <algorithm>
#include "Model/Item.h"

// ---------- Constructors ----------

Player::Player()
    : Character(EntityType::Player)
    , m_score(0)
    , m_skillPoints(0)
    , m_characterClass(CharacterClass::Knight)
{
    m_speed = PLAYER_SPEED;
    m_maxHealth = PLAYER_MAX_HEALTH;
    m_health = m_maxHealth;
    InitSkills();
}

Player::Player(Vector2 position)
    : Character(position, {TILE_SIZE * 0.5f, TILE_SIZE * 0.9f}, EntityType::Player)
    , m_score(0)
    , m_skillPoints(0)
    , m_characterClass(CharacterClass::Knight)
{
    m_speed = PLAYER_SPEED;
    m_maxHealth = PLAYER_MAX_HEALTH;
    m_health = m_maxHealth;
    m_direction = Direction::Right;
    InitSkills();
}

Player::Player(Vector2 position, CharacterClass cls)
    : Character(position, {TILE_SIZE * 0.5f, TILE_SIZE * 0.9f}, EntityType::Player)
    , m_score(0)
    , m_skillPoints(0)
    , m_characterClass(cls)
{
    m_speed = PLAYER_SPEED;
    m_maxHealth = PLAYER_MAX_HEALTH;
    m_health = m_maxHealth;
    m_direction = Direction::Right;
    InitSkills();
}

void Player::InitSkills() {
    switch (m_characterClass) {
        case CharacterClass::Fighter:
            m_skills = std::make_unique<FighterSkillSet>();
            break;
        case CharacterClass::MagicCaster:
            m_skills = std::make_unique<MagicCasterSkillSet>();
            break;
        case CharacterClass::Ninja:
            m_skills = std::make_unique<NinjaSkillSet>();
            break;
        case CharacterClass::Knight:
        default:
            m_skills = std::make_unique<KnightSkillSet>();
            break;
    }
}

void Player::SetCharacterClass(CharacterClass cls) {
    m_characterClass = cls;
    InitSkills();
}

// ---------- Update ----------

void Player::Update(float deltaTime) {
    if (IsParrying()) {
        m_velocity.x = 0.0f;
    }
    Character::Update(deltaTime);

    // Boons tick first: Adrenaline changes how fast the skill set below recovers.
    for (auto& b : m_buffs) {
        b.timer -= deltaTime;
        if (b.type == BuffType::SecondWind && IsAlive()) {
            b.tickAccumulator += deltaTime * GetBuffDef(BuffType::SecondWind).magnitude;
            const int whole = static_cast<int>(b.tickAccumulator);
            if (whole > 0) {
                b.tickAccumulator -= whole;
                m_health = std::min(m_maxHealth, m_health + whole);
            }
        }
    }
    m_buffs.erase(std::remove_if(m_buffs.begin(), m_buffs.end(),
                                 [](const ActiveBuff& b) { return b.timer <= 0.0f; }),
                  m_buffs.end());

    // Tick skill set at normal speed -- charge and active windows must keep
    // their authored timing. Adrenaline only drains the cooldowns, applied as a
    // separate extra tick below.
    if (m_skills) {
        m_skills->Update(deltaTime);
        const float extra = (GetCooldownRateMultiplier() - 1.0f) * deltaTime;
        if (extra > 0.0f) m_skills->TickCooldowns(extra);
    }

    // Tick dash cooldown
    if (m_dashCooldown > 0.0f) {
        m_dashCooldown -= deltaTime;
        if (m_dashCooldown < 0.0f) m_dashCooldown = 0.0f;
    }

    // Tick active dash
    if (m_isDashing) {
        m_dashTimer -= deltaTime;
        if (m_dashTimer <= 0.0f) {
            m_isDashing    = false;
            m_isInvincible = false;
            m_dashTimer    = 0.0f;
            if (m_dashMoving) {
                Vector2 vel = m_velocity;
                vel.x = 0.0f;
                m_velocity = vel;
            }
        } else if (m_dashMoving) {
            Vector2 vel = m_velocity;
            vel.x = m_dashDirX * DASH_SPEED;
            m_velocity = vel;
        }
    }

    // Tick hurt flash
    if (m_hurtTimer > 0.0f) {
        m_hurtTimer -= deltaTime;
        if (m_hurtTimer < 0.0f) m_hurtTimer = 0.0f;
    }

    // Tick the skill cast lock
    if (m_castTimer > 0.0f) {
        m_castTimer -= deltaTime;
        if (m_castTimer < 0.0f) m_castTimer = 0.0f;
    }

    // ---- State machine (priority: Dead > Hurt > Dash > Parry > Attack > Jump/Fall > Walk/Run > Idle) ----
    if (!IsAlive()) {
        m_state = State::Dead;
    } else if (m_hurtTimer > 0.0f) {
        m_state = State::Hurt;
    } else if (m_isDashing) {
        m_state = State::Dash;  // Use dedicated Dash state (not Jump)
    } else if (IsParrying()) {
        m_state = State::Parry;
        // Lock out movement while blocking: zero horizontal velocity
        m_velocity.x = 0.0f;
    } else if (m_attackTimer > 0.0f) {
        if (m_state != State::Attack2 && m_state != State::Attack3 && m_state != State::Ultimate) {
            m_state = State::Attack;
        }
    } else if (m_velocity.y < 0.0f) {
        m_state = State::Jump;
    } else if (m_velocity.y > 0.0f) {
        m_state = State::Fall;
    } else if (std::abs(m_velocity.x) > 1.0f) {
        m_state = m_isSprinting ? State::Run : State::Walk;
    } else {
        m_state = State::Idle;
    }
}

// ---------- Damage (with parry reduction) ----------

void Player::TakeDamage(int damage) {
    if (m_isInvincible) return;  // Dash invincibility

    if (IsParrying()) {
        damage = static_cast<int>(damage * PARRY_DAMAGE_MULT);
    }
    // Aegis stacks multiplicatively with parry rather than replacing it.
    damage = static_cast<int>(damage * GetDamageTakenMultiplier());

    Character::TakeDamage(damage);

    if (damage > 0 && IsAlive()) {
        m_hurtTimer = HURT_FLASH_DURATION;
    }
}

bool Player::IsParrying() const {
    // Check the concrete skill set for parry state
    if (auto* ks = GetKnightSkills())  return ks->IsParrying();
    if (auto* fs = GetFighterSkills()) return fs->IsParrying();
    if (auto* ms = GetMagicSkills())   return ms->IsParrying();
    if (auto* ns = GetNinjaSkills())   return ns->IsParrying();
    return false;
}

// ---------- Attack state triggers ----------

void Player::Attack(float animationDuration) {
    m_attackTimer = (animationDuration > 0.0f) ? animationDuration : m_attackCooldown;
    m_state = State::Attack;
}

void Player::Attack2(float animationDuration) {
    m_attackTimer = (animationDuration > 0.0f) ? animationDuration : m_attackCooldown;
    m_state = State::Attack2;
}

void Player::Attack3(float animationDuration) {
    m_attackTimer = (animationDuration > 0.0f) ? animationDuration : m_attackCooldown;
    m_state = State::Attack3;
}

void Player::DoUltimate(float animationDuration) {
    m_attackTimer = (animationDuration > 0.0f) ? animationDuration : m_attackCooldown;
    m_state = State::Ultimate;
}

bool Player::IsAttacking() const {
    return m_attackTimer > 0.0f;
}

// ---------- Boons ----------

// Maps an infusion boon to the element it grants. Not an infusion -> Physical.
static DamageType InfusionElement(BuffType type) {
    switch (type) {
        case BuffType::InfuseFire:    return DamageType::Fire;
        case BuffType::InfuseWater:   return DamageType::Water;
        case BuffType::InfuseThunder: return DamageType::Thunder;
        case BuffType::InfuseVoid:    return DamageType::Void;
        default:                      return DamageType::Physical;
    }
}

DamageType Player::GetAttackElement() const {
    for (const auto& b : m_buffs) {
        const DamageType element = InfusionElement(b.type);
        if (element != DamageType::Physical) return element;
    }
    return DamageType::Physical;
}

void Player::ApplyBuff(BuffType type) {
    const BuffDef& def = GetBuffDef(type);

    // Only one infusion runs at a time -- taking a second one swaps the
    // element rather than layering two, which the aura model cannot express
    // anyway since a target carries a single aura.
    if (InfusionElement(type) != DamageType::Physical) {
        m_buffs.erase(
            std::remove_if(m_buffs.begin(), m_buffs.end(),
                [type](const ActiveBuff& b) {
                    return b.type != type
                        && InfusionElement(b.type) != DamageType::Physical;
                }),
            m_buffs.end());
    }

    // Instant boons resolve here and leave nothing running.
    if (type == BuffType::Vigor) {
        const int heal = static_cast<int>(m_maxHealth * def.magnitude);
        m_health = std::min(m_maxHealth, m_health + heal);
        return;
    }
    if (type == BuffType::Focus) {
        // Cooldowns live in the skill set; ClearCooldowns is its job.
        if (auto* k = GetKnightSkills())  k->ClearCooldowns();
        if (auto* f = GetFighterSkills()) f->ClearCooldowns();
        if (auto* m = GetMagicSkills())   m->ClearCooldowns();
        if (auto* n = GetNinjaSkills())   n->ClearCooldowns();
        return;
    }

    // Re-picking a running boon refreshes it instead of stacking.
    for (auto& b : m_buffs) {
        if (b.type == type) {
            b.timer = def.duration;
            b.duration = def.duration;
            return;
        }
    }
    m_buffs.push_back({type, def.duration, def.duration, 0.0f});
}

bool Player::HasBuff(BuffType type) const {
    for (const auto& b : m_buffs) {
        if (b.type == type) return true;
    }
    return false;
}

float Player::GetSpeedMultiplier() const {
    return HasBuff(BuffType::Haste)
        ? 1.0f + GetBuffDef(BuffType::Haste).magnitude : 1.0f;
}
float Player::GetDamageMultiplier() const {
    return HasBuff(BuffType::Power)
        ? 1.0f + GetBuffDef(BuffType::Power).magnitude : 1.0f;
}
float Player::GetDamageTakenMultiplier() const {
    return HasBuff(BuffType::Aegis)
        ? 1.0f - GetBuffDef(BuffType::Aegis).magnitude : 1.0f;
}
float Player::GetCooldownRateMultiplier() const {
    return HasBuff(BuffType::Adrenaline)
        ? 1.0f + GetBuffDef(BuffType::Adrenaline).magnitude : 1.0f;
}
float Player::GetLifestealFraction() const {
    return HasBuff(BuffType::Bloodthirst)
        ? GetBuffDef(BuffType::Bloodthirst).magnitude : 0.0f;
}

void Player::OnDamageDealt(int damage) {
    const float steal = GetLifestealFraction();
    if (steal <= 0.0f || damage <= 0 || !IsAlive()) return;
    const int heal = std::max(1, static_cast<int>(damage * steal));
    m_health = std::min(m_maxHealth, m_health + heal);
}

void Player::BeginCast(float duration) {
    if (duration <= 0.0f) return;
    m_castTimer    = duration;
    m_castDuration = duration;
}

void Player::CancelCast() {
    m_castTimer    = 0.0f;
    m_castDuration = 0.0f;
}

// ---------- Sprint ----------
bool Player::IsSprinting() const { return m_isSprinting; }
void Player::SetSprinting(bool sprinting) { m_isSprinting = sprinting; }

// ---------- Dash ----------
bool Player::IsDashing()    const { return m_isDashing; }
bool Player::IsInvincible() const { return m_isInvincible; }
bool Player::CanDash()      const { return !m_isDashing && m_dashCooldown <= 0.0f; }

void Player::StartDash(bool isMoving, float dirX) {
    m_isDashing    = true;
    m_isInvincible = true;
    m_dashMoving   = isMoving;
    m_dashDirX     = dirX;
    m_dashTimer    = DASH_DURATION;
    m_dashCooldown = DASH_COOLDOWN_MAX;
}

// ---------- Skill accessors ----------

KnightSkillSet* Player::GetKnightSkills() const {
    return dynamic_cast<KnightSkillSet*>(m_skills.get());
}

FighterSkillSet* Player::GetFighterSkills() const {
    return dynamic_cast<FighterSkillSet*>(m_skills.get());
}

MagicCasterSkillSet* Player::GetMagicSkills() const {
    return dynamic_cast<MagicCasterSkillSet*>(m_skills.get());
}

NinjaSkillSet* Player::GetNinjaSkills() const {
    return dynamic_cast<NinjaSkillSet*>(m_skills.get());
}

// ---------- Inventory & Score ----------
Inventory& Player::GetInventory() { return m_inventory; }
const Inventory& Player::GetInventory() const { return m_inventory; }

int Player::GetScore() const { return m_score; }
void Player::AddScore(int amount) { m_score += amount; }
void Player::SetScore(int score) { m_score = score; }

int Player::GetSkillPoints() const { return m_skillPoints; }
void Player::SetSkillPoints(int points) { m_skillPoints = points; }
void Player::AddSkillPoints(int amount) { m_skillPoints += amount; }

const std::string& Player::GetName() const { return m_name; }
void Player::SetName(const std::string& name) { m_name = name; }
