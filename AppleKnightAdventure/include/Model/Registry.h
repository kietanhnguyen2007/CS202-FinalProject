#pragma once

#include "Model/Entity.h"
#include <unordered_map>

class Registry {
public:
    static Registry& GetInstance();

    void Register(Entity* entity);
    void Unregister(int entityId);
    Entity* Get(int entityId);
    const Entity* Get(int entityId) const;
    void Clear();

private:
    Registry() = default;
    ~Registry() = default;
    Registry(const Registry&) = delete;
    Registry& operator=(const Registry&) = delete;

    std::unordered_map<int, Entity*> m_entities;
};
