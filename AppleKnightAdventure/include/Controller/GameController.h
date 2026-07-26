#ifndef GAMECONTROLLER_H
#define GAMECONTROLLER_H

#include "Model/GameState.h"
#include "Model/LevelScoring.h"
#include "Model/Pet.h"
#include "Model/Projectile.h"
#include "Systems/CollisionSystem.h"
#include "Systems/ParticleSystem.h"
#include "Utils/Types.h"
#include "raylib.h"
#include <memory>
#include <string>
#include <vector>
#include <set>
#include <unordered_map>

class Player;
class Enemy;
class Boss;
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
    void RegisterBossVisuals(Boss* boss);          // NEW — LDtk boss spawn
    void RegisterChestVisuals(Chest* chest);
    void RegisterCheckpointVisuals(Checkpoint* checkpoint);
    void RegisterItemVisuals(Item* item);
    void UnregisterEntityVisuals(int entityId);

    void HandlePlayerInput(const struct InputCommand& cmd, float dt);
    void ApplyGravity(Character* character, float dt);
    void ResolveTileCollisions(Character* character, float dt);
    bool IsOnGround(const Character* character) const;
    bool IsRectOnGround(Rectangle box) const;  // item/entity ground check
    void UpdateEnemyAI(float dt);
    void UpdateCombat(float dt);
    void UpdateInteractions(const struct InputCommand& cmd);
    void UpdateItems(float dt);
    void UpdateItemPhysics(float dt);           // gravity + tile collision for coin scatter
    void UpdateEndgameCheckpoints();            // viewport reveal + flag animation state machine
    void UpdatePets(float dt, const struct InputCommand& cmd);
    void UpdateProjectiles(float dt);
    void SpawnPet(PetType type);
    void DespawnPet();
    void RegisterPetVisuals(Pet* pet);
    void FireDragonProjectile(Pet* pet);
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
    bool m_running        = false;
    bool m_paused         = false;
    bool m_returnToMenu   = false;
    bool m_levelComplete  = false;
    bool m_playerOnGround = false;
    int  m_defeatedEnemies = 0;
    int  m_collectedItems  = 0;
    float m_enemyAttackCooldown = 0.0f;

    // Pet system
    std::unique_ptr<Pet>               m_activePet;
    std::vector<std::unique_ptr<Projectile>> m_petProjectiles;
    bool  m_inCombat           = false;
    float m_combatExitTimer    = 0.0f;  // grace period before 'out of combat'
    static constexpr float COMBAT_EXIT_GRACE = 4.0f;
    static constexpr float PET_COMBAT_RANGE  = 350.0f;

    // Endgame checkpoint animation state machine
    // Phase: uncaptured -> flag_out playing -> captured (loop)
    std::set<int> m_endgameFlagRevealedIds;  // IDs where flag_out has been triggered
    std::set<int> m_endgameFlagCapturedIds;  // IDs now showing checkpoint_captured loop
    std::unordered_map<int, float> m_flagOutTimers; // tracks time since flag_out started
    static constexpr float FLAG_OUT_DURATION = 1.35f; // 27 frames x 0.05s
};

#endif
