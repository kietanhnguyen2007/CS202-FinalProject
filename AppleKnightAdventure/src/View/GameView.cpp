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
#include "View/UIStateManager.h"
#include "View/TutorialRenderer.h"
#include "Model/GameState.h"
#include "View/UIResourceManager.h"
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
    View::UIResourceManager::GetInstance().Init();

    // Preload Character Atlases
    View::CharacterRenderer& cr = View::CharacterRenderer::GetInstance();
    // Player per-class per-state atlases
    cr.PreloadAtlas("assets/textures/player/fighter_v2/idle_v2.json");
    cr.PreloadAtlas("assets/textures/player/fighter_v2/walk_v2.json");
    cr.PreloadAtlas("assets/textures/player/fighter_v2/run_v2.json");
    cr.PreloadAtlas("assets/textures/player/fighter_v2/jump_v2.json");
    cr.PreloadAtlas("assets/textures/player/fighter_v2/attack1_v2.json");
    cr.PreloadAtlas("assets/textures/player/fighter_v2/attack2_v2.json");
    cr.PreloadAtlas("assets/textures/player/fighter_v2/attack3_v2.json");
    cr.PreloadAtlas("assets/textures/player/fighter_v2/hurt_v2.json");
    cr.PreloadAtlas("assets/textures/player/fighter_v2/dead_v2.json");
    cr.PreloadAtlas("assets/textures/player/fighter_v2/parry_v2.json");
    cr.PreloadAtlas("assets/textures/player/fighter_v2/ultimate_skill_v2.json");
    cr.PreloadAtlas("assets/textures/player/fighter_v2/ultimate_projectile_v2.json");
    cr.PreloadAtlas("assets/textures/player/knight_v2/idle_v2.json");
    cr.PreloadAtlas("assets/textures/player/knight_v2/walk_v2.json");
    cr.PreloadAtlas("assets/textures/player/knight_v2/run_v2.json");
    cr.PreloadAtlas("assets/textures/player/knight_v2/jump_v2.json");
    cr.PreloadAtlas("assets/textures/player/knight_v2/attack1_v2.json");
    cr.PreloadAtlas("assets/textures/player/knight_v2/attack2_v2.json");
    cr.PreloadAtlas("assets/textures/player/knight_v2/attack3_v2.json");
    cr.PreloadAtlas("assets/textures/player/knight_v2/hurt_v2.json");
    cr.PreloadAtlas("assets/textures/player/knight_v2/dead_v2.json");
    cr.PreloadAtlas("assets/textures/player/knight_v2/parry_v2.json");
    cr.PreloadAtlas("assets/textures/player/knight_v2/ultimate_skill_v2.json");
    cr.PreloadAtlas("assets/textures/player/ninja_v2/idle_v2.json");
    cr.PreloadAtlas("assets/textures/player/ninja_v2/walk_v2.json");
    cr.PreloadAtlas("assets/textures/player/ninja_v2/run_v2.json");
    cr.PreloadAtlas("assets/textures/player/ninja_v2/jump_v2.json");
    cr.PreloadAtlas("assets/textures/player/ninja_v2/attack1_v2.json");
    cr.PreloadAtlas("assets/textures/player/ninja_v2/attack2_v2.json");
    cr.PreloadAtlas("assets/textures/player/ninja_v2/hurt_v2.json");
    cr.PreloadAtlas("assets/textures/player/ninja_v2/dead_v2.json");
    cr.PreloadAtlas("assets/textures/player/ninja_v2/parry_v2.json");
    cr.PreloadAtlas("assets/textures/player/ninja_v2/ultimate_skill_v2.json");
    cr.PreloadAtlas("assets/textures/player/ninja_v2/skill3_teleport_start_v2.json");
    cr.PreloadAtlas("assets/textures/player/ninja_v2/skill3_teleport_end_v2.json");
    cr.PreloadAtlas("assets/textures/player/ninja_v2/projectile_attack2_v2.json");
    cr.PreloadAtlas("assets/textures/player/ninja_v2/projectile_ultimate_attack_v2.json");
    cr.PreloadAtlas("assets/textures/player/magic_caster_v2/idle_v2.json");
    cr.PreloadAtlas("assets/textures/player/magic_caster_v2/walk_v2.json");
    cr.PreloadAtlas("assets/textures/player/magic_caster_v2/run_v2.json");
    cr.PreloadAtlas("assets/textures/player/magic_caster_v2/jump_v2.json");
    cr.PreloadAtlas("assets/textures/player/magic_caster_v2/attack1_v2.json");
    cr.PreloadAtlas("assets/textures/player/magic_caster_v2/attack2_v2.json");
    cr.PreloadAtlas("assets/textures/player/magic_caster_v2/attack3_v2.json");
    cr.PreloadAtlas("assets/textures/player/magic_caster_v2/hurt_v2.json");
    cr.PreloadAtlas("assets/textures/player/magic_caster_v2/dead_v2.json");
    cr.PreloadAtlas("assets/textures/player/magic_caster_v2/parry_v2.json");
    cr.PreloadAtlas("assets/textures/player/magic_caster_v2/projectile_attack1_v2.json");
    cr.PreloadAtlas("assets/textures/player/magic_caster_v2/projectile_attack2_v2.json");
    cr.PreloadAtlas("assets/textures/player/magic_caster_v2/projectile_attack3_v2.json");
    cr.PreloadAtlas("assets/textures/player/magic_caster_v2/ultimate_skill_v2.json");
    cr.PreloadAtlas("assets/textures/player/magic_caster_v2/ultimate_skill_projectile_v2.json");
    // Boss V2 atlases. Phase transitions live in the destination phase so
    // SwitchPhase can load the transformation immediately after SetPhase.
    static const char* bossAtlases[] = {
        "boss1/phase1/idle", "boss1/phase1/walk", "boss1/phase1/attack_1",
        "boss1/phase1/attack_2", "boss1/phase1/hurt",
        "boss1/phase2/idle", "boss1/phase2/walk", "boss1/phase2/attack_1",
        "boss1/phase2/attack_2", "boss1/phase2/hurt", "boss1/phase2/dead",
        "boss1/phase2/transition",
        "boss2/phase1/idle", "boss2/phase1/walk", "boss2/phase1/attack_1",
        "boss2/phase1/hurt", "boss2/phase1/ultimate_skill",
        "boss2/phase2/idle", "boss2/phase2/walk", "boss2/phase2/attack_1",
        "boss2/phase2/healing", "boss2/phase2/hurt", "boss2/phase2/transition",
        "boss2/phase2/ultimate_skill",
        "boss2/phase3/idle", "boss2/phase3/walk", "boss2/phase3/attack_1",
        "boss2/phase3/attack_2", "boss2/phase3/attack_3", "boss2/phase3/hurt",
        "boss2/phase3/dead", "boss2/phase3/transition", "boss2/phase3/ultimate_skill",
        "boss3/phase1/idle", "boss3/phase1/walk", "boss3/phase1/attack_1",
        "boss3/phase1/hurt",
        "boss3/phase2/idle", "boss3/phase2/walk", "boss3/phase2/attack_1",
        "boss3/phase2/hurt", "boss3/phase2/transition",
        "boss3/phase3/idle", "boss3/phase3/walk", "boss3/phase3/attack_1",
        "boss3/phase3/attack_2", "boss3/phase3/hurt", "boss3/phase3/transition",
        "boss3/phase4/idle", "boss3/phase4/walk", "boss3/phase4/attack_1",
        "boss3/phase4/attack_2", "boss3/phase4/attack_3", "boss3/phase4/hurt",
        "boss3/phase4/dead", "boss3/phase4/transition"
    };
    for (const char* atlas : bossAtlases) {
        cr.PreloadAtlas(std::string("assets/textures/boss_v2/") + atlas + ".json");
    }
    // Pets — per-state atlases
    cr.PreloadAtlas("assets/textures/pets/skull/idle.json");
    cr.PreloadAtlas("assets/textures/pets/skull/move.json");
    cr.PreloadAtlas("assets/textures/pets/skull/attack.json");
    cr.PreloadAtlas("assets/textures/pets/projectile_skull/attack.json");
    cr.PreloadAtlas("assets/textures/pets/baby_dragon/idle.json");
    cr.PreloadAtlas("assets/textures/pets/baby_dragon/move.json");
    cr.PreloadAtlas("assets/textures/pets/baby_dragon/attack.json");
    cr.PreloadAtlas("assets/textures/pets/projectile_dragon/attack.json");
    cr.PreloadAtlas("assets/textures/pets/fairy/idle.json");
    cr.PreloadAtlas("assets/textures/pets/fairy/move.json");
    cr.PreloadAtlas("assets/textures/pets/fairy/collect_item.json");
    cr.PreloadAtlas("assets/textures/pets/ghost/idle.json");
    cr.PreloadAtlas("assets/textures/pets/ghost/move.json");
    cr.PreloadAtlas("assets/textures/pets/ghost/healing.json");
    // Enemies
    cr.PreloadAtlas("assets/textures/enemies/melee_idle.json");
    cr.PreloadAtlas("assets/textures/enemies/melee_walk.json");
    cr.PreloadAtlas("assets/textures/enemies/melee.json");
    cr.PreloadAtlas("assets/textures/enemies/melee_sword.json");
    cr.PreloadAtlas("assets/textures/enemies/melee_hurt.json");
    cr.PreloadAtlas("assets/textures/enemies/melee_death.json");

    cr.PreloadAtlas("assets/textures/enemies/ranged_idle.json");
    cr.PreloadAtlas("assets/textures/enemies/ranged_run.json");
    cr.PreloadAtlas("assets/textures/enemies/ranged.json");
    cr.PreloadAtlas("assets/textures/enemies/ranged_bomb.json");
    cr.PreloadAtlas("assets/textures/enemies/ranged_hurt.json");
    cr.PreloadAtlas("assets/textures/enemies/ranged_death.json");

    cr.PreloadAtlas("assets/textures/enemies/flying_spritesheet.json");
    cr.PreloadAtlas("assets/textures/enemies/flying.json");
    cr.PreloadAtlas("assets/textures/enemies/flying_projectile.json");
    cr.PreloadAtlas("assets/textures/enemies/flying_hurt.json");
    cr.PreloadAtlas("assets/textures/enemies/flying_death.json");

    // Load Tilesets — Legacy Fantasy High Forest
    LoadTileset(1, "assets/textures/tiles/Tiles.png", 25);
    LoadTileset(2, "assets/textures/tiles/Buildings.png", 25);
    LoadTileset(3, "assets/textures/tiles/Hive.png", 25);
    LoadTileset(4, "assets/textures/tiles/Interior-01.png", 25);
    LoadTileset(5, "assets/textures/tiles/Props-Rocks.png", 18);
    LoadTileset(6, "assets/textures/tiles/Tree-Assets.png", 21);

    // Preload Objects
    cr.PreloadAtlas("assets/textures/objects/checkpoint_captured.json");
    cr.PreloadAtlas("assets/textures/objects/checkpoint_uncaptured.json");
    cr.PreloadAtlas("assets/textures/objects/checkpoint_flag_out.json");
    cr.PreloadAtlas("assets/textures/objects/chest.json");
    cr.PreloadAtlas("assets/textures/objects/chest_closed.json");
    cr.PreloadAtlas("assets/textures/objects/chest_open.json");

    // Load static textures (projectiles)
    m_magicTex = ::LoadTexture("assets/textures/projectiles/arrow.png");

    // Load backgrounds
    LoadBackgrounds();

    // Load enemy status atlas
    View::EnemyStatusRenderer::GetInstance().LoadResources("assets/textures/enemies/status_atlas.json");

    // Preload item atlases (for InventoryView / HUDView)
    cr.PreloadAtlas("assets/textures/items/apple.json");
    cr.PreloadAtlas("assets/textures/items/coin.json");
    cr.PreloadAtlas("assets/textures/items/key.json");
    cr.PreloadAtlas("assets/textures/items/bag_coins.json");
    cr.PreloadAtlas("assets/textures/items/equipment.json");
    cr.PreloadAtlas("assets/textures/items/potion_red.json");

    // Preload projectile atlases (for CharacterRenderer projectile rendering)
    cr.PreloadAtlas("assets/textures/projectiles/arrow.json");
    cr.PreloadAtlas("assets/textures/projectiles/fire_bullet.json");
    cr.PreloadAtlas("assets/textures/projectiles/explosion.json");

    return true;
}

void GameView::Update(float dt) {
    View::CharacterRenderer::GetInstance().UpdateAll(dt);
    View::EntityRenderer::GetInstance().Update(dt);
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

// ── Background Parallax ────────────────────────────────────────────

void GameView::LoadBackgrounds(BackgroundTheme theme) {
    // Unload current set before loading new one
    for (auto& bgSet : m_backgrounds)
        for (auto& layer : bgSet)
            UnloadTexture(layer.tex);
    m_backgrounds.clear();

    struct LayerDef { const char* file; float speed; };

    // ── Forest (Ansimuz Parallax Forest v2) ──────────────────────────
    static const LayerDef kForest[] = {
        {"assets/textures/backgrounds/forest/back.png",   0.10f},
        {"assets/textures/backgrounds/forest/middle.png", 0.30f},
        {"assets/textures/backgrounds/forest/front.png",  0.60f},
    };
    // ── Cold Corridor (Ansimuz Gothicvania Cold Corridors) ────────────
    static const LayerDef kColdCorridor[] = {
        {"assets/textures/backgrounds/cold_corridor/back.png",       0.05f},
        {"assets/textures/backgrounds/cold_corridor/far.png",        0.15f},
        {"assets/textures/backgrounds/cold_corridor/middle.png",     0.30f},
        {"assets/textures/backgrounds/cold_corridor/near.png",       0.50f},
        {"assets/textures/backgrounds/cold_corridor/foreground.png", 0.75f},
    };
    // ── Underwater (Ansimuz Underwater Fantasy) ───────────────────────
    static const LayerDef kUnderwater[] = {
        {"assets/textures/backgrounds/underwater/far.png",          0.05f},
        {"assets/textures/backgrounds/underwater/foreground_1.png", 0.20f},
        {"assets/textures/backgrounds/underwater/foreground_2.png", 0.40f},
        {"assets/textures/backgrounds/underwater/sand.png",         0.70f},
    };

    struct ThemeEntry { const LayerDef* layers; int count; };
    static const ThemeEntry kThemes[] = {
        { kForest,       3 },   // BackgroundTheme::Forest
        { kColdCorridor, 5 },   // BackgroundTheme::ColdCorridor
        { kUnderwater,   4 },   // BackgroundTheme::Underwater
    };

    int idx = static_cast<int>(theme);
    const int themeCount = static_cast<int>(sizeof(kThemes) / sizeof(kThemes[0]));
    if (idx < 0 || idx >= themeCount) idx = 0;

    const ThemeEntry& entry = kThemes[idx];
    m_backgrounds.resize(1);
    m_backgrounds[0].clear();
    for (int l = 0; l < entry.count; ++l) {
        BGLayerInfo info;
        info.tex           = ::LoadTexture(entry.layers[l].file);
        info.parallaxSpeed = entry.layers[l].speed;
        m_backgrounds[0].push_back(info);
    }
    m_activeBgIndex = 0;
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

    for (auto& layer : layers) {
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

void GameView::RenderTilemap(const std::vector<Tile>& tiles) {
    const float ts = (float)TILE_SIZE;

    for (const auto& tile : tiles) {
        auto it = m_tilesets.find(tile.tileType);
        if (it == m_tilesets.end()) continue;
        TilesetInfo& tsi = it->second;
        if (tsi.texture.id == 0) continue;

        float srcSize = (float)tsi.texture.width / (float)tsi.gridCols;
        float col = (float)(tile.tileId % tsi.gridCols);
        float row = (float)(tile.tileId / tsi.gridCols);

        // Apply LDtk flip flags ("f" field): bit0=flipX, bit1=flipY
        float srcW = srcSize * ((tile.flipFlags & 1) ? -1.0f : 1.0f);
        float srcH = srcSize * ((tile.flipFlags & 2) ? -1.0f : 1.0f);

        Rectangle src = {
            col * srcSize + (srcW < 0 ? srcSize : 0.0f),
            row * srcSize + (srcH < 0 ? srcSize : 0.0f),
            srcW,
            srcH
        };
        Rectangle dest = {
            (float)tile.x * ts,
            (float)tile.y * ts,
            ts,
            ts
        };
        DrawTexturePro(tsi.texture, src, dest, {0.0f, 0.0f}, 0.0f, WHITE);
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

    // 1. Submit and flush parallax background in screen space
    RenderBackground(cam);
    r.EndFrameAndFlush();

    // 2. Begin camera space rendering for world elements
    BeginMode2D(cam);
    
    // Render background and main tilemap layers
    if (m_tiles[static_cast<int>(MapLayer::Background)]) RenderTilemap(*m_tiles[static_cast<int>(MapLayer::Background)]);
    if (m_tiles[static_cast<int>(MapLayer::Main)]) RenderTilemap(*m_tiles[static_cast<int>(MapLayer::Main)]);

    RenderFakeWallHints(dt);

    // Render entities and particles
    View::CharacterRenderer::GetInstance().RenderAll();
    View::EntityRenderer::GetInstance().RenderAll();
    if (m_entities) View::TutorialRenderer::GetInstance().RenderAll(*m_entities, dt);
    View::ParticleRenderer::GetInstance().RenderAll(particles, cam, dt);

    // Render foreground tilemap layer (above entities)
    if (m_tiles[static_cast<int>(MapLayer::Foreground)]) RenderTilemap(*m_tiles[static_cast<int>(MapLayer::Foreground)]);

    View::EnemyStatusRenderer::GetInstance().Render(cam);

    // Flush world elements in camera space
    r.EndFrameAndFlush();
    EndMode2D();

    // 3. Render screen-space overlays (Floating text, FPS, UI)
    View::FloatingTextManager::GetInstance().Render(cam);

    ::DrawFPS(10, 10);

    // Render UI overlay (HUD, menu, inventory, result, etc.)
    View::UIStateManager::GetInstance().RenderAll();
}

void GameView::Shutdown() {
    View::ResultView::GetInstance().Shutdown();
    View::CharacterRenderer::GetInstance().Clear();
    View::EntityRenderer::GetInstance().Clear();
    View::EnemyStatusRenderer::GetInstance().Shutdown();
    View::ParticleRenderer::GetInstance().Shutdown();
    View::TutorialRenderer::GetInstance().Shutdown();

    // Unload static textures
    if (m_magicTex.id != 0) ::UnloadTexture(m_magicTex);
    m_magicTex = {};

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

    View::UIResourceManager::GetInstance().Shutdown();
}

void GameView::RenderFakeWallHints(float dt) {
    if (!m_entities) return;
    
    m_fakeWallPulseTimer += dt * 5.0f; // pulse speed
    float alpha = 20.0f + 30.0f * (0.5f + 0.5f * std::sin(m_fakeWallPulseTimer));
    Color hintColor = {255, 220, 80, (unsigned char)alpha};

    for (const auto& entity : *m_entities) {
        if (entity && entity->GetType() == EntityType::FakeWall && entity->IsActive()) {
            DrawRectangleRec(entity->GetBoundingBox(), hintColor);
        }
    }
}

} // namespace View
