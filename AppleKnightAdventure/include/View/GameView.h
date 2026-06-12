#pragma once

#include "raylib.h"
#include "Utils/Types.h"
#include "Utils/Constants.h"
#include <vector>
#include <string>
#include <unordered_map>

struct Particle;
class DualWorld;

namespace View {

class GameView {
public:
    static GameView& GetInstance();

    void Init();
    void Update(float dt);

    // Render world with optional dual-world shader
    void Render(const Camera2D& camera, const std::vector<Particle*>& particles = {}, float dt = 0.0f);

    void Shutdown();

    // Dual-world support
    void SetActiveWorldLayer(WorldLayer layer, const Vector2& lightWorldPos = {0,0});
    WorldLayer GetActiveWorldLayer() const;

    // Tilemap — multi-tilesheet support
    // Each tileType maps to a tilesheet; tileId is the index within that tilesheet grid
    void LoadTileset(int tileType, const std::string& texturePath, int cols);
    void RenderTilemap(const DualWorld* world);

    // Camera shake
    void Shake(float intensity, float duration);

private:
    GameView() = default;
    ~GameView() = default;

    void LoadShadowShader();

    // Each tileType → { texture, gridCols }
    struct TilesetInfo {
        Texture2D texture{};
        int gridCols = 1;
    };
    std::unordered_map<int, TilesetInfo> m_tilesets;

    WorldLayer m_activeLayer = WorldLayer::Light;
    Shader m_shadowShader{};
    bool m_shaderLoaded = false;
    Vector2 m_lightWorldPos{};

    // Camera shake
    float m_shakeTimer = 0.0f;
    float m_shakeIntensity = 0.0f;
};

} // namespace View
