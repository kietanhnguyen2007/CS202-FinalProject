#include "View/MinimapView.h"
#include "Model/GameState.h"
#include "Model/Player.h"
#include "Model/SaveManager.h"
#include "Utils/Constants.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>

namespace View {

MinimapView& MinimapView::GetInstance() {
    static MinimapView instance;
    return instance;
}

bool MinimapView::Init() {
    if (m_initialized) return true;
    m_font = LoadFont("assets/fonts/game_font.ttf");
    m_initialized = true;
    return true;
}

void MinimapView::Shutdown() {
    if (!m_initialized) return;
    if (m_font.texture.id != 0) UnloadFont(m_font);
    m_font = {};
    m_terrain.clear();
    m_relevant.clear();
    m_sessionExplored.clear();
    m_initialized = false;
}

int MinimapView::CellIndex(int cellX, int cellY) const {
    if (cellX < 0 || cellY < 0 || cellX >= m_cellsX || cellY >= m_cellsY) return -1;
    return cellY * m_cellsX + cellX;
}

int MinimapView::EncodeCell(int cellX, int cellY) const {
    return cellY * kEncodedRowStride + cellX;
}

bool MinimapView::IsCellExplored(int cellX, int cellY) const {
    if (CellIndex(cellX, cellY) < 0) return false;
    const int encoded = EncodeCell(cellX, cellY);
    if (m_level >= 1 && m_level <= 6)
        return SaveManager::GetInstance().GetExploredMinimapCells(m_level).count(encoded) != 0;
    return m_sessionExplored.count(encoded) != 0;
}

bool MinimapView::MarkCellExplored(int cellX, int cellY) {
    if (CellIndex(cellX, cellY) < 0 || IsCellExplored(cellX, cellY)) return false;
    const int encoded = EncodeCell(cellX, cellY);
    if (m_level >= 1 && m_level <= 6)
        SaveManager::GetInstance().MarkMinimapCellExplored(m_level, encoded);
    else
        m_sessionExplored.insert(encoded);
    return true;
}

void MinimapView::BeginLevel(const GameState* state) {
    if (!state) return;
    m_level = state->GetCurrentLevel();
    m_mapWidthTiles = std::max(1, state->GetMapWidth());
    m_mapHeightTiles = std::max(1, state->GetMapHeight());

    // Legacy/custom maps may not contain explicit map dimensions.
    for (const auto& tile : state->GetTiles(MapLayer::Main)) {
        m_mapWidthTiles = std::max(m_mapWidthTiles, tile.x + 1);
        m_mapHeightTiles = std::max(m_mapHeightTiles, tile.y + 1);
    }
    for (const auto& entity : state->GetAllEntities()) {
        if (!entity) continue;
        const Rectangle bounds = entity->GetBoundingBox();
        m_mapWidthTiles = std::max(m_mapWidthTiles,
            static_cast<int>(std::ceil((bounds.x + bounds.width) / TILE_SIZE)));
        m_mapHeightTiles = std::max(m_mapHeightTiles,
            static_cast<int>(std::ceil((bounds.y + bounds.height) / TILE_SIZE)));
    }

    m_cellsX = std::max(1, (m_mapWidthTiles + kCellTiles - 1) / kCellTiles);
    m_cellsY = std::max(1, (m_mapHeightTiles + kCellTiles - 1) / kCellTiles);
    m_terrain.assign(m_cellsX * m_cellsY, 0);
    m_relevant.assign(m_cellsX * m_cellsY, 0);
    m_sessionExplored.clear();

    for (const auto& tile : state->GetTiles(MapLayer::Main)) {
        if (!tile.solid) continue;
        const int cellX = tile.x / kCellTiles;
        const int cellY = tile.y / kCellTiles;
        const int index = CellIndex(cellX, cellY);
        if (index >= 0) {
            m_terrain[index] = 1;
            m_relevant[index] = 1;
        }
        const int above = CellIndex(cellX, cellY - 1);
        if (above >= 0) m_relevant[above] = 1;
    }

    for (const auto& entity : state->GetAllEntities()) {
        if (!entity) continue;
        const Vector2 position = entity->GetPosition();
        const int cellX = static_cast<int>(position.x / (TILE_SIZE * kCellTiles));
        const int cellY = static_cast<int>(position.y / (TILE_SIZE * kCellTiles));
        const int index = CellIndex(cellX, cellY);
        if (index >= 0) m_relevant[index] = 1;
    }
    m_exploredRatio = 0.0f;
    m_hasUnexploredDirection = false;
    m_discoveryFlash = 0.0f;
}

void MinimapView::Update(float dt, const GameState* state, const Player* playerOne,
                         const Player* playerTwo) {
    if (!m_initialized || !state || !playerOne) return;
    m_pulse += std::max(0.0f, dt);
    m_discoveryFlash = std::max(0.0f, m_discoveryFlash - dt * 1.8f);

    bool discoveredNewCell = false;
    auto revealAround = [&](const Player* player) {
        if (!player || !player->IsActive()) return;
        const Rectangle bounds = player->GetBoundingBox();
        const float cellWorld = TILE_SIZE * kCellTiles;
        const int centerX = static_cast<int>((bounds.x + bounds.width * 0.5f) / cellWorld);
        const int centerY = static_cast<int>((bounds.y + bounds.height * 0.5f) / cellWorld);
        constexpr int radiusX = 3;
        constexpr int radiusY = 2;
        for (int y = centerY - radiusY; y <= centerY + radiusY; ++y)
            for (int x = centerX - radiusX; x <= centerX + radiusX; ++x)
                discoveredNewCell |= MarkCellExplored(x, y);
    };
    revealAround(playerOne);
    revealAround(playerTwo);
    if (discoveredNewCell) m_discoveryFlash = 1.0f;

    int relevantCount = 0;
    int exploredCount = 0;
    for (int y = 0; y < m_cellsY; ++y) {
        for (int x = 0; x < m_cellsX; ++x) {
            const int index = CellIndex(x, y);
            if (index < 0 || !m_relevant[index]) continue;
            ++relevantCount;
            if (IsCellExplored(x, y)) ++exploredCount;
        }
    }
    m_exploredRatio = relevantCount > 0
        ? std::clamp((float)exploredCount / relevantCount, 0.0f, 1.0f) : 0.0f;

    const Rectangle playerBounds = playerOne->GetBoundingBox();
    Vector2 origin{playerBounds.x + playerBounds.width * 0.5f,
                   playerBounds.y + playerBounds.height * 0.5f};
    if (playerTwo && playerTwo->IsActive()) {
        const Rectangle second = playerTwo->GetBoundingBox();
        origin.x = (origin.x + second.x + second.width * 0.5f) * 0.5f;
        origin.y = (origin.y + second.y + second.height * 0.5f) * 0.5f;
    }

    float bestDistance = std::numeric_limits<float>::max();
    Vector2 bestDirection{};
    const float cellWorld = TILE_SIZE * kCellTiles;
    for (int y = 0; y < m_cellsY; ++y) {
        for (int x = 0; x < m_cellsX; ++x) {
            const int index = CellIndex(x, y);
            if (index < 0 || !m_relevant[index] || IsCellExplored(x, y)) continue;
            const Vector2 target{(x + 0.5f) * cellWorld, (y + 0.5f) * cellWorld};
            const float dx = target.x - origin.x;
            const float dy = target.y - origin.y;
            const float distance = dx * dx + dy * dy;
            if (distance < bestDistance) {
                bestDistance = distance;
                bestDirection = {dx, dy};
            }
        }
    }
    const float length = std::sqrt(bestDirection.x * bestDirection.x + bestDirection.y * bestDirection.y);
    m_hasUnexploredDirection = length > 0.001f && bestDistance < std::numeric_limits<float>::max();
    m_unexploredDirection = m_hasUnexploredDirection
        ? Vector2{bestDirection.x / length, bestDirection.y / length} : Vector2{};
}

Vector2 MinimapView::WorldToMap(Vector2 world, Rectangle mapRect) const {
    const float worldWidth = std::max(1.0f, m_mapWidthTiles * (float)TILE_SIZE);
    const float worldHeight = std::max(1.0f, m_mapHeightTiles * (float)TILE_SIZE);
    return {mapRect.x + std::clamp(world.x / worldWidth, 0.0f, 1.0f) * mapRect.width,
            mapRect.y + std::clamp(world.y / worldHeight, 0.0f, 1.0f) * mapRect.height};
}

void MinimapView::Render(const GameState* state, const Player* playerOne,
                         const Player* playerTwo) const {
    if (!m_initialized || !m_visible || !state || !playerOne) return;
    const float sw = (float)GetScreenWidth();
    const float sh = (float)GetScreenHeight();
    const float scale = std::clamp(std::min(sw / 1280.0f, sh / 720.0f), 0.65f, 1.35f);
    const float panelW = 300.0f * scale;
    const float panelH = 170.0f * scale;
    const Rectangle panel{sw - panelW - 18.0f * scale,
                          sh - panelH - 18.0f * scale, panelW, panelH};
    const Rectangle mapRect{panel.x + 11.0f * scale, panel.y + 34.0f * scale,
                            panel.width - 22.0f * scale, panel.height - 62.0f * scale};
    const Font font = m_font.texture.id != 0 ? m_font : GetFontDefault();
    const Color gold{239, 192, 77, 255};

    DrawRectangleRounded({panel.x + 5.0f * scale, panel.y + 7.0f * scale,
                          panel.width, panel.height}, 0.08f, 10, Color{0, 0, 0, 145});
    DrawRectangleRounded(panel, 0.08f, 10, Color{18, 13, 31, 242});
    DrawRectangleGradientV((int)panel.x + 2, (int)panel.y + 2,
                           (int)panel.width - 4, (int)(31.0f * scale),
                           Color{73, 51, 99, 248}, Color{31, 23, 48, 248});
    DrawRectangleRoundedLinesEx(panel, 0.08f, 10, 2.0f * scale,
        m_discoveryFlash > 0.0f
            ? Color{119, 231, 183, (unsigned char)(180 + 70 * m_discoveryFlash)} : gold);

    const float titleSize = std::max(10.0f, 15.0f * scale);
    DrawTextEx(font, "WORLD MAP", {panel.x + 11.0f * scale, panel.y + 9.0f * scale},
               titleSize, 0.5f, Color{255, 238, 190, 255});
    char percent[32];
    std::snprintf(percent, sizeof(percent), "EXPLORED %d%%",
                  (int)std::round(m_exploredRatio * 100.0f));
    const float percentSize = std::max(9.0f, 12.0f * scale);
    const Vector2 percentMeasure = MeasureTextEx(font, percent, percentSize, 0.3f);
    DrawTextEx(font, percent, {panel.x + panel.width - percentMeasure.x - 10.0f * scale,
               panel.y + 11.0f * scale}, percentSize, 0.3f,
               m_exploredRatio >= 0.999f ? Color{119, 231, 183, 255} : Color{196, 184, 215, 255});

    DrawRectangleRec(mapRect, Color{7, 8, 17, 248});
    const float cellW = mapRect.width / std::max(1, m_cellsX);
    const float cellH = mapRect.height / std::max(1, m_cellsY);
    for (int y = 0; y < m_cellsY; ++y) {
        for (int x = 0; x < m_cellsX; ++x) {
            if (!IsCellExplored(x, y)) continue;
            const int index = CellIndex(x, y);
            const float x0 = mapRect.x + x * cellW;
            const float y0 = mapRect.y + y * cellH;
            const float x1 = mapRect.x + (x + 1) * cellW;
            const float y1 = mapRect.y + (y + 1) * cellH;
            const Rectangle cell{x0, y0, std::max(1.0f, x1 - x0 + 0.25f),
                                 std::max(1.0f, y1 - y0 + 0.25f)};
            DrawRectangleRec(cell, index >= 0 && m_terrain[index]
                ? Color{127, 104, 129, 245} : Color{31, 61, 73, 225});
        }
    }
    DrawRectangleLinesEx(mapRect, std::max(1.0f, 1.5f * scale), Color{114, 90, 145, 245});

    auto entityMarker = [&](const Entity* entity) {
        if (!entity || !entity->IsActive()) return;
        const Rectangle bounds = entity->GetBoundingBox();
        const Vector2 center{bounds.x + bounds.width * 0.5f, bounds.y + bounds.height * 0.5f};
        const int cellX = (int)(center.x / (TILE_SIZE * kCellTiles));
        const int cellY = (int)(center.y / (TILE_SIZE * kCellTiles));
        if (!IsCellExplored(cellX, cellY)) return;
        const Vector2 p = WorldToMap(center, mapRect);
        const float radius = std::max(2.5f, 4.0f * scale);
        switch (entity->GetType()) {
            case EntityType::Checkpoint:
                DrawPoly(p, 4, radius, 45.0f, Color{112, 235, 167, 255});
                break;
            case EntityType::TeleportPortal:
                DrawRing(p, radius, radius + 1.7f * scale, 0, 360, 16, Color{91, 188, 255, 255});
                break;
            case EntityType::LevelCompleteCup:
                DrawPoly(p, 5, radius + 1.5f * scale, -90.0f, gold);
                break;
            case EntityType::Boss:
                DrawCircleV(p, radius + 1.0f * scale, Color{235, 79, 92, 255});
                DrawCircleLinesV(p, radius + 2.0f * scale, Color{255, 191, 150, 255});
                break;
            default: break;
        }
    };
    for (const auto& entity : state->GetAllEntities()) entityMarker(entity.get());

    auto playerMarker = [&](const Player* player, Color color, float offset) {
        if (!player || !player->IsActive()) return;
        const Rectangle bounds = player->GetBoundingBox();
        const Vector2 world{bounds.x + bounds.width * 0.5f, bounds.y + bounds.height * 0.5f};
        Vector2 p = WorldToMap(world, mapRect);
        p.x += offset;
        const float radius = std::max(3.5f, 5.0f * scale);
        DrawCircleV(p, radius + 2.0f * scale, Color{5, 4, 13, 235});
        DrawCircleV(p, radius, color);
        DrawCircleLinesV(p, radius + 1.0f * scale, WHITE);
    };
    playerMarker(playerOne, Color{91, 214, 255, 255}, playerTwo ? -2.5f * scale : 0.0f);
    playerMarker(playerTwo, Color{224, 157, 255, 255}, 2.5f * scale);

    if (m_hasUnexploredDirection) {
        const Rectangle bounds = playerOne->GetBoundingBox();
        const Vector2 world{bounds.x + bounds.width * 0.5f, bounds.y + bounds.height * 0.5f};
        const Vector2 playerMap = WorldToMap(world, mapRect);
        const float angle = std::atan2(m_unexploredDirection.y, m_unexploredDirection.x)
                          * 180.0f / 3.14159265f + 90.0f;
        const Vector2 arrow{playerMap.x + m_unexploredDirection.x * 13.0f * scale,
                            playerMap.y + m_unexploredDirection.y * 13.0f * scale};
        const float pulse = 1.0f + 0.16f * std::sin(m_pulse * 5.0f);
        DrawPoly(arrow, 3, 5.0f * scale * pulse, angle, gold);
    }

    const float footerSize = std::max(8.0f, 10.5f * scale);
    const float footerY = mapRect.y + mapRect.height + 7.0f * scale;
    DrawCircleV({panel.x + 16.0f * scale, footerY + 5.0f * scale}, 3.0f * scale,
                Color{91, 214, 255, 255});
    DrawTextEx(font, playerTwo ? "P1 / P2" : "YOU",
               {panel.x + 23.0f * scale, footerY}, footerSize, 0.2f, Color{193, 218, 231, 255});
    DrawPoly({panel.x + 89.0f * scale, footerY + 5.0f * scale}, 4, 3.2f * scale,
             45.0f, Color{112, 235, 167, 255});
    DrawTextEx(font, "CHECKPOINT", {panel.x + 96.0f * scale, footerY}, footerSize,
               0.2f, Color{193, 218, 231, 255});
    const char* toggle = "M  HIDE";
    const Vector2 toggleSize = MeasureTextEx(font, toggle, footerSize, 0.2f);
    DrawTextEx(font, toggle, {panel.x + panel.width - toggleSize.x - 10.0f * scale,
               footerY}, footerSize, 0.2f, Color{239, 192, 77, 235});
}

} // namespace View
