#ifndef GAMESTATE_H
#define GAMESTATE_H

#include "Entity.h"
#include "Player.h"
#include "../Utils/Types.h"
#include <vector>
#include <memory>

class GameState {
protected:
    GameMode m_mode;
    std::unique_ptr<Player> m_localPlayer;
    std::vector<std::unique_ptr<Entity>> m_entities;
    int m_currentLevel;
    int m_totalLevels;

public:
    GameState();
    explicit GameState(GameMode mode);

    GameMode GetMode() const;
    void SetMode(GameMode mode);

    Player* GetLocalPlayer() const;
    void SetLocalPlayer(std::unique_ptr<Player> player);

    void AddEntity(std::unique_ptr<Entity> entity);
    void RemoveEntity(int entityId);
    Entity* GetEntity(int entityId) const;
    const std::vector<std::unique_ptr<Entity>>& GetAllEntities() const;

    int GetCurrentLevel() const;
    void SetCurrentLevel(int level);
    int GetTotalLevels() const;

    void Update(float deltaTime);
    void Clear();
};

#endif
