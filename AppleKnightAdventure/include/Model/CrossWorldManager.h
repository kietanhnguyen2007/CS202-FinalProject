#ifndef CROSSWORLDMANAGER_H
#define CROSSWORLDMANAGER_H

#include "DualWorld.h"
#include "DualWorldPlayer.h"
#include <vector>

class CrossWorldManager {
protected:
    DualWorld* m_world;
    std::vector<DualWorldPlayer*> m_players;

public:
    CrossWorldManager();
    explicit CrossWorldManager(DualWorld* world);

    void SetWorld(DualWorld* world);
    DualWorld* GetWorld() const;

    void RegisterPlayer(DualWorldPlayer* player);
    void UnregisterPlayer(DualWorldPlayer* player);
    const std::vector<DualWorldPlayer*>& GetPlayers() const;

    void Update(float deltaTime);
    bool CanPlayerCross(DualWorldPlayer* player) const;
    bool TeleportPlayerToOtherWorld(DualWorldPlayer* player);
    std::vector<DualWorldPlayer*> GetPlayersInLayer(WorldLayer layer) const;
};

#endif
