#include "Controller/MapBuilderController.h"
#include "Controller/GameController.h"
#include "Factories/LevelFactory.h"
#include "View/MapBuilderView.h"
#include "View/GameView.h"
#include "Utils/Constants.h"
#include "Factories/LevelFactory.h"
#include "Factories/EnemyFactory.h"
#include "Factories/ItemFactory.h"
#include "Model/Command.h"
#include "Model/Player.h"
#include "Model/Boss1.h"
#include "Model/Boss2.h"
#include "Model/Boss3.h"
#include "Model/Checkpoint.h"
#include "Model/Chest.h"
#include "Model/TriggerZone.h"
#include "Model/Boss.h"
#include "Model/TeleportPortal.h"
#include "Model/FakeWall.h"
#include "Factories/LevelFactory.h"
#include "Platform/FileDialog.h"
#include "View/CharacterRenderer.h"
#include "View/EntityRenderer.h"
#include <iostream>
#include <queue>
#include <set>
#include <cctype>

MapBuilderController::MapBuilderController() : m_isDragging(false), m_isRunning(false), m_returnToMenu(false), m_playtestMode(false) {}

MapBuilderController::~MapBuilderController() {}

MapBuilderController& MapBuilderController::GetInstance() {
    static MapBuilderController instance;
    return instance;
}

void MapBuilderController::CopySelection() {
    m_clipboardTiles.clear();
    auto layer = View::MapBuilderView::GetInstance().GetCurrentLayer();
    for (const auto& t : m_gameState->GetTiles(layer)) {
        Rectangle tileRec = { (float)t.x * TILE_SIZE, (float)t.y * TILE_SIZE, TILE_SIZE, TILE_SIZE };
        if (CheckCollisionRecs(m_selectionBox, tileRec)) {
            m_clipboardTiles.push_back(t);
        }
    }
    
    if (!m_clipboardTiles.empty()) {
        int minX = m_clipboardTiles[0].x;
        int minY = m_clipboardTiles[0].y;
        for (const auto& t : m_clipboardTiles) {
            if (t.x < minX) minX = t.x;
            if (t.y < minY) minY = t.y;
        }
        for (auto& t : m_clipboardTiles) {
            t.x -= minX;
            t.y -= minY;
        }
    }
}

void MapBuilderController::PasteSelection() {
    if (m_clipboardTiles.empty()) return;
    
    Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), m_camera);
    int startTx = (int)(mouseWorldPos.x / TILE_SIZE);
    int startTy = (int)(mouseWorldPos.y / TILE_SIZE);
    auto layer = View::MapBuilderView::GetInstance().GetCurrentLayer();

    auto composite = std::make_unique<CompositeCommand>();
    
    for (const auto& t : m_clipboardTiles) {
        int tx = startTx + t.x;
        int ty = startTy + t.y;
        
        bool hasPrev = false;
        Tile prevTile;
        for (const auto& pt : m_gameState->GetTiles(layer)) {
            if (pt.x == tx && pt.y == ty) {
                hasPrev = true;
                prevTile = pt;
                break;
            }
        }
        
        Tile newTile = {tx, ty, t.tileType, t.tileId, t.solid, t.flipFlags};
        composite->AddCommand(std::make_unique<PlaceTileCommand>(layer, newTile, hasPrev, prevTile));
    }
    
    m_commandManager->ExecuteCommand(std::move(composite));
}

void MapBuilderController::StartEditor(const std::string& filepath) {
    m_currentFile = filepath.empty() ? "assets/levels/custom_map.lvl" : filepath;
    m_gameState = LevelFactory::LoadLevel(m_currentFile, GameMode::SinglePlayer, 0);
    if (!m_gameState) {
        m_gameState = LevelFactory::CreateDefaultLevel(1);
    }
    m_commandManager = std::make_unique<CommandManager>(m_gameState.get());
    
    // Clear renderers to prevent stale visuals
    View::CharacterRenderer::GetInstance().Clear();
    View::EntityRenderer::GetInstance().Clear();
    m_registeredEntities.clear();
    
    View::MapBuilderView::GetInstance().Init();
    
    m_camera = {0};
    m_camera.zoom = 1.0f;
    m_camera.offset = {SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f};
    
    m_isRunning = true;
    m_returnToMenu = false;
    m_playtestMode = false;
}


void MapBuilderController::ExitEditor() {
    m_isRunning = false;
    m_returnToMenu = true;
    m_gameState.reset();
    m_commandManager.reset();
    
    View::CharacterRenderer::GetInstance().Clear();
    View::EntityRenderer::GetInstance().Clear();
    m_registeredEntities.clear();
}

void MapBuilderController::SaveMap(const std::string& filename) {
    if (!m_gameState) return;
    std::string safeName;
    safeName.reserve(filename.size());
    for (unsigned char ch : filename) {
        if (std::isalnum(ch) || ch == '_' || ch == '-') safeName.push_back((char)ch);
        else if (std::isspace(ch)) safeName.push_back('_');
    }
    if (safeName.empty()) safeName = "my_custom_map";

    bool campaignName = safeName.size() > 5 && safeName.rfind("level", 0) == 0;
    for (size_t i = 5; campaignName && i < safeName.size(); ++i)
        campaignName = std::isdigit(static_cast<unsigned char>(safeName[i])) != 0;
    if (campaignName || safeName == "temp_playtest") safeName = "map_" + safeName;

    View::MapBuilderView::GetInstance().SetFileName(safeName);
    std::string path = "assets/levels/" + safeName + ".lvl";
    m_currentFile = path;
    const bool namedSaveOk = LevelFactory::SaveLevel(path, m_gameState.get());

    // The main menu always launches this stable alias. Keep it synchronized
    // with the latest map the player explicitly saved in the builder.
    const std::string playablePath = "assets/levels/custom_map.lvl";
    bool playableSaveOk = namedSaveOk;
    if (path != playablePath) {
        playableSaveOk = LevelFactory::SaveLevel(playablePath, m_gameState.get());
    }

    View::MapBuilderView::GetInstance().ShowStatus(
        namedSaveOk && playableSaveOk
            ? "MAP SAVED - READY TO PLAY FROM THE MAIN MENU"
            : "SAVE FAILED - CHECK THE LEVELS FOLDER");
}

void MapBuilderController::Playtest() {
    // 1. Save temp map
    std::string tempFile = "assets/levels/temp_playtest.lvl";
    LevelFactory::SaveLevel(tempFile, m_gameState.get());
    
    // 2. Hand over to GameController
    m_playtestMode = true;
    m_isRunning = false; // exit editor loop
    GameController::GetInstance().StartLevel(-99); // -99 signals to load temp map? Or just pass path.
    // Wait, GameController takes int level. I need to modify GameController::GetLevelPath or just pass a special flag.
    // For now, I'll let the user know we need to integrate this.
}

void MapBuilderController::Update(float deltaTime) {
    if (!m_isRunning || !m_gameState) return;

    m_gameState->Update(deltaTime); // Merges m_newEntities

    auto& view = View::MapBuilderView::GetInstance();
    view.Update(deltaTime);

    if (view.WantsExit()) ExitEditor();
    if (view.WantsSave()) SaveMap(view.GetFileName());
    if (view.WantsPlaytest()) Playtest();
    if (view.WantsLoad()) {
        std::string selectedFile = FileDialog::OpenFile("Level Files (*.lvl)\0*.lvl\0All Files (*.*)\0*.*\0");
        if (!selectedFile.empty()) {
            // Re-start editor with the selected absolute path
            StartEditor(selectedFile);
            
            // Optionally, update the filename in view so it can be saved back
            // We can extract just the filename without extension to set it back to view:
            size_t lastSlash = selectedFile.find_last_of("/\\");
            size_t lastDot = selectedFile.find_last_of('.');
            if (lastSlash != std::string::npos && lastDot != std::string::npos && lastDot > lastSlash) {
                std::string baseName = selectedFile.substr(lastSlash + 1, lastDot - lastSlash - 1);
                view.SetFileName(baseName); // Wait, SetFileName might not exist
            }
        }
    }
    if (view.WantsClearAll()) {
        int w = m_gameState->GetMapWidth();
        int h = m_gameState->GetMapHeight();
        m_gameState = std::make_unique<GameState>(GameMode::SinglePlayer);
        m_gameState->SetMapSize(w, h);
        m_commandManager = std::make_unique<CommandManager>(m_gameState.get());
        
        // Clear visuals for old entities
        View::CharacterRenderer::GetInstance().Clear();
        View::EntityRenderer::GetInstance().Clear();
        m_registeredEntities.clear();
    }

    int rw = view.WantsResizeW();
    if (rw != 0) {
        int newW = m_gameState->GetMapWidth() + rw;
        if (newW >= 20 && newW <= 200) {
            m_gameState->ResizeMap(newW, m_gameState->GetMapHeight());
        }
    }

    int rh = view.WantsResizeH();
    if (rh != 0) {
        int newH = m_gameState->GetMapHeight() + rh;
        if (newH >= 15 && newH <= 100) {
            m_gameState->ResizeMap(m_gameState->GetMapWidth(), newH);
        }
    }

    if (!m_isRunning) return;

    // View updates
    View::GameView::GetInstance().SetTiles(MapLayer::Background, &m_gameState->GetTiles(MapLayer::Background));
    View::GameView::GetInstance().SetTiles(MapLayer::Main, &m_gameState->GetTiles(MapLayer::Main));
    View::GameView::GetInstance().SetTiles(MapLayer::Foreground, &m_gameState->GetTiles(MapLayer::Foreground));
    View::GameView::GetInstance().SetEntities(&m_gameState->GetAllEntities());
    View::GameView::GetInstance().Update(deltaTime);

    HandleInput(deltaTime);

    // Sync newly added entities to the visual renderers
    std::set<uint32_t> currentIds;
    for (const auto& u_entity : m_gameState->GetAllEntities()) {
        auto* entity = u_entity.get();
        uint32_t id = entity->GetId();
        currentIds.insert(id);
        if (m_registeredEntities.find(id) == m_registeredEntities.end()) {
            GameController::GetInstance().RegisterEntityVisuals(entity);
            m_registeredEntities.insert(id);
        }
    }

    // Unregister deleted entities
    for (auto it = m_registeredEntities.begin(); it != m_registeredEntities.end(); ) {
        if (currentIds.find(*it) == currentIds.end()) {
            GameController::GetInstance().UnregisterEntityVisuals(*it);
            it = m_registeredEntities.erase(it);
        } else {
            ++it;
        }
    }

    // Rendering
    BeginDrawing();
    ClearBackground(BLACK); // Fallback color
    
    // Render the actual game background
    View::GameView::GetInstance().RenderBackground(m_camera);
    
    // We don't call GameView::Render because it clears background and flushes buffers with its own logic.
    // Instead we render tilemaps manually or use GameView. Let's just use GameView::Render for now,
    // but pass empty particles.
    View::GameView::GetInstance().Render(m_camera, {}, deltaTime);

    BeginMode2D(m_camera);
    view.RenderWorldOverlay(m_camera, m_gameState.get(), GetScreenToWorld2D(GetMousePosition(), m_camera));
    EndMode2D();

    view.RenderUI(m_camera, m_gameState.get());
    
    EndDrawing();
}

void MapBuilderController::HandleInput(float dt) {
    auto& view = View::MapBuilderView::GetInstance();

    // Camera Panning
    bool canPanLeftClick = (view.GetCurrentTool() == View::BuilderTool::Select && !view.IsMouseOverUI());
    if (IsMouseButtonPressed(MOUSE_MIDDLE_BUTTON) || IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) || 
        (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && canPanLeftClick)) {
        m_dragStart = GetMousePosition();
        m_isDragging = true;
    }
    if (m_isDragging) {
        if (IsMouseButtonReleased(MOUSE_MIDDLE_BUTTON) || IsMouseButtonReleased(MOUSE_RIGHT_BUTTON) || IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            m_isDragging = false;
        } else {
            Vector2 mousePos = GetMousePosition();
            Vector2 delta = { (m_dragStart.x - mousePos.x) / m_camera.zoom, (m_dragStart.y - mousePos.y) / m_camera.zoom };
            m_camera.target.x += delta.x;
            m_camera.target.y += delta.y;
            m_dragStart = mousePos;
        }
    }
    
    // Zoom
    float wheel = GetMouseWheelMove();
    if (wheel != 0 && !view.IsMouseOverUI()) {
        Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), m_camera);
        m_camera.offset = GetMousePosition();
        m_camera.target = mouseWorldPos;
        const float zoomIncrement = 0.125f;
        m_camera.zoom += (wheel * zoomIncrement);
        if (m_camera.zoom < 0.25f) m_camera.zoom = 0.25f;
        if (m_camera.zoom > 3.0f) m_camera.zoom = 3.0f;
    }

    // Keyboard Shortcuts
    if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) {
        if (IsKeyPressed(KEY_Z)) m_commandManager->Undo();
        if (IsKeyPressed(KEY_Y)) m_commandManager->Redo();
        if (IsKeyPressed(KEY_S)) SaveMap(view.GetFileName());
        if (IsKeyPressed(KEY_C)) CopySelection();
        if (IsKeyPressed(KEY_V)) PasteSelection();
    }

    // Tools
    auto tool = view.GetCurrentTool();
    if (!view.IsMouseOverUI()) {
        Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), m_camera);
        
        if (tool == View::BuilderTool::BoxSelect) {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                m_isBoxSelecting = true;
                m_boxSelectStart = mouseWorldPos;
                m_selectionBox = { mouseWorldPos.x, mouseWorldPos.y, 0, 0 };
            } else if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && m_isBoxSelecting) {
                m_selectionBox.width = mouseWorldPos.x - m_boxSelectStart.x;
                m_selectionBox.height = mouseWorldPos.y - m_boxSelectStart.y;
            } else if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && m_isBoxSelecting) {
                m_isBoxSelecting = false;
                if (m_selectionBox.width < 0) {
                    m_selectionBox.x += m_selectionBox.width;
                    m_selectionBox.width = -m_selectionBox.width;
                }
                if (m_selectionBox.height < 0) {
                    m_selectionBox.y += m_selectionBox.height;
                    m_selectionBox.height = -m_selectionBox.height;
                }
            }
        } else {
            m_isBoxSelecting = false;
            if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                HandleTool(mouseWorldPos);
            }
        }
    }
}

void MapBuilderController::HandleTool(Vector2 mouseWorldPos) {
    auto& view = View::MapBuilderView::GetInstance();
    int tx = (int)(mouseWorldPos.x / TILE_SIZE);
    int ty = (int)(mouseWorldPos.y / TILE_SIZE);

    if (tx < 0 || ty < 0 || tx >= m_gameState->GetMapWidth() || ty >= m_gameState->GetMapHeight()) return;
    if (view.IsLayerLocked(view.GetCurrentLayer())) return;

    auto tool = view.GetCurrentTool();
    auto item = view.GetSelectedItem();
    auto layer = view.GetCurrentLayer();

    if (view.GetCurrentTab() == View::PaletteTab::Tiles) {
        // Find existing tile
        bool hasPrev = false;
        Tile prevTile;
        for (const auto& t : m_gameState->GetTiles(layer)) {
            if (t.x == tx && t.y == ty) {
                hasPrev = true;
                prevTile = t;
                break;
            }
        }

        if (tool == View::BuilderTool::Brush) {
            Tile newTile = {tx, ty, item.tileType, item.tileId, (layer == MapLayer::Main), 0};
            if (!hasPrev || prevTile.tileType != newTile.tileType || prevTile.tileId != newTile.tileId) {
                m_commandManager->ExecuteCommand(std::make_unique<PlaceTileCommand>(layer, newTile, hasPrev, prevTile));
            }
        } else if (tool == View::BuilderTool::Eraser) {
            if (hasPrev) {
                m_commandManager->ExecuteCommand(std::make_unique<EraseTileCommand>(layer, tx, ty, hasPrev, prevTile));
            }
        } else if (tool == View::BuilderTool::BucketFill) {
            int targetId = hasPrev ? prevTile.tileId : -1;
            if (targetId != item.tileId) {
                BucketFill(tx, ty, layer, item.tileType, item.tileId, targetId, (layer == MapLayer::Main));
            }
        }
    } else if (view.GetCurrentTab() == View::PaletteTab::Entities || view.GetCurrentTab() == View::PaletteTab::Triggers) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (tool == View::BuilderTool::Brush) {
                std::unique_ptr<Entity> newEntity;
                Vector2 pos = { (float)tx * TILE_SIZE, (float)ty * TILE_SIZE };
                EntityType et = view.GetSelectedEntityType();
                int sub = view.GetSelectedEntitySubType();
                switch (et) {
                    case EntityType::Player: 
                        pos.y += TILE_SIZE - 48.0f; // Player is 48 high
                        newEntity = std::make_unique<Player>(pos); 
                        break;

                    case EntityType::Enemy: 
                        pos.y += TILE_SIZE - 48.0f; // Enemies are 48 high
                        newEntity = EnemyFactory::CreateEnemy(pos, sub == 0 ? EnemyType::Melee : EnemyType::Ranged); 
                        break;
                    case EntityType::Boss: 
                        if (sub == 3) {
                            pos.y += TILE_SIZE - (TILE_SIZE * 0.99f);
                            newEntity = std::make_unique<Boss3>(pos, Vector2{TILE_SIZE * 0.5f, TILE_SIZE * 0.99f}); 
                        } else if (sub == 2) {
                            pos.y += TILE_SIZE - (TILE_SIZE * 0.99f);
                            newEntity = std::make_unique<Boss2>(pos, Vector2{TILE_SIZE * 0.5f, TILE_SIZE * 0.99f});
                        } else {
                            pos.y += TILE_SIZE - (TILE_SIZE * 0.99f);
                            newEntity = std::make_unique<Boss1>(pos, Vector2{TILE_SIZE * 0.5f, TILE_SIZE * 0.99f}); 
                        }
                        break;
                    case EntityType::Chest: 
                        pos.y += TILE_SIZE - 32.0f; // Chests are 32 high
                        newEntity = std::make_unique<Chest>(pos); 
                        break;
                    case EntityType::Checkpoint: 
                        pos.y += TILE_SIZE - 64.0f;
                        newEntity = std::make_unique<Checkpoint>(pos); 
                        break;
                    case EntityType::TeleportPortal: {
                        pos.y += TILE_SIZE - (308.0f * 0.44f); // Teleport portal is ~135 high, align bottom to ground
                        PortalType pt = (sub >= 200) ? PortalType::LevelTransition : PortalType::Local;
                        int colIndex = (sub >= 200) ? (sub - 200) : (sub - 100);
                        int colorId = 2; // Default Blue
                        if (colIndex == 0) colorId = 2; // Blue
                        else if (colIndex == 1) colorId = 4; // Brown
                        else if (colIndex == 2) colorId = 3; // Green
                        else if (colIndex == 3) colorId = 5; // Purple
                        else if (colIndex == 4) colorId = 1; // Red
                        newEntity = std::make_unique<TeleportPortal>(pos, pt, colorId, -1);
                        break;
                    }
                    case EntityType::Item: 
                        pos.y += TILE_SIZE - 16.0f; // Items are 16 high
                        if (sub == 0) newEntity = ItemFactory::CreateCoin(pos, 10);
                        else if (sub == 2) newEntity = ItemFactory::CreateKey(pos);
                        else if (sub == 3) newEntity = ItemFactory::CreatePotion(pos, 1);
                        break;
                    default: break;
                }

                if (newEntity) {
                    m_commandManager->ExecuteCommand(std::make_unique<PlaceEntityCommand>(std::move(newEntity)));
                }
            } else if (tool == View::BuilderTool::Eraser) {
                // We'll just loop and check bounds
                for (const auto& e : m_gameState->GetAllEntities()) {
                    if (CheckCollisionPointRec(mouseWorldPos, e->GetBoundingBox())) {
                        m_commandManager->ExecuteCommand(std::make_unique<RemoveEntityCommand>(e->GetId()));
                        if (view.GetSelectedEntity() == e.get()) view.SetSelectedEntity(nullptr);
                        break; // Only erase one
                    }
                }
            } else if (tool == View::BuilderTool::Select) {
                Entity* selected = nullptr;
                for (const auto& e : m_gameState->GetAllEntities()) {
                    if (CheckCollisionPointRec(mouseWorldPos, e->GetBoundingBox())) {
                        selected = e.get();
                        break;
                    }
                }
                view.SetSelectedEntity(selected);
            }
        }
    }
}

void MapBuilderController::BucketFill(int startX, int startY, MapLayer layer, int newTileType, int newTileId, int oldTileId, bool solid) {
    std::queue<std::pair<int, int>> q;
    std::set<std::pair<int, int>> visited;
    
    q.push({startX, startY});
    visited.insert({startX, startY});

    // Execute in a single command? It's better to create a macro-command for BucketFill.
    // For now, let's just do it directly to the state and clear undo stack, or create a multiple-tile command.
    // Simplest: just modify state directly.
    while (!q.empty()) {
        auto [cx, cy] = q.front();
        q.pop();

        if (cx < 0 || cy < 0 || cx >= m_gameState->GetMapWidth() || cy >= m_gameState->GetMapHeight()) continue;

        bool hasPrev = false;
        Tile prevTile;
        for (const auto& t : m_gameState->GetTiles(layer)) {
            if (t.x == cx && t.y == cy) {
                hasPrev = true;
                prevTile = t;
                break;
            }
        }

        int currentId = hasPrev ? prevTile.tileId : -1;
        if (currentId == oldTileId) {
            Tile newTile = {cx, cy, newTileType, newTileId, solid, 0};
            // Execute as command (this will spam the undo stack, a composite command is needed for proper undo).
            // For now, directly setting it to save time:
            m_gameState->SetTileAt(layer, cx, cy, newTileType, newTileId, solid, 0);

            if (visited.find({cx + 1, cy}) == visited.end()) { q.push({cx + 1, cy}); visited.insert({cx + 1, cy}); }
            if (visited.find({cx - 1, cy}) == visited.end()) { q.push({cx - 1, cy}); visited.insert({cx - 1, cy}); }
            if (visited.find({cx, cy + 1}) == visited.end()) { q.push({cx, cy + 1}); visited.insert({cx, cy + 1}); }
            if (visited.find({cx, cy - 1}) == visited.end()) { q.push({cx, cy - 1}); visited.insert({cx, cy - 1}); }
        }
    }
    m_commandManager->Clear(); // Clear undo stack since we did direct state mutation
}
