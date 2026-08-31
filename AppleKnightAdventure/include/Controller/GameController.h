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
#include "Model/BuffPickup.h"
#include "Systems/CollisionSystem.h"
#include "Systems/ParticleSystem.h"
#include "Systems/ElementalSystem.h"
#include "Systems/CoreSystem.h"
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
    bool  tookDamage  = false;
    // Cores are earned for the whole run, so they have to survive the round
    // trip into a boss arena and back -- StartLevel builds a brand new Player.
    int   maxHealth   = 0;
    CoreLoadout cores;
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
    void UpdateEndgameCheckpoints();            // F-triggered finish flag animation state machine
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
    // spawnCenter is the center of the projectile's collision box, which is also
    // where its sprite is centered. Pass the point the effect should appear at.
    void SpawnPlayerProjectile(const char* atlasPath, Vector2 spawnCenter, Direction dir,
                               int damage, float speed, float lifetime,
                               float scale = 0.3f, bool facesLeft = true,
                               Vector2 hitboxSize = {0.0f, 0.0f},
                               DamageType element = DamageType::Physical);
    // Center of the drawn player sprite, and the facing edge of its body. Skill
    // effects should originate here rather than at a corner of the physics box.
    static Vector2 PlayerSkillOrigin(const Player* player);
    void SpawnLightningAt(Vector2 targetPos, int damage, float lifetime,
                          const char* atlasPath = "assets/textures/player/magic_caster_v2/projectile_attack1_v2.json",
                          float rotation = 0.0f, float scale = 0.4f,
                          DamageType element = DamageType::Physical);
    void UpdatePlayerProjectiles(float dt);
    void UpdateNinjaTeleport(Player* player, float dt);

    // Occasional wandering enemies, so a cleared stretch of map does not stay
    // empty. Deliberately sparse: see the RANDOM_SPAWN_* constants.
    void UpdateRandomEnemySpawns(float dt);
    bool FindRandomSpawnPoint(Vector2 playerPos, Vector2& outPos) const;
    bool IsInBossArena() const { return m_previousLevelId != -1; }
    // True while the player is actually engaged with a live boss, whichever way
    // they got to it. The boon systems key off this rather than IsInBossArena()
    // -- a boss placed directly in a campaign level is just as much a boss
    // fight as one behind an arena portal, and only the portal sets that flag.
    bool IsBossFightActive() const;

    // Boss-arena boons: an orb drops every few seconds, the player picks which
    // one to run for. Owned here rather than in GameState so the rest of the
    // entity pipeline does not need to know about a new entity kind.
    void UpdateBuffPickups(float dt);
    bool FindBuffSpawnPoint(Vector2 playerPos, Vector2& outPos) const;
    void CollectBuff(Player* player, BuffPickup* orb);
    void RenderBuffPickups() const;
    void RenderBuffHud() const;
    std::vector<std::unique_ptr<BuffPickup>> m_buffPickups;
    float m_buffSpawnTimer = 0.0f;

    // Boss-arena boon draft: every 10-15s the fight freezes and three boons are
    // offered, chosen with 1/2/3. Separate from the orb drops above -- the orbs
    // are ambient, this is the guaranteed pick.
    void UpdateBuffOffer(float dt);
    void RenderBuffOffer() const;
    void OpenBuffOffer();
    void TakeBuffOffer(int index);
    float NextBuffOfferDelay() const;
    bool  IsBuffOfferOpen() const { return m_buffOfferOpen; }
    std::vector<BuffType> m_buffOffer;
    float m_buffOfferTimer  = 0.0f;
    bool  m_buffOfferOpen   = false;
    float m_buffOfferAnim   = 0.0f;

    // ---- Core draft (run-long upgrades) ----
    // The campaign counterpart of the Survival3D upgrade screen. A draft is
    // earned by clearing enemies and by killing a boss, and what is picked is
    // kept for the rest of the run rather than expiring like a boon.
    void OpenCoreDraft(bool bossReward);
    void UpdateCoreDraft(float dt);
    void RenderCoreDraft() const;
    void RenderCoreHud() const;
    void TakeCoreOffer(int index);
    void OnEnemyDefeatedForCores(bool wasBoss);
    // Spends the Second Wind core if the player has one. True when the death
    // was cancelled and no respawn should happen.
    bool TryRevive(Player* player);
    std::vector<CoreId> m_coreOffer;
    bool  m_coreDraftOpen    = false;
    bool  m_coreDraftBoss    = false;
    float m_coreDraftAnim    = 0.0f;
    int   m_killsTowardCore  = 0;
    // Drafts earned but not yet shown, so two kills in one frame cannot lose a
    // draft and a draft is never opened on top of another overlay.
    int   m_pendingCoreDrafts = 0;

    // ---- Spatial broad phase ----
    // The quadtree in CollisionSystem was built but never fed. Combat queries
    // now go through it instead of sweeping every entity in the level.
    // Entities move every frame, so the tree is rebuilt once per frame, right
    // before combat runs -- entity removals all happen after that point, so no
    // query can ever see a pointer that has since been freed.
    void RebuildSpatialIndex();
    // Deduplicated: an entity straddling a node boundary is stored in several
    // leaves, and a melee swing must never hit the same target twice.
    // Returns by value on purpose -- a reaction's splash queries again from
    // inside a loop over an earlier result, and a shared scratch buffer would
    // be rewritten mid-iteration.
    std::vector<Entity*> QueryEntitiesInRect(Rectangle range) const;

    // ---- Elemental codex ----
    // A player has no way to discover a twelve-row reaction table by flailing
    // at enemies, so C opens a reference built from the live table.
    void RenderElementCodex() const;
    bool m_codexOpen = false;

    // ---- Impact feedback ----
    // Hit-stop: the simulation holds for a few dozen milliseconds on a heavy
    // hit so the moment of contact reads. Ported from the Survival3D mode,
    // which already had it. Rendering keeps running, so the frozen frame is
    // still drawn and the pause looks deliberate rather than like a stall.
    void RequestHitStop(float seconds);
    float m_hitStopTimer = 0.0f;

    // ---- Elemental combat ----
    // Single funnel for every hit that carries an element: resolves the
    // reaction, applies the scaled damage and floats the readout. Returns the
    // damage actually dealt.
    int ApplyElementalHit(Entity* target, int baseDamage, DamageType element);
    // Collateral damage to everything standing near the target. Shared by
    // elemental reactions and the Chain Spark core. Never applies an aura, so
    // it can never chain into itself.
    void SplashDamage(Entity* epicenter, int splashDamage, float radius, Color color);
    // Ticks auras, drains their damage-over-time and pushes the resulting slow
    // onto each affected character.
    void UpdateElementalEffects(float dt);
    // Kills any aura on an entity that is leaving the world.
    void ClearElementalState(Entity* entity);

    // Player skill projectile list (separate from pet projectiles)
    std::vector<std::unique_ptr<Projectile>> m_playerProjectiles;

    std::string GetLevelPath(int levelNumber) const;
    std::string GetPlayerAtlasRoot(CharacterClass playerClass) const;

    std::unique_ptr<GameState> m_gameState;
    LevelScoring m_scoring;
    CollisionSystem m_collision;
    ParticleSystem m_particles;
    ElementalSystem m_elemental;

    Camera2D m_camera{};
    Vector2 m_respawnPoint{0.0f, 0.0f};
    bool m_running        = false;
    bool m_paused         = false;
    int  m_pauseSelected  = 0;
    bool m_returnToMenu   = false;
    bool m_levelComplete  = false;
    bool m_playerOnGround = false;
    bool m_localCoop      = false;
    bool m_runTookDamage  = false;
    CharacterClass m_secondPlayerClass = CharacterClass::Knight;
    int  m_defeatedEnemies = 0;
    int  m_collectedItems  = 0;
    float m_enemyAttackCooldown = 0.0f;
    float m_footstepTimer = 0.0f;
    std::unordered_map<int, int> m_knownBossPhases;

    // Random enemy spawning
    float m_randomSpawnTimer = 0.0f;
    std::vector<int> m_randomSpawnIds;   // ids of enemies this system created

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
    uint32_t         m_activeCheckpointUid = 0;       // most recently activated respawn checkpoint
    std::set<int>    m_checkpointRespawnEnemyIds;     // enemies after the active checkpoint
    std::set<int>    m_countedDefeatedEnemyIds;       // prevents score farming after respawn
};

#endif
