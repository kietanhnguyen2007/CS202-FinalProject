#ifndef CHARACTERSKILLSET_H
#define CHARACTERSKILLSET_H

// Abstract base class for all character skill sets.
// GameController dispatches input to concrete subclasses via this interface.
class CharacterSkillSet {
public:
    virtual ~CharacterSkillSet() = default;
    virtual void Update(float deltaTime) = 0;

    // Drain every cooldown by an extra amount without advancing charge or
    // active windows -- used by the Adrenaline boon.
    virtual void TickCooldowns(float deltaTime) = 0;
    // Make every skill immediately usable again -- used by the Focus boon.
    virtual void ClearCooldowns() = 0;
};

#endif
