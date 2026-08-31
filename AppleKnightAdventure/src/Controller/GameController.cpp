#include "Controller/GameController.h"
#include "Controller/InputController.h"
#include "Factories/LevelFactory.h"
#include "Factories/EnemyFactory.h"
#include "Model/Boss.h"
#include "Model/Enemy.h"
#include "Model/Chest.h"
#include "Model/Checkpoint.h"
#include "Model/FakeWall.h"
#include "Model/Item.h"
#include "Model/KnightSkillSet.h"
#include "Model/FighterSkillSet.h"
#include "Model/MagicCasterSkillSet.h"
#include "Model/NinjaSkillSet.h"
#include "Model/TeleportPortal.h"
#include "Model/Signboard.h"
#include "Model/LevelCompleteCup.h"
#include "Model/Player.h"
#include "Model/SaveManager.h"
#include "Systems/ParticleSystem.h"
#include "View/GameView.h"
#include "View/CharacterRenderer.h"
#include "View/EntityRenderer.h"
#include "View/HUDView.h"
#include "View/SkillBarView.h"
#include "View/InteractPrompt.h"
#include "View/MenuView.h"
#include "View/OptionsView.h"
#include "View/UIStateManager.h"
#include "View/FloatingText.h"
#include "View/ParticleRenderer.h"
#include "View/TutorialRenderer.h"
#include "View/ResultView.h"
#include "View/MinimapView.h"
#include "View/ElementalFX.h"
#include "View/EnemyStatusRenderer.h"
#include "Systems/SoundManager.h"
#include "Systems/AchievementManager.h"
#include "Utils/Constants.h"
#include <nlohmann/json.hpp>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <limits>

namespace {
bool RectOverlap(const Rectangle& a, const Rectangle& b) {
    return a.x < b.x + b.width &&
           a.x + a.width > b.x &&
           a.y < b.y + b.height &&
           a.y + a.height > b.y;
}

float Distance(const Vector2& a, const Vector2& b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

Rectangle ItemPickupBox(const Entity& item) {
    Rectangle box = item.GetBoundingBox();
    const float padding = TILE_SIZE * 0.5f;
    box.x -= padding;
    box.y -= padding;
    box.width += padding * 2.0f;
    box.height += padding * 2.0f;
    return box;
}

Rectangle CheckpointInteractionBox(const Checkpoint& checkpoint) {
    const Vector2 position = checkpoint.GetPosition();

    // Checkpoint frames are 64x64 while the model hitbox is intentionally
    // small.  The pole is drawn near the middle of that padded frame and its
    // base is four tiles below the authored top-left position.  Use the visible
    // pole footprint for interaction so a player standing beside its base can
    // activate it without changing physics or respawn coordinates.
    return {
        position.x + TILE_SIZE * 0.5f,
        position.y,
        TILE_SIZE * 2.0f,
        TILE_SIZE * 4.0f
    };
}

bool LineIntersectsRect(Vector2 start, Vector2 end, const Rectangle& rect) {
    const float dx = end.x - start.x;
    const float dy = end.y - start.y;
    float nearTime = 0.0f;
    float farTime = 1.0f;

    auto ClipAxis = [&](float position, float delta, float minValue, float maxValue) {
        constexpr float EPSILON = 0.0001f;
        if (std::abs(delta) < EPSILON) {
            return position >= minValue && position <= maxValue;
        }

        float t1 = (minValue - position) / delta;
        float t2 = (maxValue - position) / delta;
        if (t1 > t2) std::swap(t1, t2);
        nearTime = std::max(nearTime, t1);
        farTime = std::min(farTime, t2);
        return nearTime <= farTime;
    };

    return ClipAxis(start.x, dx, rect.x, rect.x + rect.width)
        && ClipAxis(start.y, dy, rect.y, rect.y + rect.height)
        && farTime >= 0.0f && nearTime <= 1.0f;
}

float LoadVictoryParTime(int levelNumber) {
    constexpr float fallback = 240.0f;
    std::ifstream file("assets/config/victory_grades.json");
    if (!file.is_open()) return fallback;
    try {
        nlohmann::json root;
        file >> root;
        const float defaultTime = root.value("defaultParTime", fallback);
        if (!root.contains("levels")) return defaultTime;
        const std::string key = std::to_string(levelNumber);
        return std::max(1.0f, root["levels"].value(key, defaultTime));
    } catch (...) {
        return fallback;
    }
}

std::string CharacterClassId(CharacterClass value) {
    switch (value) {
        case CharacterClass::Fighter: return "fighter";
        case CharacterClass::MagicCaster: return "magic_caster";
        case CharacterClass::Ninja: return "ninja";
        case CharacterClass::Knight:
        default: return "knight";
    }
}
} // namespace

GameController& GameController::GetInstance() {
    static GameController instance;
    return instance;
}

void GameController::ConfigureLocalCoop(bool enabled, CharacterClass secondPlayerClass) {
    m_localCoop = enabled;
    m_secondPlayerClass = secondPlayerClass;
}

bool GameController::Init() {
    auto& snd = SoundManager::GetInstance();
    if (!snd.IsAudioInitialized()) {
        snd.InitAudio();
    }

    View::GameView::GetInstance().Init();
    View::HUDView::GetInstance().Init();
    View::HUDView::GetInstance().LoadResources("assets/ui/ui_atlas.json");
    View::SkillBarView::GetInstance().Init();
    View::SkillBarView::GetInstance().LoadResources();
    View::UIStateManager::GetInstance().Init();
    View::MinimapView::GetInstance().Init();
    View::TutorialRenderer::GetInstance().Init();
    View::ResultView::GetInstance().Init();

    m_running = true;
    m_returnToMenu = false;
    LoadTilesets();

    return true;
}

void GameController::LoadTilesets() {
    auto& gv = View::GameView::GetInstance();
    gv.LoadTileset(1, "assets/textures/tiles/Tiles.png", 25);
    gv.LoadTileset(2, "assets/textures/tiles/Buildings.png", 25);
    gv.LoadTileset(3, "assets/textures/tiles/Hive.png", 25);
    gv.LoadTileset(4, "assets/textures/tiles/Interior-01.png", 25);
    gv.LoadTileset(5, "assets/textures/tiles/Props-Rocks.png", 18);
    gv.LoadTileset(6, "assets/textures/tiles/Tree-Assets.png", 21);
    // Background sẽ được load tại StartLevel() dựa theo BackgroundTheme của map
}

std::string GameController::GetLevelPath(int levelNumber) const {
    if (levelNumber == -99) return "assets/levels/temp_playtest.lvl";
    if (levelNumber == -98) return "assets/levels/custom_map.lvl";

    if (levelNumber == 1 && std::filesystem::exists("assets/levels/tutorial.ldtk")) {
        return "assets/levels/tutorial.ldtk";
    }
    
    // Keep gameplay level numbers aligned with LDtk filenames so the map being
    // edited is always the map loaded by the game (Level 2 -> lvl2.ldtk, etc.).
    if (levelNumber >= 2 && levelNumber <= 6) {
        std::string path = "assets/levels/lvl" + std::to_string(levelNumber) + ".ldtk";
        if (std::filesystem::exists(path)) return path;
    }
    
    // Fallback to world.ldtk for Level 1 or if specific file doesn't exist
    const std::string ldtkPath = "assets/levels/world.ldtk";
    if (std::filesystem::exists(ldtkPath)) return ldtkPath;
    
    return "assets/levels/level" + std::to_string(levelNumber) + ".lvl";
}

std::string GameController::GetPlayerAtlasRoot(CharacterClass playerClass) const {
    switch (playerClass) {
        case CharacterClass::Fighter: return "assets/textures/player/fighter_v2/";
        case CharacterClass::Ninja: return "assets/textures/player/ninja_v2/";
        case CharacterClass::MagicCaster: return "assets/textures/player/magic_caster_v2/";
        case CharacterClass::Knight:
        default: return "assets/textures/player/knight_v2/";
    }
}

void GameController::RegisterPlayerVisuals(Player* player, CharacterClass playerClass) {
    if (!player) return;
    const std::string root = GetPlayerAtlasRoot(playerClass);
    const auto atlasPath = [&](const std::string& stem) {
        return root + stem + "_v2.json";
    };
    auto& cr = View::CharacterRenderer::GetInstance();
    const uint32_t id = static_cast<uint32_t>(player->GetId());

    // Register all clips with explicit aliases so CharacterRenderer can find them by name.
    cr.Register(player, atlasPath("idle"), "idle");
    cr.MergeAtlas(id, atlasPath("walk"),        "walk");
    // Jump atlases contain both "jump" and "jump_fall"; preserve both names.
    cr.MergeAtlas(id, atlasPath("jump"));
    cr.MergeAtlas(id, atlasPath("hurt"),        "hurt");
    cr.MergeAtlas(id, atlasPath("dead"),        "dead");
    cr.MergeAtlas(id, atlasPath("attack1"),     "attack");
    cr.MergeAtlas(id, atlasPath("attack2"),     "attack_2");
    if (std::filesystem::exists(atlasPath("parry")))
        cr.MergeAtlas(id, atlasPath("parry"),   "parry");
    if (std::filesystem::exists(atlasPath("ultimate_skill")))
        cr.MergeAtlas(id, atlasPath("ultimate_skill"), "ultimate_skill");
    // run.json is used exclusively for Dash animation
    if (std::filesystem::exists(atlasPath("run")))
        cr.MergeAtlas(id, atlasPath("run"),     "run");
    // Attack3: class-specific
    if (playerClass == CharacterClass::Ninja) {
        // Ninja: teleport_start plays for Attack3, teleport_end plays after snap
        if (std::filesystem::exists(atlasPath("skill3_teleport_start")))
            cr.MergeAtlas(id, atlasPath("skill3_teleport_start"), "attack_3");
        if (std::filesystem::exists(atlasPath("skill3_teleport_end")))
            cr.MergeAtlas(id, atlasPath("skill3_teleport_end"),   "skill3_teleport_end");
    } else {
        if (std::filesystem::exists(atlasPath("attack3")))
            cr.MergeAtlas(id, atlasPath("attack3"), "attack_3");
    }
}

void GameController::RegisterEnemyVisuals(Enemy* enemy) {
    if (!enemy) return;
    auto& cr = View::CharacterRenderer::GetInstance();
    const uint32_t id = static_cast<uint32_t>(enemy->GetId());

    switch (enemy->GetEnemyType()) {
        case EnemyType::Melee:
            cr.Register(enemy, "assets/textures/enemies/melee_idle.json", "idle");
            cr.MergeAtlas(id, "assets/textures/enemies/melee_walk.json");
            cr.MergeAtlas(id, "assets/textures/enemies/melee.json");
            cr.MergeAtlas(id, "assets/textures/enemies/melee_hurt.json");
            cr.MergeAtlas(id, "assets/textures/enemies/melee_death.json");
            break;
        case EnemyType::Ranged:
            cr.Register(enemy, "assets/textures/enemies/ranged_idle.json", "idle");
            cr.MergeAtlas(id, "assets/textures/enemies/ranged_run.json");
            cr.MergeAtlas(id, "assets/textures/enemies/ranged.json");
            cr.MergeAtlas(id, "assets/textures/enemies/ranged_hurt.json");
            cr.MergeAtlas(id, "assets/textures/enemies/ranged_death.json");
            break;
        case EnemyType::Flying:
            cr.Register(enemy, "assets/textures/enemies/flying_spritesheet.json", "fly");
            cr.MergeAtlas(id, "assets/textures/enemies/flying.json");
            cr.MergeAtlas(id, "assets/textures/enemies/flying_hurt.json");
            cr.MergeAtlas(id, "assets/textures/enemies/flying_death.json");
            break;
    }
}

void GameController::RegisterChestVisuals(Chest* chest) {
    if (!chest) return;
    View::EntityRenderer::GetInstance().RegisterAnimated(
        chest, "assets/textures/objects/chest_closed.json", "default");
}

void GameController::RegisterCheckpointVisuals(Checkpoint* checkpoint) {
    if (!checkpoint) return;

    // Keep the gameplay transform exactly where LDtk authored it.  A renderer
    // must never move the model because interaction and completion checks use
    // this same position for the checkpoint hitbox.
    // Endgame checkpoint: always start as uncaptured (hidden flag pole).
    // The flag_out -> captured transition starts only after the player presses F.
    // Regular checkpoint: uncaptured until activated, then flag_out.
    View::EntityRenderer::GetInstance().RegisterAnimated(
        checkpoint, "assets/textures/objects/checkpoint_uncaptured.json", "default");
}

void GameController::RegisterItemVisuals(Item* item) {
    if (!item) return;
    std::string atlasPath = "assets/textures/items/coin.json";
    switch (item->GetItemType()) {
        case ItemType::Apple: atlasPath = "assets/textures/items/apple.json"; break;
        case ItemType::Key: atlasPath = "assets/textures/items/key.json"; break;
        case ItemType::Potion: atlasPath = "assets/textures/items/potion_red.json"; break;
        case ItemType::Equipment: atlasPath = "assets/textures/items/equipment.json"; break;
        default: break;
    }
    View::EntityRenderer::GetInstance().RegisterAnimated(item, atlasPath, "default");
}

void GameController::RegisterPortalVisuals(TeleportPortal* portal) {
    if (!portal) return;

    std::string jsonPath;
    switch (portal->GetColorId()) {
        case 1: jsonPath = "assets/textures/objects/portal_red_anim.json"; break;
        case 2: jsonPath = "assets/textures/objects/portal_blue_anim.json"; break;
        case 3: jsonPath = "assets/textures/objects/portal_green_anim.json"; break;
        case 4: jsonPath = "assets/textures/objects/portal_brown_anim.json"; break;
        case 5: jsonPath = "assets/textures/objects/portal_purple_anim.json"; break;
        default: jsonPath = "assets/textures/objects/portal_blue_anim.json"; break;
    }

    View::EntityRenderer::GetInstance().RegisterAnimated(
        portal, jsonPath, "default", {0, 0}, false);
}

void GameController::RegisterBossVisuals(Boss* boss) {
    if (!boss) return;
    auto& cr = View::CharacterRenderer::GetInstance();
    const uint32_t id = static_cast<uint32_t>(boss->GetId());

    // Determine boss tier by m_bossType
    std::string root = "assets/textures/boss/boss" + std::to_string(boss->GetBossType()) + "/";
    cr.SetBossAssetRoot(id, root);

    cr.Register(boss, root + "phase1/idle.json", "idle"); // Initialize animator
    cr.SwitchPhase(id, BossPhase::Phase1); // Load all Phase 1 clips including projectiles
}

void GameController::RegisterEntityVisuals(Entity* entity) {
    if (!entity) return;
    switch (entity->GetType()) {
        case EntityType::Player:
            RegisterPlayerVisuals(static_cast<Player*>(entity), m_gameState ? m_gameState->GetPlayerClass() : CharacterClass::Knight);
            break;
        case EntityType::Enemy:
            RegisterEnemyVisuals(static_cast<Enemy*>(entity));
            break;
        case EntityType::Boss:
            RegisterBossVisuals(static_cast<Boss*>(entity));
            break;
        case EntityType::Chest:
            RegisterChestVisuals(static_cast<Chest*>(entity));
            break;
        case EntityType::Checkpoint:
            RegisterCheckpointVisuals(static_cast<Checkpoint*>(entity));
            break;
        case EntityType::Item:
            RegisterItemVisuals(static_cast<Item*>(entity));
            break;
        case EntityType::TeleportPortal:
            RegisterPortalVisuals(static_cast<TeleportPortal*>(entity));
            break;
        case EntityType::FakeWall:
            // FakeWall trông như tile thường — không cần Register visuals riêng
            break;
        case EntityType::Projectile: {
            auto proj = static_cast<Projectile*>(entity);
            if (proj->GetProjectileType() == ProjectileType::BossAttack) {
                bool foundBoss = false;
                for (auto& e : m_gameState->GetAllEntities()) {
                    if (e && e->GetType() == EntityType::Boss && e->GetId() == proj->GetOwnerId()) {
                        foundBoss = true;
                        auto boss = static_cast<Boss*>(e.get());
                        std::string texPath;
                        std::string startClip = "default";
                        Vector2 sz = proj->GetSize();
                        if (boss->GetBossType() == 2) {
                            if (proj->GetDirection() == Direction::None) {
                                texPath = "assets/textures/boss/boss2/phase3/projectile_attack2.json";
                            } else {
                                if (boss->GetPhase() == BossPhase::Phase3) {
                                    texPath = "assets/textures/boss/boss2/phase3/projectile_attack1.json";
                                } else if (boss->GetPhase() == BossPhase::Phase2) {
                                    texPath = "assets/textures/boss/boss2/phase2/projectile_attack1.json";
                                } else {
                                    texPath = "assets/textures/boss/boss2/phase1/projectile_attack1.json";
                                }
                            }
                        } else if (boss->GetBossType() == 3) {
                            int subType = proj->GetSubType();
                            if (subType == 1) {
                                texPath = "assets/textures/boss/boss3/projectiles/energy_sphere.json";
                                startClip = "energy_sphere";
                            } else if (subType == 2) {
                                texPath = "assets/textures/boss/boss3/projectiles/energy_blast.json";
                                startClip = "energy_blast";
                            } else if (subType == 3) {
                                texPath = "assets/textures/boss/boss3/projectiles/energy_beam.json";
                                startClip = "energy_beam";
                            } else if (subType == 4) {
                                texPath = "assets/textures/boss/boss3/ground_animate/default.json";
                            } else if (subType == 5) {
                                texPath = "assets/textures/boss/boss3/projectiles/energy_blast.json";
                                startClip = "energy_blast";
                            }
                        }
                        if (!texPath.empty()) {
                            // std::cout << "[DEBUG] Registering Boss Projectile! texPath=" << texPath << ", size=" << sz.x << "x" << sz.y << std::endl;
                            bool flipX = (proj->GetDirection() == Direction::Left);
                            proj->SetRotation(0.0f); // Fix rotation bug so it doesn't offset from hitbox
                            
                            View::EntityRenderer::GetInstance().RegisterAnimated(
                                proj, texPath, startClip, {0.0f, 0.0f}, flipX);
                        } else {
                            std::cout << "[DEBUG] Boss Projectile texPath is EMPTY! size=" << sz.x << "x" << sz.y << std::endl;
                        }
                        break;
                    }
                }
                if (!foundBoss) {
                    std::cout << "[DEBUG] Boss Projectile could not find boss owner! ownerId=" << proj->GetOwnerId() << std::endl;
                }
            } else if (proj->GetProjectileType() == ProjectileType::RangedBomb) {
                bool flipX = (proj->GetDirection() == Direction::Left);
                proj->SetRotation(0.0f);
                View::EntityRenderer::GetInstance().RegisterAnimated(
                    proj, "assets/textures/enemies/ranged_bomb.json", "default", {0, 0}, flipX);
            }
            break;
        }
        default:
            break;
    }
}

void GameController::UnregisterEntityVisuals(int entityId) {
    const uint32_t id = static_cast<uint32_t>(entityId);
    View::CharacterRenderer::GetInstance().Unregister(id);
    View::EntityRenderer::GetInstance().Unregister(id);
}

void GameController::StartLevel(int levelNumber) {
    // A level can be replaced mid-frame (portal or Result -> Retry). Detach all
    // view-owned raw pointers before destroying the old GameState so that the
    // render pass at the end of this same frame cannot access freed entities.
    View::HUDView::GetInstance().ClearEntityReferences();
    View::SkillBarView::GetInstance().ClearEntityReferences();
    View::GameView::GetInstance().SetEntities(nullptr);
    View::GameView::GetInstance().SetTiles(MapLayer::Background, nullptr);
    View::GameView::GetInstance().SetTiles(MapLayer::Main, nullptr);
    View::GameView::GetInstance().SetTiles(MapLayer::Foreground, nullptr);
    View::ResultView::GetInstance().Dismiss();
    View::UIStateManager::GetInstance().Clear();
    View::SkillBarView::GetInstance().Open();
    View::CharacterRenderer::GetInstance().Clear();
    View::EntityRenderer::GetInstance().Clear();
    View::UIStateManager::GetInstance().Clear();
    View::TutorialRenderer::GetInstance().HideDialog();

    m_activePet.reset();
    m_petProjectiles.clear();
    m_playerProjectiles.clear();

    // The Prepare screen persists the current loadout before calling StartLevel.
    // Always prefer that selection over a stale GameState from the previous run.
    CharacterClass cls = CharacterClass::Knight;
    const std::string charId = SaveManager::GetInstance().GetSelectedChar();
    if (charId == "magic_caster") cls = CharacterClass::MagicCaster;
    else if (charId == "ninja") cls = CharacterClass::Ninja;
    else if (charId == "fighter") cls = CharacterClass::Fighter;
    else if (charId.empty() && m_gameState) cls = m_gameState->GetPlayerClass();

    const std::string path = GetLevelPath(levelNumber);
    TraceLog(LOG_INFO, "GAME: Starting level %d from %s", levelNumber, path.c_str());
    const bool isLDtk = (path.size() >= 5 &&
                         path.substr(path.size() - 5) == ".ldtk");
    int ldtkIdx = 0;
    if (isLDtk) {
        if (path.find("world.ldtk") != std::string::npos) {
            ldtkIdx = levelNumber - 1;
        } else {
            ldtkIdx = 0; // Independent map files usually only have 1 level (index 0)
        }
    }
    m_gameState = LevelFactory::LoadLevel(path, GameMode::SinglePlayer, ldtkIdx, cls);
    if (!m_gameState) {
        m_gameState = LevelFactory::CreateDefaultLevel(levelNumber);
    }
    m_gameState->SetCurrentLevel(levelNumber);
    m_gameState->ResetTimer();
    View::MinimapView::GetInstance().BeginLevel(m_gameState.get());

    m_scoring = LevelScoring();
    m_scoring.SetTotals(m_gameState->GetTotalItems(), m_gameState->GetTotalEnemies());
    m_scoring.SetParTime(LoadVictoryParTime(levelNumber));
    m_defeatedEnemies = 0;
    m_collectedItems = 0;
    m_levelComplete = false;
    m_runTookDamage = false;
    m_paused = false;
    m_returnToMenu = false;
    m_running = true;
    // Overlays never survive a level change. The cores themselves travel with
    // the player (see PlayerSaveState); only the pending-draft bookkeeping is
    // per level, so an unclaimed draft cannot pop up in the next one.
    m_coreDraftOpen = false;
    m_coreDraftBoss = false;
    m_coreOffer.clear();
    m_pendingCoreDrafts = 0;
    m_killsTowardCore = 0;
    m_buffOfferOpen = false;
    m_buffOffer.clear();
    m_codexOpen = false;
    m_hitStopTimer = 0.0f;
    m_enemyAttackCooldown = 0.0f;
    m_footstepTimer = 0.0f;
    m_knownBossPhases.clear();

    View::GameView::GetInstance().SetTiles(MapLayer::Background, &m_gameState->GetTiles(MapLayer::Background));
    View::GameView::GetInstance().SetTiles(MapLayer::Main, &m_gameState->GetTiles(MapLayer::Main));
    View::GameView::GetInstance().SetTiles(MapLayer::Foreground, &m_gameState->GetTiles(MapLayer::Foreground));
    View::GameView::GetInstance().SetEntities(&m_gameState->GetAllEntities());

    // Load background based on the theme set by LDtk level field (or default Forest)
    View::GameView::GetInstance().LoadBackgrounds(m_gameState->GetBackgroundTheme());

    if (Player* player = m_gameState->GetLocalPlayer()) {
        // Legacy/default maps can still construct a Knight internally. Apply the
        // selected class here so its skill set and renderer always agree.
        if (player->GetCharacterClass() != cls) player->SetCharacterClass(cls);
        m_gameState->SetPlayerClass(cls);
        
        RegisterPlayerVisuals(player, cls);
        m_respawnPoint = player->GetPosition();

        if (m_localCoop) {
            Vector2 secondSpawn = player->GetPosition();
            secondSpawn.x += std::max(56.0f, player->GetSize().x + 16.0f);
            auto secondPlayer = std::make_unique<Player>(secondSpawn, m_secondPlayerClass);
            secondPlayer->SetName("Player 2");
            m_gameState->SetSecondLocalPlayer(std::move(secondPlayer));
            RegisterPlayerVisuals(m_gameState->GetSecondLocalPlayer(), m_secondPlayerClass);
        }
        
        // Auto-spawn equipped pet
        std::string petId = m_localCoop ? "" : SaveManager::GetInstance().GetSelectedPet();
        if (!petId.empty()) {
            PetType equippedType = PetType::BabyDragon; // Default fallback
            if (petId == "skull") equippedType = PetType::Skull;
            else if (petId == "fairy") equippedType = PetType::Fairy;
            else if (petId == "ghost") equippedType = PetType::Ghost;
            else if (petId == "baby_dragon") equippedType = PetType::BabyDragon;

            SpawnPet(equippedType);
        }
    }

    for (const auto& entity : m_gameState->GetAllEntities()) {
        RegisterEntityVisuals(entity.get());
    }

    m_camera = Camera2D{};
    m_camera.offset = {SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f};
    m_camera.rotation = 0.0f;
    m_camera.zoom = 1.0f;
    if (Player* player = m_gameState->GetLocalPlayer()) {
        Vector2 center = {
            player->GetPosition().x + player->GetSize().x * 0.5f,
            player->GetPosition().y + player->GetSize().y * 0.5f
        };
        m_camera.target = center;
    }

    const float mapWidth = std::max(1, m_gameState->GetMapWidth()) * TILE_SIZE;
    const float mapHeight = std::max(1, m_gameState->GetMapHeight()) * TILE_SIZE;
    if (levelNumber == 1 && path.find("tutorial.ldtk") != std::string::npos) {
        m_camera.target.y = mapHeight * 0.5f;
    }
    // Padded generously: the quadtree drops anything that does not overlap its
    // root, and entities legitimately sit outside the nominal map -- knocked
    // back past an edge, or falling below the floor before they despawn.
    const float pad = TILE_SIZE * 8.0f;
    m_collision.SetWorldBounds({-pad, -pad, mapWidth + pad * 2.0f, mapHeight + pad * 2.0f});

    bool containsBoss = false;
    for (const auto& entity : m_gameState->GetAllEntities()) {
        if (entity && entity->GetType() == EntityType::Boss) {
            containsBoss = true;
            auto* boss = static_cast<Boss*>(entity.get());
            m_knownBossPhases[entity->GetId()] = static_cast<int>(boss->GetPhase());
        }
    }
    if (containsBoss) {
        SoundManager::GetInstance().PlayMusic("bgm_boss");
    } else if (levelNumber == 1 && path.find("tutorial.ldtk") != std::string::npos) {
        SoundManager::GetInstance().PlayMusic("bgm_tutorial");
    } else {
        SoundManager::GetInstance().PlayMusic("bgm_gameplay");
    }
    
    View::HUDView::GetInstance().SetVisible(true);
    View::HUDView::GetInstance().SetPlaytestMode(IsPlaytest());
    Player* hudPlayer = m_gameState->GetLocalPlayer();
    Player* hudSecondPlayer = m_localCoop ? m_gameState->GetSecondLocalPlayer() : nullptr;
    View::HUDView::GetInstance().Update(0.0f, hudPlayer, nullptr, hudSecondPlayer);
    View::SkillBarView::GetInstance().Update(0.0f, hudPlayer, hudSecondPlayer);

    // Reset endgame checkpoint animation tracking
    m_endgameFlagRevealedIds.clear();
    m_endgameFlagCapturedIds.clear();
    m_flagOutTimers.clear();
    m_activeCheckpointUid = 0;
    m_checkpointRespawnEnemyIds.clear();
    m_countedDefeatedEnemyIds.clear();
    // Before the first checkpoint, the player spawn acts as the checkpoint:
    // every regular enemy belongs to the section ahead and may respawn.
    for (const auto& entity : m_gameState->GetAllEntities()) {
        if (entity && entity->GetType() == EntityType::Enemy) {
            m_checkpointRespawnEnemyIds.insert(entity->GetId());
        }
    }
}

bool GameController::IsOnGround(const Character* character) const {
    if (!character || !m_gameState) return false;
    Rectangle box = character->GetBoundingBox();
    return IsRectOnGround(box);
}

bool GameController::IsRectOnGround(Rectangle box) const {
    if (!m_gameState) return false;
    Rectangle probe = {box.x, box.y + box.height, box.width, 2.0f};
    for (const auto& tile : m_gameState->GetTiles(MapLayer::Main)) {
        if (!tile.solid) continue;
        Rectangle tileRect = {
            (float)tile.x * TILE_SIZE,
            (float)tile.y * TILE_SIZE,
            (float)TILE_SIZE,
            (float)TILE_SIZE
        };
        if (RectOverlap(probe, tileRect)) {
            return true;
        }
    }
    for (const auto& entity : m_gameState->GetAllEntities()) {
        if (!entity || !entity->IsActive() || entity->GetType() != EntityType::FakeWall) continue;
        if (RectOverlap(probe, entity->GetBoundingBox())) {
            return true;
        }
    }
    return false;
}

void GameController::ApplyGravity(Character* character, float dt) {
    if (!character || !character->IsActive()) return;
    Vector2 vel = character->GetVelocity();
    if (!IsOnGround(character)) {
        vel.y += GRAVITY * dt;
    } else if (vel.y > 0.0f) {
        vel.y = 0.0f;
    }
    character->SetVelocity(vel);
}

void GameController::ResolveTileCollisions(Character* character, float dt) {
    if (!character || !m_gameState) return;

    Rectangle box = character->GetBoundingBox();
    Vector2 vel = character->GetVelocity();
    Rectangle prevBox = { box.x - vel.x * dt, box.y - vel.y * dt, box.width, box.height };
    Vector2 pos = character->GetPosition();

    for (const auto& tile : m_gameState->GetTiles(MapLayer::Main)) {
        if (!tile.solid) continue;
        Rectangle tileRect = {
            (float)tile.x * TILE_SIZE,
            (float)tile.y * TILE_SIZE,
            (float)TILE_SIZE,
            (float)TILE_SIZE
        };
        if (!RectOverlap(box, tileRect)) continue;

        float overlapLeft = (box.x + box.width) - tileRect.x;
        float overlapRight = (tileRect.x + tileRect.width) - box.x;
        float overlapTop = (box.y + box.height) - tileRect.y;
        float overlapBottom = (tileRect.y + tileRect.height) - box.y;

        // Determine which axis to resolve by checking previous state
        bool wasOverlappingX = (prevBox.x < tileRect.x + tileRect.width) && (prevBox.x + prevBox.width > tileRect.x);
        bool wasOverlappingY = (prevBox.y < tileRect.y + tileRect.height) && (prevBox.y + prevBox.height > tileRect.y);

        bool resolveY = false;
        bool resolveX = false;

        if (wasOverlappingX && !wasOverlappingY) {
            resolveY = true;
        } else if (wasOverlappingY && !wasOverlappingX) {
            resolveX = true;
        } else {
            // Corner case or simultaneous hit, resolve the axis with minimum overlap
            float minOverlapX = std::min(overlapLeft, overlapRight);
            float minOverlapY = std::min(overlapTop, overlapBottom);
            if (minOverlapY < 12.0f) {
                minOverlapX = 9999.0f; // favor Y for flat ground internal edges
            }
            if (minOverlapX < minOverlapY) {
                resolveX = true;
            } else {
                resolveY = true;
            }
        }

        if (resolveX) {
            if (overlapLeft < overlapRight) {
                pos.x -= overlapLeft;
            } else {
                pos.x += overlapRight;
            }
            vel = character->GetVelocity();
            vel.x = 0.0f;
            character->SetVelocity(vel);
        } else {
            if (overlapTop < overlapBottom) {
                pos.y -= overlapTop;
                vel = character->GetVelocity();
                vel.y = 0.0f;
                character->SetVelocity(vel);
            } else {
                pos.y += overlapBottom;
                vel = character->GetVelocity();
                if (vel.y < 0.0f) vel.y = 0.0f;
                character->SetVelocity(vel);
            }
        }

        character->SetPosition(pos);
        box = character->GetBoundingBox();
        prevBox = { box.x - vel.x * dt, box.y - vel.y * dt, box.width, box.height };
    }

    // Resolve FakeWall collisions
    for (const auto& entity : m_gameState->GetAllEntities()) {
        if (!entity || !entity->IsActive() || entity->GetType() != EntityType::FakeWall) continue;
        Rectangle tileRect = entity->GetBoundingBox();
        if (!RectOverlap(box, tileRect)) continue;

        float overlapLeft = (box.x + box.width) - tileRect.x;
        float overlapRight = (tileRect.x + tileRect.width) - box.x;
        float overlapTop = (box.y + box.height) - tileRect.y;
        float overlapBottom = (tileRect.y + tileRect.height) - box.y;

        bool wasOverlappingX = (prevBox.x < tileRect.x + tileRect.width) && (prevBox.x + prevBox.width > tileRect.x);
        bool wasOverlappingY = (prevBox.y < tileRect.y + tileRect.height) && (prevBox.y + prevBox.height > tileRect.y);

        bool resolveY = false;
        bool resolveX = false;

        if (wasOverlappingX && !wasOverlappingY) {
            resolveY = true;
        } else if (wasOverlappingY && !wasOverlappingX) {
            resolveX = true;
        } else {
            float minOverlapX = std::min(overlapLeft, overlapRight);
            float minOverlapY = std::min(overlapTop, overlapBottom);
            if (minOverlapY < 12.0f) minOverlapX = 9999.0f;
            if (minOverlapX < minOverlapY) resolveX = true;
            else resolveY = true;
        }

        if (resolveX) {
            if (overlapLeft < overlapRight) pos.x -= overlapLeft;
            else pos.x += overlapRight;
            vel = character->GetVelocity();
            vel.x = 0.0f;
            character->SetVelocity(vel);
        } else {
            if (overlapTop < overlapBottom) {
                pos.y -= overlapTop;
                vel = character->GetVelocity();
                vel.y = 0.0f;
                character->SetVelocity(vel);
            } else {
                pos.y += overlapBottom;
                vel = character->GetVelocity();
                if (vel.y < 0.0f) vel.y = 0.0f;
                character->SetVelocity(vel);
            }
        }
        character->SetPosition(pos);
        box = character->GetBoundingBox();
        prevBox = { box.x - vel.x * dt, box.y - vel.y * dt, box.width, box.height };
    }
}

void GameController::HandlePlayerInput(Player* player, const InputCommand& cmd, float /*dt*/) {
    if (!player || !player->IsAlive()) return;

    KnightSkillSet* skills = player->GetKnightSkills();
    bool isLunging = skills && skills->m_isLunging;
    bool blockMovement = player->IsDashing() || isLunging;

    // --- Movement (blocked while dashing or lunging) ---
    float moveX = 0.0f;
    if (!blockMovement) {
        if (cmd.moveLeft)  moveX -= 1.0f;
        if (cmd.moveRight) moveX += 1.0f;
    }

    Vector2 vel = player->GetVelocity();

    // Sprint multiplier (only when not dashing/lunging)
    float speedMult = (cmd.sprint && !blockMovement) ? Player::SPRINT_MULTIPLIER : 1.0f;
    speedMult *= player->GetSpeedMultiplier();   // Haste boon
    if (!blockMovement) {
        vel.x = moveX * PLAYER_SPEED * speedMult;
    }
    player->SetSprinting(cmd.sprint && !player->IsDashing() && moveX != 0.0f);

    if (moveX > 0.0f) player->SetDirection(Direction::Right);
    else if (moveX < 0.0f) player->SetDirection(Direction::Left);

    m_playerOnGround = IsOnGround(player);
    if (cmd.jump && m_playerOnGround && !player->IsDashing()) {
        vel.y = PLAYER_JUMP_FORCE;
        SoundManager::GetInstance().PlaySound("player_jump");
    }
    player->SetVelocity(vel);

    // --- Dash (L) --- also blocked mid-cast, or it cancels the wind-up.
    if (cmd.dash && player->CanDash() && !player->IsCasting()) {
        // Always dash in the direction the player is currently facing
        float dirX = (player->GetDirection() == Direction::Right) ? 1.0f : -1.0f;
        player->StartDash(true, dirX);
        SoundManager::GetInstance().PlaySound("player_dash");
    }

    // --- Knight Skills ---
    uint32_t pid = static_cast<uint32_t>(player->GetId());

    // ==================== KNIGHT ====================
    if (KnightSkillSet* skills = player->GetKnightSkills()) {
        // CanStartSkill() also blocks while a wind-up is in flight, so a charged
        // skill can no longer be replaced half-way and lose its hit.
        if (player->CanStartSkill()) {
            if (cmd.attack  && skills->TryAttack1()) {
                player->Attack(KnightSkillSet::ATTACK1_ANIMATION_DURATION);
                SoundManager::GetInstance().PlaySound("knight_attack_1");
            }
            else if (cmd.parry   && skills->TryAttack2()) {
                player->Attack2(KnightSkillSet::ATTACK2_ANIMATION_DURATION);
                player->BeginCast(skills->attack2.chargeMax);
                SoundManager::GetInstance().PlaySound("knight_attack_2");
            }
            else if (cmd.skill1  && skills->TryAttack3()) {
                player->Attack3(KnightSkillSet::ATTACK3_ANIMATION_DURATION);
                SoundManager::GetInstance().PlaySound("knight_attack_3");
                float lDir = (player->GetDirection() == Direction::Right) ? 1.0f : -1.0f;
                if (cmd.moveRight && !cmd.moveLeft) { lDir = 1.0f; player->SetDirection(Direction::Right); }
                else if (cmd.moveLeft && !cmd.moveRight) { lDir = -1.0f; player->SetDirection(Direction::Left); }
                Vector2 lv = player->GetVelocity(); lv.x = lDir * skills->m_lungeSpeed;
                player->SetVelocity(lv);
            }
            else if (cmd.ultimate && skills->TryUltimate()) {
                player->DoUltimate(KnightSkillSet::ULTIMATE_ANIMATION_DURATION);
                player->BeginCast(skills->ultimate.chargeMax);
                SoundManager::GetInstance().PlaySound("knight_ultimate");
            }
        }
        // Parry can be triggered regardless of other attack state
        if (!player->IsDashing() && cmd.parryBlock && skills->TryParry()) {}

    // ==================== FIGHTER ====================
    } else if (FighterSkillSet* fs = player->GetFighterSkills()) {
        if (player->CanStartSkill()) {
            if (cmd.attack    && fs->TryAttack1()) {
                player->Attack(FighterSkillSet::ATTACK1_ANIMATION_DURATION);
                SoundManager::GetInstance().PlaySound("fighter_attack_1");
            }
            else if (cmd.parry     && fs->TryAttack2()) {
                player->Attack2(FighterSkillSet::ATTACK2_ANIMATION_DURATION);
                SoundManager::GetInstance().PlaySound("fighter_attack_2");
            }
            else if (cmd.skill1    && fs->TryAttack3()) {
                player->Attack3(FighterSkillSet::ATTACK3_ANIMATION_DURATION);
                player->BeginCast(fs->attack3.chargeMax);
                SoundManager::GetInstance().PlaySound("fighter_attack_3");
            }
            else if (cmd.ultimate  && fs->TryUltimate()) {
                player->DoUltimate(FighterSkillSet::ULTIMATE_ANIMATION_DURATION);
                player->BeginCast(fs->ultimate.chargeMax);
                SoundManager::GetInstance().PlaySound("fighter_ultimate_charge");
            }
        }
        if (!player->IsDashing() && cmd.parryBlock && fs->TryParry()) {}

    // ==================== MAGIC CASTER ====================
    } else if (MagicCasterSkillSet* ms = player->GetMagicSkills()) {
        // Every magic skill lands after a charge, so each one takes the lock.
        if (player->CanStartSkill()) {
            if (cmd.attack    && ms->TryAttack1()) {
                player->Attack(MagicCasterSkillSet::ATTACK1_ANIMATION_DURATION);
                player->BeginCast(ms->attack1.chargeMax);
                SoundManager::GetInstance().PlaySound("magic_attack_1");
            }
            else if (cmd.parry     && ms->TryAttack2()) {
                player->Attack2(MagicCasterSkillSet::ATTACK2_ANIMATION_DURATION);
                player->BeginCast(ms->attack2.chargeMax);
                SoundManager::GetInstance().PlaySound("magic_attack_2");
            }
            else if (cmd.skill1    && ms->TryAttack3()) {
                player->Attack3(MagicCasterSkillSet::ATTACK3_ANIMATION_DURATION);
                player->BeginCast(ms->attack3.chargeMax);
                SoundManager::GetInstance().PlaySound("magic_attack_3");
            }
            else if (cmd.ultimate  && ms->TryUltimate()) {
                player->DoUltimate(MagicCasterSkillSet::ULTIMATE_ANIMATION_DURATION);
                player->BeginCast(ms->ultimate.chargeMax);
                SoundManager::GetInstance().PlaySound("magic_ultimate");
            }
        }
        if (!player->IsDashing() && cmd.parryBlock && ms->TryParry()) {}

    // ==================== NINJA ====================
    } else if (NinjaSkillSet* ns = player->GetNinjaSkills()) {
        if (player->CanStartSkill()) {
            if (cmd.attack    && ns->TryAttack1()) {
                player->Attack(NinjaSkillSet::ATTACK1_ANIMATION_DURATION);
                SoundManager::GetInstance().PlaySound("ninja_attack_1");
            }
            // K (Blade Rush): the projectile only leaves on the last animation
            // frame, so the whole animation is the wind-up. Holding the lock for
            // it is what stops another key from cancelling the throw.
            else if (cmd.parry     && ns->TryAttack2()) {
                player->Attack2(NinjaSkillSet::ATTACK2_ANIMATION_DURATION);
                player->BeginCast(ns->attack2.activeDuration);
                SoundManager::GetInstance().PlaySound("ninja_attack_2");
            }
            else if (cmd.skill1    && ns->TryAttack3()) {
                const float teleportTime = NinjaSkillSet::TELEPORT_START_DURATION
                                         + NinjaSkillSet::TELEPORT_END_DURATION;
                player->Attack3(teleportTime);
                player->BeginCast(teleportTime);
                SoundManager::GetInstance().PlaySound("ninja_attack_3");
            }
            else if (cmd.ultimate  && ns->TryUltimate()) {
                player->DoUltimate(NinjaSkillSet::ULTIMATE_CAST_DURATION);
                player->BeginCast(ns->ultimate.chargeMax);
                SoundManager::GetInstance().PlaySound("ninja_ultimate");
            }
        }
        if (!player->IsDashing() && cmd.parryBlock && ns->TryParry()) {}

    // ==================== FALLBACK ====================
    } else {
        if (cmd.attack && player->CanAttack()) {
            player->Attack();
            View::CharacterRenderer::GetInstance().PlayAction(pid, View::ACTION_ATTACK);
            SoundManager::GetInstance().PlaySound("player_attack");
        }
    }
}

void GameController::UpdateEnemyAI(float dt) {
    Player* player = m_gameState->GetLocalPlayer();
    if (!player) return;

    Vector2 playerCenter = {
        player->GetPosition().x + player->GetSize().x * 0.5f,
        player->GetPosition().y + player->GetSize().y * 0.5f
    };

    for (auto& entity : m_gameState->GetAllEntities()) {
        if (!entity || !entity->IsActive()) continue;
        playerCenter = {
            player->GetPosition().x + player->GetSize().x * 0.5f,
            player->GetPosition().y + player->GetSize().y * 0.5f
        };
        if (Player* second = m_gameState->GetSecondLocalPlayer(); second && second->IsAlive()) {
            const Vector2 secondCenter = {
                second->GetPosition().x + second->GetSize().x * 0.5f,
                second->GetPosition().y + second->GetSize().y * 0.5f
            };
            const Vector2 entityCenter = {
                entity->GetPosition().x + entity->GetSize().x * 0.5f,
                entity->GetPosition().y + entity->GetSize().y * 0.5f
            };
            if (Distance(secondCenter, entityCenter) < Distance(playerCenter, entityCenter)) {
                playerCenter = secondCenter;
            }
        }
        
        if (entity->GetType() == EntityType::Boss) {
            auto* boss = static_cast<Boss*>(entity.get());
            boss->UpdateAI(playerCenter, dt, m_gameState.get());
            
            // Sync visual phase
            BossPhase currentPhase = boss->GetPhase();
            const int phaseValue = static_cast<int>(currentPhase);
            auto knownPhase = m_knownBossPhases.find(boss->GetId());
            if (knownPhase == m_knownBossPhases.end()) {
                m_knownBossPhases[boss->GetId()] = phaseValue;
            } else if (knownPhase->second != phaseValue) {
                knownPhase->second = phaseValue;
                SoundManager::GetInstance().PlaySound("boss_phase");
            }
            if (View::CharacterRenderer::GetInstance().GetBossPhase(boss->GetId()) != currentPhase) {
                View::CharacterRenderer::GetInstance().SwitchPhase(boss->GetId(), currentPhase);
            }
            
            // Apply physics
            ApplyGravity(boss, dt);
            ResolveTileCollisions(boss, dt);
            boss->SetOnGround(IsOnGround(boss));
            
            continue;
        }

        if (entity->GetType() != EntityType::Enemy) continue;
        
        auto* enemy = static_cast<Enemy*>(entity.get());
        bool targetVisible = true;
        if (enemy->GetEnemyType() == EnemyType::Flying) {
            const Rectangle enemyBounds = enemy->GetBoundingBox();
            const Vector2 enemyCenter{
                enemyBounds.x + enemyBounds.width * 0.5f,
                enemyBounds.y + enemyBounds.height * 0.5f
            };
            float nearestVisibleDistance = std::numeric_limits<float>::max();
            targetVisible = false;

            auto considerVisibleTarget = [&](const Player* candidate) {
                if (!candidate || !candidate->IsActive() || !candidate->IsAlive()) return;
                const Rectangle bounds = candidate->GetBoundingBox();
                const Vector2 center{
                    bounds.x + bounds.width * 0.5f,
                    bounds.y + bounds.height * 0.5f
                };
                if (!CheckLineOfSight(enemyCenter, center)) return;
                const float distance = Distance(enemyCenter, center);
                if (distance < nearestVisibleDistance) {
                    nearestVisibleDistance = distance;
                    playerCenter = center;
                    targetVisible = true;
                }
            };

            considerVisibleTarget(player);
            considerVisibleTarget(m_gameState->GetSecondLocalPlayer());
        }
        enemy->UpdateAI(playerCenter, dt, targetVisible);

        // Ground enemies probe one step ahead and below their feet. The old
        // code calculated groundAhead but never acted on it, so enemies still
        // chased players straight into pits.
        if (enemy->GetEnemyType() != EnemyType::Flying && IsOnGround(enemy)) {
            const Vector2 velocity = enemy->GetVelocity();
            if (std::abs(velocity.x) > 0.1f) {
                const float directionX = velocity.x > 0.0f ? 1.0f : -1.0f;
                const Rectangle bounds = enemy->GetBoundingBox();
                const float lookAhead = std::max(8.0f, std::abs(velocity.x) * dt + 4.0f);
                const float probeX = directionX > 0.0f
                    ? bounds.x + bounds.width + lookAhead
                    : bounds.x - lookAhead;
                const Rectangle supportProbe{
                    probeX - 2.0f,
                    bounds.y + bounds.height + 1.0f,
                    4.0f,
                    10.0f
                };

                bool groundAhead = false;
                for (const auto& tile : m_gameState->GetTiles(MapLayer::Main)) {
                    if (!tile.solid) continue;
                    const Rectangle tileRect{
                        tile.x * (float)TILE_SIZE,
                        tile.y * (float)TILE_SIZE,
                        (float)TILE_SIZE,
                        (float)TILE_SIZE
                    };
                    if (RectOverlap(supportProbe, tileRect)) {
                        groundAhead = true;
                        break;
                    }
                }

                // FakeWall participates in character collision and can also
                // be a valid platform, so it must count as support here.
                if (!groundAhead) {
                    for (const auto& support : m_gameState->GetAllEntities()) {
                        if (!support || !support->IsActive() || support->GetType() != EntityType::FakeWall)
                            continue;
                        if (RectOverlap(supportProbe, support->GetBoundingBox())) {
                            groundAhead = true;
                            break;
                        }
                    }
                }

                const float mapWidth = m_gameState->GetMapWidth() * (float)TILE_SIZE;
                const bool outsideMap = probeX < 0.0f || probeX >= mapWidth;
                if (!groundAhead || outsideMap) {
                    enemy->TurnAwayFromEdge(directionX);
                }
            }
        }

        if (enemy->GetEnemyType() != EnemyType::Flying) {
            ApplyGravity(enemy, dt);
        }
        ResolveTileCollisions(enemy, dt);
    }
}

void GameController::UpdateCombat(Player* player, float dt, bool updateEnemyCooldown) {
    if (!player) return;

    // ---- Helper: deal damage to enemies in a rect ----
    auto HitEnemiesInBox = [&](Rectangle attackBox, int baseDamage) {
        // Power and Bloodthirst boons apply to every skill uniformly, so they
        // are handled once here rather than at each skill's call site.
        baseDamage = static_cast<int>(baseDamage * player->GetDamageMultiplier());
        // Broad phase first: only entities whose box overlaps the swing are
        // considered, instead of every entity in the level.
        for (Entity* entity : QueryEntitiesInRect(attackBox)) {

            // Element the player's swings currently carry. Physical with no
            // infusion running, which reacts with nothing and behaves exactly
            // as this did before infusions existed.
            const DamageType swingElement = player->GetAttackElement();

            // Every swing goes through the elemental funnel, physical included.
            // A physical hit reacts with nothing and leaves no aura, so it
            // behaves as it always did -- but it still picks up the Void
            // vulnerability and the on-hit cores, which live in there.
            const int damage = baseDamage;

            if (entity->GetType() == EntityType::Enemy) {
                auto* enemy = static_cast<Enemy*>(entity);
                if (!RectOverlap(attackBox, enemy->GetBoundingBox())) continue;
                if (enemy->GetState() == EnemyState::Hurt || enemy->GetState() == EnemyState::Dead) continue;

                const int dealt = ApplyElementalHit(enemy, damage, swingElement);
                SoundManager::GetInstance().PlaySound("enemy_hurt");
                View::ParticleRenderer::GetInstance().EmitBurst(enemy->GetPosition(), 8, WHITE);
                View::GameView::GetInstance().Shake(3.0f, 0.15f);

                player->OnDamageDealt(dealt);


                if (!enemy->IsActive()) OnEntityRemoved(enemy);
            }
            else if (entity->GetType() == EntityType::Boss) {
                auto* boss = static_cast<Boss*>(entity);
                if (!RectOverlap(attackBox, boss->GetBoundingBox())) continue;
                if (!boss->IsAlive() || boss->GetBossState() == BossState::Hurt || boss->GetBossState() == BossState::Die || boss->GetBossState() == BossState::Transition) continue;

                const int dealt = ApplyElementalHit(boss, damage, swingElement);
                SoundManager::GetInstance().PlaySound("boss_hurt");
                View::ParticleRenderer::GetInstance().EmitBurst(boss->GetPosition(), 16, WHITE);
                View::GameView::GetInstance().Shake(4.0f, 0.2f);

                player->OnDamageDealt(dealt);


                if (!boss->IsActive()) OnEntityRemoved(boss);
            }
            else if (entity->GetType() == EntityType::FakeWall) {
                auto* wall = static_cast<FakeWall*>(entity);
                if (!RectOverlap(attackBox, wall->GetBoundingBox())) continue;
                if (wall->IsDestroyed()) continue;

                wall->TakeDamage(damage);
                SoundManager::GetInstance().PlaySound("stone_hit");
                View::ParticleRenderer::GetInstance().EmitBurst(wall->GetPosition(), 6, GRAY);
                View::GameView::GetInstance().Shake(2.0f, 0.1f);

                if (wall->IsDestroyed()) {
                    SoundManager::GetInstance().PlaySound("fake_wall_break");
                    m_gameState->RemoveTileAt(MapLayer::Main, wall->GetTileX(), wall->GetTileY());
                    OnEntityRemoved(wall);
                }
            }
        }
    };

    // ==================== KNIGHT COMBAT ====================
    if (KnightSkillSet* skills = player->GetKnightSkills()) {
        if (skills->IsAttack1Active())
            HitEnemiesInBox(player->GetAttackBoundingBox(), skills->attack1.damage);
        if (skills->IsAttack2Active())
            HitEnemiesInBox(skills->GetAttack2HitBox(player->GetPosition(), player->GetSize(), player->GetDirection()),
                            skills->attack2.damage);
        if (skills->IsAttack3Active())
            HitEnemiesInBox(player->GetBoundingBox(), skills->attack3.damage);
        if (skills->IsUltimateActive())
            HitEnemiesInBox(skills->GetUltimateHitBox(player->GetPosition(), player->GetSize(), player->GetDirection()),
                            skills->ultimate.damage);

    // ==================== FIGHTER COMBAT ====================
    } else if (FighterSkillSet* fs = player->GetFighterSkills()) {
        if (fs->IsAttack1Active())
            HitEnemiesInBox(fs->GetAttack1HitBox(player->GetPosition(), player->GetSize(), player->GetDirection()),
                            fs->attack1.damage);
        if (fs->IsAttack2Active())
            HitEnemiesInBox(fs->GetAttack2HitBox(player->GetPosition(), player->GetSize(), player->GetDirection()),
                            fs->attack2.damage);
        if (fs->IsAttack3Active())
            HitEnemiesInBox(fs->GetAttack3HitBox(player->GetPosition(), player->GetSize(), player->GetDirection()),
                            fs->attack3.damage);
        // Fighter Ultimate: spawn projectile when charge done
        if (fs->WantsToFire()) {
            const std::string projectileAtlas =
                "assets/textures/player/fighter_v2/ultimate_projectile_v2.json";
            SpawnPlayerProjectile(
                projectileAtlas.c_str(),
                PlayerSkillOrigin(player), player->GetDirection(),
                fs->ultimate.damage, 400.0f, 0.9f, // V2: 12 frames * 0.075s; legacy: 9 * 0.1s
                0.8f, false);  // Fighter H faces right by default
            fs->ResetFireFlag();
            SoundManager::GetInstance().PlaySound("fighter_ultimate_release");
        }

    // ==================== MAGIC CASTER COMBAT ====================
    } else if (MagicCasterSkillSet* ms = player->GetMagicSkills()) {
        const Vector2 playerCenter = {
            player->GetPosition().x + player->GetSize().x * 0.5f,
            player->GetPosition().y + player->GetSize().y * 0.5f
        };

        // J: land directly on the nearest target visible in the camera with clear LOS.
        if (ms->m_wantsLightning) {
            Vector2 targetPos{};
            if (!FindNearestVisibleEnemyInCamera(playerCenter, targetPos)) {
                const float dirX = (player->GetDirection() == Direction::Left) ? -1.0f : 1.0f;
                targetPos = {playerCenter.x + dirX * 250.0f, playerCenter.y};
                if (!CheckLineOfSight(playerCenter, targetPos)) {
                    targetPos = playerCenter;
                    for (float distance = 242.0f; distance >= 48.0f; distance -= 8.0f) {
                        Vector2 candidate = {playerCenter.x + dirX * distance, playerCenter.y};
                        if (CheckLineOfSight(playerCenter, candidate)) {
                            // Leave room for the 60px strike hitbox so it cannot reach through the wall.
                            targetPos = {candidate.x - dirX * 40.0f, candidate.y};
                            break;
                        }
                    }
                }
            }
            SoundManager::GetInstance().PlaySound("magic_attack_1_release");
            SpawnLightningAt(targetPos, ms->attack1.damage, 0.50f,
                             "assets/textures/player/magic_caster_v2/projectile_attack1_v2.json",
                             0.0f, 0.8f, MagicCasterSkillSet::ATTACK1_ELEMENT);
            ms->ResetLightning();
        }
        // Fireball (attack2): fly forward
        if (ms->m_wantsFireball) {
            SoundManager::GetInstance().PlaySound("magic_attack_2_release");
            SpawnPlayerProjectile(
                "assets/textures/player/magic_caster_v2/projectile_attack2_v2.json",
                PlayerSkillOrigin(player), player->GetDirection(),
                ms->attack2.damage,
                MagicCasterSkillSet::FIREBALL_SPEED,
                MagicCasterSkillSet::FIREBALL_RANGE / MagicCasterSkillSet::FIREBALL_SPEED,
                0.8f, false, // Fireball faces Right by default, increased scale to 0.8
                {0.0f, 0.0f}, MagicCasterSkillSet::ATTACK2_ELEMENT);
            ms->ResetFireball();
        }
        // Wave (attack3): fly forward slower
        if (ms->m_wantsWave) {
            SoundManager::GetInstance().PlaySound("magic_attack_3_release");
            SpawnPlayerProjectile(
                "assets/textures/player/magic_caster_v2/projectile_attack3_v2.json",
                PlayerSkillOrigin(player), player->GetDirection(),
                ms->attack3.damage,
                MagicCasterSkillSet::WAVE_SPEED,
                1.0f, // 1.0f matches exact animation length (5 frames * 0.2s)
                1.0f, true,
                {180.0f, 100.0f}, // close to the 197x130px visual, minus transparent edges
                MagicCasterSkillSet::ATTACK3_ELEMENT);
            ms->ResetWave();
        }
        // H: use the same nearest visible target and wall-safe line of sight as J.
        if (ms->m_wantsUltLightning) {
            Vector2 targetPos{};
            if (!FindNearestVisibleEnemyInCamera(playerCenter, targetPos)) {
                const float dirX = (player->GetDirection() == Direction::Left) ? -1.0f : 1.0f;
                targetPos = {playerCenter.x + dirX * 300.0f, playerCenter.y};
                if (!CheckLineOfSight(playerCenter, targetPos)) {
                    targetPos = playerCenter;
                    for (float distance = 292.0f; distance >= 48.0f; distance -= 8.0f) {
                        Vector2 candidate = {playerCenter.x + dirX * distance, playerCenter.y};
                        if (CheckLineOfSight(playerCenter, candidate)) {
                            targetPos = {candidate.x - dirX * 40.0f, candidate.y};
                            break;
                        }
                    }
                }
            }
            SoundManager::GetInstance().PlaySound("magic_ultimate_release");
            SpawnLightningAt(targetPos, ms->ultimate.damage, 0.50f,
                             "assets/textures/player/magic_caster_v2/ultimate_skill_projectile_v2.json",
                             0.0f, 1.0f, MagicCasterSkillSet::ULTIMATE_ELEMENT);
            ms->ResetUltLightning();
        }

    // ==================== NINJA COMBAT ====================
    } else if (NinjaSkillSet* ns = player->GetNinjaSkills()) {
        if (ns->IsAttack1Active())
            HitEnemiesInBox(ns->GetAttack1HitBox(player->GetPosition(), player->GetSize(), player->GetDirection()),
                            ns->attack1.damage);
        // Blade Rush: projectile spawns when animation ends
        if (ns->WantsBladeRush()) {
            SoundManager::GetInstance().PlaySound("ninja_attack_2_release");
            SpawnPlayerProjectile(
                "assets/textures/player/ninja_v2/projectile_attack2_v2.json",
                PlayerSkillOrigin(player), player->GetDirection(),
                ns->attack2.damage,
                NinjaSkillSet::BLADE_RUSH_SPEED,
                NinjaSkillSet::BLADE_RUSH_RANGE / NinjaSkillSet::BLADE_RUSH_SPEED,
                0.3f, false); // Ninja K sprite faces right by default

            // Twin Blades core: a second, slower blade trails the first so the
            // pair sweeps a lane rather than landing as one bigger hit.
            if (player->GetCores().BladeRushIsTwin()) {
                Vector2 trailing = PlayerSkillOrigin(player);
                trailing.y += TWIN_BLADE_OFFSET_Y;
                SpawnPlayerProjectile(
                    "assets/textures/player/ninja_v2/projectile_attack2_v2.json",
                    trailing, player->GetDirection(),
                    ns->attack2.damage,
                    NinjaSkillSet::BLADE_RUSH_SPEED * TWIN_BLADE_SPEED_SCALE,
                    NinjaSkillSet::BLADE_RUSH_RANGE / NinjaSkillSet::BLADE_RUSH_SPEED,
                    0.3f, false);
            }
            ns->ResetBladeRush();
        }
        // Teleport
        UpdateNinjaTeleport(player, dt);
        // Shadow Clone projectile
        if (ns->WantsShadowClone()) {
            SoundManager::GetInstance().PlaySound("ninja_ultimate_release");
            SpawnPlayerProjectile(
                "assets/textures/player/ninja_v2/projectile_ultimate_attack_v2.json",
                PlayerSkillOrigin(player), player->GetDirection(),
                ns->ultimate.damage,
                NinjaSkillSet::CLONE_SPEED,
                NinjaSkillSet::CLONE_ANIMATION_DURATION,
                0.5f, false); // Ninja H sprite is 0.5 scale and faces right by default
            ns->ResetShadowClone();
        }

    // ==================== FALLBACK ====================
    } else if (player->GetState() == Character::State::Attack) {
        HitEnemiesInBox(player->GetAttackBoundingBox(), 25);
    }

    if (updateEnemyCooldown) m_enemyAttackCooldown -= dt;
    if (m_enemyAttackCooldown > 0.0f) return;

    Vector2 playerCenter = {
        player->GetPosition().x + player->GetSize().x * 0.5f,
        player->GetPosition().y + player->GetSize().y * 0.5f
    };

    for (auto& entity : m_gameState->GetAllEntities()) {
        if (!entity || !entity->IsActive()) continue;
        
        bool isAttacking = false;
        int attackDamage = 0;
        float attackRange = 0.0f;
        Vector2 attackCenter;
        Rectangle attackBox;
        // Bosses are far taller than their attack range, so a center-to-center
        // radius cannot describe their reach; they use a hitbox instead.
        bool useAttackBox = false;
        Boss* attackingBoss = nullptr;

        if (entity->GetType() == EntityType::Enemy) {
            auto* enemy = static_cast<Enemy*>(entity.get());
            if (enemy->GetState() == EnemyState::Attack) {
                if (enemy->CanAttack()) {
                    attackDamage = enemy->GetDamage();
                    attackRange = enemy->GetAttackRange();
                    attackCenter = {
                        enemy->GetPosition().x + enemy->GetSize().x * 0.5f,
                        enemy->GetPosition().y + enemy->GetSize().y * 0.5f
                    };

                    // Flying enemies use direct contact damage rather than a
                    // projectile. Validate both range and LOS before consuming
                    // their cooldown so P2 can still be hit when P1 is hidden.
                    if (enemy->GetEnemyType() == EnemyType::Flying &&
                        (Distance(playerCenter, attackCenter) > attackRange ||
                         !CheckLineOfSight(attackCenter, playerCenter))) {
                        continue;
                    }

                    isAttacking = true;
                    enemy->Attack();
                    const char* enemyAttackEvent = "enemy_melee_attack";
                    if (enemy->GetEnemyType() == EnemyType::Ranged) {
                        enemyAttackEvent = "enemy_ranged_attack";
                    } else if (enemy->GetEnemyType() == EnemyType::Flying) {
                        enemyAttackEvent = "enemy_flying_attack";
                    }
                    SoundManager::GetInstance().PlaySound(enemyAttackEvent);
                    
                    if (enemy->GetEnemyType() == EnemyType::Ranged) {
                        Vector2 pSize = {32.0f, 32.0f}; // size of bomb
                        Vector2 spawnPos = {
                            enemy->GetPosition().x + (enemy->GetDirection() == Direction::Right ? enemy->GetSize().x : -pSize.x),
                            enemy->GetPosition().y + (enemy->GetSize().y - pSize.y) * 0.5f - 15.0f
                        };
                        auto proj = std::make_unique<Projectile>(
                            spawnPos, pSize, ProjectileType::RangedBomb, enemy->GetDirection(), attackDamage, enemy->GetId());
                        // Bỏ gia tốc Y (-100.0f) vì Projectile không có trọng lực, bay thẳng ngang để trúng Player.
                        proj->SetVelocity({(enemy->GetDirection() == Direction::Right ? 250.0f : -250.0f), 0.0f});
                        m_gameState->AddEntity(std::move(proj));
                        isAttacking = false; // Ranged enemies spawn projectile, no melee hit
                    }
                }
            }
        } else if (entity->GetType() == EntityType::Boss) {
            auto* boss = static_cast<Boss*>(entity.get());
            // Only melee bosses ever open a swing (ranged ones damage through
            // projectiles), so the swing flag alone is the right gate. The old
            // `GetAttackRange() <= 150` test silently excluded Boss1 (210) and
            // Boss3 (245), which is why neither could ever land a body hit.
            if (boss->IsMeleeActive() && !boss->HasMeleeHit(player->GetId())) {
                isAttacking = true;
                useAttackBox = true;
                attackingBoss = boss;
                attackDamage = boss->GetDamage();
                attackBox = boss->GetMeleeHitBox();
            }
        } else if (entity->GetType() == EntityType::Projectile) {
            auto* proj = static_cast<Projectile*>(entity.get());
            if (!proj->IsActive()) continue;

            // Resolve tile collision (stop on wall)
            Rectangle box = proj->GetBoundingBox();
            bool hitTile = false;
            for (const auto& tile : m_gameState->GetTiles(MapLayer::Main)) {
                if (!tile.solid) continue;
                Rectangle tr = { (float)tile.x * TILE_SIZE, (float)tile.y * TILE_SIZE, (float)TILE_SIZE, (float)TILE_SIZE };
                if (RectOverlap(box, tr)) {
                    hitTile = true;
                    break;
                }
            }
            if (hitTile) {
                // Despawn moving projectiles on hit; keep stationary AoE/beams active
                if (proj->GetDirection() != Direction::None && proj->GetSubType() != 3) { // Exempt stationary beams
                    proj->OnHit();
                    continue; // Skip player collision if destroyed by wall
                }
            }

            if (proj->GetDamage() > 0 && !proj->HasHitEntity(player->GetId())
                && RectOverlap(proj->GetBoundingBox(), player->GetBoundingBox())) {
                if (!player->IsInvincible()) {
                    const int healthBeforeHit = player->GetHealth();
                    player->TakeDamage(proj->GetDamage());
                    if (player->GetHealth() < healthBeforeHit) m_runTookDamage = true;
                    SoundManager::GetInstance().PlaySound(player->IsAlive() ? "player_hurt" : "player_die");
                    View::FloatingTextManager::GetInstance().Emit(
                        player->GetPosition(), "-" + std::to_string(proj->GetDamage()), RED, 1.0f);
                    View::GameView::GetInstance().Shake(4.0f, 0.2f);
                    proj->MarkHitEntity(player->GetId());
                    proj->SetHasHit(true);
                }
                // Despawn moving projectiles on hit; keep stationary AoE/beams active so visuals complete
                if (proj->GetDirection() != Direction::None && proj->GetSubType() != 3) {
                    proj->OnHit();
                }
                if (!player->IsAlive() && !TryRevive(player)) {
                    RespawnPlayer(player, !m_localCoop);
                }
            }
            continue; // Skip the regular attack check for Projectiles
        }
        
        if (!isAttacking) continue;
        if (useAttackBox) {
            if (!RectOverlap(attackBox, player->GetBoundingBox())) continue;
        } else if (Distance(playerCenter, attackCenter) > attackRange) {
            continue;
        }

        if (attackingBoss) {
            // Consume the swing per target, so it cannot hit the same player
            // twice while the window is open but can still reach both in co-op.
            attackingBoss->MarkMeleeHit(player->GetId());
            SoundManager::GetInstance().PlaySound("boss_attack");
        }

        // Invincibility from dash blocks all damage
        if (!player->IsInvincible()) {
            const int healthBeforeHit = player->GetHealth();
            player->TakeDamage(attackDamage);
            if (player->GetHealth() < healthBeforeHit) m_runTookDamage = true;
            SoundManager::GetInstance().PlaySound(player->IsAlive() ? "player_hurt" : "player_die");
            View::FloatingTextManager::GetInstance().Emit(
                player->GetPosition(), "-" + std::to_string(attackDamage), RED, 1.0f);
            View::GameView::GetInstance().Shake(4.0f, 0.2f);
        }
        m_enemyAttackCooldown = 0.4f;

        if (!player->IsAlive() && !TryRevive(player)) {
            RespawnPlayer(player, !m_localCoop);
        }
        break;
    }
}

// Second Wind core: the one death per run that does not send the player back to
// a checkpoint. Deliberately not wired into Player::TakeDamage -- falling out of
// the world also routes through that, and surviving a pit without being moved
// back onto solid ground would leave the player falling forever.
bool GameController::TryRevive(Player* player) {
    if (!player) return false;
    CoreLoadout& cores = player->GetCores();
    if (!cores.CanRevive()) return false;

    cores.ConsumeRevive();
    player->SetHealth(std::max(1, static_cast<int>(player->GetMaxHealth()
                                                  * cores.ReviveHealthFraction())));
    // Character::TakeDamage clears the active flag when health reaches zero, so
    // restoring health alone is not enough: an inactive player is skipped by
    // CharacterRenderer and by Character::Update, which left the reviving
    // player invisible and frozen while input still fired skills.  RespawnPlayer
    // re-activates for the same reason.
    player->SetActive(true);
    SoundManager::GetInstance().PlaySound("boss_phase");
    View::FloatingTextManager::GetInstance().Emit(
        player->GetPosition(), GetCoreDef(CoreId::SecondWind).name,
        RarityColor(CoreRarity::Legendary), 2.0f);
    View::ParticleRenderer::GetInstance().EmitBurst(
        player->GetPosition(), 36, RarityColor(CoreRarity::Legendary));
    View::GameView::GetInstance().Shake(9.0f, 0.35f);
    RequestHitStop(HITSTOP_MAX);
    return true;
}

void GameController::UpdateItems(Player* player, float dt) {
    (void)dt;
    if (!player) return;

    Rectangle playerBox = player->GetBoundingBox();
    std::vector<int> collectedIds;
    bool persistentCoinsChanged = false;

    for (const auto& entity : m_gameState->GetAllEntities()) {
        if (!entity || entity->GetType() != EntityType::Item || !entity->IsActive()) continue;
        auto* item = static_cast<Item*>(entity.get());
        if (!RectOverlap(playerBox, ItemPickupBox(*item))) continue;

        const char* pickupSound = "item_pickup";
        switch (item->GetItemType()) {
            case ItemType::Coin:
                pickupSound = "coin_pickup";
                player->GetInventory().AddCoins(item->GetAmount());
                if (m_localCoop && m_gameState->GetLocalPlayer() != player &&
                    m_gameState->GetLocalPlayer()) {
                    m_gameState->GetLocalPlayer()->GetInventory().AddCoins(item->GetAmount());
                }
                SaveManager::GetInstance().AddCoins(item->GetAmount());
                AchievementManager::GetInstance().OnCoinCollected(item->GetAmount());
                persistentCoinsChanged = true;
                player->AddScore(item->GetAmount() * 10);
                m_scoring.AddScore(item->GetAmount() * 10);
                break;
            case ItemType::Apple:
                pickupSound = "apple_pickup";
                player->Heal(25);
                break;
            case ItemType::Key:
                pickupSound = "key_pickup";
                player->GetInventory().AddKeys(1);
                break;
            case ItemType::Potion:
                pickupSound = "potion_pickup";
                player->Heal(50);
                AchievementManager::GetInstance().OnPotionUsed();
                break;
            default:
                player->GetInventory().AddItem(
                    std::make_unique<Item>(item->GetPosition(), item->GetItemType(), item->GetAmount()));
                break;
        }

        m_collectedItems++;
        m_scoring.CollectItem();
        SoundManager::GetInstance().PlaySound(pickupSound);
        View::FloatingTextManager::GetInstance().Emit(
            item->GetPosition(), item->GetItemName(), YELLOW, 1.0f);
        collectedIds.push_back(item->GetId());
    }

    for (int id : collectedIds) {
        UnregisterEntityVisuals(id);
        m_gameState->RemoveEntity(id);
    }
    if (persistentCoinsChanged) SaveManager::GetInstance().Save();
}

void GameController::UpdateInteractions(Player* player, const InputCommand& cmd) {
    if (!player || !cmd.interact) return;

    Rectangle playerBox = player->GetBoundingBox();

    for (auto& entity : m_gameState->GetAllEntities()) {
        if (!entity || !entity->IsActive()) continue;

        // Use a small two-dimensional interaction margin.  Checkpoint artwork
        // is larger than its gameplay hitbox, so requiring exact vertical
        // overlap makes the lower/upper visible portions unresponsive.
        Rectangle expanded = playerBox;
        expanded.x -= TILE_SIZE * 0.5f;
        expanded.y -= TILE_SIZE * 0.5f;
        expanded.width += TILE_SIZE;
        expanded.height += TILE_SIZE;
        Rectangle interactionTarget = entity->GetBoundingBox();
        if (entity->GetType() == EntityType::Checkpoint) {
            interactionTarget = CheckpointInteractionBox(
                *static_cast<const Checkpoint*>(entity.get()));
        }
        if (!RectOverlap(expanded, interactionTarget)) continue;

        if (entity->GetType() == EntityType::Signboard) {
            auto* signboard = static_cast<Signboard*>(entity.get());
            if (signboard->CanInteract(player)) {
                SoundManager::GetInstance().PlaySound("signboard_open");
                View::TutorialRenderer::GetInstance().ShowDialog(signboard->GetMessage());
                return;
            }
        }

        if (entity->GetType() == EntityType::LevelCompleteCup) {
            auto* cup = static_cast<LevelCompleteCup*>(entity.get());
            if (cup->CanInteract(player)) {
                cup->Activate();
                SoundManager::GetInstance().PlaySound("trophy_activate");
                View::ParticleRenderer::GetInstance().EmitBurst(cup->GetPosition(), 36, GOLD);
                View::GameView::GetInstance().Shake(7.0f, 0.45f);
                return;
            }
        }

        if (entity->GetType() == EntityType::Chest) {
            auto* chest = static_cast<Chest*>(entity.get());
            if (chest->IsOpened()) continue;
            auto loot = chest->Open();
            SoundManager::GetInstance().PlaySound("chest_open");

            // Switch chest visual to open state
            uint32_t chestUid = static_cast<uint32_t>(chest->GetId());
            View::EntityRenderer::GetInstance().Unregister(chestUid);
            View::EntityRenderer::GetInstance().RegisterAnimated(
                chest, "assets/textures/objects/chest_open.json", "default");

            // Scatter coins in an upward arc
            int count = static_cast<int>(loot.size());
            float chestCenterX = chest->GetPosition().x + chest->GetSize().x * 0.5f;
            float chestTopY    = chest->GetPosition().y;
            for (int i = 0; i < count; ++i) {
                // Spread angles: fan from -120deg to -60deg (upward cone)
                float t     = (count > 1) ? (float)i / (float)(count - 1) : 0.5f;
                float angle = (-120.0f + t * 60.0f) * 3.14159f / 180.0f; // radians
                float speed = 280.0f + static_cast<float>(std::rand() % 80);
                Vector2 vel = { std::cos(angle) * speed, std::sin(angle) * speed };

                auto& item = loot[i];
                item->SetPosition({ chestCenterX - item->GetSize().x * 0.5f, chestTopY });
                item->SetVelocity(vel);
                item->SetPhysicsEnabled(true);
                RegisterItemVisuals(item.get());
                m_gameState->AddEntity(std::move(item));
            }
            return;
        }

        if (entity->GetType() == EntityType::Checkpoint) {
            auto* checkpoint = static_cast<Checkpoint*>(entity.get());
            if (checkpoint->IsActivated()) continue;

            uint32_t newUid = static_cast<uint32_t>(checkpoint->GetId());
            checkpoint->Activate();
            SoundManager::GetInstance().PlaySound("checkpoint_activate");

            if (checkpoint->IsEndGame()) {
                // Reaching the finish and pressing F is sufficient: no
                // enemy-count or passive collision gate is involved.  The
                // activated state drives UpdateEndgameCheckpoints() while this
                // flag is observed by IsLevelComplete() in the same update.
                m_gameState->SetLevelCompleteByPlayer(true);
                View::ParticleRenderer::GetInstance().EmitBurst(
                    checkpoint->GetPosition(), 36, GOLD);
                View::GameView::GetInstance().Shake(7.0f, 0.45f);
                return;
            }

            // A map may loop back to the left or move vertically.  The most
            // recently interacted checkpoint is the respawn point regardless
            // of its X coordinate.
            m_activeCheckpointUid = newUid;
            m_respawnPoint = checkpoint->GetPosition();
            CaptureCheckpointEnemies(checkpoint->GetPosition().x);

            // Switch visual: uncaptured -> flag_out animation
            View::EntityRenderer::GetInstance().Unregister(newUid);
            View::EntityRenderer::GetInstance().RegisterAnimated(
                checkpoint, "assets/textures/objects/checkpoint_flag_out.json", "flag_out");
            return;
        }

        if (entity->GetType() == EntityType::TeleportPortal) {
            auto* portal = static_cast<TeleportPortal*>(entity.get());
            SoundManager::GetInstance().PlaySound("portal_use");
            if (portal->GetPortalType() == PortalType::Local) {
                if (portal->GetLinkedPortal()) {
                    Rectangle destBox = portal->GetLinkedPortal()->GetBoundingBox();
                    Vector2 dest = { destBox.x + destBox.width / 2.0f - player->GetSize().x / 2.0f, destBox.y + destBox.height - player->GetSize().y };
                    player->SetPosition(dest);
                    player->SetVelocity({0, 0});
                    if (m_localCoop) {
                        Player* partner = player == m_gameState->GetLocalPlayer()
                            ? m_gameState->GetSecondLocalPlayer()
                            : m_gameState->GetLocalPlayer();
                        if (partner) {
                            Vector2 partnerDest = dest;
                            partnerDest.x += player == m_gameState->GetLocalPlayer() ? 54.0f : -54.0f;
                            partner->SetPosition(partnerDest);
                            partner->SetVelocity({0.0f, 0.0f});
                        }
                    }
                    return; // Prevent instant back-teleportation in the same frame!
                }
            } else if (portal->GetPortalType() == PortalType::LevelTransition) {
                if (portal->GetTargetLevelId() != -1) {
                    StartLevel(portal->GetTargetLevelId());
                    return; // Stop iterating, m_gameState was replaced
                }
            } else if (portal->GetPortalType() == PortalType::BossArena) {
                int target = portal->GetTargetLevelId();
                if (target == -1) {
                    // Cổng thoát (boss arena → level cũ)
                    if (m_previousLevelId != -1) {
                        // Lưu trạng thái player sau khi đánh boss (có thể được thưởng items)
                        SavePlayerState(m_gameState->GetLocalPlayer());
                        int prev = m_previousLevelId;
                        m_previousLevelId = -1;
                        StartLevel(prev);
                        // Override spawn position và restore state sau khi level mới load
                        if (Player* newPlayer = m_gameState ? m_gameState->GetLocalPlayer() : nullptr) {
                            newPlayer->SetPosition(m_exitSpawnPos);
                            m_respawnPoint = m_exitSpawnPos;
                            RestorePlayerState(newPlayer);
                            m_hasSavedState = false;
                        }
                        return; // Stop iterating
                    }
                } else {
                    // Cổng vào boss arena
                    // Lưu vị trí exit spawn = vị trí cạnh cổng trên level hiện tại
                    Rectangle portalBox = portal->GetBoundingBox();
                    m_exitSpawnPos = { portalBox.x + portalBox.width + 8.0f, portalBox.y + portalBox.height - player->GetSize().y };
                    m_previousLevelId = m_gameState->GetCurrentLevel();
                    SavePlayerState(m_gameState->GetLocalPlayer());
                    StartLevel(target);
                    // Restore state vào level boss
                    if (Player* newPlayer = m_gameState ? m_gameState->GetLocalPlayer() : nullptr) {
                        RestorePlayerState(newPlayer);
                    }
                    return; // Stop iterating
                }
            }
            return;
        }
    }
}

void GameController::OnEntityRemoved(Entity* entity) {
    if (!entity) return;
    if (entity->GetType() == EntityType::Enemy || entity->GetType() == EntityType::Boss) {
        // A checkpoint can revive enemies in the section ahead. Count each
        // authored enemy only once so repeated deaths cannot farm score/stars.
        if (m_countedDefeatedEnemyIds.insert(entity->GetId()).second) {
            m_defeatedEnemies++;
            m_scoring.DefeatEnemy();
            m_scoring.AddScore(50);
            SoundManager::GetInstance().PlaySound(
                entity->GetType() == EntityType::Boss ? "boss_death" : "enemy_death");
            View::ParticleRenderer::GetInstance().EmitBurst(entity->GetPosition(), 20, RED);
            View::GameView::GetInstance().Shake(5.0f, 0.3f);
            AchievementManager::GetInstance().OnEnemyDefeated(entity->GetType() == EntityType::Boss);
            OnEnemyDefeatedForCores(entity->GetType() == EntityType::Boss);
        }
    }
    ClearElementalState(entity);
    UnregisterEntityVisuals(entity->GetId());
}

void GameController::CaptureCheckpointEnemies(float checkpointX) {
    m_checkpointRespawnEnemyIds.clear();
    if (!m_gameState) return;
    for (const auto& entity : m_gameState->GetAllEntities()) {
        if (!entity || entity->GetType() != EntityType::Enemy) continue;
        const auto* enemy = static_cast<const Enemy*>(entity.get());
        if (enemy->GetSpawnPosition().x > checkpointX) {
            m_checkpointRespawnEnemyIds.insert(enemy->GetId());
        }
    }
}

void GameController::RestoreCheckpointEnemies() {
    if (!m_gameState) return;

    for (const auto& projectile : m_playerProjectiles) {
        if (projectile) UnregisterEntityVisuals(projectile->GetId());
    }
    for (const auto& projectile : m_petProjectiles) {
        if (projectile) UnregisterEntityVisuals(projectile->GetId());
    }
    m_playerProjectiles.clear();
    m_petProjectiles.clear();
    std::vector<int> projectileIds;
    for (const auto& entity : m_gameState->GetAllEntities()) {
        if (!entity) continue;
        if (entity->GetType() == EntityType::Projectile) {
            projectileIds.push_back(entity->GetId());
            continue;
        }
        if (entity->GetType() != EntityType::Enemy ||
            m_checkpointRespawnEnemyIds.count(entity->GetId()) == 0) continue;

        auto* enemy = static_cast<Enemy*>(entity.get());
        enemy->SetPosition(enemy->GetSpawnPosition());
        enemy->SetVelocity({0.0f,0.0f});
        enemy->SetHealth(enemy->GetMaxHealth());
        enemy->SetState(EnemyState::Idle);
        enemy->SetActive(true);
        UnregisterEntityVisuals(enemy->GetId());
        RegisterEnemyVisuals(enemy);
    }
    for (int id : projectileIds) {
        UnregisterEntityVisuals(id);
        m_gameState->RemoveEntity(id);
    }
    m_inCombat = false;
    m_combatExitTimer = 0.0f;
}

void GameController::RespawnPlayer(Player* player, bool restoreEncounter) {
    if (!player) return;
    Vector2 respawn = m_respawnPoint;
    if (m_localCoop && !restoreEncounter && m_gameState) {
        Player* partner = player == m_gameState->GetLocalPlayer()
            ? m_gameState->GetSecondLocalPlayer()
            : m_gameState->GetLocalPlayer();
        if (partner && partner->IsAlive()) {
            respawn = partner->GetPosition();
            respawn.x += player == m_gameState->GetLocalPlayer() ? -56.0f : 56.0f;
        }
    }
    player->SetPosition(respawn);
    player->SetVelocity({0.0f, 0.0f});
    player->CancelCast();   // dying must not leave the skill lock held
    player->SetHealth(player->GetMaxHealth());
    player->SetActive(true);
    if (restoreEncounter) RestoreCheckpointEnemies();

    if (m_previousLevelId != -1) {
        for (auto& entity : m_gameState->GetAllEntities()) {
            if (entity && entity->GetType() == EntityType::Boss) {
                auto* boss = static_cast<Boss*>(entity.get());
                boss->ResetToPhase1();
                View::CharacterRenderer::GetInstance().SwitchPhase(boss->GetId(), BossPhase::Phase1);
            }
        }
    }
}

void GameController::CheckLevelComplete() {
    if (m_levelComplete || !m_gameState) return;

    if (m_gameState->IsLevelComplete()) {
        m_levelComplete = true;
        m_gameState->StopTimer();
        m_scoring.SetClearTime(m_gameState->GetClearTime());
        m_scoring.CalculateStars();
        SoundManager::GetInstance().StopAllMusic();

        Player* player = m_gameState->GetLocalPlayer();
        const int level = m_gameState->GetCurrentLevel();
        const int score = m_scoring.GetCurrentScore();
        SaveManager& save = SaveManager::GetInstance();
        const bool persistentResult = level > 0;
        const int oldHighScore = persistentResult ? save.GetLevelHighScore(level) : 0;
        const int oldBestStars = persistentResult ? save.GetLevelBestStars(level) : 0;
        const float oldBestTime = persistentResult ? save.GetLevelBestTime(level) : 0.0f;

        View::LevelResultSnapshot snapshot;
        snapshot.levelNumber = level;
        snapshot.stars = m_scoring.GetStars();
        snapshot.performance = m_scoring.GetPerformance();
        snapshot.clearTime = m_scoring.GetClearTime();
        snapshot.parTime = m_scoring.GetParTime();
        snapshot.enemiesKilled = m_scoring.GetDefeatedEnemies();
        snapshot.totalEnemies = m_scoring.GetTotalEnemies();
        snapshot.itemsCollected = m_scoring.GetCollectedItems();
        snapshot.totalItems = m_scoring.GetTotalItems();
        snapshot.score = score;
        snapshot.healthPercent = player && player->GetMaxHealth() > 0
            ? std::clamp((float)player->GetHealth() / player->GetMaxHealth(), 0.0f, 1.0f)
            : 0.0f;
        snapshot.newHighScore = persistentResult && score > oldHighScore;
        snapshot.newBestStars = persistentResult && snapshot.stars > oldBestStars;
        snapshot.newBestTime = persistentResult &&
            (oldBestTime <= 0.0f || snapshot.clearTime < oldBestTime);
        snapshot.characterClass = player ? player->GetCharacterClass() : CharacterClass::Knight;

        if (persistentResult) {
            save.SetLevelHighScore(level, score);
            save.SetLevelBestStars(level, snapshot.stars);
            save.SetLevelBestTime(level, snapshot.clearTime);
            if (level >= 1 && level <= 6) {
                LeaderboardEntry entry;
                entry.playerName = save.GetPlayerName();
                entry.characterIds.push_back(CharacterClassId(snapshot.characterClass));
                if (m_localCoop && m_gameState->GetSecondLocalPlayer()) {
                    entry.characterIds.push_back(CharacterClassId(
                        m_gameState->GetSecondLocalPlayer()->GetCharacterClass()));
                }
                entry.score = score;
                entry.timeMs = std::max(1, static_cast<int>(snapshot.clearTime * 1000.0f + 0.5f));
                entry.stars = snapshot.stars;
                entry.localCoop = m_localCoop;
                save.RecordLevelResult(level, entry);
                AchievementManager::GetInstance().OnLevelCompleted(
                    level, snapshot.stars, snapshot.clearTime, snapshot.parTime,
                    !m_runTookDamage, m_localCoop);
            }
            save.Save();
        }

        View::ResultView::GetInstance().Show(snapshot);
        View::HUDView::GetInstance().SetVisible(false);
        View::SkillBarView::GetInstance().Close();
        View::InteractPrompt::GetInstance().Hide();
        View::UIStateManager::GetInstance().Push(View::UILayer::Result);
    }
}

// ──────────────────────────────────────────────────────────────
// Boss Arena helpers
// ──────────────────────────────────────────────────────────────

void GameController::UpdateBossArenaPortals() {
    if (!m_gameState || m_previousLevelId == -1) return; // không phải trong boss arena

    // Kiểm tra còn enemy/boss sống không
    bool anyAlive = false;
    for (auto& e : m_gameState->GetAllEntities()) {
        if (!e) continue;
        auto t = e->GetType();
        if (t == EntityType::Enemy || t == EntityType::Boss) {
            auto* c = static_cast<Character*>(e.get());
            if (c->IsAlive()) { anyAlive = true; break; }
        }
    }

    // Lock/unlock cổng thoát BossArena (targetLevelId == -1)
    for (auto& e : m_gameState->GetAllEntities()) {
        if (!e || e->GetType() != EntityType::TeleportPortal) continue;
        auto* portal = static_cast<TeleportPortal*>(e.get());
        if (portal->GetPortalType() == PortalType::BossArena
                && portal->GetTargetLevelId() == -1) {
            portal->SetLocked(anyAlive);
        }
    }
}

void GameController::SavePlayerState(Player* player) {
    if (!player) return;
    m_savedPlayerState.health      = player->GetHealth();
    m_savedPlayerState.score       = player->GetScore();
    m_savedPlayerState.skillPoints = player->GetSkillPoints();
    m_savedPlayerState.coins       = player->GetInventory().GetCoins();
    m_savedPlayerState.apples      = player->GetInventory().GetApples();
    m_savedPlayerState.keys        = player->GetInventory().GetKeys();
    m_savedPlayerState.tookDamage  = m_runTookDamage;
    m_savedPlayerState.maxHealth   = player->GetMaxHealth();
    m_savedPlayerState.cores       = player->GetCores();
    m_hasSavedState = true;
}

void GameController::RestorePlayerState(Player* player) {
    if (!player || !m_hasSavedState) return;
    // Max HP first: SetHealth clamps to it, so restoring a VitalCore-inflated
    // health value against the class default pool would silently cut it down.
    if (m_savedPlayerState.maxHealth > 0) player->SetMaxHealth(m_savedPlayerState.maxHealth);
    player->SetHealth(m_savedPlayerState.health);
    player->SetScore(m_savedPlayerState.score);
    player->SetSkillPoints(m_savedPlayerState.skillPoints);
    player->GetInventory().AddCoins(m_savedPlayerState.coins);
    player->GetInventory().AddApples(m_savedPlayerState.apples);
    player->GetInventory().AddKeys(m_savedPlayerState.keys);
    player->GetCores() = m_savedPlayerState.cores;
    m_runTookDamage = m_savedPlayerState.tookDamage;
}

void GameController::Update(float dt) {
    if (!m_running || !m_gameState) return;

    View::GameView::GetInstance().SetEntities(&m_gameState->GetAllEntities());

    if (View::HUDView::GetInstance().WantsQuitTest()) {
        m_returnToMenu = true;
        m_running = false;
        return;
    }

    // Cap delta time to prevent physics tunneling during asset loading spikes
    if (dt > 0.1f) dt = 0.1f;

    InputCommand cmd = InputController::GetInstance().Poll();
    InputCommand playerOneCmd = m_localCoop
        ? InputController::GetInstance().PollPlayerOne()
        : cmd;
    InputCommand playerTwoCmd = InputController::GetInstance().PollPlayerTwo();

    if (!m_paused && IsKeyPressed(KEY_M)) {
        View::MinimapView::GetInstance().ToggleVisible();
        SoundManager::GetInstance().PlaySound("ui_confirm");
    }

    // Reaction reference. Freezes the game like the pause menu does, so it can
    // be read mid-fight without dying to it.
    if (!m_paused && !m_buffOfferOpen && IsKeyPressed(KEY_C)) {
        m_codexOpen = !m_codexOpen;
        m_gameState->SetTimerRunning(!m_codexOpen);
        SoundManager::GetInstance().PlaySound("ui_confirm");
    }
    if (m_codexOpen) {
        View::GameView::GetInstance().Update(dt);
        return;
    }
    View::MinimapView::GetInstance().Update(
        dt, m_gameState.get(), m_gameState->GetLocalPlayer(),
        m_localCoop ? m_gameState->GetSecondLocalPlayer() : nullptr);

    if (m_levelComplete) {
        View::ResultView& result = View::ResultView::GetInstance();
        result.Update(dt);
        const View::ResultAction action = result.ConsumeAction();
        if (action == View::ResultAction::Retry) {
            StartLevel(m_gameState->GetCurrentLevel());
        } else if (action == View::ResultAction::Continue ||
                   action == View::ResultAction::LevelSelect) {
            m_returnToMenu = true;
            m_running = false;
        }
        return;
    }

    if (m_paused && View::OptionsView::GetInstance().IsVisible()) {
        auto& options = View::OptionsView::GetInstance();
        options.Update(dt);
        if (options.WantsBack()) {
            options.ClearWantsBack();
            options.SetVisible(false);
            View::MenuView::GetInstance().SetVisible(true);
            View::UIStateManager::GetInstance().Push(View::UILayer::Menu);
        }
        return;
    }

    if (View::TutorialRenderer::GetInstance().IsDialogVisible()) {
        if (cmd.interact || (m_localCoop && playerTwoCmd.interact) || cmd.menuConfirm || cmd.pause) {
            SoundManager::GetInstance().PlaySound("ui_confirm");
            View::TutorialRenderer::GetInstance().HideDialog();
        }
        return;
    }

    if (cmd.pause) {
        SoundManager::GetInstance().PlaySound("ui_confirm");
        if (m_paused) {
            m_paused = false;
            m_gameState->SetTimerRunning(true);
            View::UIStateManager::GetInstance().Pop();
            View::MenuView::GetInstance().SetVisible(false);
        } else {
            m_paused = true;
            m_pauseSelected = 0;
            m_gameState->SetTimerRunning(false);
            View::MenuView::GetInstance().ShowPauseOverlay();
            View::MenuView::GetInstance().SetVisible(true);
            View::UIStateManager::GetInstance().Push(View::UILayer::Menu);
        }
        return;
    }

    if (m_paused) {
        if (cmd.menuDelta != 0) {
            SoundManager::GetInstance().PlaySound("ui_hover");
            m_pauseSelected += cmd.menuDelta;
            if (m_pauseSelected < 0) m_pauseSelected = 2;
            if (m_pauseSelected > 2) m_pauseSelected = 0;
        }

        Vector2 mousePos = GetMousePosition();
        int hovered = View::MenuView::GetInstance().GetHoveredItem(mousePos);
        if (hovered != -1) {
            m_pauseSelected = hovered;
        }

        if (cmd.menuConfirm || (hovered != -1 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))) {
            SoundManager::GetInstance().PlaySound("ui_confirm");
            if (View::MenuView::GetInstance().GetMode() == View::MenuMode::Pause) {
                if (m_pauseSelected == 0) {
                    // Resume
                    m_paused = false;
                    m_gameState->SetTimerRunning(true);
                    View::UIStateManager::GetInstance().Pop();
                    View::MenuView::GetInstance().SetVisible(false);
                } else if (m_pauseSelected == 1) {
                    auto& options = View::OptionsView::GetInstance();
                    options.ClearWantsBack();
                    View::UIStateManager::GetInstance().Pop();
                    View::MenuView::GetInstance().SetVisible(false);
                    options.SetVisible(true);
                } else if (m_pauseSelected == 2) {
                    // Quit to Menu
                    m_returnToMenu = true;
                    m_running = false;
                }
            }
        }
        View::MenuView::GetInstance().Update(dt, m_pauseSelected);
        return;
    }

    // The boon draft freezes the fight while the player picks, so it is ticked
    // before anything simulates and swallows the rest of the frame while open.
    UpdateBuffOffer(dt);
    if (m_buffOfferOpen) {
        View::GameView::GetInstance().Update(dt);
        return;
    }

    // Core draft does the same. Checked after the boon draft so the two can
    // never be open at once.
    UpdateCoreDraft(dt);
    if (m_coreDraftOpen) {
        View::GameView::GetInstance().Update(dt);
        return;
    }

    // Hit-stop. The simulation holds on the contact frame while the view keeps
    // animating, so particles and floating text still play over the frozen
    // world and the pause reads as impact rather than as a dropped frame.
    if (m_hitStopTimer > 0.0f) {
        m_hitStopTimer -= dt;
        // GameView::Update drives the floating text and camera shake, so those
        // keep playing over the held frame.
        View::GameView::GetInstance().Update(dt);
        return;
    }

    if (!View::UIStateManager::GetInstance().IsOverlayActive()) {
        HandlePlayerInput(m_gameState->GetLocalPlayer(), playerOneCmd, dt);
        UpdateInteractions(m_gameState->GetLocalPlayer(), playerOneCmd);
        if (m_localCoop) {
            HandlePlayerInput(m_gameState->GetSecondLocalPlayer(), playerTwoCmd, dt);
            UpdateInteractions(m_gameState->GetSecondLocalPlayer(), playerTwoCmd);
        }
    }

    Player* player = m_gameState->GetLocalPlayer();
    Player* secondPlayer = m_localCoop ? m_gameState->GetSecondLocalPlayer() : nullptr;
    if (player && player->IsActive()) {
        ApplyGravity(player, dt);
    }
    if (secondPlayer && secondPlayer->IsActive()) {
        ApplyGravity(secondPlayer, dt);
    }

    m_gameState->Update(dt);

    // Enemy death animation deactivates the entity after a short delay, so the
    // final score/removal event must be observed here rather than only on the
    // exact attack frame.
    for (const auto& entity : m_gameState->GetAllEntities()) {
        if (!entity || entity->GetType() != EntityType::Enemy || entity->IsActive()) continue;
        auto* enemy = static_cast<Enemy*>(entity.get());
        if (enemy->GetState() == EnemyState::Dead &&
            m_countedDefeatedEnemyIds.count(enemy->GetId()) == 0) {
            OnEntityRemoved(enemy);
        }
    }
    
    // Check and register visuals for newly spawned entities (like projectiles or items)
    for (const auto& entity : m_gameState->GetAllEntities()) {
        if (!entity || !entity->IsActive()) continue;
        uint32_t id = static_cast<uint32_t>(entity->GetId());
        
        bool isAnimated = View::EntityRenderer::GetInstance().IsRegistered(id) || 
                          View::CharacterRenderer::GetInstance().IsRegistered(id);
        bool isTutorialVisual = entity->GetType() == EntityType::InMapGuide
                             || entity->GetType() == EntityType::Signboard
                             || entity->GetType() == EntityType::LevelCompleteCup;
        
        if (!isAnimated && !isTutorialVisual) {
            RegisterEntityVisuals(entity.get());
        }
    }

    UpdateEnemyAI(dt);

    if (player && player->IsActive()) {
        ResolveTileCollisions(player, dt);
        m_playerOnGround = IsOnGround(player);

        m_footstepTimer -= dt;
        const bool isMovingOnGround = m_playerOnGround
            && std::abs(player->GetVelocity().x) > 25.0f
            && !player->IsDashing();
        if (isMovingOnGround && m_footstepTimer <= 0.0f) {
            SoundManager::GetInstance().PlaySound("player_footstep");
            m_footstepTimer = player->IsSprinting() ? 0.22f : 0.31f;
        } else if (!isMovingOnGround) {
            m_footstepTimer = std::min(m_footstepTimer, 0.08f);
        }
    }

    if (secondPlayer && secondPlayer->IsActive()) {
        ResolveTileCollisions(secondPlayer, dt);
    }

    // Everything has finished moving for this frame, and no entity is removed
    // until after combat -- the one window where a cached spatial index is
    // both accurate and safe.
    RebuildSpatialIndex();

    UpdateCombat(player, dt, true);
    if (secondPlayer) UpdateCombat(secondPlayer, dt, false);
    UpdateItemPhysics(dt);
    UpdateItems(player, dt);
    if (secondPlayer) UpdateItems(secondPlayer, dt);
    UpdatePets(dt, playerOneCmd);
    UpdateProjectiles(dt);
    UpdatePlayerProjectiles(dt);
    UpdateEndgameCheckpoints();
    UpdateBossArenaPortals();
    UpdateRandomEnemySpawns(dt);
    UpdateBuffPickups(dt);
    UpdateElementalEffects(dt);
    m_particles.Update(dt);
    m_gameState->TickTimer(dt);

    if (player && player->IsAlive()) {
        const float mapHeight = std::max(1, m_gameState->GetMapHeight()) * TILE_SIZE;
        if (player->GetPosition().y > mapHeight + TILE_SIZE * 2) {
            m_runTookDamage = true;
            player->TakeDamage(player->GetMaxHealth()); // Technically dies
            SoundManager::GetInstance().PlaySound("player_die");
            RespawnPlayer(player, !m_localCoop);
        }
    }

    if (secondPlayer && secondPlayer->IsAlive()) {
        const float mapHeight = std::max(1, m_gameState->GetMapHeight()) * TILE_SIZE;
        if (secondPlayer->GetPosition().y > mapHeight + TILE_SIZE * 2) {
            m_runTookDamage = true;
            secondPlayer->TakeDamage(secondPlayer->GetMaxHealth());
            SoundManager::GetInstance().PlaySound("player_die");
            RespawnPlayer(secondPlayer, false);
        }
    }

    if (player) {
        Vector2 center = {
            player->GetPosition().x + player->GetSize().x * 0.5f,
            player->GetPosition().y + player->GetSize().y * 0.5f
        };
        float desiredZoom = 1.0f;
        if (secondPlayer && secondPlayer->IsAlive()) {
            const Vector2 secondCenter = {
                secondPlayer->GetPosition().x + secondPlayer->GetSize().x * 0.5f,
                secondPlayer->GetPosition().y + secondPlayer->GetSize().y * 0.5f
            };
            center = {(center.x + secondCenter.x) * 0.5f, (center.y + secondCenter.y) * 0.5f};
            const float separationX = std::abs(secondCenter.x - center.x) * 2.0f;
            const float separationY = std::abs(secondCenter.y - center.y) * 2.0f;
            const float fitX = GetScreenWidth() / std::max(1.0f, separationX + 360.0f);
            const float fitY = GetScreenHeight() / std::max(1.0f, separationY + 260.0f);
            desiredZoom = std::clamp(std::min(fitX, fitY), 0.62f, 1.0f);
        }
        m_camera.zoom += (desiredZoom - m_camera.zoom) * std::min(1.0f, dt * 3.5f);
        m_camera.target.x += (center.x - m_camera.target.x) * 0.1f;
        if (m_gameState->GetCurrentLevel() == 1 &&
            std::filesystem::exists("assets/levels/tutorial.ldtk")) {
            const float tutorialCenterY = m_gameState->GetMapHeight() * TILE_SIZE * 0.5f;
            m_camera.target.y += (tutorialCenterY - m_camera.target.y) * 0.1f;
        } else {
            m_camera.target.y += (center.y - m_camera.target.y) * 0.1f;
        }
        // Always keep player centered regardless of current window size
        m_camera.offset = {GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f};
    }

    const Boss* activeBoss = nullptr;
    const float hudHalfW = (GetScreenWidth() * 0.5f) / std::max(0.01f, m_camera.zoom);
    const float hudHalfH = (GetScreenHeight() * 0.5f) / std::max(0.01f, m_camera.zoom);
    const Rectangle hudViewport = {
        m_camera.target.x - hudHalfW, m_camera.target.y - hudHalfH,
        hudHalfW * 2.0f, hudHalfH * 2.0f
    };
    for (const auto& entity : m_gameState->GetAllEntities()) {
        if (entity && entity->IsActive() && entity->GetType() == EntityType::Boss
            && RectOverlap(entity->GetBoundingBox(), hudViewport)) {
            activeBoss = static_cast<const Boss*>(entity.get());
            break;
        }
    }

    View::GameView::GetInstance().Update(dt);
    View::HUDView::GetInstance().Update(dt, player, activeBoss, secondPlayer);
    View::SkillBarView::GetInstance().Update(dt, player, secondPlayer);
    CheckLevelComplete();
}

void GameController::Render() {
    if (!m_gameState) return;

    View::Renderer::GetInstance().BeginFrame();
    View::GameView::GetInstance().Render(m_camera, m_particles.GetActive(), GetFrameTime());
    View::Renderer::GetInstance().EndFrameAndFlush();

    RenderBuffPickups();
    RenderBuffHud();
    RenderCoreHud();
    RenderBuffOffer();
    RenderCoreDraft();
    RenderElementCodex();

    if (m_localCoop && m_gameState) {
        auto drawPlayerMarker = [&](Player* localPlayer, const char* label, Color color,
                                    float horizontalOffset) {
            if (!localPlayer || !localPlayer->IsActive()) return;
            const Rectangle bounds = localPlayer->GetBoundingBox();
            const Vector2 world = {
                bounds.x + bounds.width * 0.5f,
                bounds.y + bounds.height * 0.48f
            };
            Vector2 screen = GetWorldToScreen2D(world, m_camera);
            screen.x += horizontalOffset;
            DrawCircleV(screen, 12.0f, Color{8, 7, 18, 220});
            DrawCircleLinesV(screen, 12.0f, color);
            const int size = 11;
            DrawText(label, static_cast<int>(screen.x - MeasureText(label, size) * 0.5f),
                     static_cast<int>(screen.y - size * 0.5f), size, color);
        };
        drawPlayerMarker(m_gameState->GetLocalPlayer(), "P1", Color{104, 210, 255, 255}, -48.0f);
        drawPlayerMarker(m_gameState->GetSecondLocalPlayer(), "P2", Color{220, 168, 255, 255}, 48.0f);
    }

    // Keep the map above world-space labels, but below modal overlays.
    if (!m_paused && !m_levelComplete && !View::OptionsView::GetInstance().IsVisible()) {
        View::MinimapView::GetInstance().Render(
            m_gameState.get(), m_gameState->GetLocalPlayer(),
            m_localCoop ? m_gameState->GetSecondLocalPlayer() : nullptr);
    }

    if (View::OptionsView::GetInstance().IsVisible()) {
        View::OptionsView::GetInstance().Render();
    }

    View::TutorialRenderer::GetInstance().RenderDialog();
}

void GameController::Shutdown() {
    SaveManager::GetInstance().Save();
    SoundManager::GetInstance().StopAllMusic();
    SoundManager::GetInstance().StopAllSounds();

    // Views and renderers hold non-owning Entity pointers. Detach them before
    // releasing any owner, including projectiles and the active pet.
    View::HUDView::GetInstance().ClearEntityReferences();
    View::SkillBarView::GetInstance().ClearEntityReferences();
    View::CharacterRenderer::GetInstance().Clear();
    View::EntityRenderer::GetInstance().Clear();
    View::GameView::GetInstance().SetTiles(MapLayer::Background, nullptr);
    View::GameView::GetInstance().SetTiles(MapLayer::Main, nullptr);
    View::GameView::GetInstance().SetTiles(MapLayer::Foreground, nullptr);
    View::GameView::GetInstance().SetEntities(nullptr);

    m_activePet.reset();
    m_petProjectiles.clear();
    m_playerProjectiles.clear();
    m_gameState.reset();
    m_running = false;
    View::UIStateManager::GetInstance().Clear();
    View::ResultView::GetInstance().Dismiss();
    View::OptionsView::GetInstance().SetVisible(false);
    View::HUDView::GetInstance().SetVisible(false);
    View::MenuView::GetInstance().SetVisible(false);

}

// ============================================================
// Item Physics (coin scatter from chests)
// ============================================================

void GameController::UpdateItemPhysics(float dt) {
    if (!m_gameState) return;

    const float mapHeight = std::max(1, m_gameState->GetMapHeight()) * (float)TILE_SIZE;
    const float mapWidth  = std::max(1, m_gameState->GetMapWidth())  * (float)TILE_SIZE;
    constexpr float COIN_GRAVITY    = 800.0f;   // px/s²
    constexpr float COIN_BOUNCE_Y   = -120.0f;  // small bounce on first land
    constexpr float FRICTION        = 0.82f;    // horizontal friction on ground
    constexpr float REST_THRESHOLD  = 30.0f;    // vel.y below this = settled on ground

    std::vector<int> despawnIds;

    for (const auto& entity : m_gameState->GetAllEntities()) {
        if (!entity || entity->GetType() != EntityType::Item || !entity->IsActive()) continue;
        auto* item = static_cast<Item*>(entity.get());
        if (!item->IsPhysicsEnabled()) continue;

        Vector2 vel = item->GetVelocity();
        Vector2 pos = item->GetPosition();
        Rectangle box = item->GetBoundingBox();

        // --- Apply gravity ---
        if (!IsRectOnGround(box)) {
            vel.y += COIN_GRAVITY * dt;
        } else if (vel.y > REST_THRESHOLD) {
            // Bounce once
            vel.y = COIN_BOUNCE_Y;
            vel.x *= FRICTION;
        } else if (vel.y > 0.0f) {
            // Settled — stop vertical movement and apply friction
            vel.y = 0.0f;
            vel.x *= (1.0f - dt * 4.0f); // gentle slide-to-stop
            if (std::abs(vel.x) < 2.0f) vel.x = 0.0f;
        }

        // --- Move and resolve tile collisions ---
        pos.x += vel.x * dt;
        pos.y += vel.y * dt;
        item->SetPosition(pos);
        item->SetVelocity(vel);

        // Simple vertical tile push-out (land on tiles)
        box = item->GetBoundingBox();
        for (const auto& tile : m_gameState->GetTiles(MapLayer::Main)) {
            if (!tile.solid) continue;
            Rectangle tileRect = {
                (float)tile.x * TILE_SIZE, (float)tile.y * TILE_SIZE,
                (float)TILE_SIZE, (float)TILE_SIZE
            };
            if (!RectOverlap(box, tileRect)) continue;

            float overlapTop    = (box.y + box.height) - tileRect.y;
            float overlapBottom = (tileRect.y + tileRect.height) - box.y;
            float overlapLeft   = (box.x + box.width) - tileRect.x;
            float overlapRight  = (tileRect.x + tileRect.width) - box.x;

            float minY = std::min(overlapTop, overlapBottom);
            float minX = std::min(overlapLeft, overlapRight);

            if (minY <= minX) {
                // Resolve vertical
                if (overlapTop < overlapBottom) {
                    pos.y -= overlapTop;
                    vel.y = 0.0f;
                } else {
                    pos.y += overlapBottom;
                    if (vel.y < 0.0f) vel.y = 0.0f;
                }
            } else {
                // Resolve horizontal
                if (overlapLeft < overlapRight) pos.x -= overlapLeft;
                else pos.x += overlapRight;
                vel.x = 0.0f;
            }
            item->SetPosition(pos);
            item->SetVelocity(vel);
            box = item->GetBoundingBox();
        }

        // --- Despawn if fell out of world (no ground below) ---
        if (pos.y > mapHeight + (float)TILE_SIZE * 3.0f ||
            pos.x < -(float)TILE_SIZE * 2.0f ||
            pos.x > mapWidth + (float)TILE_SIZE * 2.0f) {
            despawnIds.push_back(item->GetId());
        }
    }

    // Safe removal — no memory leak: visual unregistered then entity destroyed
    for (int id : despawnIds) {
        UnregisterEntityVisuals(id);
        m_gameState->RemoveEntity(id);
    }
}

// ============================================================
// Endgame Checkpoint — interaction-triggered flag animation
// ============================================================

void GameController::UpdateEndgameCheckpoints() {
    if (!m_gameState) return;

    float dt = GetFrameTime();

    for (const auto& entity : m_gameState->GetAllEntities()) {
        if (!entity || entity->GetType() != EntityType::Checkpoint || !entity->IsActive()) continue;
        auto* cp = static_cast<Checkpoint*>(entity.get());
        if (!cp->IsEndGame()) continue;
        if (!cp->IsActivated()) continue;

        int  id  = cp->GetId();
        uint32_t uid = static_cast<uint32_t>(id);

        bool revealed  = (m_endgameFlagRevealedIds.find(id)  != m_endgameFlagRevealedIds.end());
        bool captured  = (m_endgameFlagCapturedIds.find(id)  != m_endgameFlagCapturedIds.end());

        if (!revealed) {
            // Phase 1 — pressing F activated the finish; raise its flag.
            View::EntityRenderer::GetInstance().Unregister(uid);
            View::EntityRenderer::GetInstance().RegisterAnimated(
                cp, "assets/textures/objects/checkpoint_flag_out.json", "flag_out");
            m_endgameFlagRevealedIds.insert(id);
            m_flagOutTimers[id] = 0.0f;
        } else if (!captured) {
            // Phase 2 — flag_out is playing; count down and then switch to captured loop
            m_flagOutTimers[id] += dt;
            if (m_flagOutTimers[id] >= FLAG_OUT_DURATION) {
                // Switch to the waving captured loop
                View::EntityRenderer::GetInstance().Unregister(uid);
                View::EntityRenderer::GetInstance().RegisterAnimated(
                    cp, "assets/textures/objects/checkpoint_captured.json", "idle");
                m_endgameFlagCapturedIds.insert(id);
                m_flagOutTimers.erase(id);
            }
        }
        // Phase 3 (captured) — nothing more to do, animation loops on its own
    }
}

// ============================================================
// Pet Management
// ============================================================

void GameController::RegisterPetVisuals(Pet* pet) {
    if (!pet) return;
    std::string base;
    switch (pet->GetPetType()) {
        case PetType::BabyDragon: base = "assets/textures/pets/baby_dragon/"; break;
        case PetType::Ghost:      base = "assets/textures/pets/ghost/";       break;
        case PetType::Skull:      base = "assets/textures/pets/skull/";       break;
        case PetType::Fairy:      base = "assets/textures/pets/fairy/";       break;
        default: return;
    }
    auto& cr = View::CharacterRenderer::GetInstance();
    uint32_t id = static_cast<uint32_t>(pet->GetId());
    
    // Register base character rendering
    cr.Register(pet, base + "idle.json", "idle");
    cr.MergeAtlas(id, base + "move.json");
    
    if (pet->GetPetType() == PetType::BabyDragon) {
        cr.MergeAtlas(id, base + "attack.json");
    }
    if (pet->GetPetType() == PetType::Ghost) {
        cr.MergeAtlas(id, base + "healing.json");
    }
}

void GameController::SpawnPet(PetType type) {
    Player* player = m_gameState ? m_gameState->GetLocalPlayer() : nullptr;
    if (!player) return;

    // Ghost cannot be summoned while in combat


    DespawnPet();
    Vector2 petPos = { player->GetPosition().x - 40.0f,
                       player->GetPosition().y - 20.0f };
    m_activePet = std::make_unique<Pet>(petPos, type, player->GetId());
    RegisterPetVisuals(m_activePet.get());
}

void GameController::DespawnPet() {
    if (m_activePet) {
        UnregisterEntityVisuals(m_activePet->GetId());
        m_activePet.reset();
    }
}

void GameController::FireDragonProjectile(Pet* pet) {
    if (!pet) return;
    // Find target entity
    Entity* target = m_gameState->GetEntity(pet->GetTargetId());
    Vector2 targetPos = target ? target->GetPosition() : pet->GetPosition();

    Vector2 spawnPos = { pet->GetPosition().x + pet->GetSize().x * 0.5f,
                         pet->GetPosition().y + pet->GetSize().y * 0.5f };

    auto proj = std::make_unique<Projectile>(
        spawnPos, Vector2{16.0f, 16.0f},
        ProjectileType::FlyingProjectile,
        Direction::Right,
        pet->GetDamage(),
        pet->GetOwnerId());

    proj->SetSize({20, 20});
    proj->SetScale(1.0f / 7.0f);
    proj->SetHoming(true);
    proj->SetHomingTargetPos(targetPos);

    // Register visuals based on pet type
    std::string projVisual = "assets/textures/pets/projectile_dragon/attack.json";
    if (pet->GetPetType() == PetType::Skull) {
        projVisual = "assets/textures/pets/projectile_skull/attack.json";
    }

    View::EntityRenderer::GetInstance().RegisterAnimated(
        proj.get(), projVisual, "attack");

    m_petProjectiles.push_back(std::move(proj));
    SoundManager::GetInstance().PlaySound(
        pet->GetPetType() == PetType::Skull ? "pet_skull_attack" : "pet_dragon_attack");
    pet->ResetFireFlag();
}

void GameController::UpdatePets(float dt, const InputCommand& cmd) {
    Player* player = m_gameState ? m_gameState->GetLocalPlayer() : nullptr;
    if (!player) return;

    // Detect combat: any alive enemy within range
    Vector2 pPos = { player->GetPosition().x + player->GetSize().x * 0.5f,
                     player->GetPosition().y + player->GetSize().y * 0.5f };
    bool anyEnemyClose = false;
    for (const auto& e : m_gameState->GetAllEntities()) {
        if (!e || e->GetType() != EntityType::Enemy || !e->IsActive()) continue;
        auto* enemy = static_cast<Enemy*>(e.get());
        if (!enemy->IsAlive()) continue;
        float dx = e->GetPosition().x - pPos.x;
        float dy = e->GetPosition().y - pPos.y;
        if (std::sqrt(dx*dx + dy*dy) < PET_COMBAT_RANGE) { anyEnemyClose = true; break; }
    }

    if (anyEnemyClose) {
        m_inCombat = true;
        m_combatExitTimer = COMBAT_EXIT_GRACE;
    } else if (m_combatExitTimer > 0.0f) {
        m_combatExitTimer -= dt;
        if (m_combatExitTimer <= 0.0f) m_inCombat = false;
}



    if (!m_activePet) return;

    // Build enemy and item pointer list for pet targeting
    std::vector<Entity*> enemies;
    std::vector<Entity*> items;
    for (const auto& e : m_gameState->GetAllEntities()) {
        if (!e) continue;
        if (e->GetType() == EntityType::Enemy && e->IsActive()) {
            auto* enemy = static_cast<Enemy*>(e.get());
            if (enemy->IsAlive()) enemies.push_back(e.get());
        }
        else if (e->GetType() == EntityType::Item && e->IsActive()) {
            items.push_back(e.get());
        }
    }

    // Update homing target positions on existing projectiles
    for (auto& proj : m_petProjectiles) {
        if (!proj->IsHoming()) continue;
        Entity* tgt = m_gameState->GetEntity(proj->GetOwnerId()); // reuse: we stored target id there
        // Actually we'll just let it home wherever it last aimed; no reassignment needed
    }

    m_activePet->UpdateAI(player->GetPosition(), dt, player, enemies, items, m_inCombat);
    m_activePet->Update(dt);

    if (m_activePet->GetPetType() == PetType::Fairy) {
        ResolveTileCollisions(m_activePet.get(), dt);
    }

    // Fairy collecting items
    if (m_activePet->GetPetType() == PetType::Fairy) {
        std::vector<int> collectedIds;
        bool persistentCoinsChanged = false;
        for (Entity* itemEnt : items) {
            if (itemEnt->IsActive() &&
                RectOverlap(m_activePet->GetBoundingBox(), ItemPickupBox(*itemEnt))) {
                Item* item = static_cast<Item*>(itemEnt);
                const char* pickupSound = "item_pickup";
                switch (item->GetItemType()) {
                    case ItemType::Coin:
                        pickupSound = "coin_pickup";
                        player->GetInventory().AddCoins(item->GetAmount());
                        SaveManager::GetInstance().AddCoins(item->GetAmount());
                        AchievementManager::GetInstance().OnCoinCollected(item->GetAmount());
                        persistentCoinsChanged = true;
                        player->AddScore(item->GetAmount() * 10);
                        m_scoring.AddScore(item->GetAmount() * 10);
                        break;
                    case ItemType::Apple:
                        pickupSound = "apple_pickup";
                        player->Heal(25);
                        break;
                    case ItemType::Key:
                        pickupSound = "key_pickup";
                        player->GetInventory().AddKeys(1);
                        break;
                    case ItemType::Potion:
                        pickupSound = "potion_pickup";
                        player->Heal(50);
                        AchievementManager::GetInstance().OnPotionUsed();
                        break;
                    default:
                        player->GetInventory().AddItem(
                            std::make_unique<Item>(item->GetPosition(), item->GetItemType(), item->GetAmount()));
                        break;
                }

                m_collectedItems++;
                m_scoring.CollectItem();
                SoundManager::GetInstance().PlaySound(pickupSound);
                View::FloatingTextManager::GetInstance().Emit(
                    item->GetPosition(), item->GetItemName(), YELLOW, 1.0f);
                collectedIds.push_back(item->GetId());
            }
        }
        for (int id : collectedIds) {
            UnregisterEntityVisuals(id);
            m_gameState->RemoveEntity(id);
        }
        if (persistentCoinsChanged) SaveManager::GetInstance().Save();
    }

    // Dragon/Skull fire
    if ((m_activePet->GetPetType() == PetType::BabyDragon || m_activePet->GetPetType() == PetType::Skull) && m_activePet->WantsToFire()) {
        FireDragonProjectile(m_activePet.get());
    }

    // Sync animation clip based on pet state
    PetState ps = m_activePet->GetPetState();
    int action = View::ACTION_IDLE;
    if (ps == PetState::Following)  action = View::ACTION_WALK; // maps to 'walk' or 'move'
    if (ps == PetState::Charging)   action = View::ACTION_ATTACK;
    if (ps == PetState::Healing)    action = View::ACTION_SKILL;
    
    View::CharacterRenderer::GetInstance().PlayAction(m_activePet->GetId(), action);
}

void GameController::UpdateProjectiles(float dt) {
    Player* player = m_gameState ? m_gameState->GetLocalPlayer() : nullptr;

    for (auto& proj : m_petProjectiles) {
        if (!proj->IsActive()) continue;
        proj->Update(dt);

        // Resolve tile collision (stop on wall)
        Rectangle box = proj->GetBoundingBox();
        bool hitTile = false;
        for (const auto& tile : m_gameState->GetTiles(MapLayer::Main)) {
            if (!tile.solid) continue;
            Rectangle tr = { (float)tile.x * TILE_SIZE, (float)tile.y * TILE_SIZE, (float)TILE_SIZE, (float)TILE_SIZE };
            if (RectOverlap(box, tr)) {
                proj->OnHit();
                hitTile = true;
                break;
            }
        }
        if (hitTile) continue;

        // Check collision with enemies
        if (player) {
            for (const auto& e : m_gameState->GetAllEntities()) {
                if (!e || !e->IsActive()) continue;
                if (e->GetType() == EntityType::Enemy) {
                    auto* enemy = static_cast<Enemy*>(e.get());
                    if (enemy->GetState() == EnemyState::Dead) continue;
                    if (!RectOverlap(proj->GetBoundingBox(), enemy->GetBoundingBox())) continue;

                    enemy->TakeDamage(proj->GetDamage());
                    SoundManager::GetInstance().PlaySound("enemy_hurt");
                    View::FloatingTextManager::GetInstance().Emit(
                        enemy->GetPosition(), "-" + std::to_string(proj->GetDamage()), ORANGE, 1.0f);
                    if (!enemy->IsActive()) OnEntityRemoved(enemy);
                    proj->OnHit();
                    break;
                } else if (e->GetType() == EntityType::Boss) {
                    auto* boss = static_cast<Boss*>(e.get());
                    if (!boss->IsAlive() || boss->GetBossState() == BossState::Die || boss->GetBossState() == BossState::Transition) continue;
                    if (!RectOverlap(proj->GetBoundingBox(), boss->GetBoundingBox())) continue;

                    boss->TakeDamage(proj->GetDamage());
                    SoundManager::GetInstance().PlaySound("boss_hurt");
                    View::FloatingTextManager::GetInstance().Emit(
                        boss->GetPosition(), "-" + std::to_string(proj->GetDamage()), ORANGE, 1.0f);
                    if (!boss->IsActive()) OnEntityRemoved(boss);
                    proj->OnHit();
                    break;
                }
            }
        }
    }

    // Remove expired projectiles (and unregister visuals)
    m_petProjectiles.erase(
        std::remove_if(m_petProjectiles.begin(), m_petProjectiles.end(),
            [](const std::unique_ptr<Projectile>& p) {
                if (p->HasExpired()) {
                    View::EntityRenderer::GetInstance().Unregister(p->GetId());
                    return true;
                }
                return false;
            }),
        m_petProjectiles.end());

    // Clean up expired Boss/World projectiles in m_gameState
    if (m_gameState) {
        std::vector<int> expiredIds;
        for (const auto& e : m_gameState->GetAllEntities()) {
            if (e && e->GetType() == EntityType::Projectile && !e->IsActive()) {
                expiredIds.push_back(e->GetId());
            }
        }
        for (int id : expiredIds) {
            UnregisterEntityVisuals(id);
            m_gameState->RemoveEntity(id);
        }
    }
}

// ============================================================
// Player Skill Projectile Helpers
// ============================================================

bool GameController::CheckLineOfSight(Vector2 start, Vector2 end) const {
    if (!m_gameState) return false;

    for (const auto& tile : m_gameState->GetTiles(MapLayer::Main)) {
        if (!tile.solid) continue;
        const Rectangle tileRect = {
            static_cast<float>(tile.x * TILE_SIZE),
            static_cast<float>(tile.y * TILE_SIZE),
            static_cast<float>(TILE_SIZE),
            static_cast<float>(TILE_SIZE)
        };
        if (LineIntersectsRect(start, end, tileRect)) return false;
    }

    for (const auto& entity : m_gameState->GetAllEntities()) {
        if (!entity || !entity->IsActive() || entity->GetType() != EntityType::FakeWall) continue;
        const auto* wall = static_cast<const FakeWall*>(entity.get());
        if (!wall->IsDestroyed() && LineIntersectsRect(start, end, wall->GetBoundingBox())) {
            return false;
        }
    }
    return true;
}

bool GameController::FindNearestVisibleEnemyInCamera(Vector2 origin, Vector2& targetCenter) const {
    if (!m_gameState || m_camera.zoom <= 0.0f) return false;

    const float halfW = (SCREEN_WIDTH * 0.5f) / m_camera.zoom;
    const float halfH = (SCREEN_HEIGHT * 0.5f) / m_camera.zoom;
    const Rectangle viewport = {
        m_camera.target.x - halfW,
        m_camera.target.y - halfH,
        halfW * 2.0f,
        halfH * 2.0f
    };

    float nearestDistanceSquared = std::numeric_limits<float>::max();
    bool found = false;
    for (const auto& entity : m_gameState->GetAllEntities()) {
        if (!entity || !entity->IsActive()) continue;

        bool canTarget = false;
        if (entity->GetType() == EntityType::Enemy) {
            const auto* enemy = static_cast<const Enemy*>(entity.get());
            canTarget = enemy->GetState() != EnemyState::Dead;
        } else if (entity->GetType() == EntityType::Boss) {
            const auto* boss = static_cast<const Boss*>(entity.get());
            canTarget = boss->IsAlive()
                     && boss->GetBossState() != BossState::Die
                     && boss->GetBossState() != BossState::Transition;
        }
        if (!canTarget || !RectOverlap(entity->GetBoundingBox(), viewport)) continue;

        const Rectangle box = entity->GetBoundingBox();
        const Vector2 center = {box.x + box.width * 0.5f, box.y + box.height * 0.5f};
        if (!CheckLineOfSight(origin, center)) continue;

        const float dx = center.x - origin.x;
        const float dy = center.y - origin.y;
        const float distanceSquared = dx * dx + dy * dy;
        if (distanceSquared < nearestDistanceSquared) {
            nearestDistanceSquared = distanceSquared;
            targetCenter = center;
            found = true;
        }
    }
    return found;
}

Vector2 GameController::PlayerSkillOrigin(const Player* player) {
    // The player art is drawn about 2.3x the height of the physics box and is
    // anchored at the feet, so the drawn torso sits above the box top. Skills
    // launched from the box itself come out at the character's feet.
    constexpr float CHEST_ABOVE_BOX_CENTER = 27.0f;
    const Vector2 pos  = player->GetPosition();
    const Vector2 size = player->GetSize();
    const float edgeX = (player->GetDirection() == Direction::Right)
        ? pos.x + size.x
        : pos.x;
    return { edgeX, pos.y + size.y * 0.5f - CHEST_ABOVE_BOX_CENTER };
}

void GameController::SpawnPlayerProjectile(const char* atlasPath, Vector2 spawnCenter,
                                            Direction dir, int damage,
                                            float speed, float lifetime,
                                            float scale, bool facesLeft,
                                            Vector2 hitboxSize, DamageType element) {
    // Collision box size scales proportionally to visual scale.
    // Use a smaller base size (40x40) to prevent projectiles from clipping the floor upon spawning.
    float boxSize = 40.0f * scale;
    if (hitboxSize.x <= 0.0f || hitboxSize.y <= 0.0f) {
        hitboxSize = {boxSize, boxSize};
    }

    // Entity position is the box's top-left; callers pass the center so the
    // spawn point means the same thing regardless of the projectile's size.
    Vector2 spawnPos = {
        spawnCenter.x - hitboxSize.x * 0.5f,
        spawnCenter.y - hitboxSize.y * 0.5f
    };

    // A tall hitbox centered on the caster's chest can reach below their feet
    // and into the floor, and UpdatePlayerProjectiles despawns anything that
    // overlaps a solid tile -- the projectile would die on its spawn frame,
    // never drawing a single frame. Lift it clear before it goes live.
    auto overlapsSolid = [&](Vector2 topLeft) {
        const Rectangle box = {topLeft.x, topLeft.y, hitboxSize.x, hitboxSize.y};
        for (const auto& tile : m_gameState->GetTiles(MapLayer::Main)) {
            if (!tile.solid) continue;
            const Rectangle tr = {(float)tile.x * TILE_SIZE, (float)tile.y * TILE_SIZE,
                                  (float)TILE_SIZE, (float)TILE_SIZE};
            if (RectOverlap(box, tr)) return true;
        }
        return false;
    };
    for (int lift = 0; lift < 12 && overlapsSolid(spawnPos); ++lift) {
        spawnPos.y -= 8.0f;
    }

    auto proj = std::make_unique<Projectile>(
        spawnPos, hitboxSize,
        ProjectileType::Magic,
        dir, damage,
        m_gameState->GetLocalPlayer() ? m_gameState->GetLocalPlayer()->GetId() : 0);

    proj->SetLifetime(lifetime);
    float vx = (dir == Direction::Right) ? speed : -speed;
    proj->SetVelocity({vx, 0.0f});
    proj->SetScale(scale);
    // Callers that name an element (the Magic Caster's skills) keep it. Everyone
    // else inherits whatever infusion the player is running, so a Ninja shuriken
    // sets up the same reactions a Fireball does.
    if (Player* owner = m_gameState->GetLocalPlayer()) {
        proj->SetElement(element != DamageType::Physical
                             ? element : owner->GetAttackElement());
        proj->SetRemainingPierce(owner->GetCores().ProjectilePierceBonus());
    } else {
        proj->SetElement(element);
    }

    // If sprite naturally faces Left, flip when moving Right.
    // If sprite naturally faces Right, flip when moving Left.
    bool flipX = facesLeft ? (dir == Direction::Right) : (dir == Direction::Left);

    // Force rotation to 0 to prevent double-flipping from the Projectile constructor
    proj->SetRotation(0.0f);

    // Register visual with correct flip. Skill sheets draw the effect in the middle
    // of a large padded cell, so center the frame on the collision box; anchoring the
    // frame corner instead pushes the visual half a frame away from what it damages.
    View::EntityRenderer::GetInstance().RegisterAnimated(
        proj.get(), atlasPath, "default", {0, 0}, flipX, /*centerOnBounds=*/true);

    m_playerProjectiles.push_back(std::move(proj));
}

void GameController::SpawnLightningAt(Vector2 targetPos, int damage, float lifetime,
                                       const char* atlasPath, float rotation, float scale,
                                       DamageType element) {
    // Lightning appears instantly at target position.
    // Frame center adjustment based on scale.
    float half = 125.0f * scale; // Assuming average frame width/height is ~250px
    Vector2 centeredPos = {
        targetPos.x - half,
        targetPos.y - half
    };

    auto proj = std::make_unique<Projectile>(
        centeredPos, Vector2{30.0f, 60.0f},
        ProjectileType::Magic,
        Direction::None, damage,
        m_gameState->GetLocalPlayer() ? m_gameState->GetLocalPlayer()->GetId() : 0);

    proj->SetVelocity({0.0f, 0.0f});
    proj->SetLifetime(lifetime > 0.0f ? lifetime : 0.5f);
    proj->SetScale(scale);
    proj->SetElement(element);
    proj->SetRotation(rotation);

    View::EntityRenderer::GetInstance().RegisterAnimated(
        proj.get(), atlasPath, "default");

    // Deal damage immediately to any enemy overlapping targetPos
    // We use a logical 60x60 hitbox centered on the intended target, 
    // rather than the visual projectile's bounding box which might be offset.
    Rectangle hitBox = { targetPos.x - 30.0f, targetPos.y - 30.0f, 60.0f, 60.0f };
    for (Entity* e : QueryEntitiesInRect(hitBox)) {
        if (e->GetType() == EntityType::Enemy) {
            auto* enemy = static_cast<Enemy*>(e);
            if (enemy->GetState() == EnemyState::Dead) continue;
            if (!RectOverlap(hitBox, enemy->GetBoundingBox())) continue;
            ApplyElementalHit(enemy, damage, element);
            SoundManager::GetInstance().PlaySound("enemy_hurt");
            View::ParticleRenderer::GetInstance().EmitBurst(enemy->GetPosition(), 12, PURPLE);
            View::GameView::GetInstance().Shake(4.0f, 0.2f);
            if (!enemy->IsActive()) OnEntityRemoved(enemy);
        } else if (e->GetType() == EntityType::Boss) {
            auto* boss = static_cast<Boss*>(e);
            if (!boss->IsAlive() || boss->GetBossState() == BossState::Die || boss->GetBossState() == BossState::Transition) continue;
            if (!RectOverlap(hitBox, boss->GetBoundingBox())) continue;
            ApplyElementalHit(boss, damage, element);
            SoundManager::GetInstance().PlaySound("boss_hurt");
            View::ParticleRenderer::GetInstance().EmitBurst(boss->GetPosition(), 12, PURPLE);
            View::GameView::GetInstance().Shake(4.0f, 0.2f);
            if (!boss->IsActive()) OnEntityRemoved(boss);
        }
    }

    m_playerProjectiles.push_back(std::move(proj));
}

void GameController::RebuildSpatialIndex() {
    if (!m_gameState) return;
    std::vector<Entity*> live;
    live.reserve(m_gameState->GetAllEntities().size());
    for (const auto& e : m_gameState->GetAllEntities()) {
        if (e && e->IsActive()) live.push_back(e.get());
    }
    m_collision.Rebuild(live);
}

std::vector<Entity*> GameController::QueryEntitiesInRect(Rectangle range) const {
    std::vector<Entity*> result;
    for (Entity* e : m_collision.QueryRange(range)) {
        if (!e || !e->IsActive()) continue;
        // A node can hold the same entity in more than one leaf.
        if (std::find(result.begin(), result.end(), e) != result.end()) continue;
        result.push_back(e);
    }
    return result;
}

// The reaction reference, opened with C. Reads the live table so it can never
// disagree with what the simulation actually does.
void GameController::RenderElementCodex() const {
    if (!m_codexOpen) return;

    const int sw = GetScreenWidth();
    const int sh = GetScreenHeight();
    DrawRectangle(0, 0, sw, sh, Color{6, 5, 16, 232});

    const int panelW = std::min(940, sw - 60);
    const int panelH = std::min(620, sh - 60);
    const int px = (sw - panelW) / 2;
    const int py = (sh - panelH) / 2;

    DrawRectangle(px, py, panelW, panelH, Color{14, 12, 30, 245});
    DrawRectangleLinesEx({(float)px, (float)py, (float)panelW, (float)panelH},
                         2.0f, Color{120, 110, 190, 255});

    const char* title = "PHAN UNG NGUYEN TO";
    DrawText(title, px + (panelW - MeasureText(title, 28)) / 2, py + 18, 28, WHITE);
    const char* hint = "C de dong";
    DrawText(hint, px + panelW - MeasureText(hint, 14) - 18, py + 26, 14,
             Color{170, 170, 195, 255});

    // Element legend: which key casts which element, and the aura it leaves.
    int y = py + 62;
    struct LegendRow { const char* key; DamageType element; };
    static const LegendRow legend[] = {
        {"J", DamageType::Thunder},
        {"K", DamageType::Fire},
        {"U", DamageType::Water},
        {"H", DamageType::Void},
    };
    const int legendW = (panelW - 40) / 4;
    for (int i = 0; i < 4; ++i) {
        const ElementProfile& p = GetElementProfile(legend[i].element);
        const int x = px + 20 + i * legendW;
        DrawRectangle(x, y, legendW - 10, 54, Fade(p.color, 0.14f));
        DrawRectangleLinesEx({(float)x, (float)y, (float)legendW - 10, 54.0f},
                             1.0f, Fade(p.color, 0.8f));
        DrawText(legend[i].key, x + 10, y + 8, 20, WHITE);
        DrawText(p.name, x + 34, y + 10, 17, p.color);
        const std::string aura = std::string(StatusEffectName(p.aura)) + "  "
                               + std::to_string(static_cast<int>(p.auraDuration)) + "s";
        DrawText(aura.c_str(), x + 10, y + 33, 13, Color{200, 200, 220, 255});
    }

    // The table itself, two columns of six rows.
    y += 74;
    DrawText("Aura san co  +  Nguyen to danh vao  =  Phan ung",
             px + 20, y, 15, Color{175, 175, 200, 255});
    y += 24;

    const auto& rows = ReactionTable();
    const int rowH = 34;
    const int colW = (panelW - 40) / 2;
    for (size_t i = 0; i < rows.size(); ++i) {
        const auto& r = rows[i];
        const int col = static_cast<int>(i) % 2;
        const int line = static_cast<int>(i) / 2;
        const int x = px + 20 + col * colW;
        const int ry = y + line * rowH;

        DrawRectangle(x, ry, colW - 12, rowH - 4, Color{20, 17, 40, 220});
        DrawRectangle(x, ry, 3, rowH - 4, r.color);

        // "Aura + Element"
        const Color auraColor = StatusEffectColor(r.existing);
        const ElementProfile& inc = GetElementProfile(r.incoming);
        int tx = x + 12;
        DrawText(StatusEffectName(r.existing), tx, ry + 8, 14, auraColor);
        tx += MeasureText(StatusEffectName(r.existing), 14) + 6;
        DrawText("+", tx, ry + 8, 14, Color{150, 150, 170, 255});
        tx += 12;
        DrawText(inc.name, tx, ry + 8, 14, inc.color);

        // Reaction name and multiplier, right-aligned.
        char mult[16];
        std::snprintf(mult, sizeof(mult), "x%.1f", r.multiplier);
        const int multW = MeasureText(mult, 15);
        DrawText(mult, x + colW - 24 - multW, ry + 7, 15, WHITE);
        const int nameW = MeasureText(r.name, 14);
        DrawText(r.name, x + colW - 34 - multW - nameW, ry + 8, 14, r.color);
    }

    // Footer: the parts of the system that are not in the table.
    const int footY = py + panelH - 66;
    DrawText("Cung nguyen to danh lai: chi lam moi thoi gian, khong phan ung",
             px + 20, footY, 13, Color{165, 165, 190, 255});
    char splash[128];
    std::snprintf(splash, sizeof(splash),
                  "Phan ung ban %d%% sat thuong sang quai trong ban kinh %dpx",
                  static_cast<int>(ELEMENTAL_SPLASH_SHARE * 100.0f),
                  static_cast<int>(ELEMENTAL_SPLASH_RADIUS));
    DrawText(splash, px + 20, footY + 18, 13, Color{165, 165, 190, 255});
    char bossLine[128];
    std::snprintf(bossLine, sizeof(bossLine),
                  "Danh boss: phan ung x%d.  Hu Hoa: +%d%% sat thuong phai chiu",
                  static_cast<int>(ELEMENTAL_REACTION_BOSS_MULT),
                  static_cast<int>((ElementalSystem::CORRODE_VULNERABILITY - 1.0f) * 100.0f));
    DrawText(bossLine, px + 20, footY + 36, 13, Color{225, 190, 120, 255});
}

// Longest request wins, so a small hit landing during a big one's freeze can
// never cut it short.
void GameController::RequestHitStop(float seconds) {
    if (seconds <= 0.0f) return;
    m_hitStopTimer = std::max(m_hitStopTimer, std::min(seconds, HITSTOP_MAX));
}

// Every elemental hit lands here: the reaction is resolved against whatever
// aura the target already carries, the scaled damage is applied, and the
// readout floats over the target. Physical hits pass straight through with no
// reaction, so callers can funnel everything through this one path.
int GameController::ApplyElementalHit(Entity* target, int baseDamage, DamageType element) {
    if (!target) return 0;

    const int id = static_cast<int>(target->GetId());
    const bool isBoss = target->GetType() == EntityType::Boss;

    const Player* owner = m_gameState->GetLocalPlayer();
    const CoreLoadout* cores = owner ? &owner->GetCores() : nullptr;

    DamagePacket packet = ElementalSystem::ElementalPacket(baseDamage, element);
    // Lingering Aura core: the element sticks longer, widening the window to
    // come back and detonate it.
    if (cores) packet.effectDuration *= cores->AuraDurationMultiplier();

    const ReactionResult reaction = m_elemental.ApplyHit(id, packet);

    // Boss health pools are an order of magnitude above a normal enemy's, so a
    // reaction that reads as a burst on a mob barely dents a boss. Reactions --
    // and only reactions -- are scaled up to match, which makes setting up an
    // element and detonating it the way a caster fights a boss.
    int dealt = std::max(1, reaction.finalDamage);

    if (reaction.Reacted()) {
        if (cores) dealt = static_cast<int>(dealt * cores->ReactionMultiplier());
        if (isBoss) dealt = static_cast<int>(dealt * ELEMENTAL_REACTION_BOSS_MULT);
    }

    // Execution core: a wounded target takes more from everything.
    const auto* victim = static_cast<const Character*>(target);
    if (cores && victim->GetMaxHealth() > 0) {
        const float healthFraction =
            static_cast<float>(victim->GetHealth()) / victim->GetMaxHealth();
        if (healthFraction <= cores->ExecutionHealthThreshold()) {
            dealt = static_cast<int>(dealt * cores->ExecutionMultiplier());
        }
    }
    dealt = std::max(1, dealt);

    if (target->GetType() == EntityType::Enemy) {
        static_cast<Enemy*>(target)->TakeDamage(dealt);
    } else if (isBoss) {
        static_cast<Boss*>(target)->TakeDamage(dealt);
    } else {
        return 0;
    }

    // Physical hits keep the old yellow readout; elemental ones take the
    // element's colour so the two are told apart at a glance.
    const Color elementColor = (element == DamageType::Physical)
        ? YELLOW : GetElementProfile(element).color;
    View::FloatingTextManager::GetInstance().Emit(
        target->GetPosition(), "-" + std::to_string(dealt), elementColor, 1.0f);

    if (cores) {
        // Ember Edge: a chance to set the target burning regardless of what the
        // hit itself carried. Applied directly rather than as a hit, so it
        // seeds an aura instead of triggering a second reaction.
        const float chance = cores->EmberEdgeChance();
        if (chance > 0.0f && target->IsActive()
            && (rand() % 1000) < static_cast<int>(chance * 1000.0f)) {
            m_elemental.ApplyStatusEffect(
                id, StatusEffect::Burn, GetElementProfile(DamageType::Fire).auraDuration);
        }
    }

    if (reaction.Reacted()) {
        // A reaction is the moment worth reading, so it gets its own line
        // above the damage number plus a heavier hit-stop.
        const Vector2 above = { target->GetPosition().x,
                                target->GetPosition().y - 22.0f };
        View::FloatingTextManager::GetInstance().Emit(
            above, reaction.reactionName, reaction.displayColor, 1.5f);
        // A boss reaction is now worth ten of a mob's, so it hits the screen
        // that much harder too.
        View::ParticleRenderer::GetInstance().EmitBurst(
            target->GetPosition(), isBoss ? 44 : 22, reaction.displayColor);
        View::GameView::GetInstance().Shake(isBoss ? 11.0f : 6.0f,
                                            isBoss ? 0.40f : 0.25f);
        SoundManager::GetInstance().PlaySound("boss_phase");
        RequestHitStop(isBoss ? HITSTOP_REACTION_BOSS : HITSTOP_REACTION);
        // Rift Conflux core scales both how far the reaction reaches and how
        // much of it the neighbours take.
        const float splashScale = cores ? cores->SplashScale() : 1.0f;
        SplashDamage(target,
                     static_cast<int>(dealt * ELEMENTAL_SPLASH_SHARE * splashScale),
                     ELEMENTAL_SPLASH_RADIUS * splashScale, reaction.displayColor);
    } else if (dealt >= HITSTOP_HEAVY_THRESHOLD) {
        RequestHitStop(HITSTOP_HEAVY_HIT);
    }

    // Chain Spark core: every sixth landed hit arcs to nearby enemies.
    if (owner && m_gameState->GetLocalPlayer()
        && m_gameState->GetLocalPlayer()->GetCores().Has(CoreId::ChainSpark)) {
        if (m_gameState->GetLocalPlayer()->GetCores().RegisterHitAndCheckChain()) {
            const Color sparkColor{255, 235, 110, 255};
            SplashDamage(target,
                         static_cast<int>(dealt * GetCoreDef(CoreId::ChainSpark).magnitude),
                         CHAIN_SPARK_RADIUS, sparkColor);
            View::ParticleRenderer::GetInstance().EmitBurst(
                target->GetPosition(), 14, sparkColor);
        }
    }

    // Tint the sprite with whatever aura it is left carrying -- which is not
    // always the element that just hit it, so read it back rather than assuming.
    const StatusEffect aura = m_elemental.GetActiveEffect(id);
    switch (aura) {
        case StatusEffect::Burn:
            View::ElementalFX::GetInstance().SetElementTint(target->GetId(), DamageType::Fire);
            break;
        case StatusEffect::Wet:
            View::ElementalFX::GetInstance().SetElementTint(target->GetId(), DamageType::Water);
            break;
        case StatusEffect::Shocked:
            View::ElementalFX::GetInstance().SetElementTint(target->GetId(), DamageType::Thunder);
            break;
        case StatusEffect::Corroded:
            View::ElementalFX::GetInstance().SetElementTint(target->GetId(), DamageType::Void);
            break;
        default:
            View::ElementalFX::GetInstance().ClearElementTint(target->GetId());
            break;
    }

    return dealt;
}

// A reaction throws off enough energy to catch whoever is standing next to the
// target. Splash never reacts on its own -- it deals a share of the damage and
// leaves auras alone -- so a packed room cannot chain into a recursive blowup.
void GameController::SplashDamage(Entity* epicenter, int splashDamage,
                                  float radius, Color color) {
    if (!epicenter || !m_gameState || splashDamage <= 0 || radius <= 0.0f) return;

    const int splash = std::max(1, splashDamage);
    const Rectangle hub = epicenter->GetBoundingBox();
    const Vector2 center = {hub.x + hub.width * 0.5f, hub.y + hub.height * 0.5f};

    // Broad phase over the splash square, then an exact radius test inside it.
    const Rectangle splashBox = {center.x - radius, center.y - radius,
                                 radius * 2.0f, radius * 2.0f};
    for (Entity* entity : QueryEntitiesInRect(splashBox)) {
        if (entity == epicenter) continue;

        const EntityType type = entity->GetType();
        if (type != EntityType::Enemy && type != EntityType::Boss) continue;

        const Rectangle box = entity->GetBoundingBox();
        const Vector2 other = {box.x + box.width * 0.5f, box.y + box.height * 0.5f};
        if (Distance(center, other) > radius) continue;

        if (type == EntityType::Enemy) {
            auto* enemy = static_cast<Enemy*>(entity);
            if (enemy->GetState() == EnemyState::Dead) continue;
            enemy->TakeDamage(splash);
            if (!enemy->IsActive()) OnEntityRemoved(enemy);
        } else {
            auto* boss = static_cast<Boss*>(entity);
            if (!boss->IsAlive() || boss->GetBossState() == BossState::Die
                || boss->GetBossState() == BossState::Transition) continue;
            boss->TakeDamage(splash);
            if (!boss->IsActive()) OnEntityRemoved(boss);
        }

        View::FloatingTextManager::GetInstance().Emit(
            entity->GetPosition(), "-" + std::to_string(splash),
            Fade(color, 0.85f), 0.8f);
        View::ParticleRenderer::GetInstance().EmitBurst(entity->GetPosition(), 8, color);
    }
}

// Ticks every aura, applies the damage-over-time they accrued and pushes the
// resulting slow onto each affected character.
void GameController::UpdateElementalEffects(float dt) {
    if (!m_gameState) return;

    m_elemental.Update(dt);
    const auto tickDamage = m_elemental.DrainTickDamage();

    // An infused player is tinted with the element they are carrying, so the
    // buff is readable on the character and not only in the boon list.
    auto tintInfusion = [](Player* p) {
        if (!p) return;
        const DamageType element = p->GetAttackElement();
        if (element == DamageType::Physical) {
            View::ElementalFX::GetInstance().ClearElementTint(p->GetId());
        } else {
            View::ElementalFX::GetInstance().SetElementTint(p->GetId(), element);
        }
    };
    tintInfusion(m_gameState->GetLocalPlayer());
    if (m_localCoop) tintInfusion(m_gameState->GetSecondLocalPlayer());

    for (auto& entity : m_gameState->GetAllEntities()) {
        if (!entity || !entity->IsActive()) continue;
        const EntityType type = entity->GetType();
        if (type != EntityType::Enemy && type != EntityType::Boss) continue;

        auto* character = static_cast<Character*>(entity.get());
        const int id = static_cast<int>(entity->GetId());

        // Bosses shrug off the movement slow -- they are meant to keep coming
        // at the player -- but still take the full damage-over-time.
        character->SetSpeedScale(type == EntityType::Boss
                                     ? 1.0f
                                     : m_elemental.GetSpeedMultiplier(id));

        auto tick = tickDamage.find(id);
        if (tick == tickDamage.end() || tick->second <= 0) continue;

        const StatusEffect aura = m_elemental.GetActiveEffect(id);
        if (type == EntityType::Enemy) {
            auto* enemy = static_cast<Enemy*>(entity.get());
            if (enemy->GetState() == EnemyState::Dead) continue;
            enemy->TakeDamage(tick->second);
            if (!enemy->IsActive()) OnEntityRemoved(enemy);
        } else {
            auto* boss = static_cast<Boss*>(entity.get());
            if (!boss->IsAlive() || boss->GetBossState() == BossState::Die) continue;
            boss->TakeDamage(tick->second);
            if (!boss->IsActive()) OnEntityRemoved(boss);
        }
        View::FloatingTextManager::GetInstance().Emit(
            entity->GetPosition(), "-" + std::to_string(tick->second),
            StatusEffectColor(aura), 0.7f);
    }

    // Feed the status icons above each enemy.
    for (auto& entity : m_gameState->GetAllEntities()) {
        if (!entity || !entity->IsActive()) continue;
        const EntityType type = entity->GetType();
        if (type != EntityType::Enemy && type != EntityType::Boss) continue;
        const int id = static_cast<int>(entity->GetId());
        const StatusEffect aura = m_elemental.GetActiveEffect(id);
        if (aura == StatusEffect::None) {
            View::EnemyStatusRenderer::GetInstance().ClearStatus(entity->GetId());
        } else {
            const Rectangle box = entity->GetBoundingBox();
            View::EnemyStatusRenderer::GetInstance().SetStatus(
                entity->GetId(), {box.x + box.width * 0.5f, box.y},
                aura == StatusEffect::Burn,
                aura == StatusEffect::Wet,
                aura == StatusEffect::Shocked,
                aura == StatusEffect::Corroded);
        }
    }
}

void GameController::ClearElementalState(Entity* entity) {
    if (!entity) return;
    m_elemental.ClearEffects(static_cast<int>(entity->GetId()));
    View::ElementalFX::GetInstance().ClearElementTint(entity->GetId());
    View::EnemyStatusRenderer::GetInstance().ClearStatus(entity->GetId());
}

void GameController::UpdatePlayerProjectiles(float dt) {
    if (!m_gameState) return;

    for (auto& proj : m_playerProjectiles) {
        if (!proj->IsActive()) continue;
        proj->Update(dt);

        Vector2 vel = proj->GetVelocity();
        bool isStationary = (std::abs(vel.x) < 1.0f && std::abs(vel.y) < 1.0f);

        // Resolve tile collision (stop on wall)
        Rectangle box = proj->GetBoundingBox();

        // A fake wall is a solid tile with an entity sitting on it, so it has to
        // be resolved before the tile sweep below -- otherwise the shot dies on
        // the very tile it is meant to clear. Fire burns through: the Magic
        // Caster's K is the only skill that always carries it, and melee classes
        // already break these walls by hitting them, so this is what opens the
        // secrets up to a ranged-only build.
        if (proj->GetElement() == DamageType::Fire) {
            for (auto& e : m_gameState->GetAllEntities()) {
                if (!e || !e->IsActive() || e->GetType() != EntityType::FakeWall) continue;
                auto* wall = static_cast<FakeWall*>(e.get());
                if (wall->IsDestroyed()) continue;
                if (!RectOverlap(box, wall->GetBoundingBox())) continue;

                // One tick per wall, so a shot crossing the box cannot burn it
                // down several times over on consecutive frames.
                if (proj->HasHitEntity(wall->GetId())) continue;
                proj->MarkHitEntity(wall->GetId());

                wall->TakeDamage(proj->GetDamage());
                SoundManager::GetInstance().PlaySound("stone_hit");
                View::ParticleRenderer::GetInstance().EmitBurst(
                    wall->GetPosition(), 10, ORANGE);
                View::GameView::GetInstance().Shake(2.0f, 0.1f);

                if (wall->IsDestroyed()) {
                    SoundManager::GetInstance().PlaySound("fake_wall_break");
                    m_gameState->RemoveTileAt(MapLayer::Main, wall->GetTileX(), wall->GetTileY());
                    OnEntityRemoved(wall);
                }
            }
        }

        bool hitTile = false;

        if (!isStationary) {
            for (const auto& tile : m_gameState->GetTiles(MapLayer::Main)) {
                if (!tile.solid) continue;
                Rectangle tr = { (float)tile.x * TILE_SIZE, (float)tile.y * TILE_SIZE,
                                  (float)TILE_SIZE, (float)TILE_SIZE };
                if (RectOverlap(box, tr)) {
                    proj->OnHit();
                    hitTile = true; break; 
                }
            }
        }
        if (hitTile) continue;

        // Check collision with enemies (skip lightning which already dealt damage)
        if (isStationary) continue;

        for (auto& e : m_gameState->GetAllEntities()) {
            if (!e || !e->IsActive()) continue;
            if (e->GetType() == EntityType::Enemy) {
                auto* enemy = static_cast<Enemy*>(e.get());
                if (enemy->GetState() == EnemyState::Dead) continue;
                if (!RectOverlap(proj->GetBoundingBox(), enemy->GetBoundingBox())) continue;

                // Each target is hit at most once, so a piercing shot cannot
                // tick the same enemy every frame while passing through it.
                if (proj->HasHitEntity(enemy->GetId())) continue;
                proj->MarkHitEntity(enemy->GetId());

                ApplyElementalHit(enemy, proj->GetDamage(), proj->GetElement());
                SoundManager::GetInstance().PlaySound("enemy_hurt");
                View::ParticleRenderer::GetInstance().EmitBurst(enemy->GetPosition(), 8, ORANGE);
                View::GameView::GetInstance().Shake(3.0f, 0.15f);
                if (!enemy->IsActive()) OnEntityRemoved(enemy);

                // Gravity Lens: carry on through if there is pierce left.
                if (proj->ConsumePierce()) continue;
                proj->OnHit();
                break;
            } else if (e->GetType() == EntityType::Boss) {
                auto* boss = static_cast<Boss*>(e.get());
                if (!boss->IsAlive() || boss->GetBossState() == BossState::Die || boss->GetBossState() == BossState::Transition) continue;
                if (!RectOverlap(proj->GetBoundingBox(), boss->GetBoundingBox())) continue;

                if (proj->HasHitEntity(boss->GetId())) continue;
                proj->MarkHitEntity(boss->GetId());

                ApplyElementalHit(boss, proj->GetDamage(), proj->GetElement());
                SoundManager::GetInstance().PlaySound("boss_hurt");
                View::ParticleRenderer::GetInstance().EmitBurst(boss->GetPosition(), 8, ORANGE);
                View::GameView::GetInstance().Shake(3.0f, 0.15f);
                if (!boss->IsActive()) OnEntityRemoved(boss);

                if (proj->ConsumePierce()) continue;
                proj->OnHit();
                break;
            }
        }
    }

    // Remove expired
    m_playerProjectiles.erase(
        std::remove_if(m_playerProjectiles.begin(), m_playerProjectiles.end(),
            [](const std::unique_ptr<Projectile>& p) {
                if (p->HasExpired()) {
                    View::EntityRenderer::GetInstance().Unregister(p->GetId());
                    return true;
                }
                return false;
            }),
        m_playerProjectiles.end());
}

bool GameController::FindRandomSpawnPoint(Vector2 playerPos, Vector2& outPos) const {
    if (!m_gameState) return false;

    // A spawn point is a solid tile with clear space above it, far enough from
    // the player that the enemy is not seen popping into existence.
    const auto& tiles = m_gameState->GetTiles(MapLayer::Main);
    if (tiles.empty()) return false;

    std::vector<const Tile*> solid;
    solid.reserve(tiles.size());
    for (const auto& t : tiles) {
        if (!t.solid) continue;
        const float wx = t.x * TILE_SIZE;
        const float dist = std::abs(wx - playerPos.x);
        if (dist < RANDOM_SPAWN_MIN_DISTANCE || dist > RANDOM_SPAWN_MAX_DISTANCE) continue;
        solid.push_back(&t);
    }
    if (solid.empty()) return false;

    auto isSolidAt = [&](int tx, int ty) {
        for (const auto& t : tiles) {
            if (t.solid && t.x == tx && t.y == ty) return true;
        }
        return false;
    };

    // Sample a handful of candidates rather than scanning every tile.
    for (int attempt = 0; attempt < 24; ++attempt) {
        const Tile* t = solid[static_cast<size_t>(rand()) % solid.size()];
        // Needs two tiles of headroom for the enemy body.
        if (isSolidAt(t->x, t->y - 1) || isSolidAt(t->x, t->y - 2)) continue;
        outPos = { t->x * TILE_SIZE, (t->y - 2) * TILE_SIZE };
        return true;
    }
    return false;
}

void GameController::UpdateRandomEnemySpawns(float dt) {
    if (!m_gameState || IsInBossArena()) return;
    Player* player = m_gameState->GetLocalPlayer();
    if (!player || !player->IsAlive()) return;

    // Drop ids that are gone so the cap tracks enemies actually alive.
    m_randomSpawnIds.erase(
        std::remove_if(m_randomSpawnIds.begin(), m_randomSpawnIds.end(),
            [&](int id) {
                for (const auto& e : m_gameState->GetAllEntities()) {
                    if (e && e->GetId() == id) return !e->IsActive();
                }
                return true;   // entity no longer exists
            }),
        m_randomSpawnIds.end());

    m_randomSpawnTimer -= dt;
    if (m_randomSpawnTimer > 0.0f) return;

    const float span = RANDOM_SPAWN_MAX_INTERVAL - RANDOM_SPAWN_MIN_INTERVAL;
    m_randomSpawnTimer = RANDOM_SPAWN_MIN_INTERVAL
                       + static_cast<float>(rand() % 1000) / 1000.0f * span;

    if (static_cast<int>(m_randomSpawnIds.size()) >= RANDOM_SPAWN_MAX_ALIVE) return;

    Vector2 spawnPos{};
    if (!FindRandomSpawnPoint(player->GetPosition(), spawnPos)) return;

    // Mostly melee, occasionally ranged; flying is left to hand-placed encounters.
    const int roll = rand() % 100;
    const EnemyType type = (roll < 70) ? EnemyType::Melee : EnemyType::Ranged;

    auto enemy = EnemyFactory::CreateEnemy(spawnPos, type);
    if (!enemy) return;
    Enemy* raw = enemy.get();
    m_gameState->AddEntity(std::move(enemy));
    RegisterEnemyVisuals(raw);
    m_randomSpawnIds.push_back(raw->GetId());
}

bool GameController::FindBuffSpawnPoint(Vector2 playerPos, Vector2& outPos) const {
    if (!m_gameState) return false;
    const auto& tiles = m_gameState->GetTiles(MapLayer::Main);
    if (tiles.empty()) return false;

    auto isSolidAt = [&](int tx, int ty) {
        for (const auto& t : tiles) {
            if (t.solid && t.x == tx && t.y == ty) return true;
        }
        return false;
    };

    // Reachable spot: standing room above a floor tile, inside the arena and
    // close enough that going for it is a real decision, not a trek.
    std::vector<const Tile*> floors;
    for (const auto& t : tiles) {
        if (!t.solid) continue;
        const float wx = t.x * TILE_SIZE;
        const float d = std::abs(wx - playerPos.x);
        if (d < TILE_SIZE * 3.0f || d > TILE_SIZE * 16.0f) continue;
        floors.push_back(&t);
    }
    if (floors.empty()) return false;

    for (int attempt = 0; attempt < 24; ++attempt) {
        const Tile* t = floors[static_cast<size_t>(rand()) % floors.size()];
        if (isSolidAt(t->x, t->y - 1) || isSolidAt(t->x, t->y - 2)) continue;
        outPos = { t->x * TILE_SIZE + TILE_SIZE * 0.25f,
                   (t->y - 1) * TILE_SIZE + TILE_SIZE * 0.3f };
        return true;
    }
    return false;
}

void GameController::CollectBuff(Player* player, BuffPickup* orb) {
    if (!player || !orb) return;
    const BuffDef& def = GetBuffDef(orb->GetBuffType());
    player->ApplyBuff(orb->GetBuffType());
    orb->SetActive(false);
    SoundManager::GetInstance().PlaySound("item_pickup");
    View::FloatingTextManager::GetInstance().Emit(
        player->GetPosition(), def.name, def.color, 1.4f);
}

void GameController::UpdateBuffPickups(float dt) {
    if (!m_gameState) return;

    // Boons only exist during a boss fight.
    if (!IsBossFightActive()) {
        if (!m_buffPickups.empty()) m_buffPickups.clear();
        m_buffSpawnTimer = BUFF_SPAWN_INTERVAL;
        return;
    }

    Player* player = m_gameState->GetLocalPlayer();
    Player* second = m_localCoop ? m_gameState->GetSecondLocalPlayer() : nullptr;
    if (!player) return;

    for (auto& orb : m_buffPickups) {
        if (!orb || !orb->IsActive()) continue;
        orb->Update(dt);
        if (player->IsAlive() && RectOverlap(orb->GetBoundingBox(), player->GetBoundingBox())) {
            CollectBuff(player, orb.get());
        } else if (second && second->IsAlive()
                   && RectOverlap(orb->GetBoundingBox(), second->GetBoundingBox())) {
            CollectBuff(second, orb.get());
        }
    }
    m_buffPickups.erase(
        std::remove_if(m_buffPickups.begin(), m_buffPickups.end(),
                       [](const std::unique_ptr<BuffPickup>& o) { return !o || !o->IsActive(); }),
        m_buffPickups.end());

    m_buffSpawnTimer -= dt;
    if (m_buffSpawnTimer > 0.0f) return;
    m_buffSpawnTimer = BUFF_SPAWN_INTERVAL;

    if (static_cast<int>(m_buffPickups.size()) >= BUFF_MAX_ON_FIELD) return;

    Vector2 pos{};
    if (!FindBuffSpawnPoint(player->GetPosition(), pos)) return;
    m_buffPickups.push_back(std::make_unique<BuffPickup>(pos, RollBuff()));
}

void GameController::RenderBuffPickups() const {
    if (m_buffPickups.empty()) return;

    for (const auto& orb : m_buffPickups) {
        if (!orb || !orb->IsActive()) continue;
        const BuffDef& def = GetBuffDef(orb->GetBuffType());
        const Rectangle box = orb->GetBoundingBox();
        const Vector2 world = { box.x + box.width * 0.5f,
                                box.y + box.height * 0.5f + orb->GetBobOffset() };
        const Vector2 screen = GetWorldToScreen2D(world, m_camera);
        const float radius = box.width * 0.5f * m_camera.zoom;

        // Fade and shrink over the last couple of seconds so an orb about to
        // expire is readable at a glance.
        const float life = orb->GetLifeRatio();
        const float fade = (life < 0.2f) ? life / 0.2f : 1.0f;

        DrawCircleV(screen, radius * 1.5f, Fade(def.color, 0.18f * fade));
        DrawCircleV(screen, radius, Fade(def.color, 0.85f * fade));
        DrawCircleLinesV(screen, radius, Fade(WHITE, 0.9f * fade));

        const int fs = 12;
        const int tw = MeasureText(def.name, fs);
        DrawText(def.name, static_cast<int>(screen.x - tw * 0.5f),
                 static_cast<int>(screen.y - radius - fs - 4), fs, Fade(WHITE, fade));
    }
}

// ---------------------------------------------------------------------------
// Boon draft. Every 10-15 seconds of a boss fight the action freezes and three
// boons are offered; 1, 2 or 3 takes one and the fight resumes. This is the
// guaranteed reward -- the orbs above are ambient on top of it.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Core draft. Survival3D hands out an upgrade choice every wave; the campaign
// has no waves, so a draft is earned by clearing a run of enemies and by
// killing a boss. What is picked is kept for the rest of the run.
// ---------------------------------------------------------------------------

void GameController::OnEnemyDefeatedForCores(bool wasBoss) {
    if (wasBoss) {
        // A boss is worth a draft on its own, rolled at boss rarity.
        ++m_pendingCoreDrafts;
        m_coreDraftBoss = true;
        return;
    }
    if (++m_killsTowardCore >= CORE_DRAFT_KILL_INTERVAL) {
        m_killsTowardCore = 0;
        ++m_pendingCoreDrafts;
    }
}

void GameController::OpenCoreDraft(bool bossReward) {
    Player* player = m_gameState ? m_gameState->GetLocalPlayer() : nullptr;
    if (!player) return;

    const int classLock = static_cast<int>(player->GetCharacterClass());
    m_coreOffer = player->GetCores().RollDraft(CORE_DRAFT_COUNT, bossReward, classLock);
    if (m_coreOffer.empty()) {
        // Everything is maxed out -- nothing to offer, so do not freeze the game
        // on an empty panel.
        m_pendingCoreDrafts = 0;
        return;
    }

    m_coreDraftOpen = true;
    m_coreDraftAnim = 0.0f;
    m_gameState->SetTimerRunning(false);
    SoundManager::GetInstance().PlaySound("boss_phase");
}

void GameController::TakeCoreOffer(int index) {
    if (!m_coreDraftOpen) return;
    if (index < 0 || index >= static_cast<int>(m_coreOffer.size())) return;

    const CoreId chosen = m_coreOffer[index];
    const CoreDefinition& def = GetCoreDef(chosen);

    auto grant = [&](Player* p) {
        if (!p) return;
        // The draft is rolled for player one's class, so in co-op a class-locked
        // core must not be handed to a partner of a different class -- its
        // effect reaches into a skill set they do not have.
        if (def.classLock >= 0
            && def.classLock != static_cast<int>(p->GetCharacterClass())) {
            return;
        }
        p->AcquireCore(chosen);
        View::FloatingTextManager::GetInstance().Emit(
            p->GetPosition(), def.name, RarityColor(def.rarity), 1.8f);
    };
    grant(m_gameState->GetLocalPlayer());
    if (m_localCoop) grant(m_gameState->GetSecondLocalPlayer());

    m_coreDraftOpen = false;
    m_coreDraftBoss = false;
    m_coreOffer.clear();
    if (m_pendingCoreDrafts > 0) --m_pendingCoreDrafts;
    m_gameState->SetTimerRunning(true);
    SoundManager::GetInstance().PlaySound("item_pickup");
}

void GameController::UpdateCoreDraft(float dt) {
    if (!m_gameState) return;

    if (m_coreDraftOpen) {
        m_coreDraftAnim += dt;
        if (IsKeyPressed(KEY_X) || IsKeyPressed(KEY_ONE)   || IsKeyPressed(KEY_KP_1)) {
            TakeCoreOffer(0);
        } else if (IsKeyPressed(KEY_Y) || IsKeyPressed(KEY_TWO)   || IsKeyPressed(KEY_KP_2)) {
            TakeCoreOffer(1);
        } else if (IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_THREE) || IsKeyPressed(KEY_KP_3)) {
            TakeCoreOffer(2);
        }
        return;
    }

    if (m_pendingCoreDrafts <= 0) return;

    // Held back until the player is alive and nothing else owns the screen, so
    // a draft never opens over a death or another overlay.
    const Player* player = m_gameState->GetLocalPlayer();
    if (!player || !player->IsAlive()) return;
    if (m_buffOfferOpen || m_codexOpen || m_paused) return;

    OpenCoreDraft(m_coreDraftBoss);
}

void GameController::RenderCoreDraft() const {
    if (!m_coreDraftOpen || m_coreOffer.empty()) return;

    const int sw = GetScreenWidth();
    const int sh = GetScreenHeight();

    const float fade = std::min(1.0f, m_coreDraftAnim / 0.18f);
    DrawRectangle(0, 0, sw, sh, Fade(Color{5, 4, 14, 255}, 0.80f * fade));

    const char* title = m_coreDraftBoss ? "LOI BOSS - CHON MOT" : "CHON MOT LOI";
    const int titleSize = 34;
    DrawText(title, (sw - MeasureText(title, titleSize)) / 2,
             static_cast<int>(sh * 0.18f), titleSize,
             Fade(m_coreDraftBoss ? Color{255, 190, 75, 255} : WHITE, fade));

    const char* hint = "Nhan X / Y / Z de chon    -    giu den het man";
    DrawText(hint, (sw - MeasureText(hint, 16)) / 2,
             static_cast<int>(sh * 0.18f) + titleSize + 10, 16,
             Fade(Color{190, 190, 210, 255}, fade));

    const int count = static_cast<int>(m_coreOffer.size());
    const int cardW = 250, cardH = 238, gap = 26;
    const int totalW = count * cardW + (count - 1) * gap;
    const int startX = (sw - totalW) / 2;
    const int cardY  = static_cast<int>(sh * 0.36f);

    const Player* player = m_gameState ? m_gameState->GetLocalPlayer() : nullptr;

    static const char* kKeys[] = {"X", "Y", "Z"};
    for (int i = 0; i < count; ++i) {
        const CoreDefinition& def = GetCoreDef(m_coreOffer[i]);
        const Color accent = RarityColor(def.rarity);
        const int x = startX + i * (cardW + gap);

        const float appear = std::clamp((m_coreDraftAnim - i * 0.07f) / 0.22f, 0.0f, 1.0f);
        const int y = cardY + static_cast<int>((1.0f - appear) * 28.0f);
        const float a = appear * fade;

        DrawRectangle(x, y, cardW, cardH, Fade(Color{13, 11, 28, 255}, 0.96f * a));
        DrawRectangleLinesEx({(float)x, (float)y, (float)cardW, (float)cardH},
                             2.0f, Fade(accent, a));
        DrawRectangle(x, y, cardW, 7, Fade(accent, a));
        // Legendary cards get an outer glow so the good pull is unmistakable.
        if (def.rarity == CoreRarity::Legendary) {
            DrawRectangleLinesEx({(float)x - 3, (float)y - 3,
                                  (float)cardW + 6, (float)cardH + 6},
                                 1.0f, Fade(accent, 0.45f * a));
        }

        // Key badge.
        DrawCircle(x + 32, y + 44, 20.0f, Fade(accent, 0.20f * a));
        DrawCircleLines(x + 32, y + 44, 20.0f, Fade(accent, a));
        DrawText(kKeys[i], x + 32 - MeasureText(kKeys[i], 22) / 2, y + 33, 22,
                 Fade(WHITE, a));

        // Rarity label above the name.
        DrawText(RarityName(def.rarity), x + 62, y + 26, 12, Fade(accent, a));
        DrawText(def.name, x + 62, y + 42, 19, Fade(WHITE, a));

        // Description, wrapped by hand at the card width.
        const int descSize = 14;
        std::string desc = def.description;
        int lineY = y + 92;
        while (!desc.empty()) {
            size_t cut = desc.size();
            while (cut > 0 && MeasureText(desc.substr(0, cut).c_str(), descSize) > cardW - 32) {
                const size_t space = desc.rfind(' ', cut - 1);
                if (space == std::string::npos) { --cut; break; }
                cut = space;
            }
            DrawText(desc.substr(0, cut).c_str(), x + 16, lineY, descSize,
                     Fade(Color{205, 205, 225, 255}, a));
            lineY += descSize + 5;
            desc = (cut < desc.size()) ? desc.substr(cut + 1) : std::string();
        }

        // Stack readout, so a repeat offer reads as an upgrade not a duplicate.
        if (player && def.maxStacks > 1) {
            const int held = player->GetCores().GetStack(def.id);
            const std::string stack = "Dang co " + std::to_string(held)
                                    + " / " + std::to_string(def.maxStacks);
            DrawText(stack.c_str(), x + 16, y + cardH - 28, 13, Fade(accent, a));
        }
    }
}

// Compact list of the cores held this run, under the boon bars.
void GameController::RenderCoreHud() const {
    if (!m_gameState) return;
    const Player* player = m_gameState->GetLocalPlayer();
    if (!player || player->GetCores().Empty()) return;

    const auto& cores = player->GetCores();
    const int x = 18;
    // Sits below the boon rows, which grow downward from y = 96.
    int y = 96 + static_cast<int>(player->GetBuffs().size()) * 20 + 6;

    for (CoreId id : cores.Owned()) {
        const CoreDefinition& def = GetCoreDef(id);
        const Color accent = RarityColor(def.rarity);
        const int stack = cores.GetStack(id);

        const std::string label = (stack > 1)
            ? std::string(def.name) + " x" + std::to_string(stack)
            : std::string(def.name);
        const int w = std::max(132, MeasureText(label.c_str(), 11) + 16);

        DrawRectangle(x, y, w, 15, Color{10, 8, 22, 175});
        DrawRectangle(x, y, 3, 15, accent);
        DrawText(label.c_str(), x + 8, y + 2, 11, Fade(WHITE, 0.92f));
        y += 17;
    }
}

bool GameController::IsBossFightActive() const {
    if (!m_gameState) return false;
    const Player* player = m_gameState->GetLocalPlayer();
    if (!player || !player->IsAlive()) return false;

    const Rectangle pb = player->GetBoundingBox();
    const Vector2 playerCenter = {pb.x + pb.width * 0.5f, pb.y + pb.height * 0.5f};

    for (const auto& entity : m_gameState->GetAllEntities()) {
        if (!entity || !entity->IsActive()) continue;
        if (entity->GetType() != EntityType::Boss) continue;

        const auto* boss = static_cast<const Boss*>(entity.get());
        if (!boss->IsAlive()) continue;

        // Distance-gated so the timer does not run while the player is still
        // making their way across the level toward the arena.
        const Rectangle bb = boss->GetBoundingBox();
        const Vector2 bossCenter = {bb.x + bb.width * 0.5f, bb.y + bb.height * 0.5f};
        if (Distance(playerCenter, bossCenter) <= BOSS_FIGHT_ENGAGE_RANGE) return true;
    }
    return false;
}

float GameController::NextBuffOfferDelay() const {
    const float span = BUFF_OFFER_MAX_DELAY - BUFF_OFFER_MIN_DELAY;
    return BUFF_OFFER_MIN_DELAY + (static_cast<float>(rand() % 1001) / 1000.0f) * span;
}

void GameController::OpenBuffOffer() {
    m_buffOffer.clear();

    // Weighted rolls, rejecting repeats so the three cards are always distinct.
    for (int attempt = 0; attempt < 64
                          && static_cast<int>(m_buffOffer.size()) < BUFF_OFFER_COUNT; ++attempt) {
        const BuffType candidate = RollBuff();
        if (std::find(m_buffOffer.begin(), m_buffOffer.end(), candidate) == m_buffOffer.end()) {
            m_buffOffer.push_back(candidate);
        }
    }
    // Weighted rolling can starve if the table is tiny; top up in order.
    for (const auto& def : BuffDefs()) {
        if (static_cast<int>(m_buffOffer.size()) >= BUFF_OFFER_COUNT) break;
        if (std::find(m_buffOffer.begin(), m_buffOffer.end(), def.type) == m_buffOffer.end()) {
            m_buffOffer.push_back(def.type);
        }
    }
    if (m_buffOffer.empty()) return;

    m_buffOfferOpen = true;
    m_buffOfferAnim = 0.0f;
    m_gameState->SetTimerRunning(false);
    SoundManager::GetInstance().PlaySound("boss_phase");
}

void GameController::TakeBuffOffer(int index) {
    if (!m_buffOfferOpen) return;
    if (index < 0 || index >= static_cast<int>(m_buffOffer.size())) return;

    const BuffType chosen = m_buffOffer[index];
    const BuffDef& def = GetBuffDef(chosen);

    // In local co-op both players are in the same fight, so both take the pick.
    auto grant = [&](Player* p) {
        if (!p || !p->IsAlive()) return;
        p->ApplyBuff(chosen);
        View::FloatingTextManager::GetInstance().Emit(
            p->GetPosition(), def.name, def.color, 1.6f);
    };
    grant(m_gameState->GetLocalPlayer());
    if (m_localCoop) grant(m_gameState->GetSecondLocalPlayer());

    m_buffOfferOpen = false;
    m_buffOffer.clear();
    m_buffOfferTimer = NextBuffOfferDelay();
    m_gameState->SetTimerRunning(true);
    SoundManager::GetInstance().PlaySound("item_pickup");
}

void GameController::UpdateBuffOffer(float dt) {
    if (!m_gameState) return;

    // Only runs while a boss fight is live; the boss dying or the player
    // walking away cancels a pending draft.
    if (!IsBossFightActive()) {
        if (m_buffOfferOpen) {
            m_buffOfferOpen = false;
            m_buffOffer.clear();
            m_gameState->SetTimerRunning(true);
        }
        m_buffOfferTimer = NextBuffOfferDelay();
        return;
    }

    if (m_buffOfferOpen) {
        m_buffOfferAnim += dt;
        // X / Y / Z are the labelled keys; the number row still works as a
        // fallback for anyone who reached for it out of habit.
        if (IsKeyPressed(KEY_X) || IsKeyPressed(KEY_ONE)   || IsKeyPressed(KEY_KP_1)) {
            TakeBuffOffer(0);
        } else if (IsKeyPressed(KEY_Y) || IsKeyPressed(KEY_TWO)   || IsKeyPressed(KEY_KP_2)) {
            TakeBuffOffer(1);
        } else if (IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_THREE) || IsKeyPressed(KEY_KP_3)) {
            TakeBuffOffer(2);
        }
        return;
    }

    // A dead player cannot pick, so hold the timer until they are back up.
    const Player* player = m_gameState->GetLocalPlayer();
    if (!player || !player->IsAlive()) return;

    m_buffOfferTimer -= dt;
    if (m_buffOfferTimer <= 0.0f) OpenBuffOffer();
}

void GameController::RenderBuffOffer() const {
    if (!m_buffOfferOpen || m_buffOffer.empty()) return;

    const int sw = GetScreenWidth();
    const int sh = GetScreenHeight();

    // Dim the fight behind the cards, easing in so the freeze does not snap.
    const float fade = std::min(1.0f, m_buffOfferAnim / 0.18f);
    DrawRectangle(0, 0, sw, sh, Fade(Color{6, 5, 16, 255}, 0.72f * fade));

    const char* title = "CHON MOT AN HUE";
    const int titleSize = 34;
    DrawText(title, (sw - MeasureText(title, titleSize)) / 2,
             static_cast<int>(sh * 0.22f), titleSize, Fade(WHITE, fade));

    const char* hint = "Nhan X / Y / Z de chon    -    C: bang phan ung nguyen to";
    const int hintSize = 16;
    DrawText(hint, (sw - MeasureText(hint, hintSize)) / 2,
             static_cast<int>(sh * 0.22f) + titleSize + 10, hintSize,
             Fade(Color{190, 190, 210, 255}, fade));

    const int count = static_cast<int>(m_buffOffer.size());
    const int cardW = 236, cardH = 190, gap = 26;
    const int totalW = count * cardW + (count - 1) * gap;
    const int startX = (sw - totalW) / 2;
    const int cardY  = static_cast<int>(sh * 0.40f);

    for (int i = 0; i < count; ++i) {
        const BuffDef& def = GetBuffDef(m_buffOffer[i]);
        const int x = startX + i * (cardW + gap);

        // Cards rise into place one after another.
        const float appear = std::clamp((m_buffOfferAnim - i * 0.07f) / 0.22f, 0.0f, 1.0f);
        const int y = cardY + static_cast<int>((1.0f - appear) * 26.0f);
        const float a = appear * fade;

        DrawRectangle(x, y, cardW, cardH, Fade(Color{14, 12, 30, 255}, 0.95f * a));
        DrawRectangleLinesEx({(float)x, (float)y, (float)cardW, (float)cardH},
                             2.0f, Fade(def.color, a));
        // Colour band so the boon is recognisable before the text is read.
        DrawRectangle(x, y, cardW, 6, Fade(def.color, a));

        // Key badge.
        static const char* kOfferKeys[] = {"X", "Y", "Z"};
        const std::string key = (i < 3) ? kOfferKeys[i] : std::to_string(i + 1);
        DrawCircle(x + 30, y + 40, 19.0f, Fade(def.color, 0.22f * a));
        DrawCircleLines(x + 30, y + 40, 19.0f, Fade(def.color, a));
        DrawText(key.c_str(), x + 30 - MeasureText(key.c_str(), 22) / 2, y + 29, 22,
                 Fade(WHITE, a));

        DrawText(def.name, x + 60, y + 30, 21, Fade(WHITE, a));

        // Description, wrapped by hand at the card width.
        const int descSize = 14;
        std::string desc = def.description;
        int lineY = y + 82;
        while (!desc.empty()) {
            size_t cut = desc.size();
            while (cut > 0 && MeasureText(desc.substr(0, cut).c_str(), descSize) > cardW - 32) {
                const size_t space = desc.rfind(' ', cut - 1);
                if (space == std::string::npos) { --cut; break; }
                cut = space;
            }
            DrawText(desc.substr(0, cut).c_str(), x + 16, lineY, descSize,
                     Fade(Color{205, 205, 225, 255}, a));
            lineY += descSize + 5;
            desc = (cut < desc.size()) ? desc.substr(cut + 1) : std::string();
        }

        const std::string life = (def.duration > 0.0f)
            ? std::to_string(static_cast<int>(def.duration)) + "s"
            : std::string("Tuc thi");
        DrawText(life.c_str(), x + 16, y + cardH - 26, 13, Fade(def.color, a));
    }
}

void GameController::RenderBuffHud() const {
    if (!m_gameState) return;
    const Player* player = m_gameState->GetLocalPlayer();
    if (!player) return;
    const auto& buffs = player->GetBuffs();
    if (buffs.empty()) return;

    // Stacked under the health bar, one row per running boon with a drain bar.
    const int x = 18;
    int y = 96;
    for (const auto& b : buffs) {
        const BuffDef& def = GetBuffDef(b.type);
        const float ratio = (b.duration > 0.0f) ? b.timer / b.duration : 0.0f;
        const int w = 132, h = 16;

        DrawRectangle(x, y, w, h, Color{10, 8, 22, 190});
        DrawRectangle(x, y, static_cast<int>(w * ratio), h, Fade(def.color, 0.55f));
        DrawRectangleLines(x, y, w, h, Fade(def.color, 0.9f));
        DrawText(def.name, x + 6, y + 3, 11, WHITE);

        const std::string secs = std::to_string(static_cast<int>(b.timer + 0.99f)) + "s";
        DrawText(secs.c_str(), x + w - MeasureText(secs.c_str(), 11) - 6, y + 3, 11,
                 Fade(WHITE, 0.85f));
        y += h + 4;
    }
}

void GameController::UpdateNinjaTeleport(Player* player, float dt) {
    if (!player) return;
    NinjaSkillSet* ns = player->GetNinjaSkills();
    if (!ns || !ns->IsTeleporting() || ns->m_teleportDone) return;

    // Snap position once after the start animation window
    float elapsed = NinjaSkillSet::TELEPORT_START_DURATION + NinjaSkillSet::TELEPORT_END_DURATION
                    - ns->m_teleportTimer;
    if (elapsed >= NinjaSkillSet::TELEPORT_START_DURATION) {
        // Find nearest enemy
        Vector2 targetPos = player->GetPosition();
        float   minDist   = NinjaSkillSet::TELEPORT_MAX_RANGE;
        bool    foundEnemy = false;

        for (const auto& e : m_gameState->GetAllEntities()) {
            if (!e || e->GetType() != EntityType::Enemy || !e->IsActive()) continue;
            float dx = e->GetPosition().x - player->GetPosition().x;
            float dy = e->GetPosition().y - player->GetPosition().y;
            float d  = std::sqrt(dx*dx + dy*dy);
            if (d < minDist) {
                minDist   = d;
                targetPos = e->GetPosition();
                foundEnemy = true;
            }
        }

        Vector2 newPos;
        if (foundEnemy) {
            // Place player 80px in front of enemy (toward the player's facing)
            float dx = player->GetPosition().x - targetPos.x;
            float len = std::abs(dx);
            float signX = (len > 0.001f) ? (dx > 0.0f ? 1.0f : -1.0f) : 1.0f;
            newPos.x = targetPos.x + signX * NinjaSkillSet::TELEPORT_OFFSET;
            newPos.y = targetPos.y;
            player->SetDirection(signX > 0.0f ? Direction::Right : Direction::Left);
        } else {
            // No enemy: teleport forward
            float dirX = (player->GetDirection() == Direction::Right) ? 1.0f : -1.0f;
            newPos.x = player->GetPosition().x + dirX * NinjaSkillSet::TELEPORT_NO_ENEMY_DIST;
            newPos.y = player->GetPosition().y;
        }

        // Wall-check: make sure newPos doesn't place player inside solid tiles.
        // Scan from target back toward player until a clear position is found.
        auto isPosClear = [&](Vector2 p) -> bool {
            Vector2 sz = player->GetSize();
            // Check 4 corners (inset by 2px) of the player hitbox
            const Vector2 corners[4] = {
                {p.x + 2.0f,         p.y + 2.0f        },
                {p.x + sz.x - 2.0f, p.y + 2.0f        },
                {p.x + 2.0f,         p.y + sz.y - 2.0f },
                {p.x + sz.x - 2.0f, p.y + sz.y - 2.0f },
            };
            for (const auto& c : corners) {
                int tx = (int)(c.x / TILE_SIZE);
                int ty = (int)(c.y / TILE_SIZE);
                for (const auto& tile : m_gameState->GetTiles(MapLayer::Main)) {
                    if (tile.solid && tile.x == tx && tile.y == ty) return false;
                }
            }
            return true;
        };

        if (!isPosClear(newPos)) {
            // Step back toward player's original position until clear
            Vector2 origin = player->GetPosition();
            float dx = origin.x - newPos.x;
            float dy = origin.y - newPos.y;
            float len = std::sqrt(dx*dx + dy*dy);
            bool cleared = false;
            if (len > 0.001f) {
                float ux = dx / len;
                float uy = dy / len;
                for (float t = 8.0f; t <= len; t += 8.0f) {
                    Vector2 candidate = {newPos.x + ux * t, newPos.y + uy * t};
                    if (isPosClear(candidate)) {
                        newPos = candidate;
                        cleared = true;
                        break;
                    }
                }
            }
            if (!cleared) {
                // No safe spot found — cancel the teleport silently
                ns->m_teleportDone = true;
                return;
            }
        }

        player->SetPosition(newPos);
        player->SetVelocity({0.0f, 0.0f});
        ns->m_teleportDone = true;

        SoundManager::GetInstance().PlaySound("ninja_attack_3_arrive");

        View::ParticleRenderer::GetInstance().EmitBurst(newPos, 15, WHITE);
        // Play teleport_end animation after snap
        uint32_t pid = static_cast<uint32_t>(player->GetId());
        auto* anim = View::CharacterRenderer::GetInstance().GetAnimator(pid);
        if (anim && anim->HasClip("skill3_teleport_end")) {
            anim->Play("skill3_teleport_end");
        }
    }
}
