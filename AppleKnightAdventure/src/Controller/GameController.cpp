#include "Controller/GameController.h"
#include "Controller/InputController.h"
#include "Factories/LevelFactory.h"
#include "Model/Enemy.h"
#include "Model/Chest.h"
#include "Model/Checkpoint.h"
#include "Model/Item.h"
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
    // TODO(map-builder): chọn BackgroundTheme dựa theo level/biome của map.
    // Ví dụ:
    //   BackgroundTheme theme = BackgroundTheme::Forest;       // level rừng
    //   BackgroundTheme theme = BackgroundTheme::ColdCorridor; // level lâu đài
    //   BackgroundTheme theme = BackgroundTheme::Underwater;   // level nước
    // Sau đó:
    //   m_gameState->SetBackgroundTheme(theme);
    //   gv.LoadBackgrounds(theme);
    gv.LoadBackgrounds(); // tạm dùng Forest mặc định
}

std::string GameController::GetLevelPath(int levelNumber) const {
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
            break;
        case EnemyType::Flying:
            cr.Register(enemy, "assets/textures/enemies/flying_spritesheet.json", "idle");
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

void GameController::RegisterEntityVisuals(Entity* entity) {
    if (!entity) return;
    switch (entity->GetType()) {
        case EntityType::Player:
            RegisterPlayerVisuals(static_cast<Player*>(entity), m_gameState->GetPlayerClass());
            break;
        case EntityType::Enemy:
            RegisterEnemyVisuals(static_cast<Enemy*>(entity));
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

    m_gameState = LevelFactory::LoadLevel(GetLevelPath(levelNumber), GameMode::SinglePlayer);
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
    m_enemyAttackCooldown = 0.0f;

    LoadTilesets();
    View::GameView::GetInstance().SetTiles(&m_gameState->GetTiles());

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

    float moveX = 0.0f;
    if (cmd.moveLeft) moveX -= 1.0f;
    if (cmd.moveRight) moveX += 1.0f;

    Vector2 vel = player->GetVelocity();
    vel.x = moveX * PLAYER_SPEED;
    if (moveX > 0.0f) player->SetDirection(Direction::Right);
    else if (moveX < 0.0f) player->SetDirection(Direction::Left);

    m_playerOnGround = IsOnGround(player);
    if (cmd.jump && m_playerOnGround) {
        vel.y = PLAYER_JUMP_FORCE;
    }
    player->SetVelocity(vel);

    if (cmd.attack && player->CanAttack()) {
        player->Attack();
        View::CharacterRenderer::GetInstance().PlayAction(
            static_cast<uint32_t>(player->GetId()), View::ACTION_ATTACK);
        SoundManager::GetInstance().PlaySound("player_attack");
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

        ApplyGravity(enemy, dt);
        ResolveTileCollisions(enemy, dt);
    }
}

void GameController::UpdateCombat(float dt) {
    Player* player = m_gameState->GetLocalPlayer();
    if (!player) return;

    if (player->GetState() == Character::State::Attack) {
        Rectangle attackBox = player->GetAttackBoundingBox();
        for (auto& entity : m_gameState->GetAllEntities()) {
            if (entity->GetType() != EntityType::Enemy || !entity->IsActive()) continue;
            auto* enemy = static_cast<Enemy*>(entity.get());
            if (!RectOverlap(attackBox, enemy->GetBoundingBox())) continue;
            if (enemy->GetState() == EnemyState::Hurt) continue;

            enemy->TakeDamage(25);
            SoundManager::GetInstance().PlaySound("enemy_hurt");
            View::FloatingTextManager::GetInstance().Emit(
                enemy->GetPosition(), "-25", RED, 1.0f);
            View::ParticleRenderer::GetInstance().EmitBurst(
                enemy->GetPosition(), 8, WHITE);
            View::GameView::GetInstance().Shake(3.0f, 0.15f);

            if (!enemy->IsActive()) {
                OnEntityRemoved(enemy);
            } else {
                enemy->SetState(EnemyState::Hurt);
            }
        }
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
        player->TakeDamage(enemy->GetDamage());
        SoundManager::GetInstance().PlaySound("player_hurt");
        View::FloatingTextManager::GetInstance().Emit(
            player->GetPosition(), "-" + std::to_string(enemy->GetDamage()), RED, 1.0f);
        View::GameView::GetInstance().Shake(4.0f, 0.2f);
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

    int activeEnemies = 0;
    for (const auto& entity : m_gameState->GetAllEntities()) {
        if (entity->GetType() == EntityType::Enemy && entity->IsActive()) {
            activeEnemies++;
        }
    }

    if (activeEnemies == 0 && m_defeatedEnemies >= m_gameState->GetTotalEnemies()) {
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
    m_particles.Update(dt);
    m_gameState->TickTimer(dt);

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
    View::GameView::GetInstance().Shutdown();
    View::HUDView::GetInstance().Shutdown();
    m_gameState.reset();
    m_running = false;
}
