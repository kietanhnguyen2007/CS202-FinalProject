#ifndef PET_H
#define PET_H

#include "Character.h"
#include "../Utils/Types.h"
#include "../Utils/Constants.h"

class Pet : public Character {
protected:
    PetType m_petType;
    int m_ownerId;
    float m_followDistance;

public:
    Pet();
    Pet(Vector2 position, PetType type, int ownerId);

    void Update(float deltaTime) override;
    void Render() override;

    PetType GetPetType() const;
    int GetOwnerId() const;
    void SetOwnerId(int ownerId);

    float GetFollowDistance() const;
    void SetFollowDistance(float distance);

    void FollowPlayer(Vector2 playerPosition, float deltaTime);
    void UpdateAI(Vector2 playerPosition, float deltaTime);
};

#endif
