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
#include <cstdio>

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

    // Preload Character Atlases
    View::CharacterRenderer& cr = View::CharacterRenderer::GetInstance();
    cr.PreloadAtlas("assets/textures/player.json");
    // Boss: each clip in its own atlas
    cr.PreloadAtlas("assets/textures/boss/idle.json");
    cr.PreloadAtlas("assets/textures/boss/walk.json");
    cr.PreloadAtlas("assets/textures/boss/attack.json");
    cr.PreloadAtlas("assets/textures/boss/death.json");
    cr.PreloadAtlas("assets/textures/boss/hit.json");
    // Pets
    cr.PreloadAtlas("assets/textures/pets/baby_dragon.json");
    cr.PreloadAtlas("assets/textures/pets/fairy.json");
    cr.PreloadAtlas("assets/textures/pets/ghost.json");
    cr.PreloadAtlas("assets/textures/pets/skull.json");
    cr.PreloadAtlas("assets/textures/pets/wisp.json");
    // Enemies
    cr.PreloadAtlas("assets/textures/enemies/melee_idle.json");
    cr.PreloadAtlas("assets/textures/enemies/melee_sword.json");
    cr.PreloadAtlas("assets/textures/enemies/ranged_idle.json");
    cr.PreloadAtlas("assets/textures/enemies/ranged_run.json");
    cr.PreloadAtlas("assets/textures/enemies/ranged.json");
    cr.PreloadAtlas("assets/textures/enemies/flying_spritesheet.json");
    cr.PreloadAtlas("assets/textures/enemies/flying.json");

    // Load Tilesets
    LoadTileset(1, "assets/textures/tiles/light_tileset.png", 8);
    LoadTileset(2, "assets/textures/tiles/shadow_tileset.png", 8);

    // Load static textures (projectiles)
    m_bossAttackTex = ::LoadTexture("assets/textures/boss/boss_attack.png");
    m_magicTex = ::LoadTexture("assets/textures/projectiles/arrow.png");

    // Load backgrounds
    LoadBackgrounds();

    // Load enemy status atlas
    View::EnemyStatusRenderer::GetInstance().LoadResources("assets/textures/enemies/status_atlas.json");

    // Preload item atlases (for InventoryView / HUDView)
    cr.PreloadAtlas("assets/textures/items/apple.json");
    cr.PreloadAtlas("assets/textures/items/coin.json");
    cr.PreloadAtlas("assets/textures/items/potion.json");
    cr.PreloadAtlas("assets/textures/items/key.json");
    cr.PreloadAtlas("assets/textures/items/key_silver.json");
    cr.PreloadAtlas("assets/textures/items/bag_coins.json");
    cr.PreloadAtlas("assets/textures/items/equipment.json");
    cr.PreloadAtlas("assets/textures/items/potion_red.json");

    // Preload projectile atlases (for SkillBarView / ParticleRenderer)
    cr.PreloadAtlas("assets/textures/projectiles/arrow.json");
    cr.PreloadAtlas("assets/textures/projectiles/fire_bullet.json");
    cr.PreloadAtlas("assets/textures/projectiles/slash.json");
    cr.PreloadAtlas("assets/textures/projectiles/hit.json");
    cr.PreloadAtlas("assets/textures/projectiles/explosion.json");

    return true;
}

void GameView::LoadShadowShader() {
    if (m_shaderLoaded) return;

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
    View::EnemyStatusRenderer::GetInstance().Update(dt);
    if (View::ResultView::GetInstance().IsVisible()) {
        View::ResultView::GetInstance().Update(dt);
    }

    // Background scroll
    m_bgScrollOffset += dt * 30.0f;

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

// ── Background Parallax ────────────────────────────────────────────

void GameView::LoadBackgrounds() {
    const int bgCount = 4;
    const char* bgPaths[4] = {
        "assets/textures/backgrounds/bg_1",
        "assets/textures/backgrounds/bg_2",
        "assets/textures/backgrounds/bg_3",
        "assets/textures/backgrounds/bg_4"
    };
    const int layerCounts[4] = {4, 5, 4, 3};
    const float speeds[5] = {0.1f, 0.25f, 0.5f, 0.75f, 1.0f};

    m_backgrounds.resize(bgCount);
    for (int b = 0; b < bgCount; ++b) {
        int n = layerCounts[b];
        for (int l = 0; l < n; ++l) {
            char path[256];
            snprintf(path, sizeof(path), "%s/%d.png", bgPaths[b], l + 1);
            BGLayerInfo info;
            info.tex = ::LoadTexture(path);
            info.parallaxSpeed = speeds[l % 5];
            m_backgrounds[b].push_back(info);
        }
    }
}

void GameView::SetActiveBackground(int index) {
    if (index >= 0 && index < (int)m_backgrounds.size()) {
        m_activeBgIndex = index;
    }
}

void GameView::RenderBackground(const Camera2D& cam) {
    if (m_backgrounds.empty()) return;
    Renderer& r = Renderer::GetInstance();

    int screenW = r.GetWindowWidth();
    int screenH = r.GetWindowHeight();

    if (m_activeBgIndex < 0 || m_activeBgIndex >= (int)m_backgrounds.size()) return;
    const auto& layers = m_backgrounds[m_activeBgIndex];

    for (const auto& layer : layers) {
        if (layer.tex.id == 0) continue;
        float tw = (float)layer.tex.width;
        float th = (float)layer.tex.height;

        // Parallax offset: farther layers scroll slower
        float offsetX = m_bgScrollOffset * layer.parallaxSpeed;
        float scaleX = (float)screenW / tw;
        float scaleY = (float)screenH / th;

        // Tile horizontally to fill screen
        float wrapped = fmod(offsetX, tw);
        for (float x = -wrapped; x < (float)screenW; x += tw) {
            r.SubmitSprite(&layer.tex, {0, 0, tw, th}, {x, 0},
                           {scaleX, scaleY}, 0.0f, {0, 0},
                           WHITE, Layer::Background, -2.0f, false, 0);
        }
    }
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

    // Render background parallax (before camera transform)
    RenderBackground(cam);

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
    
    // Render entities and particles
    View::CharacterRenderer::GetInstance().RenderAll();
    View::ParticleRenderer::GetInstance().RenderAll(particles, cam, dt);

    View::EnemyStatusRenderer::GetInstance().Render(cam);
    View::FloatingTextManager::GetInstance().Render(cam);

    r.EndFrameAndFlush();
    EndMode2D();

    if (m_activeLayer == WorldLayer::Shadow && m_shaderLoaded) {
        EndShaderMode();
    }

    // Draw HUD over everything (independent of camera)
    ::DrawFPS(10, 10);
}

void GameView::Shutdown() {
    View::CharacterRenderer::GetInstance().Clear();
    View::EntityRenderer::GetInstance().Clear();
    View::ParticleRenderer::GetInstance().Shutdown();

    // Unload static textures
    if (m_bossAttackTex.id != 0) ::UnloadTexture(m_bossAttackTex);
    if (m_magicTex.id != 0) ::UnloadTexture(m_magicTex);

    // Unload tilesets
    for (auto& kv : m_tilesets) {
        if (kv.second.texture.id != 0) {
            ::UnloadTexture(kv.second.texture);
        }
    }
    m_tilesets.clear();

    // Unload backgrounds
    for (auto& bg : m_backgrounds) {
        for (auto& layer : bg) {
            if (layer.tex.id != 0) {
                ::UnloadTexture(layer.tex);
            }
        }
    }
    m_backgrounds.clear();

    if (m_shaderLoaded && m_shadowShader.id != 0) {
        UnloadShader(m_shadowShader);
        m_shaderLoaded = false;
    }
}

} // namespace View
