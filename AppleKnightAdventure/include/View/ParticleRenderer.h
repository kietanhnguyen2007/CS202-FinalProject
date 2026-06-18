#pragma once

#include "raylib.h"
#include "View/Renderer.h"
#include "View/TextureAtlas.h"
#include "View/Animator.h"
#include <vector>
#include <memory>
#include <unordered_map>

struct Particle;

namespace View {

class ParticleRenderer {
public:
    static ParticleRenderer& GetInstance();

    void RenderAll(const std::vector<Particle*>& particles, const Camera2D& camera, float dt);
    void EmitBurst(Vector2 pos, int count = 8);
    void Shutdown();

private:
    ParticleRenderer();
    ~ParticleRenderer();
    ParticleRenderer(const ParticleRenderer&) = delete;
    ParticleRenderer& operator=(const ParticleRenderer&) = delete;

    void InitProjectileAtlases();

    Texture2D m_softCircle{};
    bool m_initialized = false;

    struct ProjectileAnim {
        std::shared_ptr<Animations::TextureAtlas> atlas;
        Animations::Animator anim;
        bool animated = false;
    };
    std::unordered_map<int, ProjectileAnim> m_projectileAnims; // keyed by ProjectileType
};

} // namespace View
