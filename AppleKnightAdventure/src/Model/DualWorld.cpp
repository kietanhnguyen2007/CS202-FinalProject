#include "Model/DualWorld.h"

DualWorld::DualWorld()
    : m_activeLayer(WorldLayer::Light)
    , m_width(0)
    , m_height(0)
{
}

DualWorld::DualWorld(int width, int height)
    : m_activeLayer(WorldLayer::Light)
    , m_width(width)
    , m_height(height)
{
}

WorldLayer DualWorld::GetActiveLayer() const { return m_activeLayer; }
void DualWorld::SetActiveLayer(WorldLayer layer) { m_activeLayer = layer; }

void DualWorld::SwitchLayer() {
    m_activeLayer = (m_activeLayer == WorldLayer::Light) ? WorldLayer::Shadow : WorldLayer::Light;
}

int DualWorld::GetWidth() const { return m_width; }
int DualWorld::GetHeight() const { return m_height; }

const std::vector<Tile>& DualWorld::GetTiles(WorldLayer layer) const {
    return (layer == WorldLayer::Light) ? m_lightTiles : m_shadowTiles;
}

void DualWorld::AddTile(WorldLayer layer, Tile tile) {
    if (layer == WorldLayer::Light) {
        m_lightTiles.push_back(tile);
    } else {
        m_shadowTiles.push_back(tile);
    }
}

bool DualWorld::IsTileSolid(int x, int y, WorldLayer layer) const {
    const auto& tiles = (layer == WorldLayer::Light) ? m_lightTiles : m_shadowTiles;
    for (const auto& tile : tiles) {
        if (tile.x == x && tile.y == y && tile.tileType == 1) {
            return true;
        }
    }
    return false;
}
