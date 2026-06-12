#pragma once

#include "raylib.h"
#include "View/Renderer.h"
#include <vector>

struct Particle;

namespace View {

class ParticleRenderer {
public:
    static ParticleRenderer& GetInstance();

    void RenderAll(const std::vector<Particle*>& particles, const Camera2D& camera, float dt);
    // Simple view-side helper to spawn a short burst of debris at position
    void EmitBurst(Vector2 pos, int count = 8);
    void Shutdown();

private:
    ParticleRenderer();
    ~ParticleRenderer();
    ParticleRenderer(const ParticleRenderer&) = delete;
    ParticleRenderer& operator=(const ParticleRenderer&) = delete;

    Texture2D m_softCircle{};
    bool m_initialized = false;
};

} // namespace View
