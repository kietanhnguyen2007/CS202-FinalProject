#pragma once

#include "raylib.h"
#include "Utils/Types.h"
#include "Utils/Constants.h"
#include "Model/GameState.h"
#include "View/TextureAtlas.h"
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

struct Particle;
struct Tile;

namespace View {

class GameView {
public:
    static GameView& GetInstance();

    bool Init();
    void Update(float dt);

    void Render(const Camera2D& camera, const std::vector<Particle*>& particles = {}, float dt = 0.0f);

    void Shutdown();

    // Tilemap — multi-tilesheet support
    // Each tileType maps to a tilesheet; tileId is the index within that tilesheet grid
    void LoadTileset(int tileType, const std::string& texturePath, int cols);
    void RenderTilemap(const std::vector<Tile>& tiles);

    // Background parallax
    void LoadBackgrounds(BackgroundTheme theme = BackgroundTheme::Forest);
    void SetActiveBackground(int index);
    void RenderBackground(const Camera2D& cam);

    // Camera shake
    void Shake(float intensity, float duration);

    // Tilemap data from Controller
    void SetTiles(MapLayer layer, const std::vector<Tile>* tiles) { 
        if(static_cast<int>(layer) >= 0 && static_cast<int>(layer) < static_cast<int>(MapLayer::Count)) {
            m_tiles[static_cast<int>(layer)] = tiles; 
        }
    }

    void SetEntities(const std::vector<std::unique_ptr<Entity>>* entities) {
        m_entities = entities;
    }

    void RenderFakeWallHints(float dt);

    // Static textures
    Texture2D* GetMagicTex() { return &m_magicTex; }

    // Tileset info for MapBuilder
    struct TilesetInfo {
        Texture2D texture{};
        int gridCols = 1;
        std::string texturePath;
        std::shared_ptr<Animations::TextureAtlas> atlas;
        bool ownsTexture = false;
    };
    const TilesetInfo* GetTileset(int tileType) const {
        auto it = m_tilesets.find(tileType);
        if (it != m_tilesets.end()) return &it->second;
        return nullptr;
    }

private:
    GameView() = default;
    ~GameView() = default;


    std::unordered_map<int, TilesetInfo> m_tilesets;

    // Camera shake
    float m_shakeTimer = 0.0f;
    float m_shakeIntensity = 0.0f;

    // Background parallax
    struct BGLayerInfo {
        Texture2D tex{};
        float parallaxSpeed = 1.0f;
    };
    std::vector<std::vector<BGLayerInfo>> m_backgrounds;
    int m_activeBgIndex = 0;
    float m_bgScrollOffset = 0.0f;
    BackgroundTheme m_loadedBackgroundTheme = BackgroundTheme::Forest;
    bool m_hasLoadedBackground = false;

    // Static textures
    Texture2D m_magicTex{};

    // Tilemap reference (set by Controller)
    const std::vector<Tile>* m_tiles[static_cast<int>(MapLayer::Count)] = {nullptr};
    
    // Entity reference
    const std::vector<std::unique_ptr<Entity>>* m_entities = nullptr;
    float m_fakeWallPulseTimer = 0.0f;
};

} // namespace View
