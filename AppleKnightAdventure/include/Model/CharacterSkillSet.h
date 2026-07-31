#ifndef CHARACTERSKILLSET_H
#define CHARACTERSKILLSET_H

// Abstract base class for all character skill sets.
// GameController dispatches input to concrete subclasses via this interface.
class CharacterSkillSet {
public:
    virtual ~CharacterSkillSet() = default;
    virtual void Update(float deltaTime) = 0;
};

#endif
