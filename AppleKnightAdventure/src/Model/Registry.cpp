#include "Model/Registry.h"

Registry& Registry::GetInstance() {
    static Registry instance;
    return instance;
}

void Registry::Register(Entity* entity) {
    if (!entity) return;
    m_entities[entity->GetId()] = entity;
}

void Registry::Unregister(int entityId) {
    m_entities.erase(entityId);
}

Entity* Registry::Get(int entityId) {
    auto it = m_entities.find(entityId);
    return (it != m_entities.end()) ? it->second : nullptr;
}

const Entity* Registry::Get(int entityId) const {
    auto it = m_entities.find(entityId);
    return (it != m_entities.end()) ? it->second : nullptr;
}

void Registry::Clear() {
    m_entities.clear();
}
