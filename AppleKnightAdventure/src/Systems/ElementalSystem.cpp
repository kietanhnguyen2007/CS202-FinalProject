#include "Systems/ElementalSystem.h"
#include <algorithm>
#include <cmath>

namespace {

// How long each element's aura lingers on a target when nothing reacts.
// Water sticks longest so it can be set up and then detonated; Thunder is the
// shortest because it is the strongest reaction trigger.
constexpr float FIRE_AURA_DURATION    = 5.0f;
constexpr float WATER_AURA_DURATION   = 6.0f;
constexpr float THUNDER_AURA_DURATION = 4.0f;
constexpr float VOID_AURA_DURATION    = 8.0f;

const ElementProfile kProfiles[] = {
    { DamageType::Physical, StatusEffect::None,     0.0f,
      "Vat Ly", Color{200, 200, 200, 255} },
    { DamageType::Fire,     StatusEffect::Burn,     FIRE_AURA_DURATION,
      "Hoa",     Color{255, 120,  48, 255} },
    { DamageType::Water,    StatusEffect::Wet,      WATER_AURA_DURATION,
      "Thuy",    Color{ 90, 180, 255, 255} },
    { DamageType::Thunder,  StatusEffect::Shocked,  THUNDER_AURA_DURATION,
      "Loi",     Color{255, 235, 110, 255} },
    { DamageType::Void,     StatusEffect::Corroded, VOID_AURA_DURATION,
      "Hu Khong", Color{180, 110, 255, 255} },
};

// One row of the reaction table. `multiplier` scales the incoming hit; the
// resulting aura is what the target is left carrying afterwards.
struct ReactionRule {
    StatusEffect existing;
    DamageType   incoming;
    float        multiplier;
    StatusEffect result;      // None = both elements are consumed
    float        resultDuration;
    const char*  name;
    Color        color;
};

// Every pairing of the four elements. Reading a row: "target already carries
// `existing`, gets hit by `incoming`".
const ReactionRule kReactions[] = {
    // --- Water on the target ---
    // Thuy + Hoa: the water flashes to steam, the biggest straight damage spike.
    { StatusEffect::Wet,      DamageType::Fire,    1.80f, StatusEffect::None,      0.0f,
      "BOC HOI",  Color{255, 170, 120, 255} },
    // Thuy + Loi: conducts through the water and leaves a long paralysis.
    { StatusEffect::Wet,      DamageType::Thunder, 1.60f, StatusEffect::Shocked,   5.0f,
      "CAM UNG",  Color{170, 230, 255, 255} },
    // Thuy + Hu Khong: the water is unmade, leaving the target brittle.
    { StatusEffect::Wet,      DamageType::Void,    1.70f, StatusEffect::Corroded,  6.0f,
      "TAN RA",   Color{150, 170, 255, 255} },

    // --- Fire on the target ---
    // Hoa + Thuy: quenched -- less burst than Boc Hoi, but re-soaks the target.
    { StatusEffect::Burn,     DamageType::Water,   1.50f, StatusEffect::Wet,       4.0f,
      "DAP TAT",  Color{160, 220, 255, 255} },
    // Hoa + Loi: overload, a clean burst that consumes both elements.
    { StatusEffect::Burn,     DamageType::Thunder, 2.00f, StatusEffect::None,      0.0f,
      "QUA TAI",  Color{255, 200,  90, 255} },
    // Hoa + Hu Khong: the flame collapses into ash and rots the target.
    { StatusEffect::Burn,     DamageType::Void,    1.90f, StatusEffect::Corroded,  6.0f,
      "TAN TRO",  Color{210, 130, 200, 255} },

    // --- Thunder on the target ---
    // Loi + Hoa: overload again, and the target is left burning.
    { StatusEffect::Shocked,  DamageType::Fire,    2.00f, StatusEffect::Burn,      4.0f,
      "QUA TAI",  Color{255, 200,  90, 255} },
    // Loi + Thuy: the charge spreads out -- softer hit, target left soaked.
    { StatusEffect::Shocked,  DamageType::Water,   1.40f, StatusEffect::Wet,       5.0f,
      "LAN TRUYEN", Color{140, 210, 255, 255} },
    // Loi + Hu Khong: the charge is swallowed whole. Hardest hit in the table.
    { StatusEffect::Shocked,  DamageType::Void,    2.40f, StatusEffect::None,      0.0f,
      "TIEU TAN", Color{200, 140, 255, 255} },

    // --- Void on the target ---
    // Hu Khong + Hoa: the rot feeds the fire.
    { StatusEffect::Corroded, DamageType::Fire,    2.20f, StatusEffect::None,      0.0f,
      "THIEU HUY", Color{255, 140, 180, 255} },
    // Hu Khong + Thuy: erodes the target and soaks it.
    { StatusEffect::Corroded, DamageType::Water,   1.60f, StatusEffect::Wet,       6.0f,
      "XOI MON",  Color{130, 190, 235, 255} },
    // Hu Khong + Loi: the rift cracks open and paralyses.
    { StatusEffect::Corroded, DamageType::Thunder, 2.00f, StatusEffect::Shocked,   5.0f,
      "HU CHAN",  Color{215, 175, 255, 255} },
};

} // namespace

const ElementProfile& GetElementProfile(DamageType type) {
    for (const auto& p : kProfiles) {
        if (p.element == type) return p;
    }
    return kProfiles[0];
}

const char* StatusEffectName(StatusEffect effect) {
    switch (effect) {
        case StatusEffect::Burn:     return "Thieu Dot";
        case StatusEffect::Wet:      return "Uot Sung";
        case StatusEffect::Shocked:  return "Te Liet";
        case StatusEffect::Corroded: return "Hu Hoa";
        default:                     return "";
    }
}

Color StatusEffectColor(StatusEffect effect) {
    switch (effect) {
        case StatusEffect::Burn:     return Color{255, 120,  48, 255};
        case StatusEffect::Wet:      return Color{ 90, 180, 255, 255};
        case StatusEffect::Shocked:  return Color{255, 235, 110, 255};
        case StatusEffect::Corroded: return Color{180, 110, 255, 255};
        default:                     return WHITE;
    }
}

StatusEffectInstance::StatusEffectInstance()
    : type(StatusEffect::None)
    , duration(0.0f)
    , timer(0.0f)
    , tickAccumulator(0.0f)
{
}

StatusEffectInstance::StatusEffectInstance(StatusEffect type, float duration)
    : type(type)
    , duration(duration)
    , timer(0.0f)
    , tickAccumulator(0.0f)
{
}

void StatusEffectInstance::Update(float deltaTime) {
    timer += deltaTime;
}

bool StatusEffectInstance::IsExpired() const {
    return timer >= duration;
}

DamagePacket::DamagePacket()
    : damage(0)
    , damageType(DamageType::Physical)
    , statusEffect(StatusEffect::None)
    , effectDuration(0.0f)
{
}

DamagePacket::DamagePacket(int damage, DamageType type,
                           StatusEffect effect, float duration)
    : damage(damage)
    , damageType(type)
    , statusEffect(effect)
    , effectDuration(duration)
{
}

ReactionResult::ReactionResult()
    : finalDamage(0)
    , bonusDamage(0)
    , resultingEffect(StatusEffect::None)
    , effectDuration(0.0f)
    , displayColor(WHITE)
{
}

ElementalSystem::ElementalSystem() {
}

void ElementalSystem::ApplyStatusEffect(int entityId, StatusEffect effect, float duration) {
    if (effect == StatusEffect::None || duration <= 0.0f) return;
    auto& effects = m_entityEffects[entityId];
    for (auto& e : effects) {
        if (e.type == effect) {
            // Re-applying the same element refreshes it rather than stacking.
            e.timer = 0.0f;
            e.duration = std::max(e.duration, duration);
            return;
        }
    }
    // A target carries one aura at a time, so a different element replaces it.
    effects.clear();
    effects.emplace_back(effect, duration);
}

void ElementalSystem::RemoveStatusEffect(int entityId, StatusEffect effect) {
    auto it = m_entityEffects.find(entityId);
    if (it == m_entityEffects.end()) return;
    auto& effects = it->second;
    effects.erase(
        std::remove_if(effects.begin(), effects.end(),
            [effect](const StatusEffectInstance& e) { return e.type == effect; }),
        effects.end());
    if (effects.empty()) m_entityEffects.erase(it);
}

void ElementalSystem::ClearEffects(int entityId) {
    m_entityEffects.erase(entityId);
}

bool ElementalSystem::HasStatusEffect(int entityId, StatusEffect effect) const {
    auto it = m_entityEffects.find(entityId);
    if (it == m_entityEffects.end()) return false;
    for (const auto& e : it->second) {
        if (e.type == effect) return true;
    }
    return false;
}

StatusEffect ElementalSystem::GetActiveEffect(int entityId) const {
    auto it = m_entityEffects.find(entityId);
    if (it == m_entityEffects.end() || it->second.empty()) return StatusEffect::None;
    return it->second.front().type;
}

float ElementalSystem::GetEffectRemaining(int entityId, StatusEffect effect) const {
    auto it = m_entityEffects.find(entityId);
    if (it == m_entityEffects.end()) return 0.0f;
    for (const auto& e : it->second) {
        if (e.type == effect) return std::max(0.0f, e.Remaining());
    }
    return 0.0f;
}

const std::vector<StatusEffectInstance>* ElementalSystem::GetActiveEffects(int entityId) const {
    auto it = m_entityEffects.find(entityId);
    return (it != m_entityEffects.end()) ? &it->second : nullptr;
}

ReactionResult ElementalSystem::ApplyHit(int entityId, const DamagePacket& packet) {
    const StatusEffect existing = GetActiveEffect(entityId);
    ReactionResult result = CheckReaction(existing, packet);

    if (result.Reacted()) {
        // The reaction consumes the old aura; whatever it leaves behind is set
        // fresh, so a chain never inherits a half-expired timer.
        ClearEffects(entityId);
        if (result.resultingEffect != StatusEffect::None) {
            ApplyStatusEffect(entityId, result.resultingEffect, result.effectDuration);
        }
    } else {
        result.finalDamage = packet.damage;
        result.bonusDamage = 0;
        if (packet.statusEffect != StatusEffect::None) {
            ApplyStatusEffect(entityId, packet.statusEffect, packet.effectDuration);
            result.resultingEffect = packet.statusEffect;
            result.effectDuration  = packet.effectDuration;
        } else {
            result.resultingEffect = existing;
        }
    }

    // Void rot makes every non-reacting hit land harder. A hit that does react
    // with the rot is not amplified on top of that: its row in the table is
    // already priced for consuming a Corroded target, and stacking both would
    // make Void far and away the only element worth using.
    if (existing == StatusEffect::Corroded && !result.Reacted()) {
        result.finalDamage = static_cast<int>(result.finalDamage * CORRODE_VULNERABILITY);
        result.bonusDamage = result.finalDamage - packet.damage;
    }

    return result;
}

ReactionResult ElementalSystem::CheckReaction(StatusEffect existing,
                                               const DamagePacket& incoming) {
    ReactionResult result;
    if (existing == StatusEffect::None || incoming.damageType == DamageType::Physical) {
        return result;
    }

    for (const auto& rule : kReactions) {
        if (rule.existing != existing || rule.incoming != incoming.damageType) continue;
        result.finalDamage     = static_cast<int>(incoming.damage * rule.multiplier);
        result.bonusDamage     = result.finalDamage - incoming.damage;
        result.resultingEffect = rule.result;
        result.effectDuration  = rule.resultDuration;
        result.reactionName    = rule.name;
        result.displayColor    = rule.color;
        return result;
    }
    // Same element twice: no reaction, the aura just gets refreshed upstream.
    return result;
}

float ElementalSystem::GetSpeedMultiplier(int entityId) const {
    switch (GetActiveEffect(entityId)) {
        case StatusEffect::Wet:     return WET_SPEED_MULT;
        case StatusEffect::Shocked: return SHOCK_SPEED_MULT;
        default:                    return 1.0f;
    }
}

float ElementalSystem::GetDamageTakenMultiplier(int entityId) const {
    return HasStatusEffect(entityId, StatusEffect::Corroded) ? CORRODE_VULNERABILITY : 1.0f;
}

void ElementalSystem::Update(float deltaTime) {
    std::vector<int> emptied;
    for (auto& pair : m_entityEffects) {
        auto& effects = pair.second;
        for (auto& e : effects) {
            e.Update(deltaTime);
            // Burn and Shock chip away while they last; Wet and Corroded only
            // change how other hits land.
            const float interval = (e.type == StatusEffect::Burn)    ? BURN_TICK_INTERVAL
                                 : (e.type == StatusEffect::Shocked) ? SHOCK_TICK_INTERVAL
                                                                     : 0.0f;
            if (interval <= 0.0f) continue;
            const int perTick = (e.type == StatusEffect::Burn) ? BURN_TICK_DAMAGE
                                                               : SHOCK_TICK_DAMAGE;
            e.tickAccumulator += deltaTime;
            while (e.tickAccumulator >= interval) {
                e.tickAccumulator -= interval;
                m_pendingTickDamage[pair.first] += perTick;
            }
        }
        effects.erase(
            std::remove_if(effects.begin(), effects.end(),
                [](const StatusEffectInstance& e) { return e.IsExpired(); }),
            effects.end());
        if (effects.empty()) emptied.push_back(pair.first);
    }
    for (int id : emptied) m_entityEffects.erase(id);
}

std::unordered_map<int, int> ElementalSystem::DrainTickDamage() {
    std::unordered_map<int, int> drained;
    drained.swap(m_pendingTickDamage);
    return drained;
}

DamagePacket ElementalSystem::CreateDamagePacket(int baseDamage, DamageType type,
                                                  StatusEffect effect, float duration) {
    return DamagePacket(baseDamage, type, effect, duration);
}

DamagePacket ElementalSystem::ElementalPacket(int baseDamage, DamageType type) {
    const ElementProfile& p = GetElementProfile(type);
    return DamagePacket(baseDamage, type, p.aura, p.auraDuration);
}
