#ifndef DUALWORLD_H
#define DUALWORLD_H

#include "Model/GameState.h"
#include "Utils/Types.h"
#include <vector>

class DualWorld {
protected:
    WorldLayer m_activeLayer;
    std::vector<Tile> m_lightTiles;
    std::vector<Tile> m_shadowTiles;
    int m_width;
    int m_height;

public:
    DualWorld();
    DualWorld(int width, int height);

    WorldLayer GetActiveLayer() const;
    void SetActiveLayer(WorldLayer layer);
    void SwitchLayer();

    int GetWidth() const;
    int GetHeight() const;

    const std::vector<Tile>& GetTiles(WorldLayer layer) const;
    void AddTile(WorldLayer layer, Tile tile);
    bool IsTileSolid(int x, int y, WorldLayer layer) const;
};

#endif
