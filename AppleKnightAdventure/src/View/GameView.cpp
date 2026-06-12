#include "View/GameView.h"
#include "View/CharacterRenderer.h"
#include "View/EntityRenderer.h"
#include "View/ParticleRenderer.h"
#include "View/Renderer.h"
#include "View/HUDView.h"
#include "View/FloatingText.h"
#include "View/EnemyStatusRenderer.h"
#include "View/ResultView.h"
#include "View/MenuView.h"
#include "View/InventoryView.h"
#include "Model/DualWorld.h"
#include <cmath>
#include <cstdlib>

namespace View {

GameView& GameView::GetInstance() {
    static GameView instance;
    return instance;
}

bool GameView::Init() {
    if (!View::Renderer::GetInstance().IsInitialized()) {
        return false;
    }
    View::ParticleRenderer::GetInstance();
    LoadShadowShader();
    return true;
}

void GameView::LoadShadowShader() {
    const char* vsPath = "assets/shaders/shadow.vs";
    const char* fsPath = "assets/shaders/shadow.fs";
    m_shadowShader = LoadShader(vsPath, fsPath);
    if (m_shadowShader.id != 0) {
        m_shaderLoaded = true;
    }
}

void GameView::Update(float dt) {
    View::CharacterRenderer::GetInstance().UpdateAll(dt);
    View::FloatingTextManager::GetInstance().Update(dt);
    if (View::ResultView::GetInstance().IsVisible()) {
        View::ResultView::GetInstance().Update(dt);
    }

    // Camera shake decay
    if (m_shakeTimer > 0.0f) {
        m_shakeTimer -= dt;
        if (m_shakeTimer < 0.0f) m_shakeTimer = 0.0f;
    }
}

void GameView::SetActiveWorldLayer(WorldLayer layer, const Vector2& lightWorldPos) {
    m_activeLayer = layer;
    m_lightWorldPos = lightWorldPos;
}

WorldLayer GameView::GetActiveWorldLayer() const {
    return m_activeLayer;
}

// ── Tilemap / Multi-tilesheet ──────────────────────────────────────

void GameView::LoadTileset(int tileType, const std::string& texturePath, int cols) {
    TilesetInfo info;
    info.texture = ::LoadTexture(texturePath.c_str());
    info.gridCols = (cols > 0) ? cols : 1;
    m_tilesets[tileType] = info;
}

void GameView::RenderTilemap(const DualWorld* world) {
    if (!world) return;
    Renderer& r = Renderer::GetInstance();
    const float ts = (float)TILE_SIZE;

    auto renderLayer = [&](WorldLayer layer, Layer renderLayerEnum, float z) {
        const auto& tiles = world->GetTiles(layer);
        for (const auto& tile : tiles) {
            auto it = m_tilesets.find(tile.tileType);
            if (it == m_tilesets.end()) continue;
            TilesetInfo& tsi = it->second;
            if (tsi.texture.id == 0) continue;

            float col = (float)(tile.tileId % tsi.gridCols);
            float row = (float)(tile.tileId / tsi.gridCols);
            Rectangle src = { col * ts, row * ts, ts, ts };
            Vector2 pos = { (float)tile.x * ts, (float)tile.y * ts };

            r.SubmitSprite(&tsi.texture, src, pos, {1,1}, 0.0f, {0,0},
                           WHITE, renderLayerEnum, z, false, 0);
        }
    };

    renderLayer(WorldLayer::Light, Layer::Background, -1.0f);

    if (m_activeLayer == WorldLayer::Shadow) {
        renderLayer(WorldLayer::Shadow, Layer::World, 0.0f);
    }
}

// ── Camera shake ───────────────────────────────────────────────────

void GameView::Shake(float intensity, float duration) {
    m_shakeIntensity = intensity;
    m_shakeTimer = duration;
}

// ── Render ─────────────────────────────────────────────────────────

void GameView::Render(const Camera2D& camera, const std::vector<Particle*>& particles, float dt) {
    View::Renderer& r = View::Renderer::GetInstance();

    // Camera shake offset
    Camera2D cam = camera;
    if (m_shakeTimer > 0.0f && m_shakeIntensity > 0.0f) {
        float ox = ((float)std::rand() / (float)RAND_MAX - 0.5f) * 2.0f * m_shakeIntensity;
        float oy = ((float)std::rand() / (float)RAND_MAX - 0.5f) * 2.0f * m_shakeIntensity;
        cam.target.x += ox;
        cam.target.y += oy;
    }

    // Apply shadow shader for Shadow world layer
    if (m_activeLayer == WorldLayer::Shadow && m_shaderLoaded) {
        Vector2 screenPos = GetWorldToScreen2D(m_lightWorldPos, cam);
        SetShaderValue(m_shadowShader, GetShaderLocation(m_shadowShader, "lightScreenPos"), &screenPos, SHADER_UNIFORM_VEC2);
        float screenW = (float)r.GetWindowWidth();
        float screenH = (float)r.GetWindowHeight();
        SetShaderValue(m_shadowShader, GetShaderLocation(m_shadowShader, "screenW"), &screenW, SHADER_UNIFORM_FLOAT);
        SetShaderValue(m_shadowShader, GetShaderLocation(m_shadowShader, "screenH"), &screenH, SHADER_UNIFORM_FLOAT);
        float radius = 150.0f;
        SetShaderValue(m_shadowShader, GetShaderLocation(m_shadowShader, "radius"), &radius, SHADER_UNIFORM_FLOAT);
        BeginShaderMode(m_shadowShader);
    }

    BeginMode2D(cam);
    r.BeginFrame();

    // Render order: background tiles → world entities → foreground particles → UI overlays
    // (Tilemap is external — Controller calls RenderTilemap before Render if needed)

    View::CharacterRenderer::GetInstance().RenderAll();
    View::EntityRenderer::GetInstance().RenderAll();
    View::ParticleRenderer::GetInstance().RenderAll(particles, cam, dt);
    View::EnemyStatusRenderer::GetInstance().Render(cam);
    View::FloatingTextManager::GetInstance().Render(cam);

    r.EndFrameAndFlush();
    EndMode2D();

    if (m_activeLayer == WorldLayer::Shadow && m_shaderLoaded) {
        EndShaderMode();
    }
}

void GameView::Shutdown() {
    View::CharacterRenderer::GetInstance().Clear();
    View::ParticleRenderer::GetInstance().Shutdown();

    // Unload tilesets
    for (auto& kv : m_tilesets) {
        if (kv.second.texture.id != 0) {
            ::UnloadTexture(kv.second.texture);
        }
    }
    m_tilesets.clear();

    if (m_shaderLoaded && m_shadowShader.id != 0) {
        UnloadShader(m_shadowShader);
        m_shaderLoaded = false;
    }
}

} // namespace View
