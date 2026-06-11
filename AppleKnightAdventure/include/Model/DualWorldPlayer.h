#ifndef DUALWORLDPLAYER_H
#define DUALWORLDPLAYER_H

#include "Player.h"
#include "Utils/Types.h"

class DualWorldPlayer : public Player {
protected:
    WorldLayer m_layer;

public:
    DualWorldPlayer();
    explicit DualWorldPlayer(Vector2 position, WorldLayer layer = WorldLayer::Light);

    void Update(float deltaTime) override;
    void Render() override;

    WorldLayer GetLayer() const;
    void SetLayer(WorldLayer layer);
    void SwitchLayer();
};

#endif
