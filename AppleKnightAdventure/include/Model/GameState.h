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
    int x        = 0;
    int y        = 0;
    int tileType = 1;   // tilesheet index (1=Tiles.png … 6=Tree-Assets.png)
    int tileId   = -1;  // linear index trong tilesheet (LDtk field "t")
    bool solid   = true;
    int flipFlags = 0;  // LDtk "f": 0=none, 1=flipX, 2=flipY, 3=flipXY
};

enum class MapLayer {
    Background = 0,
    Main = 1,
    Foreground = 2,
    Count
};

class GameState {
protected:
    GameMode m_mode;
    std::unique_ptr<Player> m_localPlayer;
    std::unique_ptr<Player> m_secondLocalPlayer;
    std::vector<std::unique_ptr<Entity>> m_entities;
    std::vector<std::unique_ptr<Entity>> m_newEntities; // Buffer for dynamically added entities
    std::vector<Tile> m_tiles[static_cast<int>(MapLayer::Count)];
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
    Player* GetSecondLocalPlayer() const;
    void SetLocalPlayer(std::unique_ptr<Player> player);
    void SetSecondLocalPlayer(std::unique_ptr<Player> player);
    void AddEntity(std::unique_ptr<Entity> entity);
    void MergeNewEntities();
    void RemoveEntity(int entityId);
    std::unique_ptr<Entity> ExtractEntity(int entityId);
    Entity* GetEntity(int entityId) const;
    const std::vector<std::unique_ptr<Entity>>& GetAllEntities() const;

    int GetCurrentLevel() const;
    void SetCurrentLevel(int level);
    int GetTotalLevels() const;

    void AddTile(MapLayer layer, const Tile& tile);
    void SetTileAt(MapLayer layer, int x, int y, int tileType, int tileId, bool solid, int flipFlags);
    void RemoveTileAt(MapLayer layer, int x, int y);
    const std::vector<Tile>& GetTiles(MapLayer layer) const;
    void ClearTiles(MapLayer layer);
    void ClearAllTiles();

    Entity* GetEntityAt(float x, float y, float tolerance = 32.0f) const;
    void RemoveEntityAt(float x, float y, float tolerance = 32.0f);
    void ResizeMap(int newWidth, int newHeight);
    void ClearMap();

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
