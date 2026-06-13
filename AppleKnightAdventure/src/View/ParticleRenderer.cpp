#include "View/ParticleRenderer.h"
#include "Systems/ParticleSystem.h"
#include <cmath>
#include <cstdlib>
#include "View/EntityRenderer.h"
#include "View/GameView.h"
#include "Model/Projectile.h"

static Texture2D CreateSoftCircleTexture() {
    const int size = 16;
    Image img = GenImageColor(size, size, BLANK);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float dx = (x + 0.5f) / size - 0.5f;
            float dy = (y + 0.5f) / size - 0.5f;
            float dist = sqrtf(dx*dx + dy*dy) * 2.0f;
            unsigned char a = static_cast<unsigned char>((1.0f - fminf(dist, 1.0f)) * 255);
            Color c = {255, 255, 255, a};
            ImageDrawPixel(&img, x, y, c);
        }
    }
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    return tex;
}

namespace View {

struct SimpleParticle { Vector2 pos; Vector2 vel; Color color; float life; };
static std::vector<SimpleParticle> s_debris;

static void UpdateAndRenderDebris(Renderer& r, const Camera2D& cam, float dt) {
    if (s_debris.empty()) return;
    for (auto it = s_debris.begin(); it != s_debris.end();) {
        it->life -= dt;
        it->pos.x += it->vel.x * dt;
        it->pos.y += it->vel.y * dt;
        Vector2 screen = GetWorldToScreen2D(it->pos, cam);
        r.DrawRectangle({screen.x, screen.y}, {4, 4}, it->color, Layer::Foreground, 0.0f);
        if (it->life <= 0.0f) it = s_debris.erase(it); else ++it;
    }
}

ParticleRenderer& ParticleRenderer::GetInstance() {
    static ParticleRenderer instance;
    return instance;
}

void ParticleRenderer::InitProjectileAtlases() {
    auto loadAnim = [&](int projType, const std::string& jsonPath,
                        bool animated, const std::string& clipName) {
        auto atlas = Animations::TextureAtlas::LoadFromJSON(jsonPath);
        if (!atlas) return;
        atlas->LoadTexture();
        ProjectileAnim pa;
        pa.atlas = std::move(atlas);
        pa.animated = animated;
        if (animated) {
            pa.anim.SetTexture(pa.atlas->GetTexture());
            if (pa.atlas->HasClip(clipName)) {
                pa.anim.AddClip(pa.atlas->GetClip(clipName));
                pa.anim.Play(clipName);
            }
        }
        m_projectileAnims.emplace(projType, std::move(pa));
    };

    // BossAttack → explosion atlas (animated)
    loadAnim(static_cast<int>(ProjectileType::BossAttack),
             "assets/textures/projectiles/explosion.json", true, "explode");
    // Magic → arrow atlas (static frame)
    loadAnim(static_cast<int>(ProjectileType::Magic),
             "assets/textures/projectiles/arrow.json", false, "");
}

ParticleRenderer::ParticleRenderer() {
    m_softCircle = CreateSoftCircleTexture();
    InitProjectileAtlases();
    m_initialized = true;
}

ParticleRenderer::~ParticleRenderer() {
    Shutdown();
}

void ParticleRenderer::RenderAll(const std::vector<Particle*>& particles, const Camera2D& camera, float dt) {
    if (!m_initialized) return;

    // Update projectile animations
    for (auto& kv : m_projectileAnims) {
        if (kv.second.animated) {
            kv.second.anim.Update(dt);
        }
    }

    Rectangle src = {0, 0, static_cast<float>(m_softCircle.width),
                     static_cast<float>(m_softCircle.height)};

    for (const Particle* p : particles) {
        if (!p || !p->active) continue;

        float scaleX = p->size * 2.0f / src.width;
        float scaleY = p->size * 2.0f / src.height;

        Renderer::GetInstance().SubmitSprite(
            &m_softCircle, src, p->position,
            {scaleX, scaleY}, 0.0f,
            {src.width * 0.5f, src.height * 0.5f},
            p->color, Layer::Foreground, 0.0f, false, 0);
    }

    // Render projectiles using animated atlases
    for (const auto& [id, renderData] : EntityRenderer::GetInstance().GetEntities()) {
        const Entity* entity = renderData.entity;
        if (!entity || !entity->IsActive()) continue;

        if (entity->GetType() == EntityType::Projectile) {
            const Projectile* proj = static_cast<const Projectile*>(entity);
            int pType = static_cast<int>(proj->GetProjectileType());

            auto animIt = m_projectileAnims.find(pType);
            if (animIt != m_projectileAnims.end()) {
                const auto& pa = animIt->second;
                if (pa.atlas && pa.atlas->GetTexture() && pa.atlas->GetTexture()->id != 0) {
                    Texture2D* tex = pa.atlas->GetTexture();
                    Rectangle pSrc{};
                    if (pa.animated && pa.anim.IsPlaying()) {
                        pSrc = pa.anim.GetCurrentSrcRect();
                    } else if (pa.atlas->HasFrame("default")) {
                        pSrc = pa.atlas->GetFrameRect("default");
                    } else {
                        pSrc = {0, 0, (float)tex->width, (float)tex->height};
                    }
                    Renderer::GetInstance().SubmitSprite(
                        tex, pSrc, proj->GetPosition(),
                        {proj->GetScale(), proj->GetScale()}, proj->GetRotation(),
                        {pSrc.width * 0.5f, pSrc.height * 0.5f},
                        WHITE, Layer::World, 0.0f, false, proj->GetId());
                }
            } else {
                // Fallback to static textures from GameView
                Texture2D* tex = nullptr;
                if (proj->GetProjectileType() == ProjectileType::BossAttack) {
                    tex = GameView::GetInstance().GetBossAttackTex();
                } else if (proj->GetProjectileType() == ProjectileType::Magic) {
                    tex = GameView::GetInstance().GetMagicTex();
                }
                if (tex && tex->id != 0) {
                    Rectangle pSrc = {0, 0, (float)tex->width, (float)tex->height};
                    Renderer::GetInstance().SubmitSprite(
                        tex, pSrc, proj->GetPosition(),
                        {proj->GetScale(), proj->GetScale()}, proj->GetRotation(),
                        {pSrc.width * 0.5f, pSrc.height * 0.5f},
                        WHITE, Layer::World, 0.0f, false, proj->GetId());
                }
            }
        }
    }

    UpdateAndRenderDebris(Renderer::GetInstance(), camera, dt);
}

void ParticleRenderer::Shutdown() {
    for (auto& kv : m_projectileAnims) {
        kv.second.anim.Stop();
        kv.second.atlas.reset();
    }
    m_projectileAnims.clear();
    if (m_initialized && m_softCircle.id != 0) {
        UnloadTexture(m_softCircle);
        m_softCircle = {};
    }
    m_initialized = false;
}

void ParticleRenderer::EmitBurst(Vector2 pos, int count) {
    if (!m_initialized) return;
    for (int i = 0; i < count; ++i) {
        float ang = ((float)i / (float)count) * 2.0f * 3.14159f;
        float spd = 50.0f + (float)(rand() % 80);
        SimpleParticle p;
        p.pos = pos;
        p.vel = { cosf(ang) * spd, sinf(ang) * spd };
        p.color = (Color){200, 180, 160, 255};
        p.life = 0.8f + ((rand() % 100) / 200.0f);
        s_debris.push_back(p);
    }
}

} // namespace View
