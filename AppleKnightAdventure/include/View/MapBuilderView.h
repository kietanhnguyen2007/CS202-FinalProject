#ifndef MAPBUILDERVIEW_H
#define MAPBUILDERVIEW_H

#include "raylib.h"
#include "Model/GameState.h"
#include "Model/Entity.h"
#include <string>

namespace View {

enum class BuilderTool {
    Brush,
    Eraser,
    BucketFill,
    BoxSelect,
    Select,
    MoveCamera
};

enum class PaletteTab {
    Tiles,
    Entities,
    Triggers
};

enum class BuilderTab {
    Tiles,
    Spawns,
    Enemies,
    Objects,
    Items,
    Triggers
};

struct SelectedItem {
    BuilderTab tab = BuilderTab::Tiles;
    // For Tiles:
    int tileType = 1;
    int tileId = 0;
    // For Entities:
    EntityType entityType = EntityType::Player;
    int subType = 0;
};

class MapBuilderView {
private:
    MapBuilderView() = default;
    ~MapBuilderView() = default;

    bool m_showGrid = true;
    bool m_snapToGrid = true;
    BuilderTool m_currentTool = BuilderTool::Brush;
    PaletteTab m_currentTab = PaletteTab::Tiles;
    MapLayer m_currentLayer = MapLayer::Main;

    int m_selectedTileType = 1;
    int m_selectedTileId = 0;
    EntityType m_selectedEntityType = EntityType::Player;
    int m_selectedEntitySubType = 0;
    int m_paletteScroll = 0;

    // Entity Icons
    Texture2D m_texPlayer{};
    Texture2D m_texEnemy{};
    Texture2D m_texBoss1{};
    Texture2D m_texBoss2{};
    Texture2D m_texBoss3{};
    Texture2D m_texCoin{};
    Texture2D m_texKey{};
    Texture2D m_texPotion{};
    Texture2D m_texChest{};
    Texture2D m_texCheckpoint{};
    Texture2D m_texPortalBlue{};
    Texture2D m_texPortalBrown{};
    Texture2D m_texPortalGreen{};
    Texture2D m_texPortalPurple{};
    Texture2D m_texPortalRed{};
    Font m_uiFont{};
    bool m_resourcesLoaded = false;

    // Layer visibility
    bool m_layerVisible[static_cast<int>(MapLayer::Count)] = {true, true, true};
    bool m_layerLocked[static_cast<int>(MapLayer::Count)] = {false, false, false};

    // Properties panel
    Entity* m_selectedEntity = nullptr;

    // Minimap
    bool m_showMinimap = true;

    // Output flags
    bool m_wantsPlaytest = false;
    bool m_wantsSave = false;
    bool m_wantsLoad = false;
    bool m_wantsExit = false;
    bool m_wantsClearAll = false;
    int m_wantsResizeW = 0; // +1, -1
    int m_wantsResizeH = 0; // +1, -1

    std::string m_fileName = "custom_map";
    bool m_isTypingFileName = false;
    bool m_showSaveConfirm = false;
    std::string m_statusMessage;
    float m_statusTimer = 0.0f;

    void DrawToolbar(GameState* state, int screenW, int screenH);
    void DrawPalette(int screenW, int screenH);
    void DrawLayersPanel(int screenW, int screenH);
    void DrawPropertiesPanel(int screenW, int screenH);
    void DrawMinimap(const Camera2D& camera, GameState* state, int screenW, int screenH);
    void DrawEditorPanel(Rectangle rect, const char* title = nullptr) const;
    void DrawEditorButton(Rectangle rect, const char* label, bool active,
                          Color accent) const;
    void DrawEditorText(const char* text, Vector2 pos, float size, Color color) const;

public:
    static MapBuilderView& GetInstance();
    void Init();
    void Shutdown();
    
    // Returns true if the mouse is hovering over any UI element (meaning clicks shouldn't affect the map)
    bool IsMouseOverUI() const;

    void Update(float dt);
    void RenderUI(const Camera2D& camera, GameState* state);
    void RenderWorldOverlay(const Camera2D& camera, GameState* state, Vector2 mouseWorldPos);
    
    bool WantsResizeMapW(int& dir) { if (m_wantsResizeW != 0) { dir = m_wantsResizeW; m_wantsResizeW = 0; return true; } return false; }
    bool WantsResizeMapH(int& dir) { if (m_wantsResizeH != 0) { dir = m_wantsResizeH; m_wantsResizeH = 0; return true; } return false; }

    SelectedItem GetSelectedItem() const {
        SelectedItem item;
        item.tab = (BuilderTab)m_currentTab;
        item.tileType = m_selectedTileType;
        item.tileId = m_selectedTileId;
        item.entityType = m_selectedEntityType;
        item.subType = m_selectedEntitySubType;
        return item;
    }

    BuilderTool GetCurrentTool() const { return m_currentTool; }
    PaletteTab GetCurrentTab() const { return m_currentTab; }
    MapLayer GetCurrentLayer() const { return m_currentLayer; }

    int GetSelectedTileType() const { return m_selectedTileType; }
    int GetSelectedTileId() const { return m_selectedTileId; }
    EntityType GetSelectedEntityType() const { return m_selectedEntityType; }
    int GetSelectedEntitySubType() const { return m_selectedEntitySubType; }

    bool IsGridVisible() const { return m_showGrid; }
    bool IsSnapToGrid() const { return m_snapToGrid; }
    bool IsLayerVisible(MapLayer layer) const { return m_layerVisible[static_cast<int>(layer)]; }
    bool IsLayerLocked(MapLayer layer) const { return m_layerLocked[static_cast<int>(layer)]; }

    void SetSelectedEntity(Entity* entity) { m_selectedEntity = entity; }
    Entity* GetSelectedEntity() const { return m_selectedEntity; }

    bool WantsPlaytest() { bool v = m_wantsPlaytest; m_wantsPlaytest = false; return v; }
    bool WantsSave() { bool v = m_wantsSave; m_wantsSave = false; return v; }
    bool WantsLoad() { bool v = m_wantsLoad; m_wantsLoad = false; return v; }
    bool WantsExit() { bool v = m_wantsExit; m_wantsExit = false; return v; }
    bool WantsClearAll() { bool v = m_wantsClearAll; m_wantsClearAll = false; return v; }
    
    std::string GetFileName() const { return m_fileName; }
    void SetFileName(const std::string& name) { m_fileName = name; }
    void ShowStatus(const std::string& message) { m_statusMessage = message; m_statusTimer = 3.0f; }

    int WantsResizeW() { int v = m_wantsResizeW; m_wantsResizeW = 0; return v; }
    int WantsResizeH() { int v = m_wantsResizeH; m_wantsResizeH = 0; return v; }
};

} // namespace View

#endif
