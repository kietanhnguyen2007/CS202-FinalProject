#include "Model/GameState.h"
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

void GameState::SetLocalPlayer(std::unique_ptr<Player> player) {
    if (player && player->GetId() == 0) {
        player->SetId(GenerateEntityId());
    }
    m_localPlayer = std::move(player);
}

void GameState::AddEntity(std::unique_ptr<Entity> entity) {
    if (entity && entity->GetId() == 0) {
        entity->SetId(GenerateEntityId());
    }
    m_entities.push_back(std::move(entity));
}

void GameState::RemoveEntity(int entityId) {
    auto it = std::remove_if(m_entities.begin(), m_entities.end(),
        [entityId](const std::unique_ptr<Entity>& e) {
            return e->GetId() == entityId;
        });
    m_entities.erase(it, m_entities.end());
}

Entity* GameState::GetEntity(int entityId) const {
    for (const auto& e : m_entities) {
        if (e->GetId() == entityId) return e.get();
    }
    return nullptr;
}

const std::vector<std::unique_ptr<Entity>>& GameState::GetAllEntities() const {
    return m_entities;
}

int GameState::GetCurrentLevel() const { return m_currentLevel; }
void GameState::SetCurrentLevel(int level) { m_currentLevel = level; }
int GameState::GetTotalLevels() const { return m_totalLevels; }

void GameState::AddTile(const Tile& tile) { m_tiles.push_back(tile); }
const std::vector<Tile>& GameState::GetTiles() const { return m_tiles; }
void GameState::ClearTiles() { m_tiles.clear(); }
int GameState::GetMapWidth() const { return m_mapWidth; }
int GameState::GetMapHeight() const { return m_mapHeight; }
void GameState::SetMapSize(int width, int height) {
    m_mapWidth = width;
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

void GameState::Update(float deltaTime) {
    if (m_localPlayer) {
        m_localPlayer->Update(deltaTime);
    }
    for (auto& entity : m_entities) {
        entity->Update(deltaTime);
    }
}

void GameState::Clear() {
    m_entities.clear();
    m_localPlayer.reset();
    m_tiles.clear();
    m_nextEntityId = 1;
    m_clearTime = 0.0f;
    m_timerRunning = false;
}
