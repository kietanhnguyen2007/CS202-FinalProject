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
#include <algorithm>

namespace View {

MapBuilderView& MapBuilderView::GetInstance() {
    static MapBuilderView instance;
    return instance;
}

void MapBuilderView::Init() {
    m_showGrid = true;
    m_snapToGrid = true;
    m_currentTool = BuilderTool::Brush;
    m_currentTab = PaletteTab::Tiles;
    m_currentLayer = MapLayer::Main;
    m_selectedEntity = nullptr;
    m_statusMessage.clear();
    m_statusTimer = 0.0f;

    if (!m_resourcesLoaded) {
        m_texPlayer = LoadTexture("assets/textures/player/knight/idle.png");
        m_texEnemy = LoadTexture("assets/textures/player/ninja/idle.png"); // placeholder
        m_texBoss1 = LoadTexture("assets/textures/boss/boss1/phase1/idle.png");
        m_texBoss2 = LoadTexture("assets/textures/boss/boss2/phase1/idle.png");
        m_texBoss3 = LoadTexture("assets/textures/boss/boss3/phase1/idle.png");
        m_texCoin = LoadTexture("assets/textures/items/coin.png");
        m_texKey = LoadTexture("assets/textures/items/key.png");
        m_texPotion = LoadTexture("assets/textures/items/potion_red.png");
        m_texChest = LoadTexture("assets/textures/objects/chest_closed.png");
        m_texCheckpoint = LoadTexture("assets/textures/objects/checkpoint_uncaptured.png");
        m_texPortalBlue = LoadTexture("assets/textures/objects/portal_blue_spritesheet.png");
        m_texPortalBrown = LoadTexture("assets/textures/objects/portal_brown_spritesheet.png");
        m_texPortalGreen = LoadTexture("assets/textures/objects/portal_green_spritesheet.png");
        m_texPortalPurple = LoadTexture("assets/textures/objects/portal_purple_spritesheet.png");
        m_texPortalRed = LoadTexture("assets/textures/objects/portal_red_spritesheet.png");
        m_uiFont = LoadFont("assets/fonts/game_font.ttf");
        m_resourcesLoaded = true;
    }
}

void MapBuilderView::Shutdown() {
    if (!m_resourcesLoaded) return;

    Texture2D* textures[] = {
        &m_texPlayer, &m_texEnemy, &m_texBoss1, &m_texBoss2, &m_texBoss3,
        &m_texCoin, &m_texKey, &m_texPotion, &m_texChest, &m_texCheckpoint,
        &m_texPortalBlue, &m_texPortalBrown, &m_texPortalGreen,
        &m_texPortalPurple, &m_texPortalRed
    };
    for (Texture2D* texture : textures) {
        if (texture->id != 0) ::UnloadTexture(*texture);
        *texture = {};
    }
    if (m_uiFont.texture.id != 0) {
        ::UnloadFont(m_uiFont);
        m_uiFont = {};
    }

    m_selectedEntity = nullptr;
    m_resourcesLoaded = false;
}

bool MapBuilderView::IsMouseOverUI() const {
    if (m_showSaveConfirm) return true; // Save dialog is a full-screen modal

    Vector2 mousePos = GetMousePosition();
    float sw = (float)Renderer::GetInstance().GetWindowWidth();
    float sh = (float)Renderer::GetInstance().GetWindowHeight();

    Rectangle toolbar = {0, 0, sw, 40};
    Rectangle palette = {0, 40, 250, sh - 40};
    Rectangle layers = {sw - 200, 40, 200, 200};
    Rectangle minimap = {sw - 200, sh - 200, 200, 200};
    Rectangle properties = {sw - 200, 240, 200, 300};

    if (CheckCollisionPointRec(mousePos, toolbar)) return true;
    if (CheckCollisionPointRec(mousePos, palette)) return true;
    if (CheckCollisionPointRec(mousePos, layers)) return true;
    if (m_showMinimap && CheckCollisionPointRec(mousePos, minimap)) return true;
    if (m_selectedEntity && CheckCollisionPointRec(mousePos, properties)) return true;

    return false;
}

void MapBuilderView::Update(float dt) {
    if (m_statusTimer > 0.0f) {
        m_statusTimer = std::max(0.0f, m_statusTimer - dt);
    }

    if (!IsMouseOverUI()) return;

    Vector2 mousePos = GetMousePosition();
    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();

    if (IsMouseOverUI() && mousePos.x < 250 && mousePos.y > 100 && m_currentTab == PaletteTab::Tiles) {
        float wheel = GetMouseWheelMove();
        if (wheel > 0.0f && m_paletteScroll > 0) {
            m_paletteScroll--;
        } else if (wheel < 0.0f) {
            m_paletteScroll++;
        }
    }

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (m_showSaveConfirm) {
            float sw = (float)GetScreenWidth();
            float sh = (float)GetScreenHeight();
            Rectangle yesBtn = { sw/2 - 130, sh/2 + 10, 110, 40 };
            Rectangle cancelBtn = { sw/2 + 20, sh/2 + 10, 110, 40 };
            
            if (CheckCollisionPointRec(mousePos, yesBtn)) {
                m_wantsSave = true;
                m_showSaveConfirm = false;
            } else if (CheckCollisionPointRec(mousePos, cancelBtn)) {
                m_showSaveConfirm = false;
            }
            
            // If we click anywhere else, we do NOT close the dialog immediately, 
            // because they might accidentally click the background.
            // Actually, for a modal, usually clicking outside closes it, or does nothing.
            // Let's do nothing if they click the background of the dialog.
            return;
        }

        // Reset typing if click outside the textbox
        float x = 10;
        float textboxX = x + 90;
        if (CheckCollisionPointRec(mousePos, {textboxX, 5, 120, 30})) m_isTypingFileName = true;
        else m_isTypingFileName = false;

        // Toolbar Clicks
        if (mousePos.y < 40) {
            x = 10;
            if (CheckCollisionPointRec(mousePos, {x, 5, 80, 30})) m_showSaveConfirm = true; x += 90;
            x += 120; // Skip textbox
            x += 10; // Margin

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
                        else if (col == 3) { m_selectedEntityType = EntityType::Boss; m_selectedEntitySubType = 1; } // Boss 1
                    } else if (row == 1) {
                        if (col == 0) { m_selectedEntityType = EntityType::Boss; m_selectedEntitySubType = 2; } // Boss 2
                        else if (col == 1) { m_selectedEntityType = EntityType::Boss; m_selectedEntitySubType = 3; } // Boss 3
                        else if (col == 2) { m_selectedEntityType = EntityType::Item; m_selectedEntitySubType = 0; } // Coin
                        else if (col == 3) { m_selectedEntityType = EntityType::Item; m_selectedEntitySubType = 2; } // Key
                    } else if (row == 2) {
                        if (col == 0) { m_selectedEntityType = EntityType::Item; m_selectedEntitySubType = 3; } // Potion
                    }
                } else if (m_currentTab == PaletteTab::Triggers) {
                    int col = ((int)mousePos.x - 10) / 55;
                    int row = ((int)mousePos.y - 80) / 55;
                    if (row == 0) {
                        if (col == 0) m_selectedEntityType = EntityType::Chest;
                        else if (col == 1) m_selectedEntityType = EntityType::Checkpoint;
                        else if (col >= 2 && col <= 3) {
                            m_selectedEntityType = EntityType::TeleportPortal;
                            m_selectedEntitySubType = 100 + (col - 2); // 100 (Blue), 101 (Brown)
                        }
                    } else if (row == 1) {
                        if (col >= 0 && col <= 2) {
                            m_selectedEntityType = EntityType::TeleportPortal;
                            m_selectedEntitySubType = 102 + col; // 102 (Green), 103 (Purple), 104 (Red)
                        } else if (col == 3) {
                            m_selectedEntityType = EntityType::TeleportPortal;
                            m_selectedEntitySubType = 200; // 200 (Transition Blue)
                        }
                    } else if (row == 2) {
                        if (col >= 0 && col <= 3) {
                            m_selectedEntityType = EntityType::TeleportPortal;
                            m_selectedEntitySubType = 201 + col; // 201 (Brown), 202 (Green), 203 (Purple), 204 (Red)
                        }
                    }
                }
            }
        }
    }
}

void MapBuilderView::DrawEditorText(const char* text, Vector2 pos, float size, Color color) const {
    const Font font = (m_uiFont.texture.id != 0) ? m_uiFont : GetFontDefault();
    DrawTextEx(font, text, pos, size, 1.0f, color);
}

void MapBuilderView::DrawEditorPanel(Rectangle rect, const char* title) const {
    const Color panel = {20, 16, 38, 244};
    const Color panelTop = {43, 31, 67, 246};
    const Color gold = {226, 178, 78, 255};
    DrawRectangleRounded({rect.x + 4, rect.y + 5, rect.width, rect.height}, 0.055f, 8,
                         Fade(BLACK, 0.42f));
    DrawRectangleRounded(rect, 0.055f, 8, panel);
    DrawRectangleGradientV((int)rect.x + 2, (int)rect.y + 2,
                           (int)rect.width - 4, std::min(42, (int)rect.height - 4),
                           panelTop, panel);
    DrawRectangleRoundedLinesEx(rect, 0.055f, 8, 2.0f, Fade(gold, 0.82f));
    if (title) {
        DrawEditorText(title, {rect.x + 12, rect.y + 8}, 17.0f, gold);
        DrawLineEx({rect.x + 10, rect.y + 31}, {rect.x + rect.width - 10, rect.y + 31},
                   1.0f, Fade(gold, 0.42f));
    }
}

void MapBuilderView::DrawEditorButton(Rectangle rect, const char* label, bool active,
                                      Color accent) const {
    const bool hovered = CheckCollisionPointRec(GetMousePosition(), rect);
    Color fill = active ? Color{78, 51, 101, 255} : Color{31, 26, 52, 245};
    if (hovered) fill = active ? Color{101, 67, 128, 255} : Color{50, 39, 75, 250};
    DrawRectangleRounded({rect.x + 2, rect.y + 3, rect.width, rect.height}, 0.18f, 6,
                         Fade(BLACK, 0.45f));
    DrawRectangleRounded(rect, 0.18f, 6, fill);
    DrawRectangleRoundedLinesEx(rect, 0.18f, 6, active ? 2.0f : 1.0f,
                                active ? accent : Fade(accent, hovered ? 0.85f : 0.45f));
    const Font font = (m_uiFont.texture.id != 0) ? m_uiFont : GetFontDefault();
    const float fontSize = rect.height >= 30.0f ? 12.0f : 10.0f;
    Vector2 size = MeasureTextEx(font, label, fontSize, 1.0f);
    DrawTextEx(font, label,
               {rect.x + (rect.width - size.x) * 0.5f, rect.y + (rect.height - size.y) * 0.5f},
               fontSize, 1.0f, active ? RAYWHITE : Fade(RAYWHITE, 0.82f));
}

void MapBuilderView::RenderUI(const Camera2D& camera, GameState* state) {
    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();

    DrawToolbar(state, sw, sh);
    DrawPalette(sw, sh);
    DrawLayersPanel(sw, sh);
    if (m_selectedEntity) DrawPropertiesPanel(sw, sh);
    if (m_showMinimap) DrawMinimap(camera, state, sw, sh);

    Rectangle shortcutBar = {260.0f, sh - 30.0f, std::max(200.0f, sw - 470.0f), 24.0f};
    DrawRectangleRounded(shortcutBar, 0.3f, 6, Fade(Color{18, 14, 34, 255}, 0.90f));
    DrawRectangleRoundedLinesEx(shortcutBar, 0.3f, 6, 1.0f, Fade(Color{226, 178, 78, 255}, 0.45f));
    const char* shortcuts = "LMB  PAINT     RMB  PAN     WHEEL  ZOOM     CTRL+Z  UNDO";
    const Font uiFont = (m_uiFont.texture.id != 0) ? m_uiFont : GetFontDefault();
    Vector2 shortcutSize = MeasureTextEx(uiFont, shortcuts, 10.0f, 1.0f);
    DrawEditorText(shortcuts,
                   {shortcutBar.x + (shortcutBar.width - shortcutSize.x) * 0.5f, shortcutBar.y + 7.0f},
                   10.0f, Fade(RAYWHITE, 0.70f));

    if (m_statusTimer > 0.0f && !m_statusMessage.empty()) {
        const float alpha = std::min(1.0f, m_statusTimer * 2.0f);
        Vector2 textSize = MeasureTextEx(uiFont, m_statusMessage.c_str(), 14.0f, 1.0f);
        Rectangle toast = {sw * 0.5f - textSize.x * 0.5f - 22.0f, 50.0f,
                           textSize.x + 44.0f, 38.0f};
        DrawRectangleRounded(toast, 0.25f, 8, Fade(Color{25, 61, 54, 255}, alpha * 0.96f));
        DrawRectangleRoundedLinesEx(toast, 0.25f, 8, 2.0f,
                                    Fade(Color{105, 235, 169, 255}, alpha));
        DrawEditorText(m_statusMessage.c_str(),
                       {toast.x + 22.0f, toast.y + 11.0f}, 14.0f,
                       Fade(RAYWHITE, alpha));
    }
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
    
    if (m_showSaveConfirm) {
        DrawRectangle(0, 0, (int)sw, (int)sh, Fade(BLACK, 0.76f));
        Rectangle box = {sw/2 - 190, sh/2 - 105, 380, 210};
        DrawEditorPanel(box, "SAVE CUSTOM MAP");
        DrawEditorText("This becomes the playable custom map", {box.x + 30, box.y + 54},
                       13.0f, Fade(RAYWHITE, 0.72f));
        std::string msg = m_fileName + ".lvl";
        Rectangle namePlate = {box.x + 30, box.y + 82, box.width - 60, 42};
        DrawRectangleRounded(namePlate, 0.18f, 6, Color{13, 12, 25, 255});
        DrawRectangleRoundedLinesEx(namePlate, 0.18f, 6, 1.0f, Fade(SKYBLUE, 0.55f));
        Vector2 msgSize = MeasureTextEx(uiFont, msg.c_str(), 16.0f, 1.0f);
        DrawEditorText(msg.c_str(), {namePlate.x + (namePlate.width-msgSize.x)*0.5f, namePlate.y+13},
                       16.0f, RAYWHITE);
        DrawEditorButton({sw/2 - 130, sh/2 + 10, 110, 40}, "SAVE", true,
                         Color{105, 235, 169, 255});
        DrawEditorButton({sw/2 + 20, sh/2 + 10, 110, 40}, "CANCEL", false,
                         Color{235, 105, 118, 255});
    }
}


void MapBuilderView::DrawToolbar(GameState* state, int sw, int /*sh*/) {
    const Color gold = {226, 178, 78, 255};
    const Color mint = {105, 235, 169, 255};
    const Color coral = {235, 105, 118, 255};
    DrawRectangleGradientV(0, 0, sw, 40, Color{48, 34, 71, 255}, Color{20, 16, 38, 255});
    DrawRectangle(0, 38, sw, 2, Fade(gold, 0.75f));

    float x = 10;
    DrawEditorButton({x, 5, 80, 30}, "SAVE", false, mint); x += 90;
    
    Color boxCol = m_isTypingFileName ? gold : Fade(SKYBLUE, 0.72f);
    DrawRectangleRounded({x, 5, 120, 30}, 0.16f, 6, Color{13, 12, 25, 255});
    DrawRectangleRoundedLinesEx({x, 5, 120, 30}, 0.16f, 6,
                                m_isTypingFileName ? 2.0f : 1.0f, boxCol);
    DrawEditorText(m_fileName.c_str(), {x+7, 14}, 11.0f, RAYWHITE);
    if (m_isTypingFileName && ((int)(GetTime() * 2) % 2 == 0)) {
        const Font font = (m_uiFont.texture.id != 0) ? m_uiFont : GetFontDefault();
        float textW = MeasureTextEx(font, m_fileName.c_str(), 11.0f, 1.0f).x;
        DrawLineEx({x + 7 + textW + 2, 11}, {x + 7 + textW + 2, 28}, 1.0f, boxCol);
    }
    x += 130;
    
    DrawEditorButton({x, 5, 80, 30}, "LOAD", false, SKYBLUE); x += 90;
    DrawEditorButton({x, 5, 80, 30}, "TEST", false, mint); x += 90;
    DrawEditorButton({x, 5, 80, 30}, "EXIT", false, coral); x += 90;
    DrawEditorButton({x, 5, 80, 30}, "CLEAR", false, ORANGE); x += 100;

    DrawEditorButton({x, 5, 60, 30}, "BRUSH", m_currentTool == BuilderTool::Brush, gold); x += 70;
    DrawEditorButton({x, 5, 60, 30}, "ERASE", m_currentTool == BuilderTool::Eraser, gold); x += 70;
    DrawEditorButton({x, 5, 60, 30}, "FILL", m_currentTool == BuilderTool::BucketFill, gold); x += 70;
    DrawEditorButton({x, 5, 60, 30}, "SELECT", m_currentTool == BuilderTool::Select, gold); x += 70;
    DrawEditorButton({x, 5, 60, 30}, "BOX", m_currentTool == BuilderTool::BoxSelect, gold); x += 70;
    DrawEditorButton({x, 5, 60, 30}, "GRID", m_showGrid, mint); x += 70;

    if (state) {
        DrawEditorText(TextFormat("W  %d", state->GetMapWidth()), {x, 14}, 10.0f, RAYWHITE);
        DrawEditorButton({x + 60, 5, 20, 15}, "+", false, SKYBLUE);
        DrawEditorButton({x + 60, 20, 20, 15}, "-", false, SKYBLUE);
        x += 90;
        DrawEditorText(TextFormat("H  %d", state->GetMapHeight()), {x, 14}, 10.0f, RAYWHITE);
        DrawEditorButton({x + 60, 5, 20, 15}, "+", false, SKYBLUE);
        DrawEditorButton({x + 60, 20, 20, 15}, "-", false, SKYBLUE);
    }
}

void MapBuilderView::DrawPalette(int /*sw*/, int sh) {
    const Color gold = {226, 178, 78, 255};
    DrawEditorPanel({2, 42, 246, (float)sh - 46});
    
    // Tabs
    DrawEditorButton({10, 45, 60, 20}, "TILES", m_currentTab == PaletteTab::Tiles, gold);
    DrawEditorButton({80, 45, 60, 20}, "ACTORS", m_currentTab == PaletteTab::Entities, gold);
    DrawEditorButton({150, 45, 60, 20}, "OBJECTS", m_currentTab == PaletteTab::Triggers, gold);
    DrawLineEx({10, 70}, {238, 70}, 1.0f, Fade(gold, 0.35f));

    // Items
    if (m_currentTab == PaletteTab::Tiles) {
        // Draw tileset selector
        DrawEditorText(TextFormat("TILESET  %d", m_selectedTileType), {12, 80}, 13.0f, RAYWHITE);
        DrawEditorButton({150, 75, 20, 20}, "<", false, SKYBLUE);
        DrawEditorButton({180, 75, 20, 20}, ">", false, SKYBLUE);
        DrawLineEx({10, 100}, {238, 100}, 1.0f, Fade(gold, 0.28f));

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

                bool selected = (m_selectedTileId == id);
                DrawRectangleRounded({(float)dx, (float)dy, 36, 36}, 0.15f, 5,
                                     selected ? Color{83, 57, 106, 255} : Color{28, 24, 46, 245});
                DrawRectangleRoundedLinesEx({(float)dx, (float)dy, 36, 36}, 0.15f, 5,
                                            selected ? 2.0f : 1.0f,
                                            selected ? gold : Fade(SKYBLUE, 0.32f));

                int texX = (i % ts->gridCols) * tileW;
                int texY = (i / ts->gridCols) * tileH;
                Rectangle src = { (float)texX, (float)texY, (float)tileW, (float)tileH };
                Rectangle dest = { (float)dx + 2, (float)dy + 2, 32, 32 };
                DrawTexturePro(ts->texture, src, dest, {0,0}, 0.0f, WHITE);
                
                drawn++;
            }
        }
    } else if (m_currentTab == PaletteTab::Entities) {
        const char* names[] = {"Player", "Melee", "Ranged", "Boss 1", "Boss 2", "Boss 3", "Coin", "Key", "Potion"};
        EntityType types[] = {EntityType::Player, EntityType::Enemy, EntityType::Enemy, EntityType::Boss, EntityType::Boss, EntityType::Boss, EntityType::Item, EntityType::Item, EntityType::Item};
        int subTypes[] = {0, 0, 1, 1, 2, 3, 0, 2, 3}; // Removed apple (1)
        Texture2D texs[] = {m_texPlayer, m_texEnemy, m_texEnemy, m_texBoss1, m_texBoss2, m_texBoss3, m_texCoin, m_texKey, m_texPotion};
        
        for (int i = 0; i < 9; ++i) {
            int col = i % 4;
            int row = i / 4;
            float px = 10 + col * 55;
            float py = 80 + row * 55;

            bool isSelected = false;
            if (types[i] == EntityType::Player) {
                isSelected = (m_selectedEntityType == EntityType::Player);
            } else {
                isSelected = (m_selectedEntityType == types[i] && m_selectedEntitySubType == subTypes[i]);
            }

            DrawRectangleRounded({px, py, 50, 45}, 0.14f, 5,
                                 isSelected ? Color{83, 57, 106, 255} : Color{28, 24, 46, 245});
            DrawRectangleRoundedLinesEx({px, py, 50, 45}, 0.14f, 5,
                                        isSelected ? 2.0f : 1.0f,
                                        isSelected ? gold : Fade(SKYBLUE, 0.32f));
            if (texs[i].id != 0) {
                float frameW = (float)texs[i].height;
                if (types[i] == EntityType::Item) frameW = (float)texs[i].width;
                Rectangle src = { 0, 0, frameW, (float)texs[i].height };
                Rectangle dest = { px + 9.0f, py + 2.0f, 32.0f, 32.0f };
                DrawTexturePro(texs[i], src, dest, {0,0}, 0.0f, WHITE);
            }
            DrawEditorText(names[i], {px + 4, py + 34}, 8.0f, RAYWHITE);
        }
    } else if (m_currentTab == PaletteTab::Triggers) {
        // Types: Chest, Checkpoint, and 10 Portals (5 Local, 5 LevelTrans)
        const char* names[] = {
            "Chest", "Check",
            "L_Blue", "L_Brown", "L_Green", "L_Purple", "L_Red",
            "T_Blue", "T_Brown", "T_Green", "T_Purple", "T_Red"
        };
        EntityType types[] = {
            EntityType::Chest, EntityType::Checkpoint,
            EntityType::TeleportPortal, EntityType::TeleportPortal, EntityType::TeleportPortal, EntityType::TeleportPortal, EntityType::TeleportPortal,
            EntityType::TeleportPortal, EntityType::TeleportPortal, EntityType::TeleportPortal, EntityType::TeleportPortal, EntityType::TeleportPortal
        };
        // subTypes mapping: 100+c for Local, 200+c for Transition
        int subTypes[] = {
            0, 0,
            100, 101, 102, 103, 104,
            200, 201, 202, 203, 204
        };
        Texture2D texs[] = {
            m_texChest, m_texCheckpoint,
            m_texPortalBlue, m_texPortalBrown, m_texPortalGreen, m_texPortalPurple, m_texPortalRed,
            m_texPortalBlue, m_texPortalBrown, m_texPortalGreen, m_texPortalPurple, m_texPortalRed
        };

        for (int i = 0; i < 12; ++i) {
            int col = i % 4;
            int row = i / 4;
            float px = 10 + col * 55;
            float py = 80 + row * 55;
            
            bool isSelected = false;
            if (types[i] == EntityType::Chest || types[i] == EntityType::Checkpoint) {
                isSelected = (m_selectedEntityType == types[i]);
            } else {
                isSelected = (m_selectedEntityType == types[i] && m_selectedEntitySubType == subTypes[i]);
            }

            DrawRectangleRounded({px, py, 50, 45}, 0.14f, 5,
                                 isSelected ? Color{83, 57, 106, 255} : Color{28, 24, 46, 245});
            DrawRectangleRoundedLinesEx({px, py, 50, 45}, 0.14f, 5,
                                        isSelected ? 2.0f : 1.0f,
                                        isSelected ? gold : Fade(SKYBLUE, 0.32f));
            if (texs[i].id != 0) {
                float frameW = (float)texs[i].height;
                if (types[i] == EntityType::Chest) frameW = (float)texs[i].width;
                else if (types[i] == EntityType::TeleportPortal) frameW = 283; // from json
                Rectangle src = { 0, 0, frameW, (float)texs[i].height };
                Rectangle dest = { px + 9.0f, py + 2.0f, 32.0f, 32.0f };
                DrawTexturePro(texs[i], src, dest, {0,0}, 0.0f, WHITE);
            }
            DrawEditorText(names[i], {px + 2, py + 34}, 7.0f, RAYWHITE);
        }
    }
}

void MapBuilderView::DrawLayersPanel(int sw, int /*sh*/) {
    const Color gold = {226, 178, 78, 255};
    DrawEditorPanel({(float)sw - 198, 42, 196, 196}, "LAYERS");

    const char* layerNames[] = {"Background", "Main", "Foreground"};
    float y = 80;
    for (int i = 0; i < 3; ++i) {
        const bool selected = m_currentLayer == static_cast<MapLayer>(i);
        Rectangle row = {(float)sw - 190, y, 140, 25};
        DrawRectangleRounded(row, 0.2f, 5,
                             selected ? Color{76, 53, 98, 255} : Color{30, 25, 49, 225});
        DrawRectangleRoundedLinesEx(row, 0.2f, 5, selected ? 1.5f : 1.0f,
                                    selected ? gold : Fade(SKYBLUE, 0.25f));
        DrawEditorText(layerNames[i], {(float)sw - 181, y + 7}, 11.0f,
                       selected ? RAYWHITE : Fade(RAYWHITE, 0.72f));

        Color vColor = m_layerVisible[i] ? Color{105, 235, 169, 255} : Color{235, 105, 118, 255};
        DrawEditorButton({(float)sw - 40, y, 30, 25}, m_layerVisible[i] ? "ON" : "OFF",
                         m_layerVisible[i], vColor);
        
        y += 35;
    }

    DrawEditorText("Click a row to paint that layer", {(float)sw - 188, 194}, 9.0f,
                   Fade(RAYWHITE, 0.48f));
}

void MapBuilderView::DrawPropertiesPanel(int sw, int /*sh*/) {
    const Color gold = {226, 178, 78, 255};
    DrawEditorPanel({(float)sw - 198, 242, 196, 296}, "PROPERTIES");
    
    if (m_selectedEntity) {
        float y = 280;
        DrawEditorText(TextFormat("ID   %d", m_selectedEntity->GetId()), {(float)sw - 188, y}, 12.0f, RAYWHITE); y += 25;
        DrawEditorText(TextFormat("POS  %.0f, %.0f", m_selectedEntity->GetPosition().x, m_selectedEntity->GetPosition().y), {(float)sw - 188, y}, 12.0f, RAYWHITE); y += 25;
        DrawLineEx({(float)sw - 188, y}, {(float)sw - 12, y}, 1.0f, Fade(gold, 0.25f)); y += 16;
        
        if (auto* tz = dynamic_cast<TriggerZone*>(m_selectedEntity)) {
            DrawEditorText("TRIGGER ZONE", {(float)sw - 188, y}, 12.0f, Color{105, 235, 169, 255}); y += 25;
            DrawEditorText(TextFormat("TARGET  %s", tz->GetTargetLevelId().c_str()), {(float)sw - 188, y}, 10.0f, RAYWHITE); y += 25;
            DrawEditorButton({(float)sw - 188, y, 112, 28}, "EDIT TARGET", false, SKYBLUE);
        } else if (auto* chest = dynamic_cast<Chest*>(m_selectedEntity)) {
            (void)chest;
            DrawEditorText("CHEST", {(float)sw - 188, y}, 12.0f, ORANGE); y += 25;
        } else if (auto* boss = dynamic_cast<Boss*>(m_selectedEntity)) {
            (void)boss;
            DrawEditorText("BOSS", {(float)sw - 188, y}, 12.0f, Color{235, 105, 118, 255}); y += 25;
        } else {
            DrawEditorText(TextFormat("TYPE  %d", (int)m_selectedEntity->GetType()), {(float)sw - 188, y}, 12.0f, LIGHTGRAY); y += 25;
        }
    } else {
        DrawEditorText("NO ENTITY SELECTED", {(float)sw - 188, 280}, 11.0f, GRAY);
    }
}

void MapBuilderView::DrawMinimap(const Camera2D& camera, GameState* state, int sw, int sh) {
    if (!state) return;
    const Color gold = {226, 178, 78, 255};
    Rectangle outer = {(float)sw - 198, (float)sh - 176, 196, 174};
    DrawEditorPanel(outer, "MINIMAP");
    Rectangle mmBox = {outer.x + 8, outer.y + 38, outer.width - 16, outer.height - 46};
    DrawRectangleRounded(mmBox, 0.05f, 5, Color{11, 12, 24, 255});
    DrawRectangleRoundedLinesEx(mmBox, 0.05f, 5, 1.0f, Fade(SKYBLUE, 0.35f));

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
    DrawRectangleLinesEx({vx, vy, vw, vh}, 2.0f, gold);
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
            else if (type == EntityType::TeleportPortal) label = "PTL";

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
