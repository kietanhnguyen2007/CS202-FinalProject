#pragma once

#include "raylib.h"
#include <array>
#include <cstdint>
#include <string>

namespace Survival3D {

enum class CharacterId : std::uint8_t {
    Knight,
    MagicCaster
};

enum class WeaponId : std::uint8_t {
    None,
    KnightGreatsword,
    MagicCasterStaff
};

constexpr WeaponId DefaultWeaponFor(CharacterId character) noexcept {
    return character == CharacterId::Knight
        ? WeaponId::KnightGreatsword
        : WeaponId::MagicCasterStaff;
}

enum class Phase : std::uint8_t {
    CharacterSelect,
    PreWave,
    Combat,
    WaveClear,
    UpgradeChoice,
    RunFailed,
    RunVictory
};

enum class EnemyArchetype : std::uint8_t {
    Riftling,
    HexArcher,
    ObsidianBrute,
    BroodWarden,
    HexeyeArtillerist,
    IronrootColossus,
    EclipseChimera,
    VoidSovereign,
    BossPrototype
};

enum class EnemyAction : std::uint8_t {
    None,
    RangedShot,
    GroundSlam,
    ClawSweep,
    TargetingVolley
};

enum class UpgradeRarity : std::uint8_t {
    Common,
    Rare,
    Epic,
    Legendary
};

enum class UpgradeId : std::uint8_t {
    VitalCore,
    SwiftStep,
    TemperedGuard,
    QuickHands,
    WideArc,
    RiftPower,
    Execution,
    GravityLens,
    EmergencyBarrier,
    EmberEdge,
    ChainSpark,
    SecondWind,
    GlassRift,
    Juggernaut,
    RoyalBulwark,
    ForkedBolt,
    EventHorizon,
    RiftEssence,
    Count
};

enum class PlayerAnimation : std::uint8_t {
    Idle,
    Run,
    BasicAttack,
    SkillOne,
    SkillTwo,
    Ultimate,
    Dash,
    Hurt,
    Death
};

// Gameplay actions deliberately live beside (but are not inferred from) the
// visual animation state.  The controller keeps one of these actions alive
// through windup, contact and recovery so repeated input cannot restart an
// attack before its authored contact pose is reached.
enum class PlayerCombatAction : std::uint8_t {
    None,
    Basic,
    SkillOne,
    SkillTwo,
    Ultimate,
    Dash
};

// A compact presentation event emitted at the exact gameplay contact frame.
// `serial` is monotonic, allowing the renderer/audio layer to consume an event
// once even when several fixed simulation ticks run during one rendered frame.
enum class CombatCue : std::uint8_t {
    None,
    KnightSlash,
    KnightGuard,
    KnightRush,
    KnightUltimate,
    MagicBoltRelease,
    MagicFrostNova,
    MagicGravityWell,
    MagicUltimate,
    DashBurst,
    MagicProjectileImpact
};

struct CombatFeedbackState {
    CombatCue cue = CombatCue::None;
    std::uint32_t serial = 0;
    Vector3 origin{};
    Vector3 direction{0.0f, 0.0f, 1.0f};
    float radius = 0.0f;
    float intensity = 0.0f;
};

enum class EnemyAnimation : std::uint8_t {
    Idle,
    Run,
    BasicAttack,
    SkillOne,
    SkillTwo,
    Ultimate,
    DashSpecial,
    Hurt,
    Death
};

// Renderer-facing output of the C++ 2D locomotion blend tree.  Keeping the
// compact weights in gameplay state lets the view consume the graph without
// owning gameplay transitions or inferring direction from a pose.
struct LocomotionBlendState {
    float idle = 1.0f;
    float forward = 0.0f;
    float backward = 0.0f;
    float left = 0.0f;
    float right = 0.0f;
    float normalizedSpeed = 0.0f;
};

struct RuntimeIkPresentationState {
    Vector3 aimTarget{};
    Vector3 mainHandTarget{};
    Vector3 supportHandTarget{};
    Vector3 mainElbowPole{};
    Vector3 supportElbowPole{};
    Vector3 leftFootTarget{};
    Vector3 rightFootTarget{};
    Vector3 leftFootNormal{0.0f, 1.0f, 0.0f};
    Vector3 rightFootNormal{0.0f, 1.0f, 0.0f};
    float pelvisOffset = 0.0f;
    float aimYawRadians = 0.0f;
    float aimPitchRadians = 0.0f;
    float leftFootWeight = 0.0f;
    float rightFootWeight = 0.0f;
};

struct PlayerState {
    CharacterId character = CharacterId::Knight;
    WeaponId equippedWeapon = WeaponId::None;
    Vector3 position{0.0f, 0.0f, 0.0f};
    Vector3 velocity{0.0f, 0.0f, 0.0f};
    Vector3 facing{0.0f, 0.0f, 1.0f};
    float verticalVelocity = 0.0f;
    bool grounded = true;
    float hp = 140.0f;
    float maxHp = 140.0f;
    float moveSpeed = 5.2f;
    float damageMultiplier = 1.0f;
    float damageReduction = 0.0f;
    float cooldownMultiplier = 1.0f;
    float areaMultiplier = 1.0f;
    float executionBonus = 0.0f;
    float projectileScale = 1.0f;
    float shield = 0.0f;
    float barrierCooldown = 0.0f;
    float ultimateCharge = 0.0f;
    float invulnerableTimer = 0.0f;
    float basicCooldown = 0.0f;
    float skillOneCooldown = 0.0f;
    float skillTwoCooldown = 0.0f;
    float dashCooldown = 0.0f;
    float guardTimer = 0.0f;
    float rushTimer = 0.0f;
    int projectilePierce = 0;
    bool secondWindAvailable = false;
    bool forkedBolt = false;
    bool eventHorizon = false;
    bool royalBulwark = false;
    PlayerAnimation animation = PlayerAnimation::Idle;
    PlayerAnimation previousAnimation = PlayerAnimation::Idle;
    float animationTime = 0.0f;
    float previousAnimationTime = 0.0f;
    float animationDuration = 0.0f;
    float animationBlend = 1.0f;
    PlayerCombatAction combatAction = PlayerCombatAction::None;
    // Normalized position of the authored contact pose inside the active clip.
    float actionContactNormalized = 0.0f;
    bool actionContactTriggered = false;
    std::uint32_t actionSerial = 0;
    std::uint32_t contactSerial = 0;
    LocomotionBlendState locomotionBlend{};
    RuntimeIkPresentationState runtimeIk{};
    std::uint32_t upperBodyMask = 0;
    float upperBodyLayerWeight = 0.0f;
    std::uint8_t comboStep = 0;
    bool actionHitboxActive = false;
};

struct EnemyState {
    bool active = false;
    bool dying = false;
    std::uint16_t generation = 0;
    EnemyArchetype archetype = EnemyArchetype::Riftling;
    Vector3 position{};
    Vector3 velocity{};
    Vector3 facing{0.0f, 0.0f, 1.0f};
    float hp = 0.0f;
    float maxHp = 0.0f;
    float moveSpeed = 0.0f;
    float damage = 0.0f;
    float attackCooldown = 0.0f;
    float slowTimer = 0.0f;
    float hitFlash = 0.0f;
    float burnTimer = 0.0f;
    float burnDps = 0.0f;
    float scale = 1.0f;
    float actionTimer = 0.0f;
    float specialCooldown = 0.0f;
    float specialFxTimer = 0.0f;
    float phaseTransitionTimer = 0.0f;
    float deathTimer = 0.0f;
    float strafeDirection = 1.0f;
    EnemyAction action = EnemyAction::None;
    EnemyAnimation visualAnimation = EnemyAnimation::Idle;
    EnemyAnimation previousVisualAnimation = EnemyAnimation::Idle;
    float animationTime = 0.0f;
    float previousAnimationTime = 0.0f;
    float animationBlend = 1.0f;
    LocomotionBlendState locomotionBlend{};
    bool actionHitboxActive = false;
    std::uint32_t actionEventSerial = 0;
    std::uint32_t lastPlayerActionHitSerial = 0;
    int bossPhase = 1;
};

enum class ProjectileVisual : std::uint8_t {
    HostileOrb,
    HostileArrow,
    ArcBolt
};

struct ProjectileState {
    bool active = false;
    std::uint16_t generation = 0;
    Vector3 position{};
    Vector3 velocity{};
    float damage = 0.0f;
    float radius = 0.18f;
    float lifetime = 0.0f;
    float splashRadius = 0.0f;
    bool hostile = false;
    int remainingPierce = 0;
    ProjectileVisual visual = ProjectileVisual::HostileOrb;
};

struct UpgradeOption {
    UpgradeId upgrade = UpgradeId::VitalCore;
    std::string id;
    std::string name;
    std::string description;
    Color accent{255, 255, 255, 255};
    UpgradeRarity rarity = UpgradeRarity::Common;
    int maxStacks = 1;
};

} // namespace Survival3D
