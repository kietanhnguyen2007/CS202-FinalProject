#pragma once

#include "Survival3D/Animation/AnimationEvents.h"
#include "Survival3D/Animation/AnimationGraph.h"
#include "Survival3D/Model/SurvivalTypes.h"
#include "Survival3D/Systems/RuntimeIK.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace Survival3D {

class SurvivalController {
public:
    static SurvivalController& GetInstance();

    bool Init();
    void Start();
    void Update(float frameDt);
    void Render() const;
    void Shutdown();

    bool ShouldReturnToMenu() const { return m_returnToMenu; }
    bool IsPaused() const { return m_paused; }
    Phase GetPhase() const { return m_phase; }
    const PlayerState& GetPlayer() const { return m_player; }
    void EquipWeapon(WeaponId weapon);
    void UnequipWeapon();
    const std::vector<EnemyState>& GetEnemies() const { return m_enemies; }
    const std::vector<ProjectileState>& GetProjectiles() const { return m_projectiles; }
    const Camera3D& GetCamera() const { return m_camera; }
    const std::array<UpgradeOption, 3>& GetUpgradeOptions() const { return m_upgradeOptions; }
    int GetWave() const { return m_wave; }
    int GetActiveEnemyCount() const { return m_activeEnemies; }
    int GetRemainingSpawnCount() const { return m_remainingToSpawn; }
    int GetSelectedCharacter() const { return m_selectedCharacter; }
    int GetSelectedUpgrade() const { return m_selectedUpgrade; }
    int GetSelectedResultAction() const { return m_selectedResultAction; }
    int GetKills() const { return m_kills; }
    int GetScore() const { return m_score; }
    float GetRunTime() const { return m_runTime; }
    float GetPhaseTimer() const { return m_phaseTimer; }
    float GetAttackFxTimer() const { return m_attackFxTimer; }
    float GetSkillFxTimer() const { return m_skillFxTimer; }
    float GetSkillFxRadius() const { return m_skillFxRadius; }
    Vector3 GetSkillFxCenter() const { return m_skillFxCenter; }
    float GetGravityWellTimer() const { return m_gravityWellTimer; }
    Vector3 GetGravityWellCenter() const { return m_gravityWellCenter; }
    const CombatFeedbackState& GetCombatFeedback() const { return m_combatFeedback; }
    float GetHitStopTimer() const { return m_hitStopTimer; }
    float GetCameraShakeTimer() const { return m_cameraShakeTimer; }
    float GetCameraShakeIntensity() const { return m_cameraShakeIntensity; }
    int GetUpgradeStack(UpgradeId id) const {
        return m_upgradeStacks[static_cast<std::size_t>(id)];
    }
    const std::string& GetBalanceVersion() const { return m_balanceVersion; }
    bool IsRecordsVisible() const { return m_showRecords; }
    int GetBossesKilled() const { return m_bossesKilled; }
    int GetDamageTaken() const { return m_damageTaken; }
    bool IsHighContrast() const { return m_highContrast; }
    bool IsReducedMotion() const { return m_reducedMotion; }
    bool IsPerformanceVisible() const { return m_showPerformance; }
    float GetUiScale() const { return m_uiScale; }
    float GetAverageFrameMs() const { return m_averageFrameMs; }
    float GetPeakFrameMs() const { return m_peakFrameMs; }
    int GetDroppedTicks() const { return m_droppedTicks; }
    int GetActiveProjectileCount() const { return m_activeProjectiles; }
    int GetPeakEnemyCount() const { return m_peakEnemies; }
    bool IsTargetLockActive() const { return m_targetLock; }
    bool GetLockedTargetPosition(Vector3& position) const;

private:
    SurvivalController() = default;

    void ResetRun(CharacterId character);
    void PollFrameInput();
    void FixedUpdate(float dt);
    void UpdatePlayer(float dt);
    void UpdateWave(float dt);
    void UpdateEnemies(float dt);
    void AdvanceEnemyAnimation(std::size_t index, float dt);
    void AdvanceEnemyEventTimeline(std::size_t index, float dt,
                                   bool stateChanged);
    void UpdateProjectiles(float dt);
    void UpdateCamera(float frameDt);
    void UpdateCursorCapture();
    std::size_t FindCameraTarget(bool hardLock) const;
    Vector3 ResolveCameraCollision(Vector3 target, Vector3 desired) const;
    void BeginWave(int wave);
    void EnterUpgradeChoice();
    void GenerateUpgradeOptions();
    void ApplyUpgrade(int index);
    void FinalizeRun(bool victory);
    bool LoadBalanceConfig();

    std::size_t AcquireEnemy();
    void ReleaseEnemy(std::size_t index);
    std::size_t AcquireProjectile();
    void ReleaseProjectile(std::size_t index);
    void SpawnEnemy();
    void SpawnEnemyAt(EnemyArchetype archetype, Vector3 position);
    void SpawnArcBolt();
    void SpawnHostileProjectile(Vector3 origin, Vector3 target, float speed,
                                float damage, float radius, float splashRadius = 0.0f);
    void UpdateRiftling(std::size_t index, float dt);
    void UpdateHexArcher(std::size_t index, float dt);
    void UpdateObsidianBrute(std::size_t index, float dt);
    void UpdateBroodWarden(std::size_t index, float dt);
    void UpdateHexeyeArtillerist(std::size_t index, float dt);
    void UpdateIronrootColossus(std::size_t index, float dt);
    void UpdateEclipseChimera(std::size_t index, float dt);
    void UpdateVoidSovereign(std::size_t index, float dt);
    void DamagePlayer(float damage);
    void DamageEnemy(std::size_t index, float damage, Vector3 knockback);
    void PerformBasicAttack();
    void PerformSkillOne();
    void PerformSkillTwo();
    void PerformUltimate();
    void PerformDash();
    void SetPlayerAnimation(PlayerAnimation animation, float duration);
    void BeginPlayerAction(PlayerCombatAction action, PlayerAnimation animation,
                           float duration, float contactNormalized);
    void UpdatePlayerActionTimeline();
    void UpdatePlayerCombo();
    void BeginComboStep(Animation::ComboMove move);
    void UpdatePlayerAnimationGraph();
    void UpdatePlayerRuntimeIk(float dt);
    void ApplyPlayerRootMotion(float previousProgress, float currentProgress);
    static void OnPlayerAnimationEvent(
        const Animation::EventOccurrence& occurrence, void* user) noexcept;
    void HandlePlayerAnimationEvent(const Animation::FrameEvent& event);
    struct EnemyEventDispatchContext {
        SurvivalController* controller = nullptr;
        std::size_t index = 0;
    };
    static void OnEnemyAnimationEvent(
        const Animation::EventOccurrence& occurrence, void* user) noexcept;
    void HandleEnemyAnimationEvent(std::size_t index,
                                   const Animation::FrameEvent& event);
    void ResolvePlayerActionContact();
    void CancelPlayerAction();
    void EmitCombatFeedback(CombatCue cue, Vector3 origin, Vector3 direction,
                            float radius, float intensity,
                            float hitStopSeconds = 0.0f,
                            float cameraShakeSeconds = 0.0f);

    float NextRandom01();
    float NextUpgradeRandom01();
    float HpScale() const;
    float DamageScale() const;
    float Cooldown(float baseSeconds) const;
    void SetPhase(Phase phase, float timer = 0.0f);

    struct WaveRule {
        float swarm = 1.0f;
        float ranger = 0.0f;
        float tanker = 0.0f;
        float budgetMultiplier = 1.0f;
        bool bossWave = false;
        EnemyArchetype boss = EnemyArchetype::BroodWarden;
    };

    struct BalanceConfig {
        float hpLinear = 0.055f;
        float hpQuadratic = 0.00120f;
        float damageLinear = 0.032f;
        float damageQuadratic = 0.00045f;
        float budgetBase = 8.0f;
        float budgetLinear = 1.75f;
        float budgetQuadratic = 0.045f;
        float spawnInterval = 0.75f;
        float minSpawnInterval = 0.22f;
        int activeCapBase = 24;
        float activeCapPerWave = 1.9f;
        int activeCapMax = 120;
        float enemyHpMultiplier = 1.0f;
        float enemyDamageMultiplier = 1.0f;
        float enemySpeedMultiplier = 1.0f;
        float playerHealthMultiplier = 1.0f;
        float playerDamageMultiplier = 1.0f;
        float playerCooldownMultiplier = 1.0f;
        float waveHealFraction = 0.0f;
    };

    static constexpr float kFixedDt = 1.0f / 60.0f;
    static constexpr float kArenaHalfExtent = 19.5f;
    static constexpr std::size_t kEnemyCapacity = 144;
    static constexpr std::size_t kProjectileCapacity = 384;

    bool m_initialized = false;
    bool m_returnToMenu = false;
    bool m_paused = false;
    Phase m_phase = Phase::CharacterSelect;
    PlayerState m_player{};
    Camera3D m_camera{};
    std::vector<EnemyState> m_enemies;
    std::vector<std::size_t> m_freeEnemies;
    std::vector<ProjectileState> m_projectiles;
    std::vector<std::size_t> m_freeProjectiles;
    std::array<UpgradeOption, 3> m_upgradeOptions{};
    std::array<int, static_cast<std::size_t>(UpgradeId::Count)> m_upgradeStacks{};
    std::array<WaveRule, 50> m_waveRules{};
    BalanceConfig m_balance{};
    std::string m_balanceVersion = "fallback-1";

    int m_selectedCharacter = 0;
    int m_selectedUpgrade = 0;
    int m_selectedResultAction = 0;
    int m_wave = 1;
    int m_activeEnemies = 0;
    int m_remainingToSpawn = 0;
    int m_kills = 0;
    int m_score = 0;
    int m_bossesKilled = 0;
    int m_damageTaken = 0;
    int m_activeProjectiles = 0;
    int m_peakEnemies = 0;
    int m_peakProjectiles = 0;
    int m_droppedTicks = 0;
    std::uint64_t m_simTick = 0;
    int m_choicesWithoutEpic = 0;
    int m_chainHitCounter = 0;
    float m_accumulator = 0.0f;
    float m_runTime = 0.0f;
    float m_phaseTimer = 0.0f;
    float m_spawnTimer = 0.0f;
    float m_spawnInterval = 0.75f;
    float m_attackFxTimer = 0.0f;
    float m_skillFxTimer = 0.0f;
    float m_skillFxRadius = 0.0f;
    Vector3 m_skillFxCenter{};
    float m_gravityWellTimer = 0.0f;
    Vector3 m_gravityWellCenter{};
    CombatFeedbackState m_combatFeedback{};
    Vector3 m_playerActionAim{};
    Vector3 m_playerActionStart{};
    Vector3 m_playerActionFacing{0.0f, 0.0f, 1.0f};
    float m_hitStopTimer = 0.0f;
    float m_cameraShakeTimer = 0.0f;
    float m_cameraShakeIntensity = 0.0f;
    float m_cameraYaw = 0.7853982f;
    float m_cameraPitch = 0.56f;
    float m_cameraDistance = 7.6f;
    float m_cameraUserDistance = 7.6f;
    std::size_t m_lockedEnemyIndex = kEnemyCapacity;
    std::uint32_t m_rngState = 0xA3615EEDu;
    std::uint32_t m_upgradeRngState = 0x91E10DA5u;

    Animation::StateMachine m_playerAnimationGraph{Animation::ActorClass::Hero};
    std::vector<Animation::StateMachine> m_enemyAnimationGraphs;
    std::vector<Animation::EventCursor> m_enemyEventCursors;
    Animation::ClipLibrary m_knightClipLibrary;
    Animation::ClipLibrary m_mageClipLibrary;
    Animation::ClipLibrary m_enemyClipLibrary;
    Animation::ClipLibrary m_bossClipLibrary;
    Animation::EventCursor m_playerEventCursor;
    Animation::HeroTrackId m_playerEventTrack = Animation::HeroTrackId::KnightBasic;
    Animation::ComboBuffer m_playerCombo;
    RuntimeIK::FootIkState m_playerFootIkState;
    RuntimeIK::AimState m_playerAimIkState;
    std::uint64_t m_animationGraphSerial = 0;
    float m_previousActionProgress = 0.0f;
    bool m_playerEventTrackActive = false;

    Vector3 m_moveInput{};
    Vector3 m_aimPoint{0.0f, 0.0f, 1.0f};
    bool m_basicQueued = false;
    bool m_skillOneQueued = false;
    bool m_skillTwoQueued = false;
    bool m_ultimateQueued = false;
    bool m_dashQueued = false;
    bool m_jumpQueued = false;
    bool m_chainResolving = false;
    bool m_showRecords = false;
    bool m_runRecorded = false;
    std::string m_runId;
    bool m_highContrast = false;
    bool m_reducedMotion = false;
    bool m_showPerformance = false;
    bool m_targetLock = false;
    bool m_mouseCaptured = false;
    float m_uiScale = 1.0f;
    float m_averageFrameMs = 16.67f;
    float m_peakFrameMs = 0.0f;
};

} // namespace Survival3D
