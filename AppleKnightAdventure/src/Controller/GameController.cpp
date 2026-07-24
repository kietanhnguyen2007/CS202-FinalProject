#include "Controller/GameController.h"
#include "Controller/InputController.h"
#include "Factories/LevelFactory.h"
#include "Model/Boss.h"
#include "Model/Enemy.h"
#include "Model/Chest.h"
#include "Model/Checkpoint.h"
#include "Model/Item.h"
#include "Model/KnightSkillSet.h"
#include "View/GameView.h"
#include "View/CharacterRenderer.h"
#include "View/EntityRenderer.h"
#include "View/HUDView.h"
#include "View/MenuView.h"
#include "View/UIStateManager.h"
#include "View/FloatingText.h"
#include "View/ParticleRenderer.h"
#include "Systems/SoundManager.h"
#include "Utils/Constants.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <filesystem>

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
} // namespace

GameController& GameController::GetInstance() {
    static GameController instance;
    return instance;
}

bool GameController::Init() {
    auto& snd = SoundManager::GetInstance();
    if (!snd.IsAudioInitialized()) {
        snd.InitAudio();
    }
    snd.LoadSound("player_attack", "assets/sounds/sfx/player_attack.wav");
    snd.LoadSound("player_hurt", "assets/sounds/sfx/player_hurt.wav");
    snd.LoadSound("enemy_death", "assets/sounds/sfx/enemy_die.wav");
    snd.LoadSound("enemy_hurt", "assets/sounds/sfx/enemy_hurt.wav");
    snd.LoadSound("coin_pickup", "assets/sounds/sfx/coin_pickup.wav");
    snd.LoadSound("chest_open", "assets/sounds/sfx/chest_open.wav");
    snd.LoadMusic("bgm_gameplay", "assets/sounds/music/bgm_gameplay.wav");

    View::GameView::GetInstance().Init();
    View::HUDView::GetInstance().Init();
    View::HUDView::GetInstance().LoadResources("assets/ui/ui_atlas.json");
    View::UIStateManager::GetInstance().Init();

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
    // Ưu tiên LDtk single-world file nếu tồn tại
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
    auto& cr = View::CharacterRenderer::GetInstance();
    const uint32_t id = static_cast<uint32_t>(player->GetId());

    cr.Register(player, root + "idle.json", "idle");
    cr.MergeAtlas(id, root + "walk.json");
    cr.MergeAtlas(id, root + "jump.json");
    cr.MergeAtlas(id, root + "run.json");
    cr.MergeAtlas(id, root + "attack1.json");
    cr.MergeAtlas(id, root + "attack2.json");
    cr.MergeAtlas(id, root + "attack3.json");
    cr.MergeAtlas(id, root + "hurt.json");
    cr.MergeAtlas(id, root + "dead.json");
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
        View::EntityRenderer::GetInstance().RegisterAnimated(
            checkpoint, "assets/textures/objects/checkpoint_captured.json", "default");
    } else {
        View::EntityRenderer::GetInstance().RegisterAnimated(
            checkpoint, "assets/textures/objects/checkpoint_uncaptured.json", "default");
    }
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

void GameController::RegisterBossVisuals(Boss* boss) {
    if (!boss) return;
    auto& cr = View::CharacterRenderer::GetInstance();
    const uint32_t id = static_cast<uint32_t>(boss->GetId());

    // Determine boss tier by entity size set during LDtk load:
    // 128×128 → Boss3,  96×96 → Boss1 or Boss2
    Vector2 sz = boss->GetSize();
    std::string root;
    if (sz.x >= 127.0f) {
        root = "assets/textures/boss/boss3/phase1/";
    } else {
        root = "assets/textures/boss/boss1/phase1/";
    }

    cr.Register(boss, root + "idle.json", "idle");
    cr.MergeAtlas(id,  root + "walk.json");
    cr.MergeAtlas(id,  root + "attack_1.json");
    if (std::filesystem::exists(root + "hurt.json"))
        cr.MergeAtlas(id, root + "hurt.json");
}

void GameController::RegisterEntityVisuals(Entity* entity) {
    if (!entity) return;
    switch (entity->GetType()) {
        case EntityType::Player:
            RegisterPlayerVisuals(static_cast<Player*>(entity), m_gameState->GetPlayerClass());
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
        case EntityType::FakeWall:
            // FakeWall trông như tile thường — không cần Register visuals riêng
            break;
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
    View::CharacterRenderer::GetInstance().Clear();
    View::EntityRenderer::GetInstance().Clear();
    View::UIStateManager::GetInstance().Clear();

    m_activePet.reset();
    m_petProjectiles.clear();

    const std::string path = GetLevelPath(levelNumber);
    const bool isLDtk = (path.size() >= 5 &&
                         path.substr(path.size() - 5) == ".ldtk");
    const int ldtkIdx = isLDtk ? (levelNumber - 1) : 0;
    m_gameState = LevelFactory::LoadLevel(path, GameMode::SinglePlayer, ldtkIdx);
    if (!m_gameState) {
        m_gameState = LevelFactory::CreateDefaultLevel(levelNumber);
    }
    m_gameState->SetCurrentLevel(levelNumber);
    m_gameState->ResetTimer();

    m_scoring = LevelScoring();
    m_scoring.SetTotals(m_gameState->GetTotalItems(), m_gameState->GetTotalEnemies());
    m_defeatedEnemies = 0;
    m_collectedItems = 0;
    m_levelComplete = false;
    m_paused = false;
    m_returnToMenu = false;
    m_running = true;
    m_enemyAttackCooldown = 0.0f;

    View::GameView::GetInstance().SetTiles(&m_gameState->GetTiles());

    // Load background based on the theme set by LDtk level field (or default Forest)
    View::GameView::GetInstance().LoadBackgrounds(m_gameState->GetBackgroundTheme());

    if (Player* player = m_gameState->GetLocalPlayer()) {
        RegisterPlayerVisuals(player, m_gameState->GetPlayerClass());
        m_respawnPoint = player->GetPosition();
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
    m_collision.SetWorldBounds({0.0f, 0.0f, mapWidth, mapHeight});

    SoundManager::GetInstance().StopMusic("bgm_menu");
    SoundManager::GetInstance().PlayMusic("bgm_gameplay");
}

bool GameController::IsOnGround(const Character* character) const {
    if (!character || !m_gameState) return false;
    Rectangle box = character->GetBoundingBox();
    Rectangle probe = {box.x, box.y + box.height, box.width, 2.0f};
    for (const auto& tile : m_gameState->GetTiles()) {
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

    for (const auto& tile : m_gameState->GetTiles()) {
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
}

void GameController::HandlePlayerInput(const InputCommand& cmd, float /*dt*/) {
    Player* player = m_gameState ? m_gameState->GetLocalPlayer() : nullptr;
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
    }
    player->SetVelocity(vel);

    // --- Dash (L) ---
    if (cmd.dash && player->CanDash()) {
        bool isMoving = (cmd.moveLeft || cmd.moveRight);
        float dirX    = (player->GetDirection() == Direction::Right) ? 1.0f : -1.0f;
        player->StartDash(isMoving, dirX);
    }

    // --- Knight Skills ---
    uint32_t pid = static_cast<uint32_t>(player->GetId());

    if (skills && !player->IsDashing()) {
        // Attack1 (J) — quick slash
        if (cmd.attack && skills->TryAttack1()) {
            player->Attack();  // syncs m_attackTimer for animation state
            SoundManager::GetInstance().PlaySound("player_attack");
        }
        // Attack2 (K) — heavy strike (starts charging)
        if (cmd.parry && skills->TryAttack2()) {
            player->Attack2();
            SoundManager::GetInstance().PlaySound("player_attack");
        }
        // Attack3 (U) — lunge thrust
        if (cmd.skill1 && skills->TryAttack3()) {
            player->Attack3();
            SoundManager::GetInstance().PlaySound("player_attack");
            // Apply lunge velocity (overrides current vel.x)
            float lDir = 0.0f;
            if (cmd.moveRight && !cmd.moveLeft) {
                lDir = 1.0f;
                player->SetDirection(Direction::Right);
            } else if (cmd.moveLeft && !cmd.moveRight) {
                lDir = -1.0f;
                player->SetDirection(Direction::Left);
            } else {
                lDir = (player->GetDirection() == Direction::Right) ? 1.0f : -1.0f;
            }
            Vector2 lv = player->GetVelocity();
            lv.x = lDir * skills->m_lungeSpeed;
            player->SetVelocity(lv);
        }
    } else if (!skills) {
        // Non-knight fallback — original single attack
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
        if (entity->GetType() != EntityType::Enemy || !entity->IsActive()) continue;
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
                for (const auto& tile : m_gameState->GetTiles()) {
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

void GameController::UpdateCombat(float dt) {
    Player* player = m_gameState->GetLocalPlayer();
    if (!player) return;

    KnightSkillSet* skills = player->GetKnightSkills();

    // Helper lambda to deal damage to an enemy from a given hitbox
    auto HitEnemiesInBox = [&](Rectangle attackBox, int damage) {
        for (auto& entity : m_gameState->GetAllEntities()) {
            if (entity->GetType() != EntityType::Enemy || !entity->IsActive()) continue;
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
    };

    if (skills) {
        // Attack1 — standard slash box
        if (skills->IsAttack1Active()) {
            HitEnemiesInBox(player->GetAttackBoundingBox(), skills->attack1.damage);
        }
        // Attack2 — wider heavy hit box
        if (skills->IsAttack2Active()) {
            Rectangle heavyBox = skills->GetAttack2HitBox(
                player->GetPosition(), player->GetSize(), player->GetDirection());
            HitEnemiesInBox(heavyBox, skills->attack2.damage);
        }
        // Attack3 — player bounding box while lunging
        if (skills->IsAttack3Active()) {
            HitEnemiesInBox(player->GetBoundingBox(), skills->attack3.damage);
        }
    } else if (player->GetState() == Character::State::Attack) {
        // Fallback for non-knight classes (hardcoded 25)
        HitEnemiesInBox(player->GetAttackBoundingBox(), 25);
    }

    m_enemyAttackCooldown -= dt;
    if (m_enemyAttackCooldown > 0.0f) return;

    Vector2 playerCenter = {
        player->GetPosition().x + player->GetSize().x * 0.5f,
        player->GetPosition().y + player->GetSize().y * 0.5f
    };

    for (auto& entity : m_gameState->GetAllEntities()) {
        if (entity->GetType() != EntityType::Enemy || !entity->IsActive()) continue;
        auto* enemy = static_cast<Enemy*>(entity.get());
        if (enemy->GetState() != EnemyState::Attack) continue;

        Vector2 enemyCenter = {
            enemy->GetPosition().x + enemy->GetSize().x * 0.5f,
            enemy->GetPosition().y + enemy->GetSize().y * 0.5f
        };
        if (Distance(playerCenter, enemyCenter) > enemy->GetAttackRange()) continue;
        if (!enemy->CanAttack()) continue;

        enemy->Attack();
        // Invincibility from dash blocks all damage
        if (!player->IsInvincible()) {
            player->TakeDamage(enemy->GetDamage());
            SoundManager::GetInstance().PlaySound("player_hurt");
            View::FloatingTextManager::GetInstance().Emit(
                player->GetPosition(), "-" + std::to_string(enemy->GetDamage()), RED, 1.0f);
            View::GameView::GetInstance().Shake(4.0f, 0.2f);
        }
        m_enemyAttackCooldown = 0.4f;

        if (!player->IsAlive()) {
            RespawnPlayer();
        }
        break;
    }
}

void GameController::UpdateItems(float dt) {
    (void)dt;
    Player* player = m_gameState->GetLocalPlayer();
    if (!player) return;

    Rectangle playerBox = player->GetBoundingBox();
    std::vector<int> collectedIds;

    for (const auto& entity : m_gameState->GetAllEntities()) {
        if (entity->GetType() != EntityType::Item || !entity->IsActive()) continue;
        auto* item = static_cast<Item*>(entity.get());
        if (!RectOverlap(playerBox, item->GetBoundingBox())) continue;

        switch (item->GetItemType()) {
            case ItemType::Coin:
                player->GetInventory().AddCoins(item->GetAmount());
                player->AddScore(item->GetAmount() * 10);
                m_scoring.AddScore(item->GetAmount() * 10);
                break;
            case ItemType::Apple:
                player->Heal(25);
                break;
            case ItemType::Key:
                player->GetInventory().AddKeys(1);
                break;
            default:
                player->GetInventory().AddItem(
                    std::make_unique<Item>(item->GetPosition(), item->GetItemType(), item->GetAmount()));
                break;
        }

        m_collectedItems++;
        m_scoring.CollectItem();
        SoundManager::GetInstance().PlaySound("coin_pickup");
        View::FloatingTextManager::GetInstance().Emit(
            item->GetPosition(), item->GetItemName(), YELLOW, 1.0f);
        collectedIds.push_back(item->GetId());
    }

    for (int id : collectedIds) {
        UnregisterEntityVisuals(id);
        m_gameState->RemoveEntity(id);
    }
}

void GameController::UpdateInteractions(const InputCommand& cmd) {
    Player* player = m_gameState->GetLocalPlayer();
    if (!player || !cmd.interact) return;

    Rectangle playerBox = player->GetBoundingBox();

    for (auto& entity : m_gameState->GetAllEntities()) {
        if (!entity->IsActive()) continue;

        Rectangle expanded = playerBox;
        expanded.x -= TILE_SIZE * 0.5f;
        expanded.width += TILE_SIZE;
        if (!RectOverlap(expanded, entity->GetBoundingBox())) continue;

        if (entity->GetType() == EntityType::Chest) {
            auto* chest = static_cast<Chest*>(entity.get());
            if (chest->IsOpened()) continue;
            auto loot = chest->Open();
            SoundManager::GetInstance().PlaySound("chest_open");
            for (auto& item : loot) {
                item->SetPosition({
                    chest->GetPosition().x,
                    chest->GetPosition().y - TILE_SIZE * 0.5f
                });
                RegisterItemVisuals(item.get());
                m_gameState->AddEntity(std::move(item));
            }
            return;
        }

        if (entity->GetType() == EntityType::Checkpoint) {
            auto* checkpoint = static_cast<Checkpoint*>(entity.get());
            checkpoint->Activate();
            m_respawnPoint = checkpoint->GetPosition();
            return;
        }
    }
}

void GameController::OnEntityRemoved(Entity* entity) {
    if (!entity) return;
    if (entity->GetType() == EntityType::Enemy) {
        m_defeatedEnemies++;
        m_scoring.DefeatEnemy();
        m_scoring.AddScore(50);
        SoundManager::GetInstance().PlaySound("enemy_death");
        View::ParticleRenderer::GetInstance().EmitBurst(entity->GetPosition(), 20, RED);
        View::GameView::GetInstance().Shake(5.0f, 0.3f);
    }
    UnregisterEntityVisuals(entity->GetId());
}

void GameController::RespawnPlayer() {
    Player* player = m_gameState->GetLocalPlayer();
    if (!player) return;
    player->SetPosition(m_respawnPoint);
    player->SetVelocity({0.0f, 0.0f});
    player->SetHealth(player->GetMaxHealth());
    player->SetActive(true);
}

void GameController::CheckLevelComplete() {
    if (m_levelComplete || !m_gameState) return;

    if (m_gameState->IsLevelComplete()) {
        m_levelComplete = true;
        m_gameState->StopTimer();
        m_scoring.SetClearTime(m_gameState->GetClearTime());
        m_scoring.CalculateStars();
        SoundManager::GetInstance().StopMusic("bgm_gameplay");
    }
}

void GameController::Update(float dt) {
    if (!m_running || !m_gameState) return;

    // Cap delta time to prevent physics tunneling during asset loading spikes
    if (dt > 0.1f) dt = 0.1f;

    InputCommand cmd = InputController::GetInstance().Poll();

    if (cmd.pause) {
        if (m_paused) {
            m_paused = false;
            m_gameState->SetTimerRunning(true);
            View::UIStateManager::GetInstance().Pop();
            View::MenuView::GetInstance().SetVisible(false);
        } else {
            m_paused = true;
            m_gameState->SetTimerRunning(false);
            View::MenuView::GetInstance().ShowPauseOverlay();
            View::MenuView::GetInstance().SetVisible(true);
            View::UIStateManager::GetInstance().Push(View::UILayer::Menu);
        }
    }

    if (m_paused) {
        if (cmd.menuConfirm) {
            if (View::MenuView::GetInstance().GetMode() == View::MenuMode::Pause) {
                m_returnToMenu = true;
                m_running = false;
            }
        }
        View::MenuView::GetInstance().Update(dt, 0);
        return;
    }

    if (m_levelComplete && cmd.menuConfirm) {
        m_returnToMenu = true;
        m_running = false;
        return;
    }

    if (!View::UIStateManager::GetInstance().IsOverlayActive()) {
        HandlePlayerInput(cmd, dt);
        UpdateInteractions(cmd);
    }

    Player* player = m_gameState->GetLocalPlayer();
    if (player && player->IsActive()) {
        ApplyGravity(player, dt);
    }

    m_gameState->Update(dt);
    UpdateEnemyAI(dt);

    if (player && player->IsActive()) {
        ResolveTileCollisions(player, dt);
        m_playerOnGround = IsOnGround(player);
    }

    UpdateCombat(dt);
    UpdateItems(dt);
    UpdatePets(dt, cmd);
    UpdateProjectiles(dt);
    m_particles.Update(dt);
    m_gameState->TickTimer(dt);

    if (player && player->IsAlive()) {
        const float mapHeight = std::max(1, m_gameState->GetMapHeight()) * TILE_SIZE;
        if (player->GetPosition().y > mapHeight + TILE_SIZE * 2) {
            player->TakeDamage(player->GetMaxHealth()); // Technically dies
            SoundManager::GetInstance().PlaySound("player_hurt");
            RespawnPlayer();
        }
    }

    if (player) {
        Vector2 center = {
            player->GetPosition().x + player->GetSize().x * 0.5f,
            player->GetPosition().y + player->GetSize().y * 0.5f
        };
        m_camera.target.x += (center.x - m_camera.target.x) * 0.1f;
        m_camera.target.y += (center.y - m_camera.target.y) * 0.1f;
    }

    View::GameView::GetInstance().Update(dt);
    View::HUDView::GetInstance().Update(dt, player);
    CheckLevelComplete();
}

void GameController::Render() {
    if (!m_gameState) return;

    View::Renderer::GetInstance().BeginFrame();
    View::GameView::GetInstance().Render(m_camera, m_particles.GetActive(), GetFrameTime());
    View::Renderer::GetInstance().EndFrameAndFlush();

    if (m_levelComplete) {
        Player* player = m_gameState->GetLocalPlayer();
        int stars = m_scoring.GetStars();
        DrawText("LEVEL COMPLETE!", 420, 280, 40, GREEN);
        DrawText(TextFormat("Stars: %d", stars), 500, 340, 28, GOLD);
        DrawText(TextFormat("Time: %.1fs", m_gameState->GetClearTime()), 480, 380, 24, WHITE);
        if (player) {
            DrawText(TextFormat("Score: %d", player->GetScore()), 480, 410, 24, WHITE);
        }
        DrawText("Press ENTER to return to menu", 390, 470, 22, LIGHTGRAY);
    }
}

void GameController::Shutdown() {
    SoundManager::GetInstance().StopMusic("bgm_gameplay");
    m_gameState.reset();
    m_running = false;
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
    if (type == PetType::Ghost && m_inCombat) return;

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
        Pet::DRAGON_PROJECTILE_DMG,
        pet->GetOwnerId());

    proj->SetSize({20, 20});
    proj->SetScale(1.0f / 7.0f);
    proj->SetHoming(true);
    proj->SetHomingTargetPos(targetPos);

    // Register visuals
    View::EntityRenderer::GetInstance().RegisterAnimated(
        proj.get(), "assets/textures/pets/projectile_dragon/attack.json", "attack");

    m_petProjectiles.push_back(std::move(proj));
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

    // Pet1 (1) = Dragon, Pet2 (2) = Ghost
    if (cmd.pet1) {
        if (m_activePet && m_activePet->GetPetType() == PetType::BabyDragon) {
            DespawnPet();
        } else {
            SpawnPet(PetType::BabyDragon);
        }
    }
    if (cmd.pet2) {
        if (m_activePet && m_activePet->GetPetType() == PetType::Ghost) {
            DespawnPet();
        } else if (!m_inCombat) {
            SpawnPet(PetType::Ghost);
        }
    }

    // Auto-despawn Ghost on combat
    if (m_activePet && m_activePet->GetPetType() == PetType::Ghost && m_inCombat) {
        DespawnPet();
    }

    if (!m_activePet) return;

    // Build enemy pointer list for Dragon targeting
    std::vector<Entity*> enemies;
    for (const auto& e : m_gameState->GetAllEntities()) {
        if (e->GetType() == EntityType::Enemy && e->IsActive()) {
            auto* enemy = static_cast<Enemy*>(e.get());
            if (enemy->IsAlive()) enemies.push_back(e.get());
        }
    }

    // Update homing target positions on existing projectiles
    for (auto& proj : m_petProjectiles) {
        if (!proj->IsHoming()) continue;
        Entity* tgt = m_gameState->GetEntity(proj->GetOwnerId()); // reuse: we stored target id there
        // Actually we'll just let it home wherever it last aimed; no reassignment needed
    }

    m_activePet->UpdateAI(player->GetPosition(), dt, player, enemies, m_inCombat);
    m_activePet->Update(dt);

    // Dragon fire
    if (m_activePet->GetPetType() == PetType::BabyDragon && m_activePet->WantsToFire()) {
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

        // Check collision with enemies
        if (player) {
            for (const auto& e : m_gameState->GetAllEntities()) {
                if (e->GetType() != EntityType::Enemy || !e->IsActive()) continue;
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
}
