#ifndef MAPBUILDERCONTROLLER_H
#define MAPBUILDERCONTROLLER_H

#include "raylib.h"
#include "Model/GameState.h"
#include "Model/Command.h"
#include <memory>
#include <string>
#include <set>
#include <vector>

class MapBuilderController {
private:
    MapBuilderController();
    ~MapBuilderController();

    std::unique_ptr<GameState> m_gameState;
    std::unique_ptr<CommandManager> m_commandManager;
    std::string m_currentFile;

    // Track which entities have visuals registered so we can sync automatically
    std::set<uint32_t> m_registeredEntities;

    // Editor State
    Camera2D m_camera{};
    Vector2 m_dragStart;
    bool m_isDragging;
    
    bool m_isRunning;
    bool m_returnToMenu;
    bool m_playtestMode;

    bool m_isBoxSelecting = false;
    Vector2 m_boxSelectStart = {0,0};
    Rectangle m_selectionBox = {0,0,0,0};
    std::vector<Tile> m_clipboardTiles;

    void HandleInput(float dt);
    void HandleTool(Vector2 mouseWorldPos);
    void BucketFill(int tx, int ty, MapLayer layer, int newTileType, int newTileId, int oldTileId, bool solid);

    void CopySelection();
    void PasteSelection();

public:
    static MapBuilderController& GetInstance();

    void StartEditor(const std::string& filepath = "");
    void Update(float dt);
    
    bool IsRunning() const { return m_isRunning; }
    bool ShouldReturnToMenu() const { return m_returnToMenu; }
    void ExitEditor();

    void SaveMap(const std::string& filename);
    void Playtest();

    bool IsBoxSelecting() const { return m_isBoxSelecting; }
    Rectangle GetSelectionBox() const { return m_selectionBox; }
    
    bool WantsToPlaytest() { bool v = m_playtestMode; m_playtestMode = false; return v; }

    void ResumeEditor() {
        m_isRunning = true;
        m_returnToMenu = false;
        m_playtestMode = false;
        m_registeredEntities.clear(); // force re-registration of visuals since GameController cleared them
    }
};

#endif
