#ifndef GAMECONTROLLER_H
#define GAMECONTROLLER_H

#include "Model/GameState.h"
#include "Model/LevelScoring.h"
#include "Model/Pet.h"
#include "Model/Projectile.h"
#include "Model/FighterSkillSet.h"
#include "Model/MagicCasterSkillSet.h"
#include "Model/NinjaSkillSet.h"
#include "Model/Inventory.h"
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

// Snapshot of player state saved when entering a Boss Arena,
// restored on return so HP / score / inventory persist across the transition.
struct PlayerSaveState {
    int   health      = 0;
    int   score       = 0;
    int   skillPoints = 0;
    int   coins       = 0;
    int   apples      = 0;
    int   keys        = 0;
};

class GameController {
public:
    static GameController& GetInstance();

    bool Init();
    void ConfigureLocalCoop(bool enabled, CharacterClass secondPlayerClass = CharacterClass::Knight);
    void StartLevel(int levelNumber);
    void Update(float dt);
    void Render();
    void Shutdown();

    void RegisterEntityVisuals(Entity* entity);
    void UnregisterEntityVisuals(int entityId);

    bool ShouldReturnToMenu() const { return m_returnToMenu; }
    bool IsRunning() const { return m_running; }
    bool IsPlaytest() const { return m_gameState && m_gameState->GetCurrentLevel() == -99; }

private:
    GameController() = default;

    void LoadTilesets();
    void RegisterPlayerVisuals(Player* player, CharacterClass playerClass);
    void RegisterEnemyVisuals(Enemy* enemy);
    void RegisterBossVisuals(Boss* boss);          // NEW — LDtk boss spawn
    void RegisterPortalVisuals(class TeleportPortal* portal);
    void RegisterChestVisuals(Chest* chest);
    void RegisterCheckpointVisuals(Checkpoint* checkpoint);
    void RegisterItemVisuals(Item* item);

    void HandlePlayerInput(Player* player, const struct InputCommand& cmd, float dt);
    void ApplyGravity(Character* character, float dt);
    void ResolveTileCollisions(Character* character, float dt);
    void ClampEntityToMapBounds(Entity* entity);
    bool IsOnGround(const Character* character) const;
    bool IsRectOnGround(Rectangle box) const;  // item/entity ground check
    void UpdateEnemyAI(float dt);
    void UpdateCombat(Player* player, float dt, bool updateEnemyCooldown);
    void UpdateInteractions(Player* player, const struct InputCommand& cmd);
    void UpdateItems(Player* player, float dt);
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
    void RespawnPlayer(Player* player, bool restoreEncounter = true);
    void CaptureCheckpointEnemies(float checkpointX);
    void RestoreCheckpointEnemies();

    // Boss Arena helpers
    void UpdateBossArenaPortals();          // lock/unlock exit portal based on enemies alive
    void SavePlayerState(Player* player);   // snapshot before entering boss arena
    void RestorePlayerState(Player* player);// restore after returning

    // Skill projectile helpers
    bool CheckLineOfSight(Vector2 start, Vector2 end) const;
    bool FindNearestVisibleEnemyInCamera(Vector2 origin, Vector2& targetCenter) const;
    void SpawnPlayerProjectile(const char* atlasPath, Vector2 spawnPos, Direction dir,
                               int damage, float speed, float lifetime, 
                               float scale = 0.3f, bool facesLeft = true,
                               Vector2 hitboxSize = {0.0f, 0.0f});
    void SpawnLightningAt(Vector2 targetPos, int damage, float lifetime,
                          const char* atlasPath = "assets/textures/player/magic_caster_v2/projectile_attack1_v2.json",
                          float rotation = 0.0f, float scale = 0.4f);
    void UpdatePlayerProjectiles(float dt);
    void UpdateNinjaTeleport(Player* player, float dt);

    // Player skill projectile list (separate from pet projectiles)
    std::vector<std::unique_ptr<Projectile>> m_playerProjectiles;

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
    int  m_pauseSelected  = 0;
    bool m_returnToMenu   = false;
    bool m_levelComplete  = false;
    bool m_playerOnGround = false;
    bool m_localCoop      = false;
    CharacterClass m_secondPlayerClass = CharacterClass::Knight;
    int  m_defeatedEnemies = 0;
    int  m_collectedItems  = 0;
    float m_enemyAttackCooldown = 0.0f;
    float m_footstepTimer = 0.0f;
    std::unordered_map<int, int> m_knownBossPhases;

    // Pet system
    std::unique_ptr<Pet>               m_activePet;
    std::vector<std::unique_ptr<Projectile>> m_petProjectiles;
    bool  m_inCombat           = false;
    float m_combatExitTimer    = 0.0f;  // grace period before 'out of combat'
    static constexpr float COMBAT_EXIT_GRACE = 4.0f;
    static constexpr float PET_COMBAT_RANGE  = 350.0f;

    // Endgame checkpoint animation state machine
    // Phase: uncaptured -> flag_out playing -> captured (loop)
    std::set<int> m_endgameFlagRevealedIds;
    std::set<int> m_endgameFlagCapturedIds;
    std::unordered_map<int, float> m_flagOutTimers;
    static constexpr float FLAG_OUT_DURATION = 1.35f;

    std::set<uint32_t> m_registeredEntities;

    // Boss Arena transition state
    int              m_previousLevelId = -1;          // level to return to after boss arena
    Vector2          m_exitSpawnPos    = {0.0f, 0.0f};// position near entry portal
    PlayerSaveState  m_savedPlayerState;              // HP/score/inventory snapshot
    bool             m_hasSavedState   = false;       // true when snapshot is valid
    uint32_t         m_activeCheckpointUid = 0;       // track highest checkpoint activated
    std::set<int>    m_checkpointRespawnEnemyIds;     // enemies after the active checkpoint
    std::set<int>    m_countedDefeatedEnemyIds;       // prevents score farming after respawn
};

#endif
