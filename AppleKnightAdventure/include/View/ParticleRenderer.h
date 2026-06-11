#pragma once

#include "raylib.h"
#include "View/Renderer.h"
#include <vector>

struct Particle;

namespace View {

class ParticleRenderer {
public:
    static ParticleRenderer& GetInstance();

    void RenderAll(const std::vector<Particle*>& particles);
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
