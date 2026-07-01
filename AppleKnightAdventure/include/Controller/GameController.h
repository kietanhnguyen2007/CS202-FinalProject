#ifndef GAMECONTROLLER_H
#define GAMECONTROLLER_H

#include "Model/GameState.h"
#include "Model/LevelScoring.h"
#include "Systems/CollisionSystem.h"
#include "Systems/ParticleSystem.h"
#include "Utils/Types.h"
#include "raylib.h"
#include <memory>
#include <string>

class Player;
class Enemy;
class Chest;
class Checkpoint;
class Item;
class Entity;
struct InputCommand;

class GameController {
public:
    static GameController& GetInstance();

    bool Init();
    void StartLevel(int levelNumber);
    void Update(float dt);
    void Render();
    void Shutdown();

    bool ShouldReturnToMenu() const { return m_returnToMenu; }
    bool IsRunning() const { return m_running; }

private:
    GameController() = default;

    void LoadTilesets();
    void RegisterEntityVisuals(Entity* entity);
    void RegisterPlayerVisuals(Player* player, CharacterClass playerClass);
    void RegisterEnemyVisuals(Enemy* enemy);
    void RegisterChestVisuals(Chest* chest);
    void RegisterCheckpointVisuals(Checkpoint* checkpoint);
    void RegisterItemVisuals(Item* item);
    void UnregisterEntityVisuals(int entityId);

    void HandlePlayerInput(const struct InputCommand& cmd, float dt);
    void ApplyGravity(Character* character, float dt);
    void ResolveTileCollisions(Character* character, float dt);
    bool IsOnGround(const Character* character) const;
    void UpdateEnemyAI(float dt);
    void UpdateCombat(float dt);
    void UpdateInteractions(const struct InputCommand& cmd);
    void UpdateItems(float dt);
    void CheckLevelComplete();
    void OnEntityRemoved(Entity* entity);
    void RespawnPlayer();

    std::string GetLevelPath(int levelNumber) const;
    std::string GetPlayerAtlasRoot(CharacterClass playerClass) const;

    std::unique_ptr<GameState> m_gameState;
    LevelScoring m_scoring;
    CollisionSystem m_collision;
    ParticleSystem m_particles;

    Camera2D m_camera{};
    Vector2 m_respawnPoint{0.0f, 0.0f};
    bool m_running = false;
    bool m_paused = false;
    bool m_returnToMenu = false;
    bool m_levelComplete = false;
    bool m_playerOnGround = false;
    int m_defeatedEnemies = 0;
    int m_collectedItems = 0;
    float m_enemyAttackCooldown = 0.0f;
};

#endif
