#ifndef GAMESTATE_H
#define GAMESTATE_H

#include "Entity.h"
#include "Player.h"
#include "Utils/Types.h"
#include <vector>
#include <memory>
#include <cstdint>

// Which parallax background set to use for a level.
// Map builder sets this via GameState::SetBackgroundTheme() when loading a level.
enum class BackgroundTheme : uint8_t {
    Forest       = 0,   // Ansimuz Parallax Forest v2
    ColdCorridor = 1,   // Ansimuz Gothicvania Cold Corridors
    Underwater   = 2,   // Ansimuz Underwater Fantasy
};

struct Tile {
    int x = 0;
    int y = 0;
    int tileType = 1;
    int tileId = -1;
    bool solid = true;
};

class GameState {
protected:
    GameMode m_mode;
    std::unique_ptr<Player> m_localPlayer;
    std::vector<std::unique_ptr<Entity>> m_entities;
    std::vector<Tile> m_tiles;
    int m_currentLevel;
    int m_totalLevels;
    int m_mapWidth = 0;
    int m_mapHeight = 0;
    int m_totalItems = 0;
    int m_totalEnemies = 0;
    int m_nextEntityId = 1;
    float m_clearTime = 0.0f;
    bool m_timerRunning = false;
    CharacterClass m_playerClass = CharacterClass::Knight;
    BackgroundTheme m_backgroundTheme = BackgroundTheme::Forest;

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

    void AddTile(const Tile& tile);
    const std::vector<Tile>& GetTiles() const;
    void ClearTiles();
    int GetMapWidth() const;
    int GetMapHeight() const;
    void SetMapSize(int width, int height);

    int GetTotalItems() const;
    void SetTotalItems(int count);
    int GetTotalEnemies() const;
    void SetTotalEnemies(int count);

    float GetClearTime() const;
    void ResetTimer();
    void StopTimer();
    void SetTimerRunning(bool running);
    void TickTimer(float deltaTime);

    CharacterClass GetPlayerClass() const;
    void SetPlayerClass(CharacterClass playerClass);

    BackgroundTheme GetBackgroundTheme() const;
    void SetBackgroundTheme(BackgroundTheme theme);

    int GenerateEntityId();

    void PlayerInteract();
    bool IsLevelComplete() const;

    void Update(float deltaTime);
    void Clear();
};

#endif
