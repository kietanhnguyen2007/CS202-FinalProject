#include "Model/GameState.h"
#include "Model/Chest.h"
#include "Model/Checkpoint.h"
#include "Model/LevelCompleteCup.h"
#include <algorithm>

GameState::GameState()
    : m_mode(GameMode::SinglePlayer)
    , m_currentLevel(1)
    , m_totalLevels(6)
{
}

GameState::GameState(GameMode mode)
    : m_mode(mode)
    , m_currentLevel(1)
    , m_totalLevels(1)
{
}

GameMode GameState::GetMode() const { return m_mode; }
void GameState::SetMode(GameMode mode) { m_mode = mode; }

Player* GameState::GetLocalPlayer() const { return m_localPlayer.get(); }
Player* GameState::GetSecondLocalPlayer() const { return m_secondLocalPlayer.get(); }

void GameState::SetLocalPlayer(std::unique_ptr<Player> player) {
    if (player && player->GetId() == 0) {
        player->SetId(GenerateEntityId());
    }
    m_localPlayer = std::move(player);
}

void GameState::SetSecondLocalPlayer(std::unique_ptr<Player> player) {
    if (player && player->GetId() == 0) {
        player->SetId(GenerateEntityId());
    }
    m_secondLocalPlayer = std::move(player);
}

void GameState::AddEntity(std::unique_ptr<Entity> entity) {
    // Factories may legitimately fail and return nullptr.  Never let a failed
    // creation enter the live entity list: most gameplay loops rely on this
    // container containing real entities only.
    if (!entity) return;

    if (entity->GetId() == 0) {
        entity->SetId(GenerateEntityId());
    }
    m_newEntities.push_back(std::move(entity));
}

void GameState::RemoveEntity(int entityId) {
    auto it = std::remove_if(m_entities.begin(), m_entities.end(),
        [entityId](const std::unique_ptr<Entity>& e) {
            return !e || e->GetId() == entityId;
        });
    m_entities.erase(it, m_entities.end());
}

std::unique_ptr<Entity> GameState::ExtractEntity(int entityId) {
    auto it = std::find_if(m_entities.begin(), m_entities.end(),
        [entityId](const std::unique_ptr<Entity>& e) {
            return e && e->GetId() == entityId;
        });
    if (it != m_entities.end()) {
        std::unique_ptr<Entity> e = std::move(*it);
        m_entities.erase(it);
        return e;
    }
    return nullptr;
}

Entity* GameState::GetEntity(int entityId) const {
    for (const auto& e : m_entities) {
        if (e && e->GetId() == entityId) return e.get();
    }
    return nullptr;
}

const std::vector<std::unique_ptr<Entity>>& GameState::GetAllEntities() const {
    return m_entities;
}

int GameState::GetCurrentLevel() const { return m_currentLevel; }
void GameState::SetCurrentLevel(int level) { m_currentLevel = level; }
int GameState::GetTotalLevels() const { return m_totalLevels; }

void GameState::AddTile(MapLayer layer, const Tile& tile) { 
    if(static_cast<int>(layer) >= 0 && static_cast<int>(layer) < static_cast<int>(MapLayer::Count)) {
        m_tiles[static_cast<int>(layer)].push_back(tile);
        InvalidateSolidGrid();
    }
}

void GameState::SetTileAt(MapLayer layer, int x, int y, int tileType, int tileId, bool solid, int flipFlags) {
    if(static_cast<int>(layer) < 0 || static_cast<int>(layer) >= static_cast<int>(MapLayer::Count)) return;
    
    // Check if tile exists, if so update it
    for (auto& t : m_tiles[static_cast<int>(layer)]) {
        if (t.x == x && t.y == y) {
            t.tileType = tileType;
            t.tileId = tileId;
            t.solid = solid;
            t.flipFlags = flipFlags;
            InvalidateSolidGrid();
            return;
        }
    }
    // Else add new
    Tile t;
    t.x = x; t.y = y; t.tileType = tileType; t.tileId = tileId; t.solid = solid; t.flipFlags = flipFlags;
    m_tiles[static_cast<int>(layer)].push_back(t);
    InvalidateSolidGrid();
}

void GameState::RemoveTileAt(MapLayer layer, int x, int y) {
    if(static_cast<int>(layer) < 0 || static_cast<int>(layer) >= static_cast<int>(MapLayer::Count)) return;
    auto& tiles = m_tiles[static_cast<int>(layer)];
    tiles.erase(std::remove_if(tiles.begin(), tiles.end(),
        [x, y](const Tile& t) { return t.x == x && t.y == y; }), tiles.end());
    InvalidateSolidGrid();
}

void GameState::RebuildSolidGrid() const {
    // Size from the map bounds, but grow to cover any tile authored outside
    // them -- a level file can carry geometry past the nominal map size.
    int w = std::max(0, m_mapWidth);
    int h = std::max(0, m_mapHeight);
    for (const auto& t : m_tiles[static_cast<int>(MapLayer::Main)]) {
        if (t.x >= w) w = t.x + 1;
        if (t.y >= h) h = t.y + 1;
    }

    m_solidGridWidth  = w;
    m_solidGridHeight = h;
    m_solidGrid.assign(static_cast<size_t>(w) * static_cast<size_t>(h), false);

    for (const auto& t : m_tiles[static_cast<int>(MapLayer::Main)]) {
        if (!t.solid) continue;
        if (t.x < 0 || t.y < 0 || t.x >= w || t.y >= h) continue;
        m_solidGrid[static_cast<size_t>(t.y) * static_cast<size_t>(w)
                    + static_cast<size_t>(t.x)] = true;
    }
    m_solidGridDirty = false;
}

bool GameState::IsSolidAt(int tileX, int tileY) const {
    if (m_solidGridDirty) RebuildSolidGrid();
    if (tileX < 0 || tileY < 0
        || tileX >= m_solidGridWidth || tileY >= m_solidGridHeight) {
        return false;
    }
    return m_solidGrid[static_cast<size_t>(tileY) * static_cast<size_t>(m_solidGridWidth)
                       + static_cast<size_t>(tileX)];
}

const std::vector<Tile>& GameState::GetTiles(MapLayer layer) const {
    if(static_cast<int>(layer) >= 0 && static_cast<int>(layer) < static_cast<int>(MapLayer::Count)) {
        return m_tiles[static_cast<int>(layer)];
    }
    static std::vector<Tile> empty;
    return empty;
}

void GameState::ClearTiles(MapLayer layer) {
    if(static_cast<int>(layer) >= 0 && static_cast<int>(layer) < static_cast<int>(MapLayer::Count)) {
        m_tiles[static_cast<int>(layer)].clear();
        InvalidateSolidGrid();
    }
}

void GameState::ClearAllTiles() {
    for (int i = 0; i < static_cast<int>(MapLayer::Count); ++i) {
        m_tiles[i].clear();
        InvalidateSolidGrid();
    }
}

Entity* GameState::GetEntityAt(float x, float y, float tolerance) const {
    if (m_localPlayer) {
        Vector2 pos = m_localPlayer->GetPosition();
        if (std::abs(pos.x - x) <= tolerance && std::abs(pos.y - y) <= tolerance) {
            return m_localPlayer.get();
        }
    }
    if (m_secondLocalPlayer) {
        Vector2 pos = m_secondLocalPlayer->GetPosition();
        if (std::abs(pos.x - x) <= tolerance && std::abs(pos.y - y) <= tolerance) {
            return m_secondLocalPlayer.get();
        }
    }
    for (const auto& e : m_entities) {
        if (!e) continue;
        Vector2 pos = e->GetPosition();
        if (std::abs(pos.x - x) <= tolerance && std::abs(pos.y - y) <= tolerance) {
            return e.get();
        }
    }
    return nullptr;
}

void GameState::RemoveEntityAt(float x, float y, float tolerance) {
    if (m_localPlayer) {
        Vector2 pos = m_localPlayer->GetPosition();
        if (std::abs(pos.x - x) <= tolerance && std::abs(pos.y - y) <= tolerance) {
            m_localPlayer.reset();
            return;
        }
    }
    if (m_secondLocalPlayer) {
        Vector2 pos = m_secondLocalPlayer->GetPosition();
        if (std::abs(pos.x - x) <= tolerance && std::abs(pos.y - y) <= tolerance) {
            m_secondLocalPlayer.reset();
            return;
        }
    }
    auto it = std::remove_if(m_entities.begin(), m_entities.end(),
        [x, y, tolerance](const std::unique_ptr<Entity>& e) {
            if (!e) return true;
            Vector2 pos = e->GetPosition();
            return std::abs(pos.x - x) <= tolerance && std::abs(pos.y - y) <= tolerance;
        });
    m_entities.erase(it, m_entities.end());
}

void GameState::ResizeMap(int newWidth, int newHeight) {
    // Clamp to 20x15 min, 500x150 max
    m_mapWidth = std::max(20, std::min(500, newWidth));
    m_mapHeight = std::max(15, std::min(150, newHeight));
    
    // Remove out-of-bound tiles
    for (int i = 0; i < static_cast<int>(MapLayer::Count); ++i) {
        auto& tiles = m_tiles[i];
        tiles.erase(std::remove_if(tiles.begin(), tiles.end(),
            [this](const Tile& t) { return t.x >= m_mapWidth || t.y >= m_mapHeight; }), tiles.end());
    }
    InvalidateSolidGrid();
}

void GameState::ClearMap() {
    Clear();
}
int GameState::GetMapWidth() const { return m_mapWidth; }
int GameState::GetMapHeight() const { return m_mapHeight; }
void GameState::SetMapSize(int width, int height) {
    m_mapWidth = width;
    InvalidateSolidGrid();
    m_mapHeight = height;
}

int GameState::GetTotalItems() const { return m_totalItems; }
void GameState::SetTotalItems(int count) { m_totalItems = count; }
int GameState::GetTotalEnemies() const { return m_totalEnemies; }
void GameState::SetTotalEnemies(int count) { m_totalEnemies = count; }

float GameState::GetClearTime() const { return m_clearTime; }
void GameState::ResetTimer() { m_clearTime = 0.0f; m_timerRunning = true; }
void GameState::StopTimer() { m_timerRunning = false; }
void GameState::SetTimerRunning(bool running) { m_timerRunning = running; }
void GameState::TickTimer(float deltaTime) {
    if (m_timerRunning) {
        m_clearTime += deltaTime;
    }
}

CharacterClass GameState::GetPlayerClass() const { return m_playerClass; }
void GameState::SetPlayerClass(CharacterClass playerClass) { m_playerClass = playerClass; }

BackgroundTheme GameState::GetBackgroundTheme() const { return m_backgroundTheme; }
void GameState::SetBackgroundTheme(BackgroundTheme theme) { m_backgroundTheme = theme; }

int GameState::GenerateEntityId() { return m_nextEntityId++; }

void GameState::PlayerInteract() {
    if (!m_localPlayer) return;
    Rectangle playerBox = m_localPlayer->GetBoundingBox();
    
    std::vector<std::unique_ptr<Entity>> newEntities;

    for (auto& entity : m_entities) {
        if (!entity) continue;
        if (entity->GetType() == EntityType::Chest) {
            Chest* chest = static_cast<Chest*>(entity.get());
            if (!chest->IsOpened()) {
                Rectangle chestBox = chest->GetBoundingBox();
                if (CheckCollisionRecs(playerBox, chestBox)) {
                    auto loot = chest->Open();
                    for (auto& item : loot) {
                        newEntities.push_back(std::move(item));
                    }
                }
            }
        }
    }

    for (auto& entity : newEntities) {
        AddEntity(std::move(entity));
    }
}

bool GameState::IsLevelComplete() const {
    if (!m_localPlayer) return false;

    // Player explicitly pressed F at the endgame checkpoint.
    if (m_levelCompleteByPlayer) return true;

    for (const auto& entity : m_entities) {
        if (!entity || !entity->IsActive()) continue;
        if (entity->GetType() == EntityType::LevelCompleteCup) {
            const auto* cup = static_cast<const LevelCompleteCup*>(entity.get());
            if (cup->IsActivated()) return true;
        } else if (entity->GetType() == EntityType::Checkpoint) {
            const auto* cp = static_cast<const Checkpoint*>(entity.get());
            if (cp->IsEndGame() && cp->IsActivated()) return true;
        }
    }

    // Completion is an explicit interaction. Merely touching a finish marker,
    // entering its viewport, or clearing every enemy must not end the level.
    return false;
}

void GameState::MergeNewEntities() {
    if (m_newEntities.empty()) return;

    for (auto& entity : m_newEntities) {
        if (entity) {
            m_entities.push_back(std::move(entity));
        }
    }
    m_newEntities.clear();
}

void GameState::Update(float deltaTime) {
    // Always merge new entities immediately so they are visible (e.g. in Map Builder)
    MergeNewEntities();

    if (!m_timerRunning) return;

    if (m_localPlayer) {
        m_localPlayer->Update(deltaTime);
    }
    if (m_secondLocalPlayer) {
        m_secondLocalPlayer->Update(deltaTime);
    }
    for (auto& entity : m_entities) {
        if (entity && entity->IsActive()) {
            entity->Update(deltaTime);
        }
    }
}

void GameState::Clear() {
    m_entities.clear();
    m_newEntities.clear();
    m_localPlayer.reset();
    m_secondLocalPlayer.reset();
    for(int i = 0; i < static_cast<int>(MapLayer::Count); ++i) {
        m_tiles[i].clear();
        InvalidateSolidGrid();
    }
    m_nextEntityId = 1;
    m_clearTime = 0.0f;
    m_timerRunning = false;
    m_levelCompleteByPlayer = false;
}

void GameState::SetLevelCompleteByPlayer(bool val) {
    m_levelCompleteByPlayer = val;
}
