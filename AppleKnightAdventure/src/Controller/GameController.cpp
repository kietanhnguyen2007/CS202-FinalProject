#include "Controller/GameController.h"
#include "Controller/InputController.h"
#include "Factories/LevelFactory.h"
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
#include "Systems/SoundManager.h"
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
    
    // Map Level 2 -> lvl1.ldtk, Level 3 -> lvl2.ldtk, ...
    if (levelNumber >= 2 && levelNumber <= 5) {
        std::string path = "assets/levels/lvl" + std::to_string(levelNumber - 1) + ".ldtk";
        if (std::filesystem::exists(path)) return path;
    }
    
    // Fallback to world.ldtk for Level 1 or if specific file doesn't exist
    const std::string ldtkPath = "assets/levels/world.ldtk";
    if (std::filesystem::exists(ldtkPath)) return ldtkPath;
    
    return "assets/levels/level" + std::to_string(levelNumber) + ".lvl";
}

std::string GameController::GetPlayerAtlasRoot(CharacterClass playerClass) const {
    switch (playerClass) {
        case CharacterClass::Fighter: return "assets/textures/player/fighter/";
        case CharacterClass::Ninja: return "assets/textures/player/ninja/";
        case CharacterClass::MagicCaster: return "assets/textures/player/magic_caster/";
        case CharacterClass::Knight:
        default: return "assets/textures/player/knight/";
    }
}

void GameController::RegisterPlayerVisuals(Player* player, CharacterClass playerClass) {
    if (!player) return;
    const std::string root = GetPlayerAtlasRoot(playerClass);
    const auto atlasPath = [&](const std::string& stem) {
        std::string v2Root;
        switch (playerClass) {
            case CharacterClass::Fighter:     v2Root = "assets/textures/player/fighter_v2/"; break;
            case CharacterClass::MagicCaster: v2Root = "assets/textures/player/magic_caster_v2/"; break;
            case CharacterClass::Ninja:       v2Root = "assets/textures/player/ninja_v2/"; break;
            case CharacterClass::Knight:      v2Root = "assets/textures/player/knight_v2/"; break;
        }
        if (!v2Root.empty()) {
            const std::string v2Path = v2Root + stem + "_v2.json";
            if (std::filesystem::exists(v2Path)) return v2Path;
        }
        return root + stem + ".json";
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
        if (std::filesystem::exists(root + "skill3_teleport_start.json"))
            cr.MergeAtlas(id, root + "skill3_teleport_start.json", "attack_3");
        if (std::filesystem::exists(root + "skill3_teleport_end.json"))
            cr.MergeAtlas(id, root + "skill3_teleport_end.json",   "skill3_teleport_end");
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
    
    if (checkpoint->IsEndGame()) {
        Vector2 pos = checkpoint->GetPosition();
        pos.y -= TILE_SIZE; // Nâng lên 1 tile
        checkpoint->SetPosition(pos);
    }
    
    // Endgame checkpoint: always start as uncaptured (hidden flag pole).
    // The flag_out -> captured transition is driven by UpdateEndgameCheckpoints().
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
                    if (e->GetType() == EntityType::Boss && e->GetId() == proj->GetOwnerId()) {
                        foundBoss = true;
                        auto boss = static_cast<Boss*>(e.get());
                        std::string texPath;
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
                            } else if (subType == 2) {
                                texPath = "assets/textures/boss/boss3/projectiles/energy_blast.json";
                            } else if (subType == 3) {
                                texPath = "assets/textures/boss/boss3/projectiles/energy_beam.json";
                            } else if (subType == 4) {
                                texPath = "assets/textures/boss/boss3/ground_animate/default.json";
                            }
                        }
                        if (!texPath.empty()) {
                            // std::cout << "[DEBUG] Registering Boss Projectile! texPath=" << texPath << ", size=" << sz.x << "x" << sz.y << std::endl;
                            bool flipX = (proj->GetDirection() == Direction::Left);
                            proj->SetRotation(0.0f); // Fix rotation bug so it doesn't offset from hitbox
                            
                            Vector2 origin = {0.0f, 0.0f};
                            if (texPath.find("energy_sphere.json") != std::string::npos) {
                                origin = {219.5f, 154.0f}; // Centers 471x340 on 32x32
                            } else if (texPath.find("energy_blast.json") != std::string::npos) {
                                origin = {13.0f, 24.5f}; // Centers 154x177 on 128x128
                            } else if (texPath.find("energy_beam.json") != std::string::npos) {
                                origin = {0.0f, 67.5f}; // Centers 167 height on 32 height, aligns left edge
                            }
                            
                            View::EntityRenderer::GetInstance().RegisterAnimated(
                                proj, texPath, "default", origin, flipX);
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

    // Preserve the player's chosen class across level transitions
    CharacterClass cls = CharacterClass::Knight;
    if (m_gameState) {
        cls = m_gameState->GetPlayerClass();
    } else {
        std::string charId = SaveManager::GetInstance().GetSelectedChar();
        if (charId == "magic_caster") cls = CharacterClass::MagicCaster;
        else if (charId == "ninja") cls = CharacterClass::Ninja;
        else if (charId == "fighter") cls = CharacterClass::Fighter; // if Fighter exists, otherwise fallback to Knight
    }

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

    m_scoring = LevelScoring();
    m_scoring.SetTotals(m_gameState->GetTotalItems(), m_gameState->GetTotalEnemies());
    m_scoring.SetParTime(LoadVictoryParTime(levelNumber));
    m_defeatedEnemies = 0;
    m_collectedItems = 0;
    m_levelComplete = false;
    m_paused = false;
    m_returnToMenu = false;
    m_running = true;
    m_enemyAttackCooldown = 0.0f;
    m_footstepTimer = 0.0f;
    m_knownBossPhases.clear();

    View::GameView::GetInstance().SetTiles(MapLayer::Background, &m_gameState->GetTiles(MapLayer::Background));
    View::GameView::GetInstance().SetTiles(MapLayer::Main, &m_gameState->GetTiles(MapLayer::Main));
    View::GameView::GetInstance().SetTiles(MapLayer::Foreground, &m_gameState->GetTiles(MapLayer::Foreground));

    // Load background based on the theme set by LDtk level field (or default Forest)
    View::GameView::GetInstance().LoadBackgrounds(m_gameState->GetBackgroundTheme());

    if (Player* player = m_gameState->GetLocalPlayer()) {
        // Sync the GameState's class to the actual Player's class
        // (LDtk level files might have hardcoded a default class that overwrites GameState's field)
        m_gameState->SetPlayerClass(player->GetCharacterClass());
        
        RegisterPlayerVisuals(player, m_gameState->GetPlayerClass());
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
    m_collision.SetWorldBounds({0.0f, 0.0f, mapWidth, mapHeight});

    bool containsBoss = false;
    for (const auto& entity : m_gameState->GetAllEntities()) {
        if (entity->GetType() == EntityType::Boss) {
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
        if (entity->GetType() == EntityType::Enemy) {
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
        if (!entity->IsActive() || entity->GetType() != EntityType::FakeWall) continue;
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
        if (!entity->IsActive() || entity->GetType() != EntityType::FakeWall) continue;
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

    // --- Dash (L) ---
    if (cmd.dash && player->CanDash()) {
        // Always dash in the direction the player is currently facing
        float dirX = (player->GetDirection() == Direction::Right) ? 1.0f : -1.0f;
        player->StartDash(true, dirX);
        SoundManager::GetInstance().PlaySound("player_dash");
    }

    // --- Knight Skills ---
    uint32_t pid = static_cast<uint32_t>(player->GetId());

    // ==================== KNIGHT ====================
    if (KnightSkillSet* skills = player->GetKnightSkills()) {
        if (!player->IsDashing() && !player->IsParrying()) {
            if (cmd.attack  && skills->TryAttack1()) {
                player->Attack(KnightSkillSet::ATTACK1_ANIMATION_DURATION);
                SoundManager::GetInstance().PlaySound("knight_attack_1");
            }
            if (cmd.parry   && skills->TryAttack2()) {
                player->Attack2(KnightSkillSet::ATTACK2_ANIMATION_DURATION);
                SoundManager::GetInstance().PlaySound("knight_attack_2");
            }
            if (cmd.skill1  && skills->TryAttack3()) {
                player->Attack3(KnightSkillSet::ATTACK3_ANIMATION_DURATION);
                SoundManager::GetInstance().PlaySound("knight_attack_3");
                float lDir = (player->GetDirection() == Direction::Right) ? 1.0f : -1.0f;
                if (cmd.moveRight && !cmd.moveLeft) { lDir = 1.0f; player->SetDirection(Direction::Right); }
                else if (cmd.moveLeft && !cmd.moveRight) { lDir = -1.0f; player->SetDirection(Direction::Left); }
                Vector2 lv = player->GetVelocity(); lv.x = lDir * skills->m_lungeSpeed;
                player->SetVelocity(lv);
            }
            if (cmd.ultimate && skills->TryUltimate()) {
                player->DoUltimate(KnightSkillSet::ULTIMATE_ANIMATION_DURATION);
                SoundManager::GetInstance().PlaySound("knight_ultimate");
            }
        }
        // Parry can be triggered regardless of other attack state
        if (!player->IsDashing() && cmd.parryBlock && skills->TryParry()) {}

    // ==================== FIGHTER ====================
    } else if (FighterSkillSet* fs = player->GetFighterSkills()) {
        if (!player->IsDashing() && !player->IsParrying()) {
            if (cmd.attack    && fs->TryAttack1()) {
                player->Attack(FighterSkillSet::ATTACK1_ANIMATION_DURATION);
                SoundManager::GetInstance().PlaySound("fighter_attack_1");
            }
            if (cmd.parry     && fs->TryAttack2()) {
                player->Attack2(FighterSkillSet::ATTACK2_ANIMATION_DURATION);
                SoundManager::GetInstance().PlaySound("fighter_attack_2");
            }
            if (cmd.skill1    && fs->TryAttack3()) {
                player->Attack3(FighterSkillSet::ATTACK3_ANIMATION_DURATION);
                SoundManager::GetInstance().PlaySound("fighter_attack_3");
            }
            if (cmd.ultimate  && fs->TryUltimate()) {
                player->DoUltimate(FighterSkillSet::ULTIMATE_ANIMATION_DURATION);
                SoundManager::GetInstance().PlaySound("fighter_ultimate");
            }
        }
        if (!player->IsDashing() && cmd.parryBlock && fs->TryParry()) {}

    // ==================== MAGIC CASTER ====================
    } else if (MagicCasterSkillSet* ms = player->GetMagicSkills()) {
        if (!player->IsDashing() && !player->IsParrying()) {
            if (cmd.attack    && ms->TryAttack1()) {
                player->Attack();
                SoundManager::GetInstance().PlaySound("magic_attack_1");
            }
            if (cmd.parry     && ms->TryAttack2()) {
                player->Attack2(MagicCasterSkillSet::ATTACK2_ANIMATION_DURATION);
                SoundManager::GetInstance().PlaySound("magic_attack_2");
            }
            if (cmd.skill1    && ms->TryAttack3()) {
                player->Attack3();
                SoundManager::GetInstance().PlaySound("magic_attack_3");
            }
            if (cmd.ultimate  && ms->TryUltimate()) {
                player->DoUltimate();
                SoundManager::GetInstance().PlaySound("magic_ultimate");
            }
        }
        if (!player->IsDashing() && cmd.parryBlock && ms->TryParry()) {}

    // ==================== NINJA ====================
    } else if (NinjaSkillSet* ns = player->GetNinjaSkills()) {
        if (!player->IsDashing() && !player->IsParrying()) {
            if (cmd.attack    && ns->TryAttack1()) {
                player->Attack();
                SoundManager::GetInstance().PlaySound("ninja_attack_1");
            }
            if (cmd.parry     && ns->TryAttack2()) {
                player->Attack2();
                SoundManager::GetInstance().PlaySound("ninja_attack_2");
            }
            if (cmd.skill1    && ns->TryAttack3()) {
                player->Attack3();  // triggers teleport_start animation
                SoundManager::GetInstance().PlaySound("ninja_attack_3");
            }
            if (cmd.ultimate  && ns->TryUltimate()) {
                player->DoUltimate(NinjaSkillSet::ULTIMATE_CAST_DURATION);
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
        if (!entity->IsActive()) continue;
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
        enemy->UpdateAI(playerCenter, dt);

        // Ground-based enemy pit/edge detection
        if (enemy->GetEnemyType() != EnemyType::Flying) {
            Vector2 vel = enemy->GetVelocity();
            if (std::abs(vel.x) > 0.1f) {
                // Check slightly ahead in the direction of movement
                float checkX = enemy->GetPosition().x + (vel.x > 0.0f ? enemy->GetSize().x : 0.0f) + (vel.x > 0.0f ? 8.0f : -8.0f);
                float checkY = enemy->GetPosition().y + enemy->GetSize().y + 8.0f; // slightly below feet

                bool groundAhead = false;
                for (const auto& tile : m_gameState->GetTiles(MapLayer::Main)) {
                    if (!tile.solid) continue;
                    float tx = tile.x * TILE_SIZE;
                    float ty = tile.y * TILE_SIZE;
                    // Check if there is a tile below checkX
                    if (checkX >= tx && checkX <= tx + TILE_SIZE &&
                        checkY >= ty && checkY <= ty + TILE_SIZE) {
                        groundAhead = true;
                        break;
                    }
                }

                float mapWidth = m_gameState->GetMapWidth() * TILE_SIZE;
                if (checkX < 0.0f || checkX > mapWidth) {
                    // Out of map bounds! Stop and turn back
                    enemy->SetStateTimer(enemy->GetStateTimer() + 3.14159f); // Add PI to reverse Patrol sin wave
                    vel.x = 0.0f;
                    enemy->SetVelocity(vel);
                    if (enemy->GetState() == EnemyState::Patrol) {
                        enemy->SetState(EnemyState::Idle); // Pause patrol state to reverse direction
                    }
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
    auto HitEnemiesInBox = [&](Rectangle attackBox, int damage) {
        for (auto& entity : m_gameState->GetAllEntities()) {
            if (!entity->IsActive()) continue;

            if (entity->GetType() == EntityType::Enemy) {
                auto* enemy = static_cast<Enemy*>(entity.get());
                if (!RectOverlap(attackBox, enemy->GetBoundingBox())) continue;
                if (enemy->GetState() == EnemyState::Hurt || enemy->GetState() == EnemyState::Dead) continue;

                enemy->TakeDamage(damage);
                SoundManager::GetInstance().PlaySound("enemy_hurt");
                View::FloatingTextManager::GetInstance().Emit(
                    enemy->GetPosition(), "-" + std::to_string(damage), YELLOW, 1.0f);
                View::ParticleRenderer::GetInstance().EmitBurst(enemy->GetPosition(), 8, WHITE);
                View::GameView::GetInstance().Shake(3.0f, 0.15f);

                if (!enemy->IsActive()) OnEntityRemoved(enemy);
            } 
            else if (entity->GetType() == EntityType::Boss) {
                auto* boss = static_cast<Boss*>(entity.get());
                if (!RectOverlap(attackBox, boss->GetBoundingBox())) continue;
                if (!boss->IsAlive() || boss->GetBossState() == BossState::Hurt || boss->GetBossState() == BossState::Die || boss->GetBossState() == BossState::Transition) continue;

                boss->TakeDamage(damage);
                SoundManager::GetInstance().PlaySound("enemy_hurt"); 
                View::FloatingTextManager::GetInstance().Emit(
                    boss->GetPosition(), "-" + std::to_string(damage), YELLOW, 1.0f);
                View::ParticleRenderer::GetInstance().EmitBurst(boss->GetPosition(), 16, WHITE);
                View::GameView::GetInstance().Shake(4.0f, 0.2f);
                
                if (!boss->IsActive()) OnEntityRemoved(boss);
            }
            else if (entity->GetType() == EntityType::FakeWall) {
                auto* wall = static_cast<FakeWall*>(entity.get());
                if (!RectOverlap(attackBox, wall->GetBoundingBox())) continue;
                if (wall->IsDestroyed()) continue;

                wall->TakeDamage(damage);
                SoundManager::GetInstance().PlaySound("enemy_hurt");
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
                std::filesystem::exists("assets/textures/player/fighter_v2/ultimate_projectile_v2.json")
                    ? "assets/textures/player/fighter_v2/ultimate_projectile_v2.json"
                    : "assets/textures/player/fighter/ultimate_projectile.json";
            // Spawn at horizontal edge of player, raised high enough to not clip ground
            float spawnX = (player->GetDirection() == Direction::Right)
                ? player->GetPosition().x + player->GetSize().x
                : player->GetPosition().x;
            Vector2 spawnPos = {
                spawnX,
                player->GetPosition().y + 10.0f  // centered on player body
            };
            SpawnPlayerProjectile(
                projectileAtlas.c_str(),
                spawnPos, player->GetDirection(),
                fs->ultimate.damage, 400.0f, 0.9f, // V2: 12 frames * 0.075s; legacy: 9 * 0.1s
                0.8f, false);  // Fighter H faces right by default
            fs->ResetFireFlag();
            SoundManager::GetInstance().PlaySound("fighter_ultimate");
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
            SpawnLightningAt(targetPos, ms->attack1.damage, 0.12f,
                             "assets/textures/player/magic_caster/projectile_attack1.json",
                             0.0f, 0.8f);
            ms->ResetLightning();
        }
        // Fireball (attack2): fly forward
        if (ms->m_wantsFireball) {
            float fbSpawnX = (player->GetDirection() == Direction::Right)
                ? player->GetPosition().x + player->GetSize().x
                : player->GetPosition().x;
            Vector2 spawnPos = {
                fbSpawnX,
                player->GetPosition().y - 10.0f // raised even higher
            };
            SpawnPlayerProjectile(
                "assets/textures/player/magic_caster/projectile_attack2.json",
                spawnPos, player->GetDirection(),
                ms->attack2.damage,
                MagicCasterSkillSet::FIREBALL_SPEED,
                MagicCasterSkillSet::FIREBALL_RANGE / MagicCasterSkillSet::FIREBALL_SPEED,
                0.8f, false); // Fireball faces Right by default, increased scale to 0.8
            ms->ResetFireball();
        }
        // Wave (attack3): fly forward slower
        if (ms->m_wantsWave) {
            float waveSpawnX = (player->GetDirection() == Direction::Right)
                ? player->GetPosition().x + player->GetSize().x
                : player->GetPosition().x;
            Vector2 spawnPos = {
                waveSpawnX,
                player->GetPosition().y - 75.0f  // keep the enlarged hitbox just above ground
            };
            SpawnPlayerProjectile(
                "assets/textures/player/magic_caster/projectile_attack3.json",
                spawnPos, player->GetDirection(),
                ms->attack3.damage,
                MagicCasterSkillSet::WAVE_SPEED,
                1.0f, // 1.0f matches exact animation length (5 frames * 0.2s)
                1.0f, true,
                {180.0f, 100.0f}); // close to the 197x130px visual, minus transparent edges
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
            SpawnLightningAt(targetPos, ms->ultimate.damage, 0.12f,
                             "assets/textures/player/magic_caster/ultimate_skill_projectile.json",
                             0.0f, 1.0f);
            ms->ResetUltLightning();
        }

    // ==================== NINJA COMBAT ====================
    } else if (NinjaSkillSet* ns = player->GetNinjaSkills()) {
        if (ns->IsAttack1Active())
            HitEnemiesInBox(ns->GetAttack1HitBox(player->GetPosition(), player->GetSize(), player->GetDirection()),
                            ns->attack1.damage);
        // Blade Rush: projectile spawns when animation ends
        if (ns->WantsBladeRush()) {
            float brSpawnX = (player->GetDirection() == Direction::Right)
                ? player->GetPosition().x + player->GetSize().x
                : player->GetPosition().x;
            Vector2 spawnPos = {
                brSpawnX,
                player->GetPosition().y - 32.0f  // keep the tallest K frame above the floor
            };
            SpawnPlayerProjectile(
                "assets/textures/player/ninja/projectile_attack2.json",
                spawnPos, player->GetDirection(),
                ns->attack2.damage,
                NinjaSkillSet::BLADE_RUSH_SPEED,
                NinjaSkillSet::BLADE_RUSH_RANGE / NinjaSkillSet::BLADE_RUSH_SPEED,
                0.3f, false); // Ninja K sprite faces right by default
            ns->ResetBladeRush();
        }
        // Teleport
        UpdateNinjaTeleport(player, dt);
        // Shadow Clone projectile
        if (ns->WantsShadowClone()) {
            float cloneSpawnX = (player->GetDirection() == Direction::Right)
                ? player->GetPosition().x + player->GetSize().x
                : player->GetPosition().x;
            // Ninja H sprite at 0.5 scale is ~75px tall.
            // Raise it so it doesn't clip below the ground block.
            Vector2 spawnPos = {
                cloneSpawnX,
                player->GetPosition().y - 66.0f
            };
            SpawnPlayerProjectile(
                "assets/textures/player/ninja/projectile_ultimate_attack.json",
                spawnPos, player->GetDirection(),
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
        if (!entity->IsActive()) continue;
        
        bool isAttacking = false;
        int attackDamage = 0;
        float attackRange = 0.0f;
        Vector2 attackCenter;
        Rectangle attackBox;
        
        if (entity->GetType() == EntityType::Enemy) {
            auto* enemy = static_cast<Enemy*>(entity.get());
            if (enemy->GetState() == EnemyState::Attack) {
                if (enemy->CanAttack()) {
                    isAttacking = true;
                    attackDamage = enemy->GetDamage();
                    attackRange = enemy->GetAttackRange();
                    attackCenter = {
                        enemy->GetPosition().x + enemy->GetSize().x * 0.5f,
                        enemy->GetPosition().y + enemy->GetSize().y * 0.5f
                    };
                    enemy->Attack();
                    SoundManager::GetInstance().PlaySound("enemy_attack");
                    
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
            BossState bs = boss->GetBossState();
            if (bs == BossState::Skill1 || bs == BossState::Skill2 || bs == BossState::Skill3 || bs == BossState::Skill4) {
                // Only deal direct body damage if it's a melee attack (range <= 150)
                if (boss->GetAttackRange() <= 150.0f && boss->WantsMelee()) {
                    isAttacking = true;
                    attackDamage = boss->GetDamage();
                    attackRange = boss->GetAttackRange();
                    attackCenter = {
                        boss->GetPosition().x + boss->GetSize().x * 0.5f,
                        boss->GetPosition().y + boss->GetSize().y * 0.5f
                    };
                    boss->ResetMelee();
                    SoundManager::GetInstance().PlaySound("boss_attack");
                }
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

            if (!proj->HasHit() && RectOverlap(proj->GetBoundingBox(), player->GetBoundingBox())) {
                if (!player->IsInvincible()) {
                    player->TakeDamage(proj->GetDamage());
                    SoundManager::GetInstance().PlaySound(player->IsAlive() ? "player_hurt" : "player_die");
                    View::FloatingTextManager::GetInstance().Emit(
                        player->GetPosition(), "-" + std::to_string(proj->GetDamage()), RED, 1.0f);
                    View::GameView::GetInstance().Shake(4.0f, 0.2f);
                    proj->SetHasHit(true);
                }
                // Despawn moving projectiles on hit; keep stationary AoE/beams active so visuals complete
                if (proj->GetDirection() != Direction::None && proj->GetSubType() != 3) {
                    proj->OnHit();
                }
                if (!player->IsAlive()) RespawnPlayer(player, !m_localCoop);
            }
            continue; // Skip the regular attack check for Projectiles
        }
        
        if (!isAttacking) continue;
        if (Distance(playerCenter, attackCenter) > attackRange) continue;
        
        // Invincibility from dash blocks all damage
        if (!player->IsInvincible()) {
            player->TakeDamage(attackDamage);
            SoundManager::GetInstance().PlaySound(player->IsAlive() ? "player_hurt" : "player_die");
            View::FloatingTextManager::GetInstance().Emit(
                player->GetPosition(), "-" + std::to_string(attackDamage), RED, 1.0f);
            View::GameView::GetInstance().Shake(4.0f, 0.2f);
        }
        m_enemyAttackCooldown = 0.4f;

        if (!player->IsAlive()) {
            RespawnPlayer(player, !m_localCoop);
        }
        break;
    }
}

void GameController::UpdateItems(Player* player, float dt) {
    (void)dt;
    if (!player) return;

    Rectangle playerBox = player->GetBoundingBox();
    std::vector<int> collectedIds;
    bool persistentCoinsChanged = false;

    for (const auto& entity : m_gameState->GetAllEntities()) {
        if (entity->GetType() != EntityType::Item || !entity->IsActive()) continue;
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
                persistentCoinsChanged = true;
                player->AddScore(item->GetAmount() * 10);
                m_scoring.AddScore(item->GetAmount() * 10);
                break;
            case ItemType::Apple:
                player->Heal(25);
                break;
            case ItemType::Key:
                player->GetInventory().AddKeys(1);
                break;
            case ItemType::Potion:
                pickupSound = "potion_pickup";
                player->Heal(50);
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
        if (!entity->IsActive()) continue;

        Rectangle expanded = playerBox;
        expanded.x -= TILE_SIZE * 0.5f;
        expanded.width += TILE_SIZE;
        if (!RectOverlap(expanded, entity->GetBoundingBox())) continue;

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
            if (checkpoint->IsEndGame() || checkpoint->IsActivated()) continue;

            uint32_t newUid = static_cast<uint32_t>(checkpoint->GetId());
            if (checkpoint->GetPosition().x >= m_respawnPoint.x) {
                m_activeCheckpointUid = newUid;
                checkpoint->Activate();
                SoundManager::GetInstance().PlaySound("checkpoint_activate");
                m_respawnPoint = checkpoint->GetPosition();
                CaptureCheckpointEnemies(checkpoint->GetPosition().x);

                // Switch visual: uncaptured -> flag_out animation
                View::EntityRenderer::GetInstance().Unregister(newUid);
                View::EntityRenderer::GetInstance().RegisterAnimated(
                    checkpoint, "assets/textures/objects/checkpoint_flag_out.json", "flag_out");
            }
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
            SoundManager::GetInstance().PlaySound("enemy_death");
            View::ParticleRenderer::GetInstance().EmitBurst(entity->GetPosition(), 20, RED);
            View::GameView::GetInstance().Shake(5.0f, 0.3f);
        }
    }
    UnregisterEntityVisuals(entity->GetId());
}

void GameController::CaptureCheckpointEnemies(float checkpointX) {
    m_checkpointRespawnEnemyIds.clear();
    if (!m_gameState) return;
    for (const auto& entity : m_gameState->GetAllEntities()) {
        if (entity->GetType() != EntityType::Enemy) continue;
        const auto* enemy = static_cast<const Enemy*>(entity.get());
        if (enemy->GetSpawnPosition().x > checkpointX) {
            m_checkpointRespawnEnemyIds.insert(enemy->GetId());
        }
    }
}

void GameController::RestoreCheckpointEnemies() {
    if (!m_gameState) return;

    m_playerProjectiles.clear();
    m_petProjectiles.clear();
    std::vector<int> projectileIds;
    for (const auto& entity : m_gameState->GetAllEntities()) {
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
    player->SetHealth(player->GetMaxHealth());
    player->SetActive(true);
    if (restoreEncounter) RestoreCheckpointEnemies();

    if (m_previousLevelId != -1) {
        for (auto& entity : m_gameState->GetAllEntities()) {
            if (entity->GetType() == EntityType::Boss) {
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
        auto t = e->GetType();
        if (t == EntityType::Enemy || t == EntityType::Boss) {
            auto* c = static_cast<Character*>(e.get());
            if (c->IsAlive()) { anyAlive = true; break; }
        }
    }

    // Lock/unlock cổng thoát BossArena (targetLevelId == -1)
    for (auto& e : m_gameState->GetAllEntities()) {
        if (e->GetType() != EntityType::TeleportPortal) continue;
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
    m_hasSavedState = true;
}

void GameController::RestorePlayerState(Player* player) {
    if (!player || !m_hasSavedState) return;
    player->SetHealth(m_savedPlayerState.health);
    player->SetScore(m_savedPlayerState.score);
    player->SetSkillPoints(m_savedPlayerState.skillPoints);
    player->GetInventory().AddCoins(m_savedPlayerState.coins);
    player->GetInventory().AddApples(m_savedPlayerState.apples);
    player->GetInventory().AddKeys(m_savedPlayerState.keys);
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
        if (entity->GetType() != EntityType::Enemy || entity->IsActive()) continue;
        auto* enemy = static_cast<Enemy*>(entity.get());
        if (enemy->GetState() == EnemyState::Dead &&
            m_countedDefeatedEnemyIds.count(enemy->GetId()) == 0) {
            OnEntityRemoved(enemy);
        }
    }
    
    // Check and register visuals for newly spawned entities (like projectiles or items)
    for (const auto& entity : m_gameState->GetAllEntities()) {
        if (!entity->IsActive()) continue;
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
    m_particles.Update(dt);
    m_gameState->TickTimer(dt);

    if (player && player->IsAlive()) {
        const float mapHeight = std::max(1, m_gameState->GetMapHeight()) * TILE_SIZE;
        if (player->GetPosition().y > mapHeight + TILE_SIZE * 2) {
            player->TakeDamage(player->GetMaxHealth()); // Technically dies
            SoundManager::GetInstance().PlaySound("player_die");
            RespawnPlayer(player, !m_localCoop);
        }
    }

    if (secondPlayer && secondPlayer->IsAlive()) {
        const float mapHeight = std::max(1, m_gameState->GetMapHeight()) * TILE_SIZE;
        if (secondPlayer->GetPosition().y > mapHeight + TILE_SIZE * 2) {
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
        if (entity->IsActive() && entity->GetType() == EntityType::Boss
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

    if (View::OptionsView::GetInstance().IsVisible()) {
        View::OptionsView::GetInstance().Render();
    }

    View::TutorialRenderer::GetInstance().RenderDialog();
}

void GameController::Shutdown() {
    SaveManager::GetInstance().Save();
    SoundManager::GetInstance().StopAllMusic();
    SoundManager::GetInstance().StopAllSounds();
    m_gameState.reset();
    m_running = false;
    View::CharacterRenderer::GetInstance().Clear();
    View::EntityRenderer::GetInstance().Clear();
    View::UIStateManager::GetInstance().Clear();
    View::ResultView::GetInstance().Dismiss();
    View::OptionsView::GetInstance().SetVisible(false);
    View::HUDView::GetInstance().SetVisible(false);
    View::MenuView::GetInstance().SetVisible(false);

    // Clear GameView pointers to prevent dangling references in other modes (e.g. Map Builder)
    View::GameView::GetInstance().SetTiles(MapLayer::Background, nullptr);
    View::GameView::GetInstance().SetTiles(MapLayer::Main, nullptr);
    View::GameView::GetInstance().SetTiles(MapLayer::Foreground, nullptr);
    View::GameView::GetInstance().SetEntities(nullptr);
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
        if (entity->GetType() != EntityType::Item || !entity->IsActive()) continue;
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
// Endgame Checkpoint — Viewport-triggered flag animation
// ============================================================

void GameController::UpdateEndgameCheckpoints() {
    if (!m_gameState) return;

    // Camera viewport in world space
    float halfW = (SCREEN_WIDTH  * 0.5f) / m_camera.zoom;
    float halfH = (SCREEN_HEIGHT * 0.5f) / m_camera.zoom;
    Rectangle viewport = {
        m_camera.target.x - halfW,
        m_camera.target.y - halfH,
        halfW * 2.0f,
        halfH * 2.0f
    };

    float dt = GetFrameTime();

    for (const auto& entity : m_gameState->GetAllEntities()) {
        if (entity->GetType() != EntityType::Checkpoint || !entity->IsActive()) continue;
        auto* cp = static_cast<Checkpoint*>(entity.get());
        if (!cp->IsEndGame()) continue;

        int  id  = cp->GetId();
        uint32_t uid = static_cast<uint32_t>(id);

        bool revealed  = (m_endgameFlagRevealedIds.find(id)  != m_endgameFlagRevealedIds.end());
        bool captured  = (m_endgameFlagCapturedIds.find(id)  != m_endgameFlagCapturedIds.end());

        if (!revealed) {
            // Phase 1 — wait until the checkpoint enters the player's viewport
            if (RectOverlap(cp->GetBoundingBox(), viewport)) {
                // Trigger flag-raise animation
                View::EntityRenderer::GetInstance().Unregister(uid);
                View::EntityRenderer::GetInstance().RegisterAnimated(
                    cp, "assets/textures/objects/checkpoint_flag_out.json", "flag_out");
                m_endgameFlagRevealedIds.insert(id);
                m_flagOutTimers[id] = 0.0f;
            }
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
    SoundManager::GetInstance().PlaySound("pet_attack");
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
        if (e->GetType() != EntityType::Enemy || !e->IsActive()) continue;
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
                        persistentCoinsChanged = true;
                        player->AddScore(item->GetAmount() * 10);
                        m_scoring.AddScore(item->GetAmount() * 10);
                        break;
                    case ItemType::Apple:
                        player->Heal(25);
                        break;
                    case ItemType::Key:
                        player->GetInventory().AddKeys(1);
                        break;
                    case ItemType::Potion:
                        pickupSound = "potion_pickup";
                        player->Heal(50);
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
                if (!e->IsActive()) continue;
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
                    SoundManager::GetInstance().PlaySound("enemy_hurt");
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
            if (e->GetType() == EntityType::Projectile && !e->IsActive()) {
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
        if (!entity->IsActive() || entity->GetType() != EntityType::FakeWall) continue;
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
        if (!entity->IsActive()) continue;

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

void GameController::SpawnPlayerProjectile(const char* atlasPath, Vector2 spawnPos,
                                            Direction dir, int damage,
                                            float speed, float lifetime,
                                            float scale, bool facesLeft,
                                            Vector2 hitboxSize) {
    // Collision box size scales proportionally to visual scale.
    // Use a smaller base size (40x40) to prevent projectiles from clipping the floor upon spawning.
    float boxSize = 40.0f * scale;
    if (hitboxSize.x <= 0.0f || hitboxSize.y <= 0.0f) {
        hitboxSize = {boxSize, boxSize};
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

    // If sprite naturally faces Left, flip when moving Right.
    // If sprite naturally faces Right, flip when moving Left.
    bool flipX = facesLeft ? (dir == Direction::Right) : (dir == Direction::Left);

    // Force rotation to 0 to prevent double-flipping from the Projectile constructor
    proj->SetRotation(0.0f);

    // Register visual with correct flip
    View::EntityRenderer::GetInstance().RegisterAnimated(proj.get(), atlasPath, "default", {0, 0}, flipX);

    m_playerProjectiles.push_back(std::move(proj));
}

void GameController::SpawnLightningAt(Vector2 targetPos, int damage, float lifetime,
                                       const char* atlasPath, float rotation, float scale) {
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
    proj->SetLifetime(0.6f);  // 5-7 frames × 0.1s
    proj->SetScale(scale);
    proj->SetRotation(rotation);

    View::EntityRenderer::GetInstance().RegisterAnimated(
        proj.get(), atlasPath, "default");

    // Deal damage immediately to any enemy overlapping targetPos
    // We use a logical 60x60 hitbox centered on the intended target, 
    // rather than the visual projectile's bounding box which might be offset.
    Rectangle hitBox = { targetPos.x - 30.0f, targetPos.y - 30.0f, 60.0f, 60.0f };
    for (auto& e : m_gameState->GetAllEntities()) {
        if (!e->IsActive()) continue;
        if (e->GetType() == EntityType::Enemy) {
            auto* enemy = static_cast<Enemy*>(e.get());
            if (enemy->GetState() == EnemyState::Dead) continue;
            if (!RectOverlap(hitBox, enemy->GetBoundingBox())) continue;
            enemy->TakeDamage(damage);
            SoundManager::GetInstance().PlaySound("enemy_hurt");
            View::FloatingTextManager::GetInstance().Emit(
                enemy->GetPosition(), "-" + std::to_string(damage), PURPLE, 1.0f);
            View::ParticleRenderer::GetInstance().EmitBurst(enemy->GetPosition(), 12, PURPLE);
            View::GameView::GetInstance().Shake(4.0f, 0.2f);
            if (!enemy->IsActive()) OnEntityRemoved(enemy);
        } else if (e->GetType() == EntityType::Boss) {
            auto* boss = static_cast<Boss*>(e.get());
            if (!boss->IsAlive() || boss->GetBossState() == BossState::Die || boss->GetBossState() == BossState::Transition) continue;
            if (!RectOverlap(hitBox, boss->GetBoundingBox())) continue;
            boss->TakeDamage(damage);
            SoundManager::GetInstance().PlaySound("enemy_hurt");
            View::FloatingTextManager::GetInstance().Emit(
                boss->GetPosition(), "-" + std::to_string(damage), PURPLE, 1.0f);
            View::ParticleRenderer::GetInstance().EmitBurst(boss->GetPosition(), 12, PURPLE);
            View::GameView::GetInstance().Shake(4.0f, 0.2f);
            if (!boss->IsActive()) OnEntityRemoved(boss);
        }
    }

    m_playerProjectiles.push_back(std::move(proj));
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
            if (!e->IsActive()) continue;
            if (e->GetType() == EntityType::Enemy) {
                auto* enemy = static_cast<Enemy*>(e.get());
                if (enemy->GetState() == EnemyState::Dead) continue;
                if (!RectOverlap(proj->GetBoundingBox(), enemy->GetBoundingBox())) continue;

                enemy->TakeDamage(proj->GetDamage());
                SoundManager::GetInstance().PlaySound("enemy_hurt");
                View::FloatingTextManager::GetInstance().Emit(
                    enemy->GetPosition(), "-" + std::to_string(proj->GetDamage()), ORANGE, 1.0f);
                View::ParticleRenderer::GetInstance().EmitBurst(enemy->GetPosition(), 8, ORANGE);
                View::GameView::GetInstance().Shake(3.0f, 0.15f);
                if (!enemy->IsActive()) OnEntityRemoved(enemy);
                
                proj->OnHit();
                break;
            } else if (e->GetType() == EntityType::Boss) {
                auto* boss = static_cast<Boss*>(e.get());
                if (!boss->IsAlive() || boss->GetBossState() == BossState::Die || boss->GetBossState() == BossState::Transition) continue;
                if (!RectOverlap(proj->GetBoundingBox(), boss->GetBoundingBox())) continue;

                boss->TakeDamage(proj->GetDamage());
                SoundManager::GetInstance().PlaySound("enemy_hurt");
                View::FloatingTextManager::GetInstance().Emit(
                    boss->GetPosition(), "-" + std::to_string(proj->GetDamage()), ORANGE, 1.0f);
                View::ParticleRenderer::GetInstance().EmitBurst(boss->GetPosition(), 8, ORANGE);
                View::GameView::GetInstance().Shake(3.0f, 0.15f);
                if (!boss->IsActive()) OnEntityRemoved(boss);
                
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
            if (e->GetType() != EntityType::Enemy || !e->IsActive()) continue;
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

        player->SetPosition(newPos);
        player->SetVelocity({0.0f, 0.0f});
        ns->m_teleportDone = true;

        View::ParticleRenderer::GetInstance().EmitBurst(newPos, 15, WHITE);
        // Play teleport_end animation after snap
        uint32_t pid = static_cast<uint32_t>(player->GetId());
        auto* anim = View::CharacterRenderer::GetInstance().GetAnimator(pid);
        if (anim && anim->HasClip("skill3_teleport_end")) {
            anim->Play("skill3_teleport_end");
        }
    }
}
