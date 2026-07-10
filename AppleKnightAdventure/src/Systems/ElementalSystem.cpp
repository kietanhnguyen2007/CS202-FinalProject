#include "Systems/ElementalSystem.h"
#include <algorithm>
#include <cmath>

StatusEffectInstance::StatusEffectInstance()
    : type(StatusEffect::None)
    , duration(0.0f)
    , timer(0.0f)
{
}

StatusEffectInstance::StatusEffectInstance(StatusEffect type, float duration)
    : type(type)
    , duration(duration)
    , timer(0.0f)
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
    : bonusDamage(0)
    , resultingEffect(StatusEffect::None)
    , effectDuration(0.0f)
{
}

ElementalSystem::ElementalSystem() {
}

void ElementalSystem::ApplyStatusEffect(int entityId, StatusEffect effect, float duration) {
    if (effect == StatusEffect::None) return;
    auto& effects = m_entityEffects[entityId];
    for (auto& e : effects) {
        if (e.type == effect) {
            e.timer = 0.0f;
            e.duration = std::max(e.duration, duration);
            return;
        }
    }
    effects.emplace_back(effect, duration);
}

void ElementalSystem::RemoveStatusEffect(int entityId, StatusEffect effect) {
    auto it = m_entityEffects.find(entityId);
    if (it != m_entityEffects.end()) {
        auto& effects = it->second;
        effects.erase(
            std::remove_if(effects.begin(), effects.end(),
                [effect](const StatusEffectInstance& e) {
                    return e.type == effect;
                }),
            effects.end());
        if (effects.empty()) {
            m_entityEffects.erase(it);
        }
    }
}

void ElementalSystem::ClearEffects(int entityId) {
    m_entityEffects.erase(entityId);
}

bool ElementalSystem::HasStatusEffect(int entityId, StatusEffect effect) const {
    auto it = m_entityEffects.find(entityId);
    if (it != m_entityEffects.end()) {
        for (const auto& e : it->second) {
            if (e.type == effect) return true;
        }
    }
    return false;
}

const std::vector<StatusEffectInstance>* ElementalSystem::GetActiveEffects(int entityId) const {
    auto it = m_entityEffects.find(entityId);
    if (it != m_entityEffects.end()) {
        return &it->second;
    }
    return nullptr;
}

ReactionResult ElementalSystem::ApplyReaction(int entityId, const DamagePacket& packet) {
    ReactionResult result;
    if (HasStatusEffect(entityId, StatusEffect::None)) {
        ClearEffects(entityId);
    }

    for (const auto& existing : m_entityEffects[entityId]) {
        if (existing.type != StatusEffect::None) {
            result = CheckReaction(existing.type, packet);
            break;
        }
    }

    if (result.reactionName.empty()) {
        if (packet.statusEffect != StatusEffect::None) {
            ApplyStatusEffect(entityId, packet.statusEffect, packet.effectDuration);
        }
    } else {
        ClearEffects(entityId);
        if (result.resultingEffect != StatusEffect::None) {
            ApplyStatusEffect(entityId, result.resultingEffect, result.effectDuration);
        }
    }

    return result;
}

ReactionResult ElementalSystem::CheckReaction(StatusEffect existing,
                                               const DamagePacket& incoming) {
    ReactionResult result;

    if (existing == StatusEffect::Burn && incoming.damageType == DamageType::Water) {
        result.bonusDamage = incoming.damage;
        result.reactionName = "Vaporize";
    }
    else if (existing == StatusEffect::Wet && incoming.damageType == DamageType::Thunder) {
        result.bonusDamage = static_cast<int>(incoming.damage * 1.5f);
        result.resultingEffect = StatusEffect::Shocked;
        result.effectDuration = 2.0f;
        result.reactionName = "Conduct";
    }
    else if (existing == StatusEffect::Wet && incoming.damageType == DamageType::Fire) {
        result.bonusDamage = static_cast<int>(incoming.damage * 1.2f);
        result.reactionName = "Vaporize";
    }
    else if (existing == StatusEffect::Shocked && incoming.damageType == DamageType::Fire) {
        result.bonusDamage = static_cast<int>(incoming.damage * 2.0f);
        result.resultingEffect = StatusEffect::Burn;
        result.effectDuration = 3.0f;
        result.reactionName = "Overload";
    }
    else if (existing == StatusEffect::Burn && incoming.damageType == DamageType::Thunder) {
        result.bonusDamage = static_cast<int>(incoming.damage * 1.8f);
        result.reactionName = "Overload";
    }

    return result;
}

void ElementalSystem::Update(float deltaTime) {
    std::vector<int> expiredEntities;
    for (auto& pair : m_entityEffects) {
        auto& effects = pair.second;
        effects.erase(
            std::remove_if(effects.begin(), effects.end(),
                [deltaTime](StatusEffectInstance& e) {
                    e.Update(deltaTime);
                    return e.IsExpired();
                }),
            effects.end());
        if (effects.empty()) {
            expiredEntities.push_back(pair.first);
        }
    }
    for (int id : expiredEntities) {
        m_entityEffects.erase(id);
    }
}

DamagePacket ElementalSystem::CreateDamagePacket(int baseDamage, DamageType type,
                                                  StatusEffect effect, float duration) {
    return DamagePacket(baseDamage, type, effect, duration);
}
