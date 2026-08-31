#ifndef PLAYER_H
#define PLAYER_H

#include "Character.h"
#include "CharacterSkillSet.h"
#include "KnightSkillSet.h"
#include "FighterSkillSet.h"
#include "MagicCasterSkillSet.h"
#include "NinjaSkillSet.h"
#include "Utils/Types.h"
#include "Systems/BuffSystem.h"
#include "Systems/CoreSystem.h"
#include <string>
#include <memory>
#include <vector>

class Player : public Character {
public:
    static constexpr float SPRINT_MULTIPLIER  = 1.4f;
    static constexpr float DASH_DURATION      = 0.22f;
    static constexpr float DASH_COOLDOWN_MAX  = 1.0f;
    static constexpr float DASH_SPEED         = 520.0f;

    // Parry damage reduction: 70% less damage while parrying
    static constexpr float PARRY_DAMAGE_MULT  = 0.3f;
    // Hurt flash duration after taking damage
    static constexpr float HURT_FLASH_DURATION = 0.67f;

protected:
    int          m_score;
    int          m_skillPoints;
    std::string  m_name;
    CharacterClass m_characterClass = CharacterClass::Knight;

    // Sprint
    bool         m_isSprinting  = false;

    // Dash
    bool         m_isDashing    = false;
    bool         m_isInvincible = false;
    bool         m_dashMoving   = false; // true = lunge direction, false = in-place dodge
    float        m_dashTimer    = 0.0f;
    float        m_dashCooldown = 0.0f;
    float        m_dashDirX    = 0.0f;

    // Hurt flash
    float        m_hurtTimer    = 0.0f;

    // Character skill system (polymorphic — one per class)
    std::unique_ptr<CharacterSkillSet> m_skills;

public:
    Player();
    explicit Player(Vector2 position);
    Player(Vector2 position, CharacterClass cls);

    void Update(float deltaTime) override;

    // Score and progression earned during the current run.
    int GetScore() const;
    void AddScore(int amount);
    void SetScore(int score);
    int GetSkillPoints() const;
    void SetSkillPoints(int points);
    void AddSkillPoints(int amount);
    const std::string& GetName() const;
    void SetName(const std::string& name);

    // Character class
    CharacterClass GetCharacterClass() const { return m_characterClass; }
    void SetCharacterClass(CharacterClass cls);

    // Sprint
    bool IsSprinting() const;
    void SetSprinting(bool sprinting);

    // Dash
    bool IsDashing()    const;
    bool IsInvincible() const;
    bool CanDash()      const;
    float GetDashCooldownRemaining() const { return m_dashCooldown; }
    float GetDashCooldownRatio() const {
        return DASH_COOLDOWN_MAX > 0.0f ? 1.0f - m_dashCooldown / DASH_COOLDOWN_MAX : 1.0f;
    }
    void StartDash(bool isMoving, float dirX);

    // Attack state triggers (for animation syncing)
    void Attack(float animationDuration = -1.0f);
    void Attack2(float animationDuration = -1.0f);
    void Attack3(float animationDuration = -1.0f);
    void DoUltimate(float animationDuration = -1.0f);
    bool IsAttacking() const;
    bool IsParrying()  const;

    // ── Skill cast lock ──────────────────────────────────────────────
    // Skills whose effect lands after a wind-up (ninja Blade Rush, fighter
    // Ultimate, magic Wave...) hold this lock for the wind-up. While it is
    // held, no other skill may start, so a cast cannot be silently replaced
    // half-way through and lose its projectile.
    void BeginCast(float duration);
    void CancelCast();
    bool IsCasting() const { return m_castTimer > 0.0f; }
    float GetCastRemaining() const { return m_castTimer; }
    float GetCastDuration()  const { return m_castDuration; }
    float GetCastProgress()  const {
        return m_castDuration > 0.0f ? 1.0f - m_castTimer / m_castDuration : 1.0f;
    }
    // True when the player may start any new skill this frame.
    bool CanStartSkill() const { return !IsDashing() && !IsParrying() && !IsCasting(); }

    // ── Boons (boss arena buffs) ─────────────────────────────────────
    void ApplyBuff(BuffType type);
    void ClearBuffs() { m_buffs.clear(); }
    const std::vector<ActiveBuff>& GetBuffs() const { return m_buffs; }
    bool HasBuff(BuffType type) const;
    // Read back as multipliers/fractions so callers stay unaware of the table.
    float GetSpeedMultiplier()       const;  // Haste
    float GetDamageMultiplier()      const;  // Power
    float GetDamageTakenMultiplier() const;  // Aegis
    float GetCooldownRateMultiplier()const;  // Adrenaline
    float GetLifestealFraction()     const;  // Bloodthirst
    // Element the player's ordinary attacks currently carry. Physical when no
    // infusion is running, which is the default and behaves exactly as before.
    DamageType GetAttackElement()    const;

    // ── Cores (run-long upgrades) ────────────────────────────────────
    // Boons above are timers that expire; cores are kept for the whole run and
    // stack. Their modifiers are folded into the getters above, so callers do
    // not need to know which of the two a bonus came from.
    CoreLoadout&       GetCores()       { return m_cores; }
    const CoreLoadout& GetCores() const { return m_cores; }
    // Applies a drafted core, including any immediate effect it has on health.
    void AcquireCore(CoreId id);
    // Called by the controller when this player's attack lands.
    void OnDamageDealt(int damage);

    // Damage with parry reduction support
    void TakeDamage(int damage) override;

    // Generic skill access (cast to concrete type as needed)
    CharacterSkillSet*    GetSkills()        const { return m_skills.get(); }
    KnightSkillSet*       GetKnightSkills()  const;
    FighterSkillSet*      GetFighterSkills() const;
    MagicCasterSkillSet*  GetMagicSkills()   const;
    NinjaSkillSet*        GetNinjaSkills()   const;

private:
    void InitSkills();

    float m_castTimer    = 0.0f;
    float m_castDuration = 0.0f;
    std::vector<ActiveBuff> m_buffs;
    CoreLoadout m_cores;
};

#endif
