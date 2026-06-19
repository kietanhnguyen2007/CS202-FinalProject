#pragma once

#include "raylib.h"
#include "View/Renderer.h"
#include <vector>

struct Particle;

namespace View {

enum class ReactionType {
    Vaporize,
    Conduct,
    Overload
};

class ParticleRenderer {
public:
    static ParticleRenderer& GetInstance();

    void RenderAll(const std::vector<Particle*>& particles, const Camera2D& camera, float dt);
    void EmitBurst(Vector2 pos, int count = 8);
    void EmitReaction(Vector2 pos, ReactionType type);
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
