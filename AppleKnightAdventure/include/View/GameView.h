#pragma once

#include "raylib.h"
#include "Utils/Types.h"
#include <vector>

struct Particle;

namespace View {

class GameView {
public:
    static GameView& GetInstance();

    // Initialize view subsystems
    void Init();
    void Update(float dt);

    // Render world with optional dual-world shader
    // If activeLayer != Light and shadow shader is loaded, post-process darkness
    // lightWorldPos is the player's world position (used to compute light circle in screen space)
    void Render(const Camera2D& camera, const std::vector<Particle*>& particles = {}, float dt = 0.0f);

    void Shutdown();

    // Dual-world support
    void SetActiveWorldLayer(WorldLayer layer, const Vector2& lightWorldPos = {0,0});
    WorldLayer GetActiveWorldLayer() const;

private:
    GameView() = default;
    ~GameView() = default;

    void LoadShadowShader();

    WorldLayer m_activeLayer = WorldLayer::Light;
    Shader m_shadowShader{};
    bool m_shaderLoaded = false;
    Vector2 m_lightWorldPos{};
};

} // namespace View
