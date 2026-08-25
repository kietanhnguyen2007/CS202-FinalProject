#pragma once

#include "raylib.h"
#include <cstdint>
#include <set>
#include <vector>

class GameState;
class Player;

namespace View {

class MinimapView {
public:
    static MinimapView& GetInstance();

    bool Init();
    void Shutdown();
    void BeginLevel(const GameState* state);
    void Update(float dt, const GameState* state, const Player* playerOne,
                const Player* playerTwo = nullptr);
    void Render(const GameState* state, const Player* playerOne,
                const Player* playerTwo = nullptr) const;

    void ToggleVisible() { m_visible = !m_visible; }
    bool IsVisible() const { return m_visible; }
    float GetExploredRatio() const { return m_exploredRatio; }

private:
    MinimapView() = default;
    int EncodeCell(int cellX, int cellY) const;
    bool IsCellExplored(int cellX, int cellY) const;
    bool MarkCellExplored(int cellX, int cellY);
    int CellIndex(int cellX, int cellY) const;
    Vector2 WorldToMap(Vector2 world, Rectangle mapRect) const;

    static constexpr int kCellTiles = 2;
    static constexpr int kEncodedRowStride = 100000;

    bool m_initialized = false;
    bool m_visible = true;
    int m_level = 0;
    int m_mapWidthTiles = 1;
    int m_mapHeightTiles = 1;
    int m_cellsX = 1;
    int m_cellsY = 1;
    std::vector<std::uint8_t> m_terrain;
    std::vector<std::uint8_t> m_relevant;
    std::set<int> m_sessionExplored;
    float m_exploredRatio = 0.0f;
    Vector2 m_unexploredDirection{};
    bool m_hasUnexploredDirection = false;
    float m_pulse = 0.0f;
    float m_discoveryFlash = 0.0f;
    Font m_font{};
};

} // namespace View
