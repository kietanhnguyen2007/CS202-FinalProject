#ifndef ELEMENTALSYSTEM_H
#define ELEMENTALSYSTEM_H

#include "Utils/Types.h"
#include "Model/Entity.h"
#include <string>
#include <unordered_map>
#include <vector>

struct StatusEffectInstance {
    StatusEffect type;
    float duration;
    float timer;

    StatusEffectInstance();
    StatusEffectInstance(StatusEffect type, float duration);
    void Update(float deltaTime);
    bool IsExpired() const;
};

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

struct ReactionResult {
    int bonusDamage;
    StatusEffect resultingEffect;
    float effectDuration;
    std::string reactionName;

    ReactionResult();
};

class ElementalSystem {
protected:
    std::unordered_map<int, std::vector<StatusEffectInstance>> m_entityEffects;

public:
    ElementalSystem();

    void ApplyStatusEffect(int entityId, StatusEffect effect, float duration);
    void RemoveStatusEffect(int entityId, StatusEffect effect);
    void ClearEffects(int entityId);
    bool HasStatusEffect(int entityId, StatusEffect effect) const;
    const std::vector<StatusEffectInstance>* GetActiveEffects(int entityId) const;

    ReactionResult ApplyReaction(int entityId, const DamagePacket& packet);
    static ReactionResult CheckReaction(StatusEffect existing,
                                         const DamagePacket& incoming);

    void Update(float deltaTime);

    DamagePacket CreateDamagePacket(int baseDamage, DamageType type,
                                    StatusEffect effect = StatusEffect::None,
                                    float duration = 0.0f);
};

#endif
