#include "View/MapBuilderView.h"
#include "View/Renderer.h"
#include "View/GameView.h"
#include "Utils/Constants.h"
#include "Model/TriggerZone.h"
#include "Model/Chest.h"
#include "Model/Boss.h"
#include "Model/Checkpoint.h"
#include "Model/Item.h"
#include "Controller/MapBuilderController.h"
#include <iostream>

namespace View {

MapBuilderView& MapBuilderView::GetInstance() {
    static MapBuilderView instance;
    return instance;
}

void MapBuilderView::Init() {
    m_showGrid = true;
    m_snapToGrid = true;
    m_currentTool = BuilderTool::Brush;
    m_currentLayer = MapLayer::Main;
    m_selectedEntity = nullptr;

    m_texPlayer = LoadTexture("assets/textures/player/knight/idle.png");
    m_texEnemy = LoadTexture("assets/textures/player/ninja/idle.png"); // placeholder
    m_texBoss = LoadTexture("assets/textures/boss/boss1/phase1/idle.png"); // placeholder
    m_texCoin = LoadTexture("assets/textures/items/coin.png");
    m_texApple = LoadTexture("assets/textures/items/apple.png");
    m_texKey = LoadTexture("assets/textures/items/key.png");
    m_texPotion = LoadTexture("assets/textures/items/potion_red.png");
    m_texChest = LoadTexture("assets/textures/objects/chest_closed.png");
    m_texCheckpoint = LoadTexture("assets/textures/objects/checkpoint_uncaptured.png");
}

bool MapBuilderView::IsMouseOverUI() const {
    Vector2 mousePos = GetMousePosition();
    float sw = (float)Renderer::GetInstance().GetWindowWidth();
    float sh = (float)Renderer::GetInstance().GetWindowHeight();

    Rectangle toolbar = {0, 0, sw, 40};
    Rectangle palette = {0, 40, 250, sh - 40};
    Rectangle layers = {sw - 200, 40, 200, 200};
    Rectangle properties = {sw - 200, 240, 200, 300};
    Rectangle minimap = {sw - 200, 540, 200, 150};

    if (CheckCollisionPointRec(mousePos, toolbar)) return true;
    if (CheckCollisionPointRec(mousePos, palette)) return true;
    if (CheckCollisionPointRec(mousePos, layers)) return true;
    if (m_showMinimap && CheckCollisionPointRec(mousePos, minimap)) return true;
    if (m_selectedEntity && CheckCollisionPointRec(mousePos, properties)) return true;

    return false;
}

void MapBuilderView::Update(float /*dt*/) {
    if (!IsMouseOverUI()) return;

    Vector2 mousePos = GetMousePosition();
    float sw = (float)Renderer::GetInstance().GetWindowWidth();
    float sh = (float)Renderer::GetInstance().GetWindowHeight();

    if (IsMouseOverUI() && mousePos.x < 250 && mousePos.y > 100 && m_currentTab == PaletteTab::Tiles) {
        float wheel = GetMouseWheelMove();
        if (wheel > 0.0f && m_paletteScroll > 0) {
            m_paletteScroll--;
        } else if (wheel < 0.0f) {
            m_paletteScroll++;
        }
    }

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        // Toolbar Clicks
        if (mousePos.y < 40) {
            float x = 10;
            if (CheckCollisionPointRec(mousePos, {x, 5, 80, 30})) m_wantsSave = true; x += 90;
            if (CheckCollisionPointRec(mousePos, {x, 5, 120, 30})) m_isTypingFileName = true;
            else m_isTypingFileName = false;
            x += 130;
            if (CheckCollisionPointRec(mousePos, {x, 5, 80, 30})) m_wantsLoad = true; x += 90;
            if (CheckCollisionPointRec(mousePos, {x, 5, 80, 30})) m_wantsPlaytest = true; x += 90;
            if (CheckCollisionPointRec(mousePos, {x, 5, 80, 30})) m_wantsExit = true; x += 90;
            if (CheckCollisionPointRec(mousePos, {x, 5, 80, 30})) m_wantsClearAll = true; x += 100;
            
            if (CheckCollisionPointRec(mousePos, {x, 5, 60, 30})) m_currentTool = BuilderTool::Brush; x += 70;
            if (CheckCollisionPointRec(mousePos, {x, 5, 60, 30})) m_currentTool = BuilderTool::Eraser; x += 70;
            if (CheckCollisionPointRec(mousePos, {x, 5, 60, 30})) m_currentTool = BuilderTool::BucketFill; x += 70;
            if (CheckCollisionPointRec(mousePos, {x, 5, 60, 30})) m_currentTool = BuilderTool::Select; x += 70;
            if (CheckCollisionPointRec(mousePos, {x, 5, 60, 30})) m_currentTool = BuilderTool::BoxSelect; x += 70;
            if (CheckCollisionPointRec(mousePos, {x, 5, 60, 30})) m_showGrid = !m_showGrid; x += 70;

            // Map Size Buttons
            if (CheckCollisionPointRec(mousePos, {x + 60, 5, 20, 15})) m_wantsResizeW = 1;
            if (CheckCollisionPointRec(mousePos, {x + 60, 20, 20, 15})) m_wantsResizeW = -1;
            x += 90;
            if (CheckCollisionPointRec(mousePos, {x + 60, 5, 20, 15})) m_wantsResizeH = 1;
            if (CheckCollisionPointRec(mousePos, {x + 60, 20, 20, 15})) m_wantsResizeH = -1;
        }
        
        // Layers Panel Clicks
        if (mousePos.x > sw - 200 && mousePos.y > 40 && mousePos.y < 240) {
            float ly = 80;
            for (int i = 0; i < 3; ++i) {
                if (CheckCollisionPointRec(mousePos, {sw - 190, ly, 140, 25})) {
                    m_currentLayer = static_cast<MapLayer>(i);
                }
                if (CheckCollisionPointRec(mousePos, {sw - 40, ly, 30, 25})) {
                    m_layerVisible[i] = !m_layerVisible[i];
                }
                ly += 30;
            }
        }
        
        // Palette Clicks (Left Panel)
        if (mousePos.x < 250 && mousePos.y >= 40) {
            // Tabs
            if (mousePos.y >= 40 && mousePos.y < 70) {
                if (CheckCollisionPointRec(mousePos, {10, 45, 60, 20})) m_currentTab = PaletteTab::Tiles;
                if (CheckCollisionPointRec(mousePos, {80, 45, 60, 20})) m_currentTab = PaletteTab::Entities;
                if (CheckCollisionPointRec(mousePos, {150, 45, 60, 20})) m_currentTab = PaletteTab::Triggers;
            }
            
            if (m_currentTab == PaletteTab::Tiles) {
                // Tileset switcher
                if (mousePos.y >= 70 && mousePos.y < 100) {
                    if (CheckCollisionPointRec(mousePos, {150, 75, 20, 20})) {
                        m_selectedTileType--;
                        if (m_selectedTileType < 1) m_selectedTileType = 6;
                        m_selectedTileId = 0;
                        m_paletteScroll = 0;
                    }
                    if (CheckCollisionPointRec(mousePos, {180, 75, 20, 20})) {
                        m_selectedTileType++;
                        if (m_selectedTileType > 6) m_selectedTileType = 1;
                        m_selectedTileId = 0;
                        m_paletteScroll = 0;
                    }
                }
                // Tile selection
                if (mousePos.y >= 100) {
                    int col = ((int)mousePos.x - 10) / 40;
                    int row = ((int)mousePos.y - 110) / 40;
                    int cols = 5;
                    if (col >= 0 && col < cols && row >= 0) {
                        int idx = m_paletteScroll * cols + row * cols + col;
                        m_selectedTileId = idx + 1;
                    }
                }
            } else if (mousePos.y >= 70) {
                if (m_currentTab == PaletteTab::Entities) {
                    int col = ((int)mousePos.x - 10) / 55;
                    int row = ((int)mousePos.y - 80) / 55;
                    if (row == 0) {
                        if (col == 0) m_selectedEntityType = EntityType::Player;
                        else if (col == 1) { m_selectedEntityType = EntityType::Enemy; m_selectedEntitySubType = 0; } // Melee
                        else if (col == 2) { m_selectedEntityType = EntityType::Enemy; m_selectedEntitySubType = 1; } // Ranged
                        else if (col == 3) m_selectedEntityType = EntityType::Boss;
                    } else if (row == 1) {
                        if (col == 0) { m_selectedEntityType = EntityType::Item; m_selectedEntitySubType = 0; } // Coin
                        else if (col == 1) { m_selectedEntityType = EntityType::Item; m_selectedEntitySubType = 1; } // Apple
                        else if (col == 2) { m_selectedEntityType = EntityType::Item; m_selectedEntitySubType = 2; } // Key
                        else if (col == 3) { m_selectedEntityType = EntityType::Item; m_selectedEntitySubType = 3; } // Potion
                    }
                } else if (m_currentTab == PaletteTab::Triggers) {
                    int col = ((int)mousePos.x - 10) / 55;
                    int row = ((int)mousePos.y - 80) / 55;
                    if (row == 0) {
                        if (col == 0) m_selectedEntityType = EntityType::Chest;
                        else if (col == 1) m_selectedEntityType = EntityType::Checkpoint;
                        else if (col == 2) m_selectedEntityType = EntityType::TriggerZone;
                        else if (col == 3) m_selectedEntityType = EntityType::FakeWall;
                    }
                }
            }
        }
    }
}

void MapBuilderView::RenderUI(const Camera2D& camera, GameState* state) {
    float sw = (float)Renderer::GetInstance().GetWindowWidth();
    float sh = (float)Renderer::GetInstance().GetWindowHeight();

    DrawToolbar(state, sw, sh);
    DrawPalette(sw, sh);
    DrawLayersPanel(sw, sh);
    if (m_selectedEntity) DrawPropertiesPanel(sw, sh);
    if (m_showMinimap) DrawMinimap(camera, state, sw, sh);
    if (m_isTypingFileName) {
        int key = GetCharPressed();
        while (key > 0) {
            if ((key >= 32) && (key <= 125) && m_fileName.length() < 20) m_fileName += (char)key;
            key = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE) && m_fileName.length() > 0) {
            m_fileName.pop_back();
        }
    }
}


void MapBuilderView::DrawToolbar(GameState* state, int sw, int /*sh*/) {
    DrawRectangle(0, 0, sw, 40, Fade(BLACK, 0.8f));
    DrawLine(0, 40, sw, 40, WHITE);

    float x = 10;
    DrawRectangleLines(x, 5, 80, 30, WHITE); DrawText("Save", x+20, 12, 10, WHITE); x += 90;
    
    Color boxCol = m_isTypingFileName ? YELLOW : WHITE;
    DrawRectangleLines(x, 5, 120, 30, boxCol);
    DrawText(m_fileName.c_str(), x+5, 15, 10, boxCol);
    x += 130;
    
    DrawRectangleLines(x, 5, 80, 30, WHITE); DrawText("Load", x+20, 12, 10, WHITE); x += 90;
    DrawRectangleLines(x, 5, 80, 30, GREEN); DrawText("Test", x+20, 12, 10, GREEN); x += 90;
    DrawRectangleLines(x, 5, 80, 30, RED);   DrawText("Exit", x+20, 12, 10, RED); x += 90;
    DrawRectangleLines(x, 5, 80, 30, ORANGE); DrawText("Clear", x+20, 12, 10, ORANGE); x += 100;

    Color bColor = (m_currentTool == BuilderTool::Brush) ? YELLOW : WHITE;
    DrawRectangleLines(x, 5, 60, 30, bColor); DrawText("Brush", x+15, 12, 10, bColor); x += 70;
    
    Color eColor = (m_currentTool == BuilderTool::Eraser) ? YELLOW : WHITE;
    DrawRectangleLines(x, 5, 60, 30, eColor); DrawText("Erase", x+15, 12, 10, eColor); x += 70;
    
    Color fColor = (m_currentTool == BuilderTool::BucketFill) ? YELLOW : WHITE;
    DrawRectangleLines(x, 5, 60, 30, fColor); DrawText("Fill", x+20, 12, 10, fColor); x += 70;

    Color sColor = (m_currentTool == BuilderTool::Select) ? YELLOW : WHITE;
    DrawRectangleLines(x, 5, 60, 30, sColor); DrawText("Select", x+15, 12, 10, sColor); x += 70;
    
    Color bxColor = (m_currentTool == BuilderTool::BoxSelect) ? YELLOW : WHITE;
    DrawRectangleLines(x, 5, 60, 30, bxColor); DrawText("BoxSel", x+10, 12, 10, bxColor); x += 70;

    Color gColor = m_showGrid ? GREEN : GRAY;
    DrawRectangleLines(x, 5, 60, 30, gColor); DrawText("Grid", x+15, 12, 10, gColor); x += 70;

    if (state) {
        DrawText(TextFormat("W: %d", state->GetMapWidth()), x, 12, 10, WHITE);
        DrawRectangleLines(x + 60, 5, 20, 15, WHITE); DrawText("+", x + 66, 8, 10, WHITE);
        DrawRectangleLines(x + 60, 20, 20, 15, WHITE); DrawText("-", x + 66, 23, 10, WHITE);
        x += 90;
        DrawText(TextFormat("H: %d", state->GetMapHeight()), x, 12, 10, WHITE);
        DrawRectangleLines(x + 60, 5, 20, 15, WHITE); DrawText("+", x + 66, 8, 10, WHITE);
        DrawRectangleLines(x + 60, 20, 20, 15, WHITE); DrawText("-", x + 66, 23, 10, WHITE);
    }
}

void MapBuilderView::DrawPalette(int /*sw*/, int sh) {
    DrawRectangle(0, 40, 250, sh - 40, Fade(BLACK, 0.8f));
    DrawLine(250, 40, 250, sh, WHITE);
    
    // Tabs
    Color tColor = (m_currentTab == PaletteTab::Tiles) ? YELLOW : WHITE;
    DrawText("Tiles", 10, 50, 15, tColor);
    
    Color eColor = (m_currentTab == PaletteTab::Entities) ? YELLOW : WHITE;
    DrawText("Entities", 80, 50, 15, eColor);
    
    Color trColor = (m_currentTab == PaletteTab::Triggers) ? YELLOW : WHITE;
    DrawText("Triggers", 150, 50, 15, trColor);
    
    DrawLine(0, 70, 250, 70, GRAY);

    // Items
    if (m_currentTab == PaletteTab::Tiles) {
        // Draw tileset selector
        DrawText(TextFormat("Tileset: %d", m_selectedTileType), 10, 80, 15, WHITE);
        DrawRectangleLines(150, 75, 20, 20, WHITE); DrawText("<", 155, 77, 15, WHITE);
        DrawRectangleLines(180, 75, 20, 20, WHITE); DrawText(">", 185, 77, 15, WHITE);
        DrawLine(0, 100, 250, 100, GRAY);

        auto ts = View::GameView::GetInstance().GetTileset(m_selectedTileType);
        if (ts && ts->texture.id != 0) {
            int tileW = ts->texture.width / ts->gridCols;
            int tileH = tileW;
            if (tileH == 0) tileH = 32;

            int totalTiles = (ts->texture.width / tileW) * (ts->texture.height / tileH);
            int cols = 5;
            int startIdx = m_paletteScroll * cols;
            int drawn = 0;
            
            int maxRows = (sh - 100) / 40;
            for (int i = startIdx; i < totalTiles && drawn < cols * maxRows; ++i) {
                int id = i + 1;
                int row = drawn / cols;
                int col = drawn % cols;
                int dx = 10 + col * 40;
                int dy = 110 + row * 40;

                Color c = (m_selectedTileId == id) ? YELLOW : WHITE;
                DrawRectangleLines(dx, dy, 36, 36, c);

                int texX = (i % ts->gridCols) * tileW;
                int texY = (i / ts->gridCols) * tileH;
                Rectangle src = { (float)texX, (float)texY, (float)tileW, (float)tileH };
                Rectangle dest = { (float)dx + 2, (float)dy + 2, 32, 32 };
                DrawTexturePro(ts->texture, src, dest, {0,0}, 0.0f, WHITE);
                
                drawn++;
            }
        }
    } else if (m_currentTab == PaletteTab::Entities) {
        const char* names[] = {"Player", "Melee", "Ranged", "Boss", "Coin", "Apple", "Key", "Potion"};
        EntityType types[] = {EntityType::Player, EntityType::Enemy, EntityType::Enemy, EntityType::Boss, EntityType::Item, EntityType::Item, EntityType::Item, EntityType::Item};
        int subTypes[] = {0, 0, 1, 0, 0, 1, 2, 3}; // 0=Melee, 1=Ranged, 0=Coin, 1=Apple, 2=Key, 3=Potion
        Texture2D texs[] = {m_texPlayer, m_texEnemy, m_texEnemy, m_texBoss, m_texCoin, m_texApple, m_texKey, m_texPotion};
        
        for (int i = 0; i < 8; ++i) {
            int col = i % 4;
            int row = i / 4;
            float px = 10 + col * 55;
            float py = 80 + row * 55;

            Color c = (m_selectedEntityType == types[i] && m_selectedEntitySubType == subTypes[i]) ? YELLOW : WHITE;
            DrawRectangleLines(px, py, 50, 45, c);
            if (texs[i].id != 0) {
                float frameW = (float)texs[i].height;
                if (types[i] == EntityType::Item) frameW = (float)texs[i].width;
                Rectangle src = { 0, 0, frameW, (float)texs[i].height };
                Rectangle dest = { px + 9.0f, py + 2.0f, 32.0f, 32.0f };
                DrawTexturePro(texs[i], src, dest, {0,0}, 0.0f, WHITE);
            }
            DrawText(names[i], px + 5, py + 35, 10, WHITE);
        }
    } else if (m_currentTab == PaletteTab::Triggers) {
        const char* names[] = {"Chest", "Check", "Zone", "Wall"};
        EntityType types[] = {EntityType::Chest, EntityType::Checkpoint, EntityType::TriggerZone, EntityType::FakeWall};
        Texture2D texs[] = {m_texChest, m_texCheckpoint, {0}, {0}};

        for (int i = 0; i < 4; ++i) {
            float px = 10 + i * 55;
            float py = 80;
            Color c = (m_selectedEntityType == types[i]) ? YELLOW : WHITE;
            DrawRectangleLines(px, py, 50, 45, c);
            if (texs[i].id != 0) {
                float frameW = (float)texs[i].height;
                if (types[i] == EntityType::Chest) frameW = (float)texs[i].width;
                Rectangle src = { 0, 0, frameW, (float)texs[i].height };
                Rectangle dest = { px + 9.0f, py + 2.0f, 32.0f, 32.0f };
                DrawTexturePro(texs[i], src, dest, {0,0}, 0.0f, WHITE);
            }
            DrawText(names[i], px + 5, py + 35, 10, WHITE);
        }
    }
}

void MapBuilderView::DrawLayersPanel(int sw, int /*sh*/) {
    DrawRectangle(sw - 200, 40, 200, 200, Fade(BLACK, 0.8f));
    DrawLine(sw - 200, 40, sw - 200, 240, WHITE);
    DrawLine(sw - 200, 240, sw, 240, WHITE);
    DrawText("Layers", sw - 190, 50, 20, WHITE);

    const char* layerNames[] = {"Background", "Main", "Foreground"};
    float y = 80;
    for (int i = 0; i < 3; ++i) {
        Color c = (m_currentLayer == static_cast<MapLayer>(i)) ? YELLOW : WHITE;
        DrawText(layerNames[i], sw - 190, y, 20, c);
        
        Color vColor = m_layerVisible[i] ? GREEN : RED;
        DrawRectangleLines(sw - 40, y, 30, 25, vColor);
        DrawText(m_layerVisible[i] ? "V" : "H", sw - 30, y+5, 10, vColor);
        
        y += 35;
    }
}

void MapBuilderView::DrawPropertiesPanel(int sw, int /*sh*/) {
    DrawRectangle(sw - 200, 240, 200, 300, Fade(BLACK, 0.8f));
    DrawLine(sw - 200, 240, sw, 240, WHITE);
    DrawLine(sw - 200, 540, sw, 540, WHITE);
    DrawText("Properties", sw - 190, 250, 20, YELLOW);
    
    if (m_selectedEntity) {
        float y = 280;
        DrawText(TextFormat("ID: %d", m_selectedEntity->GetId()), sw - 190, y, 15, WHITE); y += 25;
        DrawText(TextFormat("Pos: %.1f, %.1f", m_selectedEntity->GetPosition().x, m_selectedEntity->GetPosition().y), sw - 190, y, 15, WHITE); y += 25;
        
        if (auto* tz = dynamic_cast<TriggerZone*>(m_selectedEntity)) {
            DrawText("Type: TriggerZone", sw - 190, y, 15, GREEN); y += 25;
            DrawText(TextFormat("Target: %s", tz->GetTargetLevelId().c_str()), sw - 190, y, 15, WHITE); y += 25;
            DrawRectangleLines(sw - 190, y, 100, 25, WHITE);
            DrawText("Edit Target", sw - 185, y+5, 15, WHITE); // Button placeholder
        } else if (auto* chest = dynamic_cast<Chest*>(m_selectedEntity)) {
            DrawText("Type: Chest", sw - 190, y, 15, ORANGE); y += 25;
        } else if (auto* boss = dynamic_cast<Boss*>(m_selectedEntity)) {
            DrawText("Type: Boss", sw - 190, y, 15, RED); y += 25;
        } else {
            DrawText(TextFormat("Type: %d", (int)m_selectedEntity->GetType()), sw - 190, y, 15, LIGHTGRAY); y += 25;
        }
    } else {
        DrawText("No entity selected", sw - 190, 280, 15, GRAY);
    }
}

void MapBuilderView::DrawMinimap(const Camera2D& camera, GameState* state, int sw, int sh) {
    if (!state) return;
    Rectangle mmBox = { (float)sw - 200, 540, 200, 150 };
    DrawRectangleRec(mmBox, Fade(BLACK, 0.5f));
    DrawLine(sw - 200, 540, sw, 540, WHITE);
    DrawRectangleLinesEx(mmBox, 1.0f, WHITE);

    float mapW = state->GetMapWidth() * TILE_SIZE;
    float mapH = state->GetMapHeight() * TILE_SIZE;
    if (mapW == 0 || mapH == 0) return;

    float scaleX = mmBox.width / mapW;
    float scaleY = mmBox.height / mapH;

    for (const auto& tile : state->GetTiles(MapLayer::Main)) {
        if (!tile.solid) continue;
        float tx = mmBox.x + tile.x * TILE_SIZE * scaleX;
        float ty = mmBox.y + tile.y * TILE_SIZE * scaleY;
        DrawRectangle(tx, ty, TILE_SIZE * scaleX, TILE_SIZE * scaleY, GRAY);
    }

    // Draw camera viewport in minimap
    float vw = (sw / camera.zoom) * scaleX;
    float vh = (sh / camera.zoom) * scaleY;
    float vx = mmBox.x + (camera.target.x - (sw/camera.zoom)/2.0f) * scaleX;
    float vy = mmBox.y + (camera.target.y - (sh/camera.zoom)/2.0f) * scaleY;
    DrawRectangleLines(vx, vy, vw, vh, YELLOW);
}

void MapBuilderView::RenderWorldOverlay(const Camera2D& camera, GameState* state, Vector2 mouseWorldPos) {
    if (m_showGrid && state) {
        float mapW = state->GetMapWidth() * TILE_SIZE;
        float mapH = state->GetMapHeight() * TILE_SIZE;

        // Draw grid lines
        for (float x = 0; x <= mapW; x += TILE_SIZE) {
            DrawLineV({x, 0}, {x, mapH}, Fade(LIGHTGRAY, 0.3f));
        }
        for (float y = 0; y <= mapH; y += TILE_SIZE) {
            DrawLineV({0, y}, {mapW, y}, Fade(LIGHTGRAY, 0.3f));
        }
    }

    // Highlight hovered tile
    if (!IsMouseOverUI()) {
        int tx = (int)(mouseWorldPos.x / TILE_SIZE);
        int ty = (int)(mouseWorldPos.y / TILE_SIZE);
        DrawRectangleLines(tx * TILE_SIZE, ty * TILE_SIZE, TILE_SIZE, TILE_SIZE, YELLOW);
    }

    // Draw Entities from GameState
    if (state) {
        for (const auto& entity : state->GetAllEntities()) {
            Vector2 pos = entity->GetPosition();
            EntityType type = entity->GetType();
            
            // Draw a colored bounding box for the entity
            Color ec = MAGENTA;
            if (type == EntityType::Player) ec = BLUE;
            else if (type == EntityType::Enemy || type == EntityType::Boss) ec = RED;
            else if (type == EntityType::Item) ec = YELLOW;
            else if (type == EntityType::Chest) ec = ORANGE;
            else if (type == EntityType::Checkpoint) ec = GREEN;

            DrawRectangleLines(pos.x, pos.y, TILE_SIZE, TILE_SIZE, ec);
            
            // Also draw a label to be clear
            const char* label = "E";
            if (type == EntityType::Player) label = "P";
            else if (type == EntityType::Enemy) label = "E";
            else if (type == EntityType::Boss) label = "B";
            else if (type == EntityType::Item) label = "I";
            else if (type == EntityType::Chest) label = "C";
            else if (type == EntityType::Checkpoint) label = "CP";
            else if (type == EntityType::TriggerZone) label = "TZ";
            else if (type == EntityType::FakeWall) label = "FW";

            DrawText(label, pos.x + 4, pos.y + 4, 10, ec);
        }
    }

    // Draw selection box if active or if we have a selection
    auto& ctrl = MapBuilderController::GetInstance();
    if (ctrl.IsBoxSelecting() || m_currentTool == BuilderTool::BoxSelect) {
        Rectangle selBox = ctrl.GetSelectionBox();
        if (selBox.width != 0 || selBox.height != 0) {
            DrawRectangleLinesEx(selBox, 2.0f, RED);
            DrawRectangleRec(selBox, Fade(RED, 0.2f));
        }
    }
}

} // namespace View
