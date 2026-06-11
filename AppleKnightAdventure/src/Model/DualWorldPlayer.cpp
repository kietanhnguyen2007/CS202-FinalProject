#include "Model/DualWorldPlayer.h"

DualWorldPlayer::DualWorldPlayer()
    : Player()
    , m_layer(WorldLayer::Light)
{
    m_type = EntityType::DualWorldPlayer;
}

DualWorldPlayer::DualWorldPlayer(Vector2 position, WorldLayer layer)
    : Player(position)
    , m_layer(layer)
{
    m_type = EntityType::DualWorldPlayer;
}

void DualWorldPlayer::Update(float deltaTime) {
    Player::Update(deltaTime);
}

void DualWorldPlayer::Render() {
    Player::Render();
}

WorldLayer DualWorldPlayer::GetLayer() const { return m_layer; }
void DualWorldPlayer::SetLayer(WorldLayer layer) { m_layer = layer; }

void DualWorldPlayer::SwitchLayer() {
    m_layer = (m_layer == WorldLayer::Light) ? WorldLayer::Shadow : WorldLayer::Light;
}
