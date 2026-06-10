#include "../../include/Model/CrossWorldManager.h"
#include <algorithm>

CrossWorldManager::CrossWorldManager()
    : m_world(nullptr)
{
}

CrossWorldManager::CrossWorldManager(DualWorld* world)
    : m_world(world)
{
}

void CrossWorldManager::SetWorld(DualWorld* world) { m_world = world; }
DualWorld* CrossWorldManager::GetWorld() const { return m_world; }

void CrossWorldManager::RegisterPlayer(DualWorldPlayer* player) {
    if (player && std::find(m_players.begin(), m_players.end(), player) == m_players.end()) {
        m_players.push_back(player);
    }
}

void CrossWorldManager::UnregisterPlayer(DualWorldPlayer* player) {
    auto it = std::remove(m_players.begin(), m_players.end(), player);
    m_players.erase(it, m_players.end());
}

const std::vector<DualWorldPlayer*>& CrossWorldManager::GetPlayers() const {
    return m_players;
}

void CrossWorldManager::Update(float deltaTime) {
    for (auto* player : m_players) {
        player->Update(deltaTime);
    }
}

bool CrossWorldManager::CanPlayerCross(DualWorldPlayer* player) const {
    if (!player || !m_world) return false;
    return true;
}

bool CrossWorldManager::TeleportPlayerToOtherWorld(DualWorldPlayer* player) {
    if (!player) return false;
    player->SwitchLayer();
    if (m_world) {
        m_world->SwitchLayer();
    }
    return true;
}

std::vector<DualWorldPlayer*> CrossWorldManager::GetPlayersInLayer(WorldLayer layer) const {
    std::vector<DualWorldPlayer*> result;
    for (auto* player : m_players) {
        if (player && player->GetLayer() == layer) {
            result.push_back(player);
        }
    }
    return result;
}
