#ifndef MAPBUILDERCONTROLLER_H
#define MAPBUILDERCONTROLLER_H

#include "raylib.h"
#include "Model/GameState.h"
#include "Model/Command.h"
#include <memory>
#include <string>

class MapBuilderController {
private:
    MapBuilderController();
    ~MapBuilderController();

    std::unique_ptr<GameState> m_gameState;
    std::unique_ptr<CommandManager> m_commandManager;
    
    Camera2D m_camera;
    Vector2 m_dragStart;
    bool m_isDragging;
    
    std::string m_currentFile;
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
};

#endif
