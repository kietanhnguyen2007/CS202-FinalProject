#include "Model/Pet.h"
#include <cmath>

Pet::Pet()
    : Character(EntityType::Pet)
    , m_petType(PetType::Skull)
    , m_ownerId(-1)
    , m_followDistance(PET_FOLLOW_DISTANCE)
{
    m_speed = PET_SPEED;
}

Pet::Pet(Vector2 position, PetType type, int ownerId)
    : Character(position, {TILE_SIZE * 0.5f, TILE_SIZE * 0.5f}, EntityType::Pet)
    , m_petType(type)
    , m_ownerId(ownerId)
    , m_followDistance(PET_FOLLOW_DISTANCE)
{
    m_speed = PET_SPEED;
    switch (type) {
        case PetType::Skull: m_damage = 8; break;
        case PetType::Ghost: m_damage = 6; break;
        case PetType::BabyDragon: m_damage = 12; break;
        case PetType::Fairy: m_damage = 4; break;
    }
}

void Pet::Update(float deltaTime) {
    Character::Update(deltaTime);
}


PetType Pet::GetPetType() const { return m_petType; }
int Pet::GetOwnerId() const { return m_ownerId; }
void Pet::SetOwnerId(int ownerId) { m_ownerId = ownerId; }
float Pet::GetFollowDistance() const { return m_followDistance; }
void Pet::SetFollowDistance(float distance) { m_followDistance = distance; }

void Pet::FollowPlayer(Vector2 playerPosition, float deltaTime) {
    float dx = playerPosition.x - m_position.x;
    float dy = playerPosition.y - m_position.y;
    float dist = std::sqrt(dx * dx + dy * dy);
    if (dist > m_followDistance && dist > 0) {
        Vector2 dir = {dx / dist, dy / dist};
        Move(dir, deltaTime);
        m_direction = (dx > 0) ? Direction::Right : Direction::Left;
    }
}

void Pet::UpdateAI(Vector2 playerPosition, float deltaTime) {
    FollowPlayer(playerPosition, deltaTime);
}
