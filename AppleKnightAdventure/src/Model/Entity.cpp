#include "Model/Entity.h"

Entity::Entity()
    : m_id(0)
    , m_type(EntityType::Effect)
    , m_position({0, 0})
    , m_size({0, 0})
    , m_velocity({0, 0})
    , m_rotation(0.0f)
    , m_scale(1.0f)
    , m_active(true)
{
}

Entity::Entity(EntityType type)
    : m_id(0)
    , m_type(type)
    , m_position({0, 0})
    , m_size({0, 0})
    , m_velocity({0, 0})
    , m_rotation(0.0f)
    , m_scale(1.0f)
    , m_active(true)
{
}

Entity::Entity(Vector2 position, Vector2 size, EntityType type)
    : m_id(0)
    , m_type(type)
    , m_position(position)
    , m_size(size)
    , m_velocity({0, 0})
    , m_rotation(0.0f)
    , m_scale(1.0f)
    , m_active(true)
{
}

int Entity::GetId() const { return m_id; }
void Entity::SetId(int id) { m_id = id; }
EntityType Entity::GetType() const { return m_type; }

Vector2 Entity::GetPosition() const { return m_position; }
void Entity::SetPosition(Vector2 position) { m_position = position; }
Vector2 Entity::GetSize() const { return m_size; }
void Entity::SetSize(Vector2 size) { m_size = size; }
Vector2 Entity::GetVelocity() const { return m_velocity; }
void Entity::SetVelocity(Vector2 velocity) { m_velocity = velocity; }
float Entity::GetRotation() const { return m_rotation; }
void Entity::SetRotation(float rotation) { m_rotation = rotation; }
float Entity::GetScale() const { return m_scale; }
void Entity::SetScale(float scale) { m_scale = scale; }

bool Entity::IsActive() const { return m_active; }
void Entity::SetActive(bool active) { m_active = active; }

Rectangle Entity::GetBoundingBox() const {
    return {m_position.x, m_position.y, m_size.x * m_scale, m_size.y * m_scale};
}
