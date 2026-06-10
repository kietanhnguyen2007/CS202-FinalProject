#include "../../include/Model/GameState.h"
#include <algorithm>

GameState::GameState()
    : m_mode(GameMode::SinglePlayer)
    , m_currentLevel(1)
    , m_totalLevels(1)
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
    m_localPlayer = std::move(player);
}

void GameState::AddEntity(std::unique_ptr<Entity> entity) {
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
}
