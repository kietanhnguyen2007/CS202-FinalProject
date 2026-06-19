#pragma once

#include "raylib.h"
#include "Utils/Types.h"
#include "Utils/Constants.h"
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
    void LoadBackgrounds();
    void SetActiveBackground(int index);
    void RenderBackground(const Camera2D& cam);

    // Camera shake
    void Shake(float intensity, float duration);

    // Tilemap data from Controller
    void SetTiles(const std::vector<Tile>* tiles) { m_tiles = tiles; }

    // Static textures
    Texture2D* GetMagicTex() { return &m_magicTex; }

private:
    GameView() = default;
    ~GameView() = default;

    // Each tileType → { texture, gridCols }
    struct TilesetInfo {
        Texture2D texture{};
        int gridCols = 1;
    };
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

    // Static textures
    Texture2D m_magicTex{};

    // Tilemap reference (set by Controller)
    const std::vector<Tile>* m_tiles = nullptr;
};

} // namespace View
