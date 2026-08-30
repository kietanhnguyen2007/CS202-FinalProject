#ifndef ELEMENTALSYSTEM_H
#define ELEMENTALSYSTEM_H

#include "Utils/Types.h"
#include "Model/Entity.h"
#include <string>
#include <unordered_map>
#include <vector>

// One element currently sitting on a target. A target carries at most one at a
// time: a second element either reacts with it (consuming both) or replaces it.
struct StatusEffectInstance {
    StatusEffect type;
    float duration;
    float timer;            // counts up; the effect ends at `duration`
    float tickAccumulator;  // damage-over-time pacing

    StatusEffectInstance();
    StatusEffectInstance(StatusEffect type, float duration);
    void Update(float deltaTime);
    bool IsExpired() const;
    float Remaining() const { return duration - timer; }
};

// What one hit carries: how hard it lands, which element it is, and the aura it
// leaves behind if nothing reacts.
struct DamagePacket {
    int damage;
    DamageType damageType;
    StatusEffect statusEffect;
    float effectDuration;

    DamagePacket();
    DamagePacket(int damage, DamageType type,
                 StatusEffect effect = StatusEffect::None,
                 float duration = 0.0f);
};

// Outcome of one elemental hit: the damage actually dealt, the aura left on the
// target afterwards, and the reaction name to float over its head (empty when
// no reaction fired).
struct ReactionResult {
    int finalDamage;        // damage after the reaction multiplier
    int bonusDamage;        // finalDamage - base, for readouts
    StatusEffect resultingEffect;
    float effectDuration;
    std::string reactionName;
    Color displayColor;

    ReactionResult();
    bool Reacted() const { return !reactionName.empty(); }
};

// Static per-element data: the aura an element leaves and how long it lingers.
struct ElementProfile {
    DamageType   element;
    StatusEffect aura;
    float        auraDuration;
    const char*  name;       // element name shown in-game
    Color        color;
};

const ElementProfile& GetElementProfile(DamageType type);
const char* StatusEffectName(StatusEffect effect);
Color StatusEffectColor(StatusEffect effect);

// One row of the reaction table, exposed so the in-game codex quotes exactly
// the numbers the simulation runs on instead of a copy that can drift.
struct ReactionEntry {
    StatusEffect existing;
    DamageType   incoming;
    float        multiplier;
    StatusEffect result;
    float        resultDuration;
    const char*  name;
    Color        color;
};
const std::vector<ReactionEntry>& ReactionTable();

class ElementalSystem {
protected:
    std::unordered_map<int, std::vector<StatusEffectInstance>> m_entityEffects;
    // Damage-over-time accrued since the last drain, keyed by entity id. The
    // controller owns the entity list, so it drains this and applies the hits.
    std::unordered_map<int, int> m_pendingTickDamage;

public:
    // How strong each status is. Public so the HUD and tooltips can quote the
    // same numbers the simulation uses.
    static constexpr float BURN_TICK_INTERVAL   = 0.5f;
    static constexpr int   BURN_TICK_DAMAGE     = 7;
    static constexpr float SHOCK_TICK_INTERVAL  = 0.5f;
    static constexpr int   SHOCK_TICK_DAMAGE    = 4;
    static constexpr float WET_SPEED_MULT       = 0.65f;  // -35% move speed
    static constexpr float SHOCK_SPEED_MULT     = 0.80f;  // -20% move speed
    static constexpr float CORRODE_VULNERABILITY = 1.30f; // +30% damage taken

    ElementalSystem();

    void ApplyStatusEffect(int entityId, StatusEffect effect, float duration);
    void RemoveStatusEffect(int entityId, StatusEffect effect);
    void ClearEffects(int entityId);
    bool HasStatusEffect(int entityId, StatusEffect effect) const;
    StatusEffect GetActiveEffect(int entityId) const;
    float GetEffectRemaining(int entityId, StatusEffect effect) const;
    const std::vector<StatusEffectInstance>* GetActiveEffects(int entityId) const;

    // Resolves one elemental hit against whatever aura the target carries and
    // updates that aura. Returns the damage to actually apply.
    ReactionResult ApplyHit(int entityId, const DamagePacket& packet);
    // Pure lookup of the reaction table -- no state touched.
    static ReactionResult CheckReaction(StatusEffect existing,
                                        const DamagePacket& incoming);

    // Movement scale and incoming-damage scale from the current aura.
    float GetSpeedMultiplier(int entityId) const;
    float GetDamageTakenMultiplier(int entityId) const;

    void Update(float deltaTime);
    // Hands over (and clears) the damage-over-time accrued since the last call.
    std::unordered_map<int, int> DrainTickDamage();

    DamagePacket CreateDamagePacket(int baseDamage, DamageType type,
                                    StatusEffect effect = StatusEffect::None,
                                    float duration = 0.0f);
    // Packet for a skill of the given element, using that element's own aura
    // and duration.
    static DamagePacket ElementalPacket(int baseDamage, DamageType type);
};

#endif
