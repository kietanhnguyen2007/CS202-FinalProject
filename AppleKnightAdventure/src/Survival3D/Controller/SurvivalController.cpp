#include "Survival3D/Controller/SurvivalController.h"
#include "Survival3D/View/SurvivalView.h"
#include "Survival3D/Systems/SurvivalRunService.h"
#include "Systems/SoundManager.h"
#include "Model/SaveManager.h"
#include "raymath.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <limits>

namespace Survival3D {
namespace {

float LengthXZ(Vector3 value) {
    return std::sqrt(value.x * value.x + value.z * value.z);
}

Vector3 NormalizeXZ(Vector3 value) {
    const float length = LengthXZ(value);
    if (length < 0.0001f) return {0.0f, 0.0f, 0.0f};
    return {value.x / length, 0.0f, value.z / length};
}

float DistanceXZ(Vector3 a, Vector3 b) {
    return LengthXZ({a.x - b.x, 0.0f, a.z - b.z});
}

float DotXZ(Vector3 a, Vector3 b) {
    return a.x * b.x + a.z * b.z;
}

float ShortestAngleDelta(float from, float to) {
    constexpr float kPi = 3.14159265358979323846f;
    constexpr float kTau = kPi * 2.0f;
    float delta = std::fmod(to - from, kTau);
    if (delta > kPi) delta -= kTau;
    if (delta < -kPi) delta += kTau;
    return delta;
}

Vector3 ClampToArena(Vector3 position, float halfExtent, float padding = 0.0f) {
    position.x = std::clamp(position.x, -halfExtent + padding, halfExtent - padding);
    position.z = std::clamp(position.z, -halfExtent + padding, halfExtent - padding);
    position.y = 0.0f;
    return position;
}

constexpr float kArenaPillarRadius = 0.95f;
constexpr std::array<Vector3, 4> kArenaPillars{{
    {-15.5f, 0.0f, -15.5f},
    { 15.5f, 0.0f, -15.5f},
    {-15.5f, 0.0f,  15.5f},
    { 15.5f, 0.0f,  15.5f}
}};

float DistancePointToSegmentXZ(Vector3 point, Vector3 start, Vector3 end) {
    const Vector3 segment{end.x - start.x, 0.0f, end.z - start.z};
    const Vector3 relative{point.x - start.x, 0.0f, point.z - start.z};
    const float lengthSquared = segment.x * segment.x + segment.z * segment.z;
    if (lengthSquared < 0.0001f) return DistanceXZ(point, start);
    const float t = std::clamp(DotXZ(relative, segment) / lengthSquared, 0.0f, 1.0f);
    return DistanceXZ(point, {start.x + segment.x * t, 0.0f, start.z + segment.z * t});
}

bool SegmentIntersectsArenaPillar(Vector3 start, Vector3 end, float padding = 0.0f) {
    for (const Vector3 pillar : kArenaPillars) {
        if (DistancePointToSegmentXZ(pillar, start, end)
            <= kArenaPillarRadius + std::max(0.0f, padding)) {
            return true;
        }
    }
    return false;
}

bool HasArenaLineOfSight(Vector3 start, Vector3 end) {
    return !SegmentIntersectsArenaPillar(start, end, 0.05f);
}

Vector3 ResolveArenaPillars(Vector3 position, float radius) {
    for (const Vector3 pillar : kArenaPillars) {
        Vector3 apart{position.x - pillar.x, 0.0f, position.z - pillar.z};
        float distance = LengthXZ(apart);
        const float minimumDistance = kArenaPillarRadius + std::max(0.0f, radius);
        if (distance >= minimumDistance) continue;
        if (distance < 0.0001f) {
            apart = {1.0f, 0.0f, 0.0f};
            distance = 1.0f;
        }
        const float correction = minimumDistance - distance;
        position.x += apart.x / distance * correction;
        position.z += apart.z / distance * correction;
    }
    return position;
}

Vector3 StopBeforeArenaPillar(Vector3 start, Vector3 end, float radius) {
    const Vector3 travel{end.x - start.x, 0.0f, end.z - start.z};
    const float a = DotXZ(travel, travel);
    if (a < 0.0001f) return ResolveArenaPillars(end, radius);

    float earliest = 1.0f;
    const float clearance = kArenaPillarRadius + std::max(0.0f, radius);
    for (const Vector3 pillar : kArenaPillars) {
        const Vector3 relative{start.x - pillar.x, 0.0f, start.z - pillar.z};
        const float b = 2.0f * DotXZ(relative, travel);
        const float c = DotXZ(relative, relative) - clearance * clearance;
        const float discriminant = b * b - 4.0f * a * c;
        if (discriminant < 0.0f) continue;
        const float contact = (-b - std::sqrt(discriminant)) / (2.0f * a);
        if (contact >= 0.0f && contact <= earliest) earliest = contact;
    }
    if (earliest < 1.0f) {
        const float travelLength = std::sqrt(a);
        earliest = std::max(0.0f, earliest - 0.04f / travelLength);
        end.x = start.x + travel.x * earliest;
        end.z = start.z + travel.z * earliest;
    }
    return ResolveArenaPillars(end, radius);
}

bool IsBoss(EnemyArchetype archetype) {
    return archetype == EnemyArchetype::BroodWarden
        || archetype == EnemyArchetype::HexeyeArtillerist
        || archetype == EnemyArchetype::IronrootColossus
        || archetype == EnemyArchetype::EclipseChimera
        || archetype == EnemyArchetype::VoidSovereign
        || archetype == EnemyArchetype::BossPrototype;
}

float CollisionRadius(const EnemyState& enemy) {
    switch (enemy.archetype) {
        case EnemyArchetype::Riftling: return 0.55f;
        case EnemyArchetype::HexArcher: return 0.50f;
        case EnemyArchetype::ObsidianBrute: return 1.30f;
        case EnemyArchetype::BroodWarden: return 1.60f;
        case EnemyArchetype::HexeyeArtillerist: return 1.20f;
        case EnemyArchetype::IronrootColossus: return 2.45f;
        case EnemyArchetype::EclipseChimera: return 3.20f;
        case EnemyArchetype::VoidSovereign:
            return enemy.bossPhase == 1 ? 0.65f : (enemy.bossPhase == 2 ? 1.25f : 1.95f);
        case EnemyArchetype::BossPrototype: return 1.30f;
    }
    return 0.55f;
}

EnemyAnimation ResolveEnemyAnimation(const EnemyState& enemy) {
    if (enemy.dying) return EnemyAnimation::Death;
    if (enemy.hitFlash > 0.0f) return EnemyAnimation::Hurt;
    if (enemy.phaseTransitionTimer > 0.0f) return EnemyAnimation::Ultimate;
    if (enemy.specialFxTimer > 0.0f) return EnemyAnimation::DashSpecial;
    switch (enemy.action) {
        // The currently authored Archer release and Brute slam both live in
        // the Basic clip. TargetingVolley is the charged SkillOne animation.
        case EnemyAction::RangedShot: return EnemyAnimation::BasicAttack;
        case EnemyAction::GroundSlam: return EnemyAnimation::BasicAttack;
        case EnemyAction::ClawSweep: return EnemyAnimation::BasicAttack;
        case EnemyAction::TargetingVolley: return EnemyAnimation::SkillOne;
        default: break;
    }
    return LengthXZ(enemy.velocity) > 0.10f
        ? EnemyAnimation::Run : EnemyAnimation::Idle;
}

Animation::State GraphState(PlayerAnimation animation) {
    switch (animation) {
        case PlayerAnimation::Run: return Animation::State::Locomotion;
        case PlayerAnimation::BasicAttack: return Animation::State::BasicAttack;
        case PlayerAnimation::SkillOne: return Animation::State::SkillOne;
        case PlayerAnimation::SkillTwo: return Animation::State::SkillTwo;
        case PlayerAnimation::Ultimate: return Animation::State::Ultimate;
        case PlayerAnimation::Dash: return Animation::State::Dash;
        case PlayerAnimation::Hurt: return Animation::State::Hurt;
        case PlayerAnimation::Death: return Animation::State::Death;
        case PlayerAnimation::Idle: break;
    }
    return Animation::State::Idle;
}

Animation::State GraphState(EnemyAnimation animation) {
    switch (animation) {
        case EnemyAnimation::Run: return Animation::State::Locomotion;
        case EnemyAnimation::BasicAttack: return Animation::State::BasicAttack;
        case EnemyAnimation::SkillOne: return Animation::State::SkillOne;
        case EnemyAnimation::SkillTwo: return Animation::State::SkillTwo;
        case EnemyAnimation::Ultimate: return Animation::State::PhaseTransition;
        case EnemyAnimation::DashSpecial: return Animation::State::Special;
        case EnemyAnimation::Hurt: return Animation::State::Hurt;
        case EnemyAnimation::Death: return Animation::State::Death;
        case EnemyAnimation::Idle: break;
    }
    return Animation::State::Idle;
}

Animation::TransitionReason GraphReason(EnemyAnimation animation) {
    switch (animation) {
        case EnemyAnimation::Death: return Animation::TransitionReason::Death;
        case EnemyAnimation::Hurt: return Animation::TransitionReason::HitReaction;
        case EnemyAnimation::Ultimate: return Animation::TransitionReason::PhaseChange;
        case EnemyAnimation::Idle:
        case EnemyAnimation::Run: return Animation::TransitionReason::Locomotion;
        default: return Animation::TransitionReason::Gameplay;
    }
}

Animation::HeroTrackId HeroTrack(CharacterId character,
                                  PlayerCombatAction action) {
    const bool mage = character == CharacterId::MagicCaster;
    switch (action) {
        case PlayerCombatAction::SkillOne:
            return mage ? Animation::HeroTrackId::MageSkillOne
                        : Animation::HeroTrackId::KnightSkillOne;
        case PlayerCombatAction::SkillTwo:
            return mage ? Animation::HeroTrackId::MageSkillTwo
                        : Animation::HeroTrackId::KnightSkillTwo;
        case PlayerCombatAction::Ultimate:
            return mage ? Animation::HeroTrackId::MageUltimate
                        : Animation::HeroTrackId::KnightUltimate;
        case PlayerCombatAction::Dash:
            return mage ? Animation::HeroTrackId::MageDash
                        : Animation::HeroTrackId::KnightDash;
        case PlayerCombatAction::Basic:
        case PlayerCombatAction::None:
            return mage ? Animation::HeroTrackId::MageBasic
                        : Animation::HeroTrackId::KnightBasic;
    }
    return Animation::HeroTrackId::KnightBasic;
}

bool NonHeroTrack(EnemyAnimation animation,
                  Animation::NonHeroTrackId& track) noexcept {
    switch (animation) {
        case EnemyAnimation::Run:
            track = Animation::NonHeroTrackId::Run;
            return true;
        case EnemyAnimation::BasicAttack:
            track = Animation::NonHeroTrackId::Basic;
            return true;
        case EnemyAnimation::SkillOne:
            track = Animation::NonHeroTrackId::SkillOne;
            return true;
        case EnemyAnimation::SkillTwo:
            track = Animation::NonHeroTrackId::SkillTwo;
            return true;
        case EnemyAnimation::Ultimate:
            track = Animation::NonHeroTrackId::UltimatePhase;
            return true;
        case EnemyAnimation::DashSpecial:
            track = Animation::NonHeroTrackId::Special;
            return true;
        case EnemyAnimation::Hurt:
            track = Animation::NonHeroTrackId::Hurt;
            return true;
        case EnemyAnimation::Death:
            track = Animation::NonHeroTrackId::Death;
            return true;
        case EnemyAnimation::Idle:
            break;
    }
    return false;
}

RuntimeIK::Vec3 IkVector(Vector3 value) noexcept {
    return {value.x, value.y, value.z};
}

Vector3 RayVector(RuntimeIK::Vec3 value) noexcept {
    return {value.x, value.y, value.z};
}

bool ProbeArenaFloor(const RuntimeIK::GroundRay& ray,
                     RuntimeIK::GroundHit& hit, void*) noexcept {
    if (ray.direction.y >= -0.0001f || ray.origin.y < 0.0f) return false;
    const float distance = ray.origin.y / -ray.direction.y;
    if (distance < 0.0f || distance > ray.maxDistance) return false;
    hit.point = ray.origin + ray.direction * distance;
    hit.point.y = 0.0f;
    hit.normal = {0.0f, 1.0f, 0.0f};
    hit.distance = distance;
    return true;
}

int SpawnCost(EnemyArchetype archetype) {
    if (archetype == EnemyArchetype::HexArcher) return 3;
    if (archetype == EnemyArchetype::ObsidianBrute) return 7;
    return 1;
}

struct UpgradeDefinition {
    UpgradeId id;
    const char* key;
    const char* name;
    const char* description;
    UpgradeRarity rarity;
    int maxStacks;
    int characterLock; // -1 shared, 0 Knight, 1 Magic Caster
};

const std::array<UpgradeDefinition, static_cast<std::size_t>(UpgradeId::Count)> kUpgrades{{
    {UpgradeId::VitalCore, "vital_core", "VITAL CORE", "+12 Max HP and heal 12 HP", UpgradeRarity::Common, 8, -1},
    {UpgradeId::SwiftStep, "swift_step", "SWIFT STEP", "+4% movement speed", UpgradeRarity::Common, 5, -1},
    {UpgradeId::TemperedGuard, "tempered_guard", "TEMPERED GUARD", "Take 4% less damage", UpgradeRarity::Common, 5, -1},
    {UpgradeId::QuickHands, "quick_hands", "QUICK HANDS", "All cooldowns recover 4% faster", UpgradeRarity::Common, 6, -1},
    {UpgradeId::WideArc, "wide_arc", "WIDE ARC", "+8% ability area and attack range", UpgradeRarity::Common, 6, -1},
    {UpgradeId::RiftPower, "rift_power", "RIFT POWER", "+18% damage for every ability", UpgradeRarity::Rare, 5, -1},
    {UpgradeId::Execution, "execution", "EXECUTION", "+20% damage to enemies below 25% HP", UpgradeRarity::Rare, 1, -1},
    {UpgradeId::GravityLens, "gravity_lens", "GRAVITY LENS", "Projectiles grow 12% and pierce once", UpgradeRarity::Rare, 2, -1},
    {UpgradeId::EmergencyBarrier, "emergency_barrier", "EMERGENCY BARRIER", "Gain a 20% HP shield below 30% HP", UpgradeRarity::Rare, 1, -1},
    {UpgradeId::EmberEdge, "ember_edge", "EMBER EDGE", "Hits have 20% chance to Burn for 3s", UpgradeRarity::Rare, 1, -1},
    {UpgradeId::ChainSpark, "chain_spark", "CHAIN SPARK", "Every sixth hit chains to nearby enemies", UpgradeRarity::Epic, 1, -1},
    {UpgradeId::SecondWind, "second_wind", "SECOND WIND", "Revive once with 35% HP", UpgradeRarity::Legendary, 1, -1},
    {UpgradeId::GlassRift, "glass_rift", "GLASS RIFT", "+45% damage but -25% Max HP", UpgradeRarity::Legendary, 1, -1},
    {UpgradeId::Juggernaut, "juggernaut", "JUGGERNAUT", "Shield Rush grows wider and hits harder", UpgradeRarity::Epic, 1, 0},
    {UpgradeId::RoyalBulwark, "royal_bulwark", "ROYAL BULWARK", "Guard lasts longer and protects at full strength", UpgradeRarity::Legendary, 1, 0},
    {UpgradeId::ForkedBolt, "forked_bolt", "FORKED BOLT", "Arc Bolt forks into two weaker bolts", UpgradeRarity::Rare, 1, 1},
    {UpgradeId::EventHorizon, "event_horizon", "EVENT HORIZON", "Gravity Well destroys hostile projectiles", UpgradeRarity::Epic, 1, 1},
    {UpgradeId::RiftEssence, "rift_essence", "RIFT ESSENCE", "+3% damage and +3% Max HP", UpgradeRarity::Common, 99, -1}
}};

Color RarityColor(UpgradeRarity rarity) {
    switch (rarity) {
        case UpgradeRarity::Rare: return Color{76, 170, 255, 255};
        case UpgradeRarity::Epic: return Color{190, 91, 255, 255};
        case UpgradeRarity::Legendary: return Color{255, 190, 64, 255};
        default: return Color{106, 229, 154, 255};
    }
}

} // namespace

SurvivalController& SurvivalController::GetInstance() {
    static SurvivalController instance;
    return instance;
}

bool SurvivalController::LoadBalanceConfig() {
    // Always create a complete fallback table first. Invalid designer data can
    // therefore disable only the bad entry instead of crashing the whole game.
    for (int wave = 1; wave <= 50; ++wave) {
        WaveRule rule;
        if (wave >= 3) {
            rule.ranger = std::min(0.40f, 0.15f + 0.012f * (wave - 3));
            rule.swarm -= rule.ranger;
        }
        if (wave >= 11) {
            rule.tanker = std::min(0.25f, 0.10f + 0.006f * (wave - 11));
            rule.swarm = std::max(0.25f, 1.0f - rule.ranger - rule.tanker);
        }
        if (wave % 10 == 0) {
            rule.bossWave = true;
            if (wave == 10) rule.boss = EnemyArchetype::BroodWarden;
            else if (wave == 20) rule.boss = EnemyArchetype::HexeyeArtillerist;
            else if (wave == 30) rule.boss = EnemyArchetype::IronrootColossus;
            else if (wave == 40) rule.boss = EnemyArchetype::EclipseChimera;
            else rule.boss = EnemyArchetype::VoidSovereign;
        }
        m_waveRules[wave - 1] = rule;
    }

    bool loaded = false;
    try {
        std::ifstream balanceFile("assets/survival3d/config/balance.json");
        if (balanceFile) {
            nlohmann::json root;
            balanceFile >> root;
            m_balanceVersion = root.value("balanceVersion", std::string("fallback-1"));
            const auto& scaling = root.at("scaling");
            m_balance.hpLinear = std::max(0.0f, scaling.value("hpLinear", m_balance.hpLinear));
            m_balance.hpQuadratic = std::max(0.0f, scaling.value("hpQuadratic", m_balance.hpQuadratic));
            m_balance.damageLinear = std::max(0.0f, scaling.value("damageLinear", m_balance.damageLinear));
            m_balance.damageQuadratic = std::max(0.0f, scaling.value("damageQuadratic", m_balance.damageQuadratic));
            const auto& director = root.at("director");
            m_balance.budgetBase = std::max(1.0f, director.value("budgetBase", m_balance.budgetBase));
            m_balance.budgetLinear = std::max(0.0f, director.value("budgetLinear", m_balance.budgetLinear));
            m_balance.budgetQuadratic = std::max(0.0f, director.value("budgetQuadratic", m_balance.budgetQuadratic));
            m_balance.spawnInterval = std::max(0.1f, director.value("spawnInterval", m_balance.spawnInterval));
            m_balance.minSpawnInterval = std::max(0.05f, director.value("minSpawnInterval", m_balance.minSpawnInterval));
            m_balance.activeCapBase = std::max(1, director.value("activeCapBase", m_balance.activeCapBase));
            m_balance.activeCapPerWave = std::max(0.0f, director.value("activeCapPerWave", m_balance.activeCapPerWave));
            m_balance.activeCapMax = std::clamp(director.value("activeCapMax", m_balance.activeCapMax), 1, (int)kEnemyCapacity);
            if (root.contains("assist")) {
                const auto& assist = root.at("assist");
                m_balance.enemyHpMultiplier = std::clamp(
                    assist.value("enemyHpMultiplier", 1.0f), 0.10f, 2.0f);
                m_balance.enemyDamageMultiplier = std::clamp(
                    assist.value("enemyDamageMultiplier", 1.0f), 0.10f, 2.0f);
                m_balance.enemySpeedMultiplier = std::clamp(
                    assist.value("enemySpeedMultiplier", 1.0f), 0.40f, 1.5f);
                m_balance.playerHealthMultiplier = std::clamp(
                    assist.value("playerHealthMultiplier", 1.0f), 0.50f, 3.0f);
                m_balance.playerDamageMultiplier = std::clamp(
                    assist.value("playerDamageMultiplier", 1.0f), 0.50f, 4.0f);
                m_balance.playerCooldownMultiplier = std::clamp(
                    assist.value("playerCooldownMultiplier", 1.0f), 0.25f, 1.5f);
                m_balance.waveHealFraction = std::clamp(
                    assist.value("waveHealFraction", 0.0f), 0.0f, 1.0f);
            }
            loaded = true;
        }

        std::ifstream wavesFile("assets/survival3d/config/waves.json");
        if (wavesFile) {
            nlohmann::json root;
            wavesFile >> root;
            for (const auto& entry : root.at("waves")) {
                const int wave = entry.value("wave", 0);
                if (wave < 1 || wave > 50) continue;
                WaveRule rule = m_waveRules[wave - 1];
                rule.budgetMultiplier = std::max(0.25f, entry.value("budgetMultiplier", 1.0f));
                if (entry.contains("mix")) {
                    const auto& mix = entry.at("mix");
                    const float swarm = std::max(0.0f, mix.value("swarm", 0.0f));
                    const float ranger = std::max(0.0f, mix.value("ranger", 0.0f));
                    const float tanker = std::max(0.0f, mix.value("tanker", 0.0f));
                    const float total = swarm + ranger + tanker;
                    if (total > 0.99f && total < 1.01f) {
                        rule.swarm = swarm;
                        rule.ranger = ranger;
                        rule.tanker = tanker;
                    }
                }
                if (entry.contains("boss")) {
                    const std::string boss = entry.value("boss", std::string{});
                    rule.bossWave = true;
                    if (boss == "brood_warden") rule.boss = EnemyArchetype::BroodWarden;
                    else if (boss == "hexeye_artillerist") rule.boss = EnemyArchetype::HexeyeArtillerist;
                    else if (boss == "ironroot_colossus") rule.boss = EnemyArchetype::IronrootColossus;
                    else if (boss == "eclipse_chimera") rule.boss = EnemyArchetype::EclipseChimera;
                    else if (boss == "void_sovereign") rule.boss = EnemyArchetype::VoidSovereign;
                    else rule.bossWave = false;
                }
                m_waveRules[wave - 1] = rule;
            }
            loaded = true;
        }
    } catch (const std::exception&) {
        m_balanceVersion = "fallback-1";
        return false;
    }
    return loaded;
}

bool SurvivalController::Init() {
    if (m_initialized) return true;

    m_enemies.resize(kEnemyCapacity);
    m_enemyAnimationGraphs.clear();
    m_enemyAnimationGraphs.reserve(kEnemyCapacity);
    for (std::size_t i = 0; i < kEnemyCapacity; ++i)
        m_enemyAnimationGraphs.emplace_back(Animation::ActorClass::Enemy);
    m_enemyEventCursors.clear();
    m_enemyEventCursors.resize(kEnemyCapacity);
    m_freeEnemies.reserve(kEnemyCapacity);
    for (std::size_t i = kEnemyCapacity; i > 0; --i) m_freeEnemies.push_back(i - 1);

    m_projectiles.resize(kProjectileCapacity);
    m_freeProjectiles.reserve(kProjectileCapacity);
    for (std::size_t i = kProjectileCapacity; i > 0; --i) m_freeProjectiles.push_back(i - 1);

    m_camera.position = {9.0f, 12.0f, 9.0f};
    m_camera.target = {0.0f, 1.0f, 0.0f};
    m_camera.up = {0.0f, 1.0f, 0.0f};
    m_camera.fovy = 52.0f;
    m_camera.projection = CAMERA_PERSPECTIVE;

    LoadBalanceConfig();
    m_knightClipLibrary = Animation::BuildStep6HeroClipLibrary(
        Animation::HeroStyle::Knight);
    m_mageClipLibrary = Animation::BuildStep6HeroClipLibrary(
        Animation::HeroStyle::MagicCaster);
    m_enemyClipLibrary = Animation::BuildStep6NonHeroClipLibrary(
        Animation::ActorClass::Enemy);
    m_bossClipLibrary = Animation::BuildStep6NonHeroClipLibrary(
        Animation::ActorClass::Boss);

    if (!SurvivalView::GetInstance().Init()) return false;
    m_initialized = true;
    return true;
}

void SurvivalController::Start() {
    if (!m_initialized && !Init()) return;
    m_returnToMenu = false;
    m_paused = false;
    m_selectedCharacter = 0;
    m_selectedUpgrade = 0;
    m_showRecords = false;
    const auto& save = SaveManager::GetInstance();
    m_highContrast = save.GetSurvivalHighContrast();
    m_reducedMotion = save.GetSurvivalReducedMotion();
    m_uiScale = save.GetSurvivalUiScale();
    m_phase = Phase::CharacterSelect;
    m_phaseTimer = 0.0f;
    m_camera.position = {0.0f, 8.0f, 13.5f};
    m_camera.target = {0.0f, 1.2f, 0.0f};
    m_targetLock = false;
    m_lockedEnemyIndex = kEnemyCapacity;
    if (m_mouseCaptured && IsWindowReady()) EnableCursor();
    m_mouseCaptured = false;
}

void SurvivalController::EquipWeapon(WeaponId weapon) {
    m_player.equippedWeapon = weapon;
}

void SurvivalController::UnequipWeapon() {
    m_player.equippedWeapon = WeaponId::None;
}

void SurvivalController::ResetRun(CharacterId character) {
    for (auto& enemy : m_enemies) enemy.active = false;
    for (auto& cursor : m_enemyEventCursors) cursor.Stop();
    m_freeEnemies.clear();
    for (std::size_t i = kEnemyCapacity; i > 0; --i) m_freeEnemies.push_back(i - 1);
    for (auto& projectile : m_projectiles) projectile.active = false;
    m_freeProjectiles.clear();
    for (std::size_t i = kProjectileCapacity; i > 0; --i) m_freeProjectiles.push_back(i - 1);

    m_player = {};
    m_player.character = character;
    m_player.equippedWeapon = DefaultWeaponFor(character);
    m_playerAnimationGraph.Reset(Animation::State::Idle);
    m_playerEventCursor.Stop();
    m_playerCombo.Cancel();
    m_playerFootIkState = {};
    m_playerAimIkState = {};
    m_playerEventTrackActive = false;
    m_animationGraphSerial = 0;
    m_previousActionProgress = 0.0f;
    if (character == CharacterId::MagicCaster) {
        m_player.hp = m_player.maxHp = 90.0f * m_balance.playerHealthMultiplier;
        m_player.moveSpeed = 5.8f * 1.12f;
    } else {
        m_player.hp = m_player.maxHp = 140.0f * m_balance.playerHealthMultiplier;
        m_player.moveSpeed = 5.2f * 1.12f;
    }
    m_player.damageMultiplier = m_balance.playerDamageMultiplier;
    m_player.cooldownMultiplier = m_balance.playerCooldownMultiplier;
    m_player.shield = m_player.maxHp * 0.20f;
    m_player.ultimateCharge = 50.0f;

    m_wave = 1;
    m_kills = 0;
    m_score = 0;
    m_bossesKilled = 0;
    m_damageTaken = 0;
    m_activeProjectiles = 0;
    m_peakEnemies = 0;
    m_peakProjectiles = 0;
    m_droppedTicks = 0;
    m_simTick = 0;
    m_averageFrameMs = 16.67f;
    m_peakFrameMs = 0.0f;
    m_choicesWithoutEpic = 0;
    m_chainHitCounter = 0;
    m_chainResolving = false;
    m_upgradeStacks.fill(0);
    m_runRecorded = false;
    m_runId = SurvivalRunService::GetInstance().BeginRun(character, m_balanceVersion);
    m_activeEnemies = 0;
    m_remainingToSpawn = 0;
    m_runTime = 0.0f;
    m_accumulator = 0.0f;
    m_attackFxTimer = 0.0f;
    m_skillFxTimer = 0.0f;
    m_gravityWellTimer = 0.0f;
    m_combatFeedback = {};
    m_playerActionAim = {};
    m_playerActionStart = {};
    m_playerActionFacing = {0.0f, 0.0f, 1.0f};
    m_hitStopTimer = 0.0f;
    m_cameraShakeTimer = 0.0f;
    m_cameraShakeIntensity = 0.0f;
    m_cameraYaw = 0.7853982f;
    m_cameraPitch = 0.56f;
    m_cameraDistance = 7.6f;
    m_cameraUserDistance = 7.6f;
    m_targetLock = false;
    m_lockedEnemyIndex = kEnemyCapacity;
    m_jumpQueued = false;
    m_rngState = 0xA3615EEDu ^ (character == CharacterId::Knight ? 0x4B4E4947u : 0x4D414749u);
    m_upgradeRngState = 0x91E10DA5u ^ (character == CharacterId::Knight ? 0x4B4E4947u : 0x4D414749u);
    m_paused = false;
    BeginWave(1);
    SoundManager::GetInstance().PlayMusic("bgm_gameplay");
    SoundManager::GetInstance().PlaySound("start");
}

void SurvivalController::Update(float frameDt) {
    if (!m_initialized || m_returnToMenu) return;
    frameDt = std::clamp(frameDt, 0.0f, 0.1f);
    const float frameMs = frameDt * 1000.0f;
    m_averageFrameMs += (frameMs - m_averageFrameMs) * 0.06f;
    m_peakFrameMs = std::max(m_peakFrameMs, frameMs);
    m_peakEnemies = std::max(m_peakEnemies, m_activeEnemies);
    m_peakProjectiles = std::max(m_peakProjectiles, m_activeProjectiles);
    if (IsKeyPressed(KEY_F3)) m_showPerformance = !m_showPerformance;
    if (IsKeyPressed(KEY_F4)) m_highContrast = !m_highContrast;
    if (IsKeyPressed(KEY_F5)) m_reducedMotion = !m_reducedMotion;
    if (IsKeyPressed(KEY_F6)) {
        m_uiScale = m_uiScale < 0.95f ? 1.0f : (m_uiScale < 1.10f ? 1.20f : 0.85f);
    }
    UpdateCursorCapture();

    if (m_phase == Phase::CharacterSelect) {
        if (m_showRecords) {
            if (IsKeyPressed(KEY_TAB) || IsKeyPressed(KEY_ESCAPE)) m_showRecords = false;
            return;
        }
        if (IsKeyPressed(KEY_TAB)) {
            m_showRecords = true;
            SurvivalRunService::GetInstance().RequestLeaderboardRefresh();
            return;
        }
        if (IsKeyPressed(KEY_ESCAPE)) {
            m_returnToMenu = true;
            return;
        }
        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) m_selectedCharacter = 0;
        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) m_selectedCharacter = 1;

        const float sw = (float)GetScreenWidth();
        const float sh = (float)GetScreenHeight();
        const Rectangle leftCard{sw * 0.035f, sh * 0.48f, sw * 0.275f, sh * 0.32f};
        const Rectangle rightCard{sw * 0.690f, sh * 0.48f, sw * 0.275f, sh * 0.32f};
        const Rectangle deployButton{sw * 0.385f, sh * 0.835f,
                                     sw * 0.230f, sh * 0.082f};
        const Vector2 mouse = GetMousePosition();
        if (CheckCollisionPointRec(mouse, leftCard)) m_selectedCharacter = 0;
        if (CheckCollisionPointRec(mouse, rightCard)) m_selectedCharacter = 1;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (CheckCollisionPointRec(mouse, leftCard)) m_selectedCharacter = 0;
            if (CheckCollisionPointRec(mouse, rightCard)) m_selectedCharacter = 1;
            if (CheckCollisionPointRec(mouse, deployButton)) {
                ResetRun(m_selectedCharacter == 0 ? CharacterId::Knight : CharacterId::MagicCaster);
            }
        } else if (IsKeyPressed(KEY_ENTER)) {
            ResetRun(m_selectedCharacter == 0 ? CharacterId::Knight : CharacterId::MagicCaster);
        }
        return;
    }

    if (m_phase == Phase::RunFailed || m_phase == Phase::RunVictory) {
        if (m_phase == Phase::RunFailed && m_player.animation == PlayerAnimation::Death) {
            m_player.animationTime = std::min(1.60f, m_player.animationTime + frameDt);
            m_player.previousAnimationTime += frameDt;
            m_player.animationBlend = std::min(1.0f, m_player.animationBlend + frameDt / 0.14f);
        }
        if (m_showRecords) {
            if (IsKeyPressed(KEY_TAB) || IsKeyPressed(KEY_ESCAPE)) m_showRecords = false;
            UpdateCamera(frameDt);
            return;
        }
        if (IsKeyPressed(KEY_TAB)) {
            m_showRecords = true;
            SurvivalRunService::GetInstance().RequestLeaderboardRefresh();
            return;
        }
        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A))
            m_selectedResultAction = (m_selectedResultAction + 2) % 3;
        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D))
            m_selectedResultAction = (m_selectedResultAction + 1) % 3;

        const float sw = static_cast<float>(GetScreenWidth());
        const float sh = static_cast<float>(GetScreenHeight());
        const float buttonW = sw * 0.18f;
        const float buttonGap = sw * 0.025f;
        const float startX = (sw - buttonW * 3.0f - buttonGap * 2.0f) * 0.5f;
        const float buttonY = sh * 0.785f;
        const float buttonH = sh * 0.082f;
        const Vector2 mouse = GetMousePosition();
        for (int i = 0; i < 3; ++i) {
            const Rectangle button{startX + i * (buttonW + buttonGap),
                                   buttonY, buttonW, buttonH};
            if (CheckCollisionPointRec(mouse, button)) m_selectedResultAction = i;
        }
        const bool click = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        const bool confirm = IsKeyPressed(KEY_ENTER);
        if (IsKeyPressed(KEY_R)) {
            ResetRun(m_player.character);
        } else if (IsKeyPressed(KEY_TAB)) {
            m_showRecords = true;
            SurvivalRunService::GetInstance().RequestLeaderboardRefresh();
        } else if (click || confirm) {
            bool activate = confirm;
            if (click) {
                const Rectangle button{startX + m_selectedResultAction * (buttonW + buttonGap),
                                       buttonY, buttonW, buttonH};
                activate = CheckCollisionPointRec(mouse, button);
            }
            if (activate && m_selectedResultAction == 0) {
                ResetRun(m_player.character);
            } else if (activate && m_selectedResultAction == 1) {
                m_showRecords = true;
                SurvivalRunService::GetInstance().RequestLeaderboardRefresh();
            } else if (activate && m_selectedResultAction == 2) {
                m_returnToMenu = true;
            }
        } else if (IsKeyPressed(KEY_ESCAPE)) {
            m_returnToMenu = true;
        }
        UpdateCamera(frameDt);
        return;
    }

    if (m_phase == Phase::UpgradeChoice) {
        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A))
            m_selectedUpgrade = (m_selectedUpgrade + 2) % 3;
        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D))
            m_selectedUpgrade = (m_selectedUpgrade + 1) % 3;

        const float sw = (float)GetScreenWidth();
        const float sh = (float)GetScreenHeight();
        const float cardW = sw * 0.25f;
        const float gap = sw * 0.025f;
        const float startX = (sw - cardW * 3.0f - gap * 2.0f) * 0.5f;
        const Vector2 mouse = GetMousePosition();
        for (int i = 0; i < 3; ++i) {
            const Rectangle card{startX + i * (cardW + gap), sh * 0.32f, cardW, sh * 0.40f};
            if (CheckCollisionPointRec(mouse, card)) m_selectedUpgrade = i;
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, card)) {
                ApplyUpgrade(i);
                return;
            }
        }
        if (IsKeyPressed(KEY_ONE)) ApplyUpgrade(0);
        else if (IsKeyPressed(KEY_TWO)) ApplyUpgrade(1);
        else if (IsKeyPressed(KEY_THREE)) ApplyUpgrade(2);
        else if (IsKeyPressed(KEY_ENTER)) ApplyUpgrade(m_selectedUpgrade);
        UpdateCamera(frameDt);
        return;
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        m_paused = !m_paused;
        UpdateCursorCapture();
    }
    if (m_paused) {
        const float sw = static_cast<float>(GetScreenWidth());
        const float sh = static_cast<float>(GetScreenHeight());
        const Rectangle panel{sw * 0.20f, sh * 0.20f,
                              sw * 0.60f, sh * 0.60f};
        const float buttonW = panel.width * 0.31f;
        const float buttonH = panel.height * 0.145f;
        const float buttonY = panel.y + panel.height * 0.53f;
        const Rectangle resumeButton{panel.x + panel.width * 0.17f,
                                     buttonY, buttonW, buttonH};
        const Rectangle menuButton{panel.x + panel.width * 0.52f,
                                   buttonY, buttonW, buttonH};
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            const Vector2 mouse = GetMousePosition();
            if (CheckCollisionPointRec(mouse, resumeButton)) {
                m_paused = false;
                UpdateCursorCapture();
            } else if (CheckCollisionPointRec(mouse, menuButton)) {
                m_returnToMenu = true;
            }
        }
        if (IsKeyPressed(KEY_Q)) m_returnToMenu = true;
        return;
    }

    PollFrameInput();
    m_accumulator += frameDt;
    int catchUpTicks = 0;
    while (m_accumulator >= kFixedDt && catchUpTicks < 6) {
        FixedUpdate(kFixedDt);
        m_accumulator -= kFixedDt;
        ++catchUpTicks;
    }
    if (catchUpTicks == 6 && m_accumulator >= kFixedDt) {
        m_accumulator = 0.0f;
        ++m_droppedTicks;
    }
    UpdateCamera(frameDt);
}

void SurvivalController::PollFrameInput() {
    const Vector2 mouseDelta = GetMouseDelta();
    m_cameraYaw -= mouseDelta.x * 0.0038f;
    m_cameraPitch = std::clamp(m_cameraPitch - mouseDelta.y * 0.0030f,
                               0.31f, 1.04f);
    // Roblox-style scroll zoom: a wide range makes the change clearly
    // readable, from a close over-the-shoulder view to a full arena view.
    m_cameraUserDistance = std::clamp(
        m_cameraUserDistance - GetMouseWheelMove() * 1.15f, 2.15f, 16.0f);

    if (IsKeyPressed(KEY_T) || IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE)) {
        if (m_targetLock) {
            m_targetLock = false;
            m_lockedEnemyIndex = kEnemyCapacity;
        } else {
            m_lockedEnemyIndex = FindCameraTarget(true);
            m_targetLock = m_lockedEnemyIndex < m_enemies.size();
        }
    }
    if (m_targetLock
        && (m_lockedEnemyIndex >= m_enemies.size()
            || !m_enemies[m_lockedEnemyIndex].active
            || m_enemies[m_lockedEnemyIndex].dying)) {
        m_lockedEnemyIndex = FindCameraTarget(true);
        m_targetLock = m_lockedEnemyIndex < m_enemies.size();
    }

    Vector3 cameraForward = NormalizeXZ({m_camera.target.x - m_camera.position.x,
                                         0.0f,
                                         m_camera.target.z - m_camera.position.z});
    if (LengthXZ(cameraForward) < 0.01f) cameraForward = {0.0f, 0.0f, -1.0f};
    const Vector3 cameraRight{-cameraForward.z, 0.0f, cameraForward.x};
    float forwardInput = 0.0f;
    float rightInput = 0.0f;
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) forwardInput += 1.0f;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) forwardInput -= 1.0f;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) rightInput += 1.0f;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) rightInput -= 1.0f;
    m_moveInput = NormalizeXZ({cameraForward.x * forwardInput
                               + cameraRight.x * rightInput,
                               0.0f,
                               cameraForward.z * forwardInput
                               + cameraRight.z * rightInput});

    const Ray ray = GetScreenToWorldRay(GetMousePosition(), m_camera);
    if (std::abs(ray.direction.y) > 0.0001f) {
        const float t = -ray.position.y / ray.direction.y;
        if (t > 0.0f) {
            m_aimPoint = {
                ray.position.x + ray.direction.x * t,
                0.0f,
                ray.position.z + ray.direction.z * t
            };
        }
    }

    const bool actionPressed = IsKeyPressed(KEY_J)
        || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)
        || IsKeyPressed(KEY_K) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)
        || IsKeyPressed(KEY_U) || IsKeyPressed(KEY_Q)
        || IsKeyPressed(KEY_H) || IsKeyPressed(KEY_R);
    if (m_targetLock && m_lockedEnemyIndex < m_enemies.size()) {
        m_aimPoint = m_enemies[m_lockedEnemyIndex].position;
    } else if (actionPressed) {
        const std::size_t assisted = FindCameraTarget(false);
        if (assisted < m_enemies.size()) m_aimPoint = m_enemies[assisted].position;
    }

    m_basicQueued |= IsKeyPressed(KEY_J) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    m_skillOneQueued |= IsKeyPressed(KEY_K) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
    m_skillTwoQueued |= IsKeyPressed(KEY_U) || IsKeyPressed(KEY_Q);
    m_ultimateQueued |= IsKeyPressed(KEY_H) || IsKeyPressed(KEY_R);
    m_dashQueued |= IsKeyPressed(KEY_L);
    m_jumpQueued |= IsKeyPressed(KEY_SPACE);
}

void SurvivalController::FixedUpdate(float dt) {
    ++m_simTick;
    // Hit-stop is presentation-only: hold the authored contact pose for a few
    // rendered frames, while wave/runtime timers and deterministic simulation
    // continue. This keeps impact readable without extending a scored run.
    const bool holdContactPose = m_hitStopTimer > 0.0f;
    if (holdContactPose) {
        m_hitStopTimer = std::max(0.0f, m_hitStopTimer - dt);
    }
    if (!holdContactPose) {
        const float previousProgress = m_player.animationDuration > 0.0f
            ? std::clamp(m_player.animationTime / m_player.animationDuration, 0.0f, 1.0f)
            : 0.0f;
        m_player.animationTime += dt;
        m_player.previousAnimationTime += dt;
        m_player.animationBlend = std::min(1.0f, m_player.animationBlend + dt / 0.14f);
        const float currentProgress = m_player.animationDuration > 0.0f
            ? std::clamp(m_player.animationTime / m_player.animationDuration, 0.0f, 1.0f)
            : 0.0f;
        ApplyPlayerRootMotion(previousProgress, currentProgress);
        m_previousActionProgress = currentProgress;
        // Resolve gameplay at the authored contact pose before the clip can expire.
        // Previously all five attacks dealt damage at time zero, so their visuals
        // always lagged behind the hit and rapid input could replace the clip.
        UpdatePlayerActionTimeline();
        UpdatePlayerCombo();
        if (m_player.animationDuration > 0.0f
            && m_player.animationTime >= m_player.animationDuration) {
            CancelPlayerAction();
            m_playerAnimationGraph.CompleteCurrent(LengthXZ(m_moveInput) > 0.1f);
            m_player.previousAnimation = m_player.animation;
            m_player.previousAnimationTime = m_player.animationTime;
            m_player.animationDuration = 0.0f;
            m_player.animationTime = 0.0f;
            m_player.animation = LengthXZ(m_moveInput) > 0.1f
                ? PlayerAnimation::Run : PlayerAnimation::Idle;
            m_player.animationBlend = 0.0f;
        }
    }
    m_player.basicCooldown = std::max(0.0f, m_player.basicCooldown - dt);
    m_player.skillOneCooldown = std::max(0.0f, m_player.skillOneCooldown - dt);
    m_player.skillTwoCooldown = std::max(0.0f, m_player.skillTwoCooldown - dt);
    m_player.dashCooldown = std::max(0.0f, m_player.dashCooldown - dt);
    m_player.barrierCooldown = std::max(0.0f, m_player.barrierCooldown - dt);
    m_player.invulnerableTimer = std::max(0.0f, m_player.invulnerableTimer - dt);
    m_player.guardTimer = std::max(0.0f, m_player.guardTimer - dt);
    m_player.rushTimer = std::max(0.0f, m_player.rushTimer - dt);
    m_attackFxTimer = std::max(0.0f, m_attackFxTimer - dt);
    m_skillFxTimer = std::max(0.0f, m_skillFxTimer - dt);
    m_gravityWellTimer = std::max(0.0f, m_gravityWellTimer - dt);
    m_cameraShakeTimer = std::max(0.0f, m_cameraShakeTimer - dt);
    if (m_cameraShakeTimer <= 0.0f) m_cameraShakeIntensity = 0.0f;

    UpdatePlayer(dt);
    UpdateWave(dt);
    if (m_phase == Phase::Combat) {
        m_runTime += dt;
        UpdateEnemies(dt);
        UpdateProjectiles(dt);
    }

    m_basicQueued = false;
    m_skillOneQueued = false;
    m_skillTwoQueued = false;
    m_ultimateQueued = false;
    m_dashQueued = false;
    m_jumpQueued = false;
}

void SurvivalController::UpdatePlayer(float dt) {
    if (m_phase != Phase::PreWave && m_phase != Phase::Combat
        && m_phase != Phase::WaveClear) return;

    const Vector3 aimDirection = NormalizeXZ({
        m_aimPoint.x - m_player.position.x, 0.0f, m_aimPoint.z - m_player.position.z
    });
    if (m_player.combatAction != PlayerCombatAction::None) {
        // Do not let mouse motion twist an authored strike after its windup has
        // begun; hit tests and the animation now share this locked direction.
        m_player.facing = m_playerActionFacing;
    } else if (LengthXZ(aimDirection) > 0.1f) {
        m_player.facing = aimDirection;
    } else if (LengthXZ(m_moveInput) > 0.1f) {
        m_player.facing = m_moveInput;
    }

    float actionMoveScale = 1.0f;
    switch (m_player.combatAction) {
        case PlayerCombatAction::Basic:
            actionMoveScale = m_player.character == CharacterId::Knight ? 0.35f : 0.58f;
            break;
        case PlayerCombatAction::SkillOne: actionMoveScale = 0.35f; break;
        case PlayerCombatAction::SkillTwo:
            actionMoveScale = m_player.character == CharacterId::Knight ? 0.0f : 0.18f;
            break;
        case PlayerCombatAction::Ultimate: actionMoveScale = 0.08f; break;
        case PlayerCombatAction::Dash: actionMoveScale = 0.0f; break;
        case PlayerCombatAction::None: break;
    }
    const Vector3 previousPosition = m_player.position;
    if (m_jumpQueued && m_player.grounded) {
        m_player.verticalVelocity = 7.25f;
        m_player.grounded = false;
    }
    m_player.position.x += m_moveInput.x * m_player.moveSpeed * actionMoveScale * dt;
    m_player.position.z += m_moveInput.z * m_player.moveSpeed * actionMoveScale * dt;
    if (!m_player.grounded) {
        m_player.verticalVelocity -= 19.5f * dt;
        m_player.position.y += m_player.verticalVelocity * dt;
        if (m_player.position.y <= 0.0f) {
            m_player.position.y = 0.0f;
            m_player.verticalVelocity = 0.0f;
            m_player.grounded = true;
        }
    }
    const float verticalPosition = m_player.position.y;
    m_player.position = ClampToArena(m_player.position, kArenaHalfExtent, 0.45f);
    m_player.position = ResolveArenaPillars(m_player.position, 0.45f);
    m_player.position.y = verticalPosition;
    m_player.velocity = dt > 0.0f
        ? Vector3{(m_player.position.x - previousPosition.x) / dt,
                  (m_player.position.y - previousPosition.y) / dt,
                  (m_player.position.z - previousPosition.z) / dt}
        : Vector3{};

    if (m_player.animationDuration <= 0.0f) {
        const PlayerAnimation locomotion = LengthXZ(m_moveInput) > 0.1f
            ? PlayerAnimation::Run : PlayerAnimation::Idle;
        if (m_player.animation != locomotion) {
            m_player.previousAnimation = m_player.animation;
            m_player.previousAnimationTime = m_player.animationTime;
            m_player.animation = locomotion;
            m_player.animationTime = 0.0f;
            m_player.animationBlend = 0.0f;
        }
    }

    // Only one action may own the authored clip timeline.  Use a deterministic
    // priority so simultaneous inputs never restart/freeze one another.
    if (m_player.combatAction == PlayerCombatAction::None
        && m_player.animationDuration <= 0.0f) {
        if (m_dashQueued && m_player.dashCooldown <= 0.0f) PerformDash();
        else if (m_ultimateQueued && m_player.ultimateCharge >= 100.0f) PerformUltimate();
        else if (m_skillTwoQueued && m_player.skillTwoCooldown <= 0.0f) PerformSkillTwo();
        else if (m_skillOneQueued && m_player.skillOneCooldown <= 0.0f) PerformSkillOne();
        else if (m_basicQueued && m_player.basicCooldown <= 0.0f) PerformBasicAttack();
    }
    UpdatePlayerAnimationGraph();
    UpdatePlayerRuntimeIk(dt);
}

void SurvivalController::UpdateWave(float dt) {
    if (m_phase == Phase::PreWave) {
        m_phaseTimer -= dt;
        if (m_phaseTimer <= 0.0f) SetPhase(Phase::Combat);
        return;
    }

    if (m_phase == Phase::Combat) {
        m_spawnTimer -= dt;
        const int activeCap = std::min(m_balance.activeCapMax,
            m_balance.activeCapBase + (int)std::floor(m_balance.activeCapPerWave * m_wave));
        if (m_remainingToSpawn > 0 && m_activeEnemies < activeCap && m_spawnTimer <= 0.0f) {
            SpawnEnemy();
            m_spawnTimer = m_spawnInterval;
        }
        if (m_remainingToSpawn == 0 && m_activeEnemies == 0) {
            SetPhase(Phase::WaveClear, 1.2f);
            SoundManager::GetInstance().PlaySound("victory_reveal");
        }
        return;
    }

    if (m_phase == Phase::WaveClear) {
        m_phaseTimer -= dt;
        if (m_phaseTimer <= 0.0f) {
            if (m_wave >= 50) SetPhase(Phase::RunVictory);
            else EnterUpgradeChoice();
        }
    }
}

void SurvivalController::BeginWave(int wave) {
    m_wave = wave;
    if (wave > 1) {
        m_player.hp = std::min(
            m_player.maxHp,
            m_player.hp + m_player.maxHp * m_balance.waveHealFraction);
        m_player.shield = std::max(m_player.shield, m_player.maxHp * 0.12f);
        m_player.ultimateCharge = std::min(100.0f,
                                           m_player.ultimateCharge + 20.0f);
    }
    const float t = (float)(wave - 1);
    const WaveRule& rule = m_waveRules[std::clamp(wave - 1, 0, 49)];
    if (rule.bossWave) {
        m_remainingToSpawn = 1;
        SoundManager::GetInstance().PlayMusic("bgm_boss");
    } else {
        const float budget = m_balance.budgetBase + m_balance.budgetLinear * wave
                           + m_balance.budgetQuadratic * wave * wave;
        m_remainingToSpawn = std::max(1, (int)std::round(budget * rule.budgetMultiplier));
        SoundManager::GetInstance().PlayMusic("bgm_gameplay");
    }
    m_spawnInterval = std::max(m_balance.minSpawnInterval,
                               m_balance.spawnInterval / (1.0f + 0.03f * t));
    m_spawnTimer = 0.0f;
    SetPhase(Phase::PreWave, 3.0f);
}

void SurvivalController::EnterUpgradeChoice() {
    m_selectedUpgrade = 0;
    GenerateUpgradeOptions();
    SetPhase(Phase::UpgradeChoice);
}

void SurvivalController::GenerateUpgradeOptions() {
    const bool bossReward = m_wave % 10 == 0;
    bool offeredEpic = false;
    std::array<int, 3> chosen{{-1, -1, -1}};

    for (int slot = 0; slot < 3; ++slot) {
        UpgradeRarity desired = UpgradeRarity::Common;
        const float roll = NextUpgradeRandom01();
        if (bossReward) {
            if (slot == 0) desired = roll < 0.18f ? UpgradeRarity::Legendary : UpgradeRarity::Epic;
            else if (roll < 0.10f) desired = UpgradeRarity::Legendary;
            else if (roll < 0.42f) desired = UpgradeRarity::Epic;
            else desired = UpgradeRarity::Rare;
        } else if (slot == 0 && m_choicesWithoutEpic >= 7) {
            desired = UpgradeRarity::Epic;
        } else if (roll < 0.62f) desired = UpgradeRarity::Common;
        else if (roll < 0.90f) desired = UpgradeRarity::Rare;
        else if (roll < 0.99f) desired = UpgradeRarity::Epic;
        else desired = UpgradeRarity::Legendary;

        std::array<int, static_cast<std::size_t>(UpgradeId::Count)> candidates{};
        int candidateCount = 0;
        const int character = m_player.character == CharacterId::Knight ? 0 : 1;
        for (std::size_t definitionIndex = 0; definitionIndex < kUpgrades.size(); ++definitionIndex) {
            const UpgradeDefinition& definition = kUpgrades[definitionIndex];
            if (definition.characterLock >= 0 && definition.characterLock != character) continue;
            if (m_upgradeStacks[static_cast<std::size_t>(definition.id)] >= definition.maxStacks) continue;
            if (definition.rarity != desired) continue;
            bool duplicate = false;
            for (int previous : chosen) duplicate |= previous == (int)definitionIndex;
            if (!duplicate) candidates[candidateCount++] = (int)definitionIndex;
        }

        // A rarity may be exhausted by max stacks. Fall back to any valid card
        // while still keeping the three offers distinct.
        if (candidateCount == 0) {
            for (std::size_t definitionIndex = 0; definitionIndex < kUpgrades.size(); ++definitionIndex) {
                const UpgradeDefinition& definition = kUpgrades[definitionIndex];
                if (definition.characterLock >= 0 && definition.characterLock != character) continue;
                if (m_upgradeStacks[static_cast<std::size_t>(definition.id)] >= definition.maxStacks) continue;
                bool duplicate = false;
                for (int previous : chosen) duplicate |= previous == (int)definitionIndex;
                if (!duplicate) candidates[candidateCount++] = (int)definitionIndex;
            }
        }

        const int selectedDefinition = candidateCount > 0
            ? candidates[std::min(candidateCount - 1, (int)(NextUpgradeRandom01() * candidateCount))]
            : (int)UpgradeId::RiftEssence;
        chosen[slot] = selectedDefinition;
        const UpgradeDefinition& definition = kUpgrades[selectedDefinition];
        m_upgradeOptions[slot] = UpgradeOption{
            definition.id, definition.key, definition.name, definition.description,
            RarityColor(definition.rarity), definition.rarity, definition.maxStacks
        };
        offeredEpic |= definition.rarity == UpgradeRarity::Epic
                    || definition.rarity == UpgradeRarity::Legendary;
    }
    m_choicesWithoutEpic = offeredEpic ? 0 : m_choicesWithoutEpic + 1;
}

void SurvivalController::ApplyUpgrade(int index) {
    index = std::clamp(index, 0, 2);
    const UpgradeId upgrade = m_upgradeOptions[index].upgrade;
    ++m_upgradeStacks[static_cast<std::size_t>(upgrade)];
    switch (upgrade) {
        case UpgradeId::VitalCore:
            m_player.maxHp += 12.0f;
            m_player.hp = std::min(m_player.maxHp, m_player.hp + 12.0f);
            break;
        case UpgradeId::SwiftStep: m_player.moveSpeed *= 1.04f; break;
        case UpgradeId::TemperedGuard:
            m_player.damageReduction = std::min(0.60f, m_player.damageReduction + 0.04f);
            break;
        case UpgradeId::QuickHands: m_player.cooldownMultiplier *= 0.96f; break;
        case UpgradeId::WideArc: m_player.areaMultiplier *= 1.08f; break;
        case UpgradeId::RiftPower: m_player.damageMultiplier *= 1.18f; break;
        case UpgradeId::Execution: m_player.executionBonus = 0.20f; break;
        case UpgradeId::GravityLens:
            m_player.projectileScale *= 1.12f;
            ++m_player.projectilePierce;
            break;
        case UpgradeId::EmergencyBarrier: m_player.barrierCooldown = 0.0f; break;
        case UpgradeId::SecondWind: m_player.secondWindAvailable = true; break;
        case UpgradeId::GlassRift:
            m_player.damageMultiplier *= 1.45f;
            m_player.maxHp *= 0.75f;
            m_player.hp = std::min(m_player.hp, m_player.maxHp);
            break;
        case UpgradeId::Juggernaut: m_player.damageMultiplier *= 1.10f; break;
        case UpgradeId::RoyalBulwark: m_player.royalBulwark = true; break;
        case UpgradeId::ForkedBolt: m_player.forkedBolt = true; break;
        case UpgradeId::EventHorizon: m_player.eventHorizon = true; break;
        case UpgradeId::RiftEssence:
            m_player.damageMultiplier *= 1.03f;
            m_player.maxHp *= 1.03f;
            m_player.hp = std::min(m_player.maxHp, m_player.hp + m_player.maxHp * 0.03f);
            break;
        default: break;
    }
    SoundManager::GetInstance().PlaySound("ui_confirm");
    BeginWave(m_wave + 1);
}

std::size_t SurvivalController::AcquireEnemy() {
    if (m_freeEnemies.empty()) return std::numeric_limits<std::size_t>::max();
    const std::size_t index = m_freeEnemies.back();
    m_freeEnemies.pop_back();
    const std::uint16_t generation = (std::uint16_t)(m_enemies[index].generation + 1);
    m_enemies[index] = {};
    m_enemies[index].generation = generation;
    m_enemies[index].active = true;
    ++m_activeEnemies;
    return index;
}

void SurvivalController::ReleaseEnemy(std::size_t index) {
    if (index >= m_enemies.size() || !m_enemies[index].active) return;
    m_enemies[index].active = false;
    if (index < m_enemyEventCursors.size()) m_enemyEventCursors[index].Stop();
    m_freeEnemies.push_back(index);
    m_activeEnemies = std::max(0, m_activeEnemies - 1);
}

std::size_t SurvivalController::AcquireProjectile() {
    if (m_freeProjectiles.empty()) return std::numeric_limits<std::size_t>::max();
    const std::size_t index = m_freeProjectiles.back();
    m_freeProjectiles.pop_back();
    const std::uint16_t generation = (std::uint16_t)(m_projectiles[index].generation + 1);
    m_projectiles[index] = {};
    m_projectiles[index].generation = generation;
    m_projectiles[index].active = true;
    ++m_activeProjectiles;
    return index;
}

void SurvivalController::ReleaseProjectile(std::size_t index) {
    if (index >= m_projectiles.size() || !m_projectiles[index].active) return;
    m_projectiles[index].active = false;
    m_freeProjectiles.push_back(index);
    m_activeProjectiles = std::max(0, m_activeProjectiles - 1);
}

void SurvivalController::SpawnEnemy() {
    const WaveRule& rule = m_waveRules[std::clamp(m_wave - 1, 0, 49)];
    const bool bossWave = rule.bossWave;
    EnemyArchetype archetype = EnemyArchetype::Riftling;
    if (bossWave) {
        archetype = rule.boss;
    } else {
        const float roll = NextRandom01();
        if (roll < rule.tanker) archetype = EnemyArchetype::ObsidianBrute;
        else if (roll < rule.tanker + rule.ranger) archetype = EnemyArchetype::HexArcher;

        // Remaining spawn value is a threat budget, not a raw unit count.
        if (SpawnCost(archetype) > m_remainingToSpawn) {
            archetype = m_remainingToSpawn >= 3
                ? EnemyArchetype::HexArcher : EnemyArchetype::Riftling;
        }
    }

    const int gate = std::min(3, (int)(NextRandom01() * 4.0f));
    const float offset = (NextRandom01() * 2.0f - 1.0f) * 12.0f;
    Vector3 position{};
    switch (gate) {
        case 0: position = {-18.5f, 0.0f, offset}; break;
        case 1: position = {18.5f, 0.0f, offset}; break;
        case 2: position = {offset, 0.0f, -18.5f}; break;
        default: position = {offset, 0.0f, 18.5f}; break;
    }

    SpawnEnemyAt(archetype, position);
    m_remainingToSpawn = bossWave ? 0
        : std::max(0, m_remainingToSpawn - SpawnCost(archetype));
}

void SurvivalController::SpawnEnemyAt(EnemyArchetype archetype, Vector3 position) {
    const std::size_t index = AcquireEnemy();
    if (index == std::numeric_limits<std::size_t>::max()) return;
    EnemyState& enemy = m_enemies[index];
    enemy.archetype = archetype;
    enemy.position = ClampToArena(position, kArenaHalfExtent, CollisionRadius(enemy));
    enemy.strafeDirection = NextRandom01() < 0.5f ? -1.0f : 1.0f;
    enemy.attackCooldown = 0.35f + NextRandom01() * 0.5f;
    if (index < m_enemyAnimationGraphs.size()) {
        m_enemyAnimationGraphs[index] = Animation::StateMachine(
            IsBoss(archetype) ? Animation::ActorClass::Boss
                              : Animation::ActorClass::Enemy);
        m_enemyAnimationGraphs[index].Reset(Animation::State::Idle);
    }
    if (index < m_enemyEventCursors.size()) m_enemyEventCursors[index].Stop();

    const float speedScale = 1.0f + std::min(0.30f, 0.006f * (m_wave - 1));
    switch (archetype) {
        case EnemyArchetype::HexArcher:
            enemy.hp = enemy.maxHp = 55.0f * HpScale();
            enemy.moveSpeed = 4.2f * speedScale;
            enemy.damage = 11.0f * DamageScale();
            enemy.scale = 1.0f;
            break;
        case EnemyArchetype::ObsidianBrute:
            enemy.hp = enemy.maxHp = 320.0f * HpScale();
            enemy.moveSpeed = 2.7f * speedScale;
            enemy.damage = 26.0f * DamageScale();
            enemy.scale = 1.0f;
            enemy.attackCooldown = 1.0f;
            break;
        case EnemyArchetype::BroodWarden:
            enemy.hp = enemy.maxHp = 2200.0f * HpScale();
            enemy.moveSpeed = 3.4f;
            enemy.damage = 25.0f * DamageScale();
            enemy.scale = 3.2f;
            enemy.specialCooldown = 5.5f;
            break;
        case EnemyArchetype::HexeyeArtillerist:
            enemy.hp = enemy.maxHp = 3400.0f * HpScale();
            enemy.moveSpeed = 3.6f;
            enemy.damage = 18.0f * DamageScale();
            enemy.scale = 3.8f;
            enemy.specialCooldown = 7.0f;
            break;
        case EnemyArchetype::IronrootColossus:
            enemy.hp = enemy.maxHp = 5200.0f * HpScale();
            enemy.moveSpeed = 2.35f;
            enemy.damage = 32.0f * DamageScale();
            enemy.scale = 5.2f;
            enemy.specialCooldown = 3.5f;
            break;
        case EnemyArchetype::EclipseChimera:
            enemy.hp = enemy.maxHp = 7000.0f * HpScale();
            enemy.moveSpeed = 4.0f;
            enemy.damage = 35.0f * DamageScale();
            enemy.scale = 5.8f;
            enemy.specialCooldown = 12.0f;
            break;
        case EnemyArchetype::VoidSovereign:
            enemy.hp = enemy.maxHp = 4500.0f * HpScale();
            enemy.moveSpeed = 4.8f;
            enemy.damage = 38.0f * DamageScale();
            enemy.scale = 2.4f;
            enemy.specialCooldown = 5.0f;
            enemy.bossPhase = 1;
            break;
        case EnemyArchetype::BossPrototype:
            enemy.hp = enemy.maxHp = 700.0f * (m_wave / 10.0f) * HpScale();
            enemy.moveSpeed = 2.7f;
            enemy.damage = 25.0f * DamageScale();
            enemy.scale = 3.0f;
            break;
        default:
            enemy.hp = enemy.maxHp = 22.0f * HpScale();
            enemy.moveSpeed = 6.8f * speedScale;
            enemy.damage = 7.0f * DamageScale();
            enemy.scale = 1.0f;
            break;
    }
    enemy.moveSpeed *= m_balance.enemySpeedMultiplier;
}

void SurvivalController::AdvanceEnemyAnimation(std::size_t index, float dt) {
    if (index >= m_enemies.size()) return;
    EnemyState& enemy = m_enemies[index];
    const EnemyAnimation target = ResolveEnemyAnimation(enemy);

    if (target != enemy.visualAnimation && index < m_enemyAnimationGraphs.size()) {
        Animation::StateMachine& graph = m_enemyAnimationGraphs[index];
        const float progress = enemy.actionTimer > 0.0f
            ? std::clamp(enemy.animationTime
                         / std::max(0.001f, enemy.animationTime + enemy.actionTimer),
                         0.0f, 1.0f)
            : 1.0f;
        const Animation::State targetState = GraphState(target);
        const bool targetIsBase = Animation::StateMachine::IsBaseState(targetState);
        const auto result = targetIsBase
            && !Animation::StateMachine::IsBaseState(graph.Current())
            ? graph.CompleteCurrent(target == EnemyAnimation::Run)
            : graph.Submit({targetState, GraphReason(target),
                            ++m_animationGraphSerial}, progress);
        // Presentation must never conceal a gameplay-critical death or phase
        // telegraph. Other rejected requests stay on the committed graph pose.
        if (!result.Accepted()
            && target != EnemyAnimation::Death
            && target != EnemyAnimation::Ultimate) {
            enemy.animationTime += dt;
            enemy.previousAnimationTime += dt;
            enemy.animationBlend = std::min(1.0f, enemy.animationBlend + dt / 0.14f);
            AdvanceEnemyEventTimeline(index, dt, false);
            return;
        }
    }

    const bool stateChanged = target != enemy.visualAnimation;
    if (target != enemy.visualAnimation) {
        enemy.previousVisualAnimation = enemy.visualAnimation;
        enemy.previousAnimationTime = enemy.animationTime;
        enemy.visualAnimation = target;
        enemy.animationTime = 0.0f;
        enemy.animationBlend = 0.0f;
    } else {
        enemy.animationTime += dt;
    }
    enemy.previousAnimationTime += dt;
    enemy.animationBlend = std::min(1.0f, enemy.animationBlend + dt / 0.14f);

    const auto local = Animation::WorldToLocalVelocity(
        enemy.velocity.x, enemy.velocity.z, enemy.facing.x, enemy.facing.z);
    const auto weights = Animation::SolveLocomotionBlend(
        local, std::max(0.1f, enemy.moveSpeed));
    enemy.locomotionBlend = {weights.idle, weights.forward, weights.backward,
                             weights.left, weights.right, weights.normalizedSpeed};
    AdvanceEnemyEventTimeline(index, dt, stateChanged);
}

void SurvivalController::AdvanceEnemyEventTimeline(std::size_t index, float dt,
                                                    bool stateChanged) {
    if (index >= m_enemies.size() || index >= m_enemyEventCursors.size()) return;
    EnemyState& enemy = m_enemies[index];
    Animation::NonHeroTrackId trackId{};
    if (!NonHeroTrack(enemy.visualAnimation, trackId)) {
        m_enemyEventCursors[index].Stop();
        enemy.actionHitboxActive = false;
        return;
    }

    Animation::EventCursor& cursor = m_enemyEventCursors[index];
    if (stateChanged || !cursor.IsStarted()) {
        cursor.Start(0, true);
        enemy.actionHitboxActive = false;
    }
    const Animation::EventTrack& track = Animation::GetNonHeroTrack(trackId);
    if (!track.IsValid()) return;

    std::uint64_t absoluteFrame = 0;
    if (track.looping) {
        absoluteFrame = static_cast<std::uint64_t>(
            std::floor(std::max(0.0f, enemy.animationTime) * 60.0f));
    } else {
        float remaining = 0.0f;
        std::uint16_t terminalFrame = track.frameCount > 0
            ? static_cast<std::uint16_t>(track.frameCount - 1u) : 0u;
        switch (enemy.visualAnimation) {
            case EnemyAnimation::BasicAttack:
                remaining = std::max(0.0f, enemy.actionTimer - dt);
                terminalFrame = 30;
                break;
            case EnemyAnimation::SkillOne:
                remaining = std::max(0.0f, enemy.actionTimer - dt);
                terminalFrame = 50;
                break;
            case EnemyAnimation::SkillTwo:
                remaining = std::max(0.0f, enemy.actionTimer - dt);
                terminalFrame = 44;
                break;
            case EnemyAnimation::Ultimate:
                remaining = std::max(0.0f, enemy.phaseTransitionTimer - dt);
                break;
            case EnemyAnimation::DashSpecial:
                remaining = std::max(0.0f, enemy.specialFxTimer - dt);
                break;
            case EnemyAnimation::Hurt:
                remaining = std::max(0.0f, enemy.hitFlash / 5.0f);
                break;
            case EnemyAnimation::Death:
                remaining = std::max(0.0f, enemy.deathTimer - dt);
                break;
            case EnemyAnimation::Idle:
            case EnemyAnimation::Run:
                break;
        }
        const float duration = enemy.animationTime + remaining;
        const float progress = duration > 0.0001f
            ? std::clamp(enemy.animationTime / duration, 0.0f, 1.0f) : 1.0f;
        absoluteFrame = static_cast<std::uint64_t>(std::floor(
            progress * static_cast<float>(terminalFrame) + 0.0001f));
    }

    EnemyEventDispatchContext context{this, index};
    cursor.Advance(track, absoluteFrame,
                   &SurvivalController::OnEnemyAnimationEvent, &context);
}

void SurvivalController::OnEnemyAnimationEvent(
    const Animation::EventOccurrence& occurrence, void* user) noexcept {
    if (user == nullptr || occurrence.event == nullptr) return;
    EnemyEventDispatchContext& context =
        *static_cast<EnemyEventDispatchContext*>(user);
    if (context.controller == nullptr) return;
    context.controller->HandleEnemyAnimationEvent(context.index,
                                                   *occurrence.event);
}

void SurvivalController::HandleEnemyAnimationEvent(
    std::size_t index, const Animation::FrameEvent& event) {
    if (index >= m_enemies.size() || !m_enemies[index].active) return;
    EnemyState& enemy = m_enemies[index];
    if (event.type == Animation::EventType::HitboxOn) {
        enemy.actionHitboxActive = true;
        return;
    }
    if (event.type == Animation::EventType::HitboxOff) {
        enemy.actionHitboxActive = false;
        return;
    }
    if (event.type != Animation::EventType::Vfx) return;
    switch (event.id) {
        case Animation::EventId::NonHeroMelee:
        case Animation::EventId::NonHeroProjectile:
        case Animation::EventId::NonHeroSkill:
        case Animation::EventId::NonHeroUltimate:
        case Animation::EventId::NonHeroSpecial:
            ++enemy.actionEventSerial;
            break;
        default:
            break;
    }
}

void SurvivalController::UpdateEnemies(float dt) {
    for (std::size_t i = 0; i < m_enemies.size(); ++i) {
        EnemyState& enemy = m_enemies[i];
        if (!enemy.active) continue;
        if (enemy.dying) {
            AdvanceEnemyAnimation(i, dt);
            enemy.deathTimer = std::max(0.0f, enemy.deathTimer - dt);
            enemy.velocity = {};
            if (enemy.deathTimer <= 0.0f) ReleaseEnemy(i);
            continue;
        }
        enemy.attackCooldown = std::max(0.0f, enemy.attackCooldown - dt);
        enemy.slowTimer = std::max(0.0f, enemy.slowTimer - dt);
        enemy.hitFlash = std::max(0.0f, enemy.hitFlash - dt * 5.0f);
        enemy.specialFxTimer = std::max(0.0f, enemy.specialFxTimer - dt);
        enemy.phaseTransitionTimer = std::max(0.0f, enemy.phaseTransitionTimer - dt);

        const bool hurtLocked = enemy.hitFlash > 0.0f;
        const bool phaseLocked = enemy.phaseTransitionTimer > 0.0f;
        const bool specialLocked = enemy.specialFxTimer > 0.0f;
        // A hit interrupts an unreadable/invisible wind-up. Phase changes also
        // start from a clean action state; specials pause behavior until their
        // presentation has completed.
        if ((hurtLocked || phaseLocked) && enemy.action != EnemyAction::None) {
            enemy.action = EnemyAction::None;
            enemy.actionTimer = 0.0f;
        }
        AdvanceEnemyAnimation(i, dt);
        if (enemy.burnTimer > 0.0f) {
            enemy.burnTimer = std::max(0.0f, enemy.burnTimer - dt);
            enemy.hp -= enemy.burnDps * dt;
            if (enemy.hp <= 0.0f) {
                DamageEnemy(i, 0.0f, {});
                continue;
            }
        }
        if (hurtLocked || phaseLocked || specialLocked) {
            enemy.velocity = {};
            continue;
        }

        const bool farLod = !IsBoss(enemy.archetype)
            && DistanceXZ(enemy.position, m_player.position) > 18.0f;
        if (farLod && ((m_simTick + i) & 1u) != 0u) continue;
        const float behaviorDt = farLod ? dt * 2.0f : dt;
        const Vector3 previousPosition = enemy.position;
        const Vector3 look = NormalizeXZ({m_player.position.x - enemy.position.x, 0.0f,
                                          m_player.position.z - enemy.position.z});
        if (LengthXZ(look) > 0.01f) enemy.facing = look;
        switch (enemy.archetype) {
            case EnemyArchetype::HexArcher: UpdateHexArcher(i, behaviorDt); break;
            case EnemyArchetype::ObsidianBrute: UpdateObsidianBrute(i, behaviorDt); break;
            case EnemyArchetype::BroodWarden: UpdateBroodWarden(i, behaviorDt); break;
            case EnemyArchetype::HexeyeArtillerist: UpdateHexeyeArtillerist(i, behaviorDt); break;
            case EnemyArchetype::IronrootColossus: UpdateIronrootColossus(i, behaviorDt); break;
            case EnemyArchetype::EclipseChimera: UpdateEclipseChimera(i, behaviorDt); break;
            case EnemyArchetype::VoidSovereign: UpdateVoidSovereign(i, behaviorDt); break;
            case EnemyArchetype::BossPrototype: UpdateObsidianBrute(i, behaviorDt); break;
            default: UpdateRiftling(i, behaviorDt); break;
        }
        if (m_phase == Phase::RunFailed) return;
        if (behaviorDt > 0.0f) {
            enemy.velocity = {(enemy.position.x - previousPosition.x) / behaviorDt, 0.0f,
                              (enemy.position.z - previousPosition.z) / behaviorDt};
        }

        // Cheap local separation keeps the prototype swarm readable without
        // introducing a full RVO dependency in the first vertical slice.
        const float radius = CollisionRadius(enemy);
        for (std::size_t j = i + 1; j < m_enemies.size(); ++j) {
            EnemyState& other = m_enemies[j];
            if (!other.active || other.dying) continue;
            Vector3 apart{other.position.x - enemy.position.x, 0.0f,
                          other.position.z - enemy.position.z};
            const float distance = LengthXZ(apart);
            const float desired = radius + CollisionRadius(other);
            if (distance > 0.001f && distance < desired) {
                const Vector3 normal = NormalizeXZ(apart);
                const float correction = (desired - distance) * 0.5f;
                enemy.position.x -= normal.x * correction;
                enemy.position.z -= normal.z * correction;
                other.position.x += normal.x * correction;
                other.position.z += normal.z * correction;
            }
        }
        enemy.position = ClampToArena(enemy.position, kArenaHalfExtent, radius);
        enemy.position = ResolveArenaPillars(enemy.position, radius);
    }
}

void SurvivalController::UpdateRiftling(std::size_t index, float dt) {
    EnemyState& enemy = m_enemies[index];
    Vector3 toPlayer{m_player.position.x - enemy.position.x, 0.0f,
                     m_player.position.z - enemy.position.z};
    const float distance = LengthXZ(toPlayer);
    Vector3 direction = NormalizeXZ(toPlayer);
    if (enemy.action == EnemyAction::ClawSweep) {
        enemy.actionTimer -= dt;
        enemy.velocity = {};
        if (enemy.actionTimer <= 0.0f) {
            if (distance <= 1.15f && HasArenaLineOfSight(enemy.position, m_player.position)) {
                DamagePlayer(enemy.damage);
            }
            enemy.action = EnemyAction::None;
            enemy.attackCooldown = 0.90f;
            SoundManager::GetInstance().PlaySound("enemy_attack");
        }
        return;
    }
    if (m_gravityWellTimer > 0.0f) {
        Vector3 toWell{m_gravityWellCenter.x - enemy.position.x, 0.0f,
                       m_gravityWellCenter.z - enemy.position.z};
        const float wellDistance = LengthXZ(toWell);
        if (wellDistance < 5.5f && wellDistance > 0.1f) {
            const Vector3 pull = NormalizeXZ(toWell);
            direction = NormalizeXZ({direction.x + pull.x * 1.8f, 0.0f,
                                     direction.z + pull.z * 1.8f});
        }
    }
    if (distance > 1.0f) {
        const float slow = enemy.slowTimer > 0.0f ? 0.55f : 1.0f;
        enemy.velocity = {direction.x * enemy.moveSpeed * slow, 0.0f,
                          direction.z * enemy.moveSpeed * slow};
        enemy.position.x += enemy.velocity.x * dt;
        enemy.position.z += enemy.velocity.z * dt;
    } else {
        enemy.velocity = {};
        if (enemy.attackCooldown <= 0.0f) {
            enemy.action = EnemyAction::ClawSweep;
            enemy.actionTimer = 0.42f;
        }
    }
}

void SurvivalController::UpdateHexArcher(std::size_t index, float dt) {
    EnemyState& enemy = m_enemies[index];
    Vector3 toPlayer{m_player.position.x - enemy.position.x, 0.0f,
                     m_player.position.z - enemy.position.z};
    const float distance = LengthXZ(toPlayer);
    const Vector3 direction = NormalizeXZ(toPlayer);

    if (enemy.action == EnemyAction::RangedShot) {
        enemy.actionTimer -= dt;
        enemy.velocity = {};
        if (enemy.actionTimer <= 0.0f) {
            SpawnHostileProjectile({enemy.position.x, 1.05f, enemy.position.z},
                                   {m_player.position.x, 0.65f, m_player.position.z},
                                   10.5f, enemy.damage, 0.18f);
            enemy.action = EnemyAction::None;
            enemy.attackCooldown = 1.65f;
            SoundManager::GetInstance().PlaySound("enemy_attack");
        }
        return;
    }

    Vector3 move{};
    const Vector3 side{-direction.z * enemy.strafeDirection, 0.0f,
                       direction.x * enemy.strafeDirection};
    if (distance < 7.0f) move = {-direction.x + side.x * 0.35f, 0.0f,
                                 -direction.z + side.z * 0.35f};
    else if (distance > 12.5f) move = {direction.x + side.x * 0.25f, 0.0f,
                                       direction.z + side.z * 0.25f};
    else move = side;
    move = NormalizeXZ(move);
    const float slow = enemy.slowTimer > 0.0f ? 0.55f : 1.0f;
    enemy.velocity = {move.x * enemy.moveSpeed * slow, 0.0f,
                      move.z * enemy.moveSpeed * slow};
    enemy.position.x += enemy.velocity.x * dt;
    enemy.position.z += enemy.velocity.z * dt;

    if (distance <= 14.0f && enemy.attackCooldown <= 0.0f) {
        enemy.action = EnemyAction::RangedShot;
        enemy.actionTimer = 0.50f;
    }
}

void SurvivalController::UpdateObsidianBrute(std::size_t index, float dt) {
    EnemyState& enemy = m_enemies[index];
    Vector3 toPlayer{m_player.position.x - enemy.position.x, 0.0f,
                     m_player.position.z - enemy.position.z};
    const float distance = LengthXZ(toPlayer);
    const Vector3 direction = NormalizeXZ(toPlayer);

    if (enemy.action == EnemyAction::GroundSlam) {
        enemy.actionTimer -= dt;
        enemy.velocity = {};
        if (enemy.actionTimer <= 0.0f) {
            if (distance <= 4.2f && HasArenaLineOfSight(enemy.position, m_player.position)) {
                DamagePlayer(enemy.damage);
                if (m_phase != Phase::RunFailed) {
                    m_player.position.x += direction.x * 2.2f;
                    m_player.position.z += direction.z * 2.2f;
                    const float airborneHeight = m_player.position.y;
                    m_player.position = ClampToArena(m_player.position, kArenaHalfExtent, 0.45f);
                    m_player.position = ResolveArenaPillars(m_player.position, 0.45f);
                    m_player.position.y = airborneHeight;
                }
            }
            enemy.action = EnemyAction::None;
            enemy.attackCooldown = 2.5f;
            SoundManager::GetInstance().PlaySound("boss_attack");
        }
        return;
    }

    if (distance > 3.6f) {
        const float slow = enemy.slowTimer > 0.0f ? 0.65f : 1.0f;
        enemy.velocity = {direction.x * enemy.moveSpeed * slow, 0.0f,
                          direction.z * enemy.moveSpeed * slow};
        enemy.position.x += enemy.velocity.x * dt;
        enemy.position.z += enemy.velocity.z * dt;
    } else if (enemy.attackCooldown <= 0.0f) {
        enemy.velocity = {};
        enemy.action = EnemyAction::GroundSlam;
        enemy.actionTimer = 0.80f;
    }
}

void SurvivalController::UpdateBroodWarden(std::size_t index, float dt) {
    EnemyState& enemy = m_enemies[index];
    if (enemy.bossPhase == 1 && enemy.hp <= enemy.maxHp * 0.55f) {
        enemy.bossPhase = 2;
        enemy.moveSpeed *= 1.15f;
        enemy.phaseTransitionTimer = 1.2f;
        enemy.action = EnemyAction::None;
        enemy.actionTimer = 0.0f;
        enemy.velocity = {};
        SoundManager::GetInstance().PlaySound("boss_phase");
        return;
    }
    enemy.specialCooldown -= dt;
    if (enemy.action == EnemyAction::None
        && enemy.specialCooldown <= 0.0f && m_activeEnemies < 116) {
        const int broodCount = enemy.bossPhase == 1 ? 2 : 3;
        for (int n = 0; n < broodCount; ++n) {
            const float angle = (6.2831853f / broodCount) * n;
            SpawnEnemyAt(EnemyArchetype::Riftling,
                         {enemy.position.x + std::cos(angle) * 2.2f, 0.0f,
                          enemy.position.z + std::sin(angle) * 2.2f});
        }
        enemy.specialCooldown = enemy.bossPhase == 1 ? 7.5f : 5.5f;
        enemy.specialFxTimer = 0.85f;
        SoundManager::GetInstance().PlaySound("boss_phase");
        return;
    }

    Vector3 toPlayer{m_player.position.x - enemy.position.x, 0.0f,
                     m_player.position.z - enemy.position.z};
    const float distance = LengthXZ(toPlayer);
    const Vector3 direction = NormalizeXZ(toPlayer);
    if (enemy.action == EnemyAction::ClawSweep) {
        enemy.actionTimer -= dt;
        if (enemy.actionTimer <= 0.0f) {
            if (distance <= 3.8f && HasArenaLineOfSight(enemy.position, m_player.position)) {
                DamagePlayer(enemy.damage);
            }
            enemy.action = EnemyAction::None;
            enemy.attackCooldown = enemy.bossPhase == 1 ? 1.8f : 1.45f;
            enemy.specialFxTimer = 0.28f;
            SoundManager::GetInstance().PlaySound("boss_attack");
        }
        return;
    }
    if (distance > 3.1f) {
        enemy.position.x += direction.x * enemy.moveSpeed * dt;
        enemy.position.z += direction.z * enemy.moveSpeed * dt;
    } else if (enemy.attackCooldown <= 0.0f) {
        enemy.action = EnemyAction::ClawSweep;
        enemy.actionTimer = 0.60f;
    }
}

void SurvivalController::UpdateHexeyeArtillerist(std::size_t index, float dt) {
    EnemyState& enemy = m_enemies[index];
    if (enemy.bossPhase == 1 && enemy.hp <= enemy.maxHp * 0.50f) {
        enemy.bossPhase = 2;
        enemy.phaseTransitionTimer = 1.2f;
        enemy.action = EnemyAction::None;
        enemy.actionTimer = 0.0f;
        enemy.velocity = {};
        SoundManager::GetInstance().PlaySound("boss_phase");
        return;
    }
    enemy.specialCooldown -= dt;
    if (enemy.action == EnemyAction::None
        && enemy.specialCooldown <= 0.0f && m_activeEnemies < 116) {
        SpawnEnemyAt(EnemyArchetype::HexArcher,
                     {enemy.position.x - 2.2f, 0.0f, enemy.position.z + 1.4f});
        SpawnEnemyAt(EnemyArchetype::HexArcher,
                     {enemy.position.x + 2.2f, 0.0f, enemy.position.z - 1.4f});
        enemy.specialCooldown = enemy.bossPhase == 1 ? 9.0f : 7.0f;
        enemy.specialFxTimer = 0.85f;
        SoundManager::GetInstance().PlaySound("boss_phase");
        return;
    }

    Vector3 toPlayer{m_player.position.x - enemy.position.x, 0.0f,
                     m_player.position.z - enemy.position.z};
    const float distance = LengthXZ(toPlayer);
    const Vector3 direction = NormalizeXZ(toPlayer);
    if (enemy.action == EnemyAction::TargetingVolley) {
        enemy.actionTimer -= dt;
        if (enemy.actionTimer <= 0.0f) {
            const Vector3 side{-direction.z, 0.0f, direction.x};
            const int shotCount = enemy.bossPhase == 1 ? 3 : 5;
            for (int shot = 0; shot < shotCount; ++shot) {
                const float offset = (shot - (shotCount - 1) * 0.5f) * 0.70f;
                SpawnHostileProjectile({enemy.position.x, 2.6f, enemy.position.z},
                    {m_player.position.x + side.x * offset, 0.65f,
                     m_player.position.z + side.z * offset},
                    9.0f, enemy.damage, 0.24f, 0.75f);
            }
            enemy.action = EnemyAction::None;
            enemy.attackCooldown = enemy.bossPhase == 1 ? 2.6f : 2.0f;
            enemy.specialFxTimer = 0.32f;
            SoundManager::GetInstance().PlaySound("enemy_attack");
        }
        return;
    }

    Vector3 move{};
    const Vector3 side{-direction.z * enemy.strafeDirection, 0.0f,
                       direction.x * enemy.strafeDirection};
    if (distance < 8.0f) move = {-direction.x, 0.0f, -direction.z};
    else if (distance > 13.0f) move = direction;
    else move = side;
    enemy.position.x += move.x * enemy.moveSpeed * dt;
    enemy.position.z += move.z * enemy.moveSpeed * dt;
    if (distance <= 16.0f && enemy.attackCooldown <= 0.0f) {
        enemy.action = EnemyAction::TargetingVolley;
        enemy.actionTimer = 0.90f;
    }
}

void SurvivalController::UpdateIronrootColossus(std::size_t index, float dt) {
    EnemyState& enemy = m_enemies[index];
    if (enemy.bossPhase == 1 && enemy.hp <= enemy.maxHp * 0.50f) {
        enemy.bossPhase = 2;
        enemy.moveSpeed *= 1.18f;
        enemy.phaseTransitionTimer = 1.2f;
        enemy.action = EnemyAction::None;
        enemy.actionTimer = 0.0f;
        enemy.velocity = {};
        SoundManager::GetInstance().PlaySound("boss_phase");
        return;
    }
    Vector3 toPlayer{m_player.position.x - enemy.position.x, 0.0f,
                     m_player.position.z - enemy.position.z};
    const float distance = LengthXZ(toPlayer);
    const Vector3 direction = NormalizeXZ(toPlayer);

    if (enemy.action == EnemyAction::GroundSlam) {
        enemy.actionTimer -= dt;
        if (enemy.actionTimer <= 0.0f) {
            if (distance <= 5.2f && HasArenaLineOfSight(enemy.position, m_player.position)) {
                DamagePlayer(enemy.damage);
            }
            const Vector3 side{-direction.z, 0.0f, direction.x};
            const int fissures = enemy.bossPhase == 1 ? 3 : 5;
            for (int shot = 0; shot < fissures; ++shot) {
                const float spread = (shot - (fissures - 1) * 0.5f) * 0.9f;
                SpawnHostileProjectile({enemy.position.x, 0.25f, enemy.position.z},
                    {m_player.position.x + side.x * spread, 0.25f,
                     m_player.position.z + side.z * spread},
                    8.0f, enemy.damage * 0.65f, 0.34f, 0.65f);
            }
            enemy.action = EnemyAction::None;
            enemy.attackCooldown = enemy.bossPhase == 1 ? 2.8f : 2.2f;
            enemy.specialFxTimer = 0.90f;
            SoundManager::GetInstance().PlaySound("boss_attack");
        }
        return;
    }
    if (distance > 4.3f) {
        enemy.position.x += direction.x * enemy.moveSpeed * dt;
        enemy.position.z += direction.z * enemy.moveSpeed * dt;
    } else if (enemy.attackCooldown <= 0.0f) {
        enemy.action = EnemyAction::GroundSlam;
        enemy.actionTimer = enemy.bossPhase == 1 ? 1.05f : 0.82f;
    }
}

void SurvivalController::UpdateEclipseChimera(std::size_t index, float dt) {
    EnemyState& enemy = m_enemies[index];
    enemy.specialCooldown -= dt;
    if (enemy.specialCooldown <= 0.0f) {
        enemy.bossPhase = enemy.bossPhase == 1 ? 2 : 1;
        enemy.phaseTransitionTimer = 1.0f;
        enemy.specialCooldown = enemy.hp <= enemy.maxHp * 0.50f ? 12.0f : 18.0f;
        enemy.action = EnemyAction::None;
        enemy.actionTimer = 0.0f;
        enemy.velocity = {};
        SoundManager::GetInstance().PlaySound("boss_phase");
        return;
    }

    Vector3 toPlayer{m_player.position.x - enemy.position.x, 0.0f,
                     m_player.position.z - enemy.position.z};
    const float distance = LengthXZ(toPlayer);
    const Vector3 direction = NormalizeXZ(toPlayer);
    if (enemy.action == EnemyAction::ClawSweep) {
        enemy.actionTimer -= dt;
        if (enemy.actionTimer <= 0.0f) {
            if (distance <= 4.8f && HasArenaLineOfSight(enemy.position, m_player.position)) {
                DamagePlayer(enemy.damage);
            }
            enemy.action = EnemyAction::None;
            enemy.attackCooldown = 1.65f;
            enemy.specialFxTimer = 0.32f;
            SoundManager::GetInstance().PlaySound("boss_attack");
        }
        return;
    }
    if (enemy.action == EnemyAction::TargetingVolley) {
        enemy.actionTimer -= dt;
        if (enemy.actionTimer <= 0.0f) {
            const Vector3 side{-direction.z, 0.0f, direction.x};
            for (int shot = -2; shot <= 2; ++shot) {
                SpawnHostileProjectile({enemy.position.x, 2.4f, enemy.position.z},
                    {m_player.position.x + side.x * shot * 0.9f, 0.6f,
                     m_player.position.z + side.z * shot * 0.9f},
                    7.4f, enemy.damage * 0.72f, 0.28f, 0.55f);
            }
            enemy.action = EnemyAction::None;
            enemy.attackCooldown = 2.2f;
            enemy.specialFxTimer = 0.40f;
            SoundManager::GetInstance().PlaySound("boss_attack");
        }
        return;
    }

    if (enemy.bossPhase == 1) {
        if (distance > 4.0f) {
            enemy.position.x += direction.x * enemy.moveSpeed * dt;
            enemy.position.z += direction.z * enemy.moveSpeed * dt;
        } else if (enemy.attackCooldown <= 0.0f) {
            enemy.action = EnemyAction::ClawSweep;
            enemy.actionTimer = 0.62f;
        }
    } else {
        Vector3 move{};
        if (distance < 7.0f) move = {-direction.x, 0.0f, -direction.z};
        else if (distance > 11.5f) move = direction;
        else move = {-direction.z * enemy.strafeDirection, 0.0f,
                     direction.x * enemy.strafeDirection};
        enemy.position.x += move.x * enemy.moveSpeed * dt;
        enemy.position.z += move.z * enemy.moveSpeed * dt;
        if (distance < 15.0f && enemy.attackCooldown <= 0.0f) {
            enemy.action = EnemyAction::TargetingVolley;
            enemy.actionTimer = 0.85f;
        }
    }
}

void SurvivalController::UpdateVoidSovereign(std::size_t index, float dt) {
    EnemyState& enemy = m_enemies[index];
    enemy.specialCooldown -= dt;
    if (enemy.action == EnemyAction::None && enemy.bossPhase >= 2
        && enemy.specialCooldown <= 0.0f && m_activeEnemies < 112) {
        const int summons = enemy.bossPhase == 2 ? 2 : 3;
        for (int n = 0; n < summons; ++n) {
            const float angle = 6.2831853f * n / summons;
            const EnemyArchetype summon = n % 3 == 0 ? EnemyArchetype::ObsidianBrute
                : (n % 2 == 0 ? EnemyArchetype::HexArcher : EnemyArchetype::Riftling);
            SpawnEnemyAt(summon, {enemy.position.x + std::cos(angle) * 3.5f, 0.0f,
                                  enemy.position.z + std::sin(angle) * 3.5f});
        }
        enemy.specialCooldown = enemy.bossPhase == 2 ? 9.0f : 6.5f;
        enemy.specialFxTimer = 0.90f;
        SoundManager::GetInstance().PlaySound("boss_phase");
        return;
    }

    Vector3 toPlayer{m_player.position.x - enemy.position.x, 0.0f,
                     m_player.position.z - enemy.position.z};
    const float distance = LengthXZ(toPlayer);
    const Vector3 direction = NormalizeXZ(toPlayer);
    if (enemy.action == EnemyAction::ClawSweep || enemy.action == EnemyAction::GroundSlam) {
        enemy.actionTimer -= dt;
        if (enemy.actionTimer <= 0.0f) {
            const float radius = enemy.action == EnemyAction::GroundSlam ? 5.5f : 3.6f;
            if (distance <= radius && HasArenaLineOfSight(enemy.position, m_player.position)) {
                DamagePlayer(enemy.damage);
            }
            enemy.action = EnemyAction::None;
            enemy.attackCooldown = enemy.bossPhase == 3 ? 1.2f : 1.65f;
            enemy.specialFxTimer = enemy.bossPhase == 3 ? 0.70f : 0.38f;
            SoundManager::GetInstance().PlaySound("boss_attack");
        }
        return;
    }
    if (enemy.action == EnemyAction::TargetingVolley) {
        enemy.actionTimer -= dt;
        if (enemy.actionTimer <= 0.0f) {
            const Vector3 side{-direction.z, 0.0f, direction.x};
            const int count = enemy.bossPhase == 2 ? 5 : 7;
            for (int shot = 0; shot < count; ++shot) {
                const float offset = (shot - (count - 1) * 0.5f) * 0.72f;
                SpawnHostileProjectile({enemy.position.x, 2.0f + enemy.bossPhase * 0.3f,
                                        enemy.position.z},
                    {m_player.position.x + side.x * offset, 0.65f,
                     m_player.position.z + side.z * offset},
                    enemy.bossPhase == 3 ? 10.5f : 9.0f,
                    enemy.damage * 0.70f, 0.26f, 0.65f);
            }
            enemy.action = EnemyAction::None;
            enemy.attackCooldown = enemy.bossPhase == 3 ? 1.35f : 2.0f;
            enemy.specialFxTimer = 0.42f;
            SoundManager::GetInstance().PlaySound("boss_attack");
        }
        return;
    }

    if (enemy.bossPhase == 1) {
        if (distance > 3.2f) {
            enemy.position.x += direction.x * enemy.moveSpeed * dt;
            enemy.position.z += direction.z * enemy.moveSpeed * dt;
        } else if (enemy.attackCooldown <= 0.0f) {
            enemy.action = EnemyAction::ClawSweep;
            enemy.actionTimer = 0.52f;
        }
    } else if (enemy.bossPhase == 2) {
        if (distance < 8.0f) {
            enemy.position.x -= direction.x * enemy.moveSpeed * dt;
            enemy.position.z -= direction.z * enemy.moveSpeed * dt;
        }
        if (enemy.attackCooldown <= 0.0f) {
            enemy.action = EnemyAction::TargetingVolley;
            enemy.actionTimer = 0.75f;
        }
    } else {
        if (distance > 4.2f) {
            enemy.position.x += direction.x * enemy.moveSpeed * dt;
            enemy.position.z += direction.z * enemy.moveSpeed * dt;
        }
        if (enemy.attackCooldown <= 0.0f) {
            if (NextRandom01() < 0.45f) {
                enemy.action = EnemyAction::GroundSlam;
                enemy.actionTimer = 0.62f;
            } else {
                enemy.action = EnemyAction::TargetingVolley;
                enemy.actionTimer = 0.68f;
            }
        }
    }
}

void SurvivalController::PerformBasicAttack() {
    constexpr float kBasicContact = 28.0f / 62.0f; // frame 188 in [160, 222]
    const bool knight = m_player.character == CharacterId::Knight;
    m_playerCombo.Begin(Animation::GetComboDefinition(
        knight ? Animation::ComboChainId::KnightThreeHit
               : Animation::ComboChainId::MageCastChain), 0);
    m_player.comboStep = 0;
    BeginPlayerAction(PlayerCombatAction::Basic, PlayerAnimation::BasicAttack,
                      knight ? 0.62f : 0.66f, kBasicContact);
    m_player.basicCooldown = Cooldown(knight ? 0.38f : 0.32f);
}

void SurvivalController::SpawnArcBolt() {
    const auto spawnBolt = [&](Vector3 direction, float damageMultiplier) {
        const std::size_t index = AcquireProjectile();
        if (index == std::numeric_limits<std::size_t>::max()) return;
        ProjectileState& projectile = m_projectiles[index];
        projectile.position = {m_player.position.x + direction.x * 0.8f,
                               m_player.position.y + 0.9f,
                               m_player.position.z + direction.z * 0.8f};
        projectile.velocity = {direction.x * 13.0f, 0.0f, direction.z * 13.0f};
        projectile.damage = 18.0f * m_player.damageMultiplier * damageMultiplier;
        projectile.radius = 0.22f * m_player.projectileScale;
        projectile.splashRadius = 1.2f * m_player.areaMultiplier;
        projectile.lifetime = 2.0f;
        projectile.hostile = false;
        projectile.visual = ProjectileVisual::ArcBolt;
        projectile.remainingPierce = m_player.projectilePierce;
    };
    spawnBolt(m_player.facing, 1.0f);
    if (m_player.forkedBolt) {
        constexpr float angle = 0.20f;
        const float c = std::cos(angle);
        const float s = std::sin(angle);
        const Vector3 left{m_player.facing.x * c - m_player.facing.z * s, 0.0f,
                           m_player.facing.x * s + m_player.facing.z * c};
        const Vector3 right{m_player.facing.x * c + m_player.facing.z * s, 0.0f,
                            -m_player.facing.x * s + m_player.facing.z * c};
        spawnBolt(left, 0.55f);
        spawnBolt(right, 0.55f);
    }
}

void SurvivalController::SpawnHostileProjectile(Vector3 origin, Vector3 target, float speed,
                                                 float damage, float radius,
                                                 float splashRadius) {
    const std::size_t index = AcquireProjectile();
    if (index == std::numeric_limits<std::size_t>::max()) return;
    ProjectileState& projectile = m_projectiles[index];
    const Vector3 direction = Vector3Normalize(Vector3Subtract(target, origin));
    projectile.position = origin;
    projectile.velocity = Vector3Scale(direction, speed);
    projectile.damage = damage;
    projectile.radius = radius;
    projectile.splashRadius = splashRadius;
    projectile.lifetime = 3.2f;
    projectile.hostile = true;
    projectile.visual = ProjectileVisual::HostileOrb;
}

void SurvivalController::PerformSkillOne() {
    constexpr float kSkillOneContact = 50.0f / 78.0f; // frame 274 in [224, 302]
    const bool knight = m_player.character == CharacterId::Knight;
    BeginPlayerAction(PlayerCombatAction::SkillOne, PlayerAnimation::SkillOne,
                      knight ? 0.95f : 1.08f, kSkillOneContact);
    m_player.skillOneCooldown = Cooldown(knight ? 6.0f : 7.0f);
}

void SurvivalController::PerformSkillTwo() {
    constexpr float kSkillTwoContact = 40.0f / 78.0f; // frame 344 in [304, 382]
    const bool knight = m_player.character == CharacterId::Knight;
    BeginPlayerAction(PlayerCombatAction::SkillTwo, PlayerAnimation::SkillTwo,
                      knight ? 0.98f : 1.16f, kSkillTwoContact);
    m_player.skillTwoCooldown = Cooldown(knight ? 8.0f : 10.0f);
    if (knight)
        m_player.invulnerableTimer = std::max(m_player.invulnerableTimer, 0.58f);
}

void SurvivalController::PerformUltimate() {
    constexpr float kUltimateContact = 64.0f / 94.0f; // frame 448 in [384, 478]
    const bool knight = m_player.character == CharacterId::Knight;
    BeginPlayerAction(PlayerCombatAction::Ultimate, PlayerAnimation::Ultimate,
                      knight ? 1.55f : 1.68f, kUltimateContact);
    m_player.ultimateCharge = 0.0f;
    m_player.invulnerableTimer = std::max(m_player.invulnerableTimer,
                                          knight ? 1.15f : 1.20f);
}

void SurvivalController::PerformDash() {
    constexpr float kDashContact = 25.0f / 46.0f; // frame 505 in [480, 526]
    Vector3 direction = LengthXZ(m_moveInput) > 0.1f ? m_moveInput : m_player.facing;
    if (LengthXZ(direction) <= 0.1f) direction = {0.0f, 0.0f, 1.0f};
    m_player.facing = NormalizeXZ(direction);
    const bool knight = m_player.character == CharacterId::Knight;
    BeginPlayerAction(PlayerCombatAction::Dash, PlayerAnimation::Dash,
                      knight ? 0.38f : 0.42f, kDashContact);
    m_player.dashCooldown = Cooldown(knight ? 1.2f : 1.5f);
    m_player.invulnerableTimer = std::max(m_player.invulnerableTimer,
                                          knight ? 0.34f : 0.32f);
}

void SurvivalController::BeginPlayerAction(PlayerCombatAction action,
                                           PlayerAnimation animation,
                                           float duration,
                                           float contactNormalized) {
    m_playerAnimationGraph.Submit({GraphState(animation),
                                   Animation::TransitionReason::Gameplay,
                                   ++m_animationGraphSerial},
                                  m_player.animationDuration > 0.0f
                                      ? std::clamp(m_player.animationTime
                                                   / m_player.animationDuration,
                                                   0.0f, 1.0f)
                                      : 1.0f);
    SetPlayerAnimation(animation, duration);
    m_player.combatAction = action;
    m_player.actionContactNormalized = std::clamp(contactNormalized, 0.0f, 1.0f);
    m_player.actionContactTriggered = false;
    ++m_player.actionSerial;
    m_playerActionAim = m_aimPoint;
    m_playerActionStart = m_player.position;
    m_playerActionFacing = NormalizeXZ(m_player.facing);
    if (LengthXZ(m_playerActionFacing) <= 0.1f)
        m_playerActionFacing = {0.0f, 0.0f, 1.0f};
    m_player.actionHitboxActive = false;
    m_previousActionProgress = 0.0f;
    m_playerEventTrack = HeroTrack(m_player.character, action);
    m_playerEventCursor.Start(0, true);
    m_playerEventTrackActive = true;
}

void SurvivalController::UpdatePlayerActionTimeline() {
    if (m_player.combatAction == PlayerCombatAction::None
        || m_player.animationDuration <= 0.0f) return;
    const float progress = m_player.animationTime / m_player.animationDuration;
    if (m_playerEventTrackActive) {
        const Animation::EventTrack& track = Animation::GetHeroTrack(m_playerEventTrack);
        const std::uint64_t frame = track.frameCount > 1
            ? static_cast<std::uint64_t>(std::floor(
                std::clamp(progress, 0.0f, 1.0f) * (track.frameCount - 1u)))
            : 0u;
        const auto advanced = m_playerEventCursor.Advance(
            track, frame, &SurvivalController::OnPlayerAnimationEvent, this);
        if (advanced != Animation::CursorAdvance::InvalidTrack) return;
        m_playerEventTrackActive = false;
    }

    // Defensive compatibility path for an invalid/missing event track.
    if (m_player.actionContactTriggered
        || progress + 0.0001f < m_player.actionContactNormalized) return;
    m_player.actionContactTriggered = true;
    ++m_player.contactSerial;
    ResolvePlayerActionContact();
}

void SurvivalController::UpdatePlayerCombo() {
    if (!m_playerCombo.IsActive() || m_player.animationDuration <= 0.0f)
        return;
    const bool knight = m_player.character == CharacterId::Knight;
    const Animation::ComboDefinition& definition = Animation::GetComboDefinition(
        knight ? Animation::ComboChainId::KnightThreeHit
               : Animation::ComboChainId::MageCastChain);
    const std::uint8_t stepIndex = m_playerCombo.StepIndex();
    if (stepIndex >= definition.stepCount) {
        m_playerCombo.Cancel();
        m_player.comboStep = 0;
        return;
    }
    const Animation::ComboStep& step = definition.steps[stepIndex];
    const float progress = std::clamp(
        m_player.animationTime / m_player.animationDuration, 0.0f, 1.0f);
    const std::uint64_t absoluteFrame = m_playerCombo.StepStartFrame()
        + static_cast<std::uint64_t>(std::floor(progress * step.durationFrames));

    const bool wantsNext = knight
        ? m_basicQueued
        : (stepIndex == 0
            ? (m_skillOneQueued && m_player.skillOneCooldown <= 0.0f)
            : (stepIndex == 1
                && m_skillTwoQueued && m_player.skillTwoCooldown <= 0.0f));
    if (wantsNext) m_playerCombo.QueueInput(absoluteFrame);

    const Animation::ComboUpdate update = m_playerCombo.Update(absoluteFrame);
    if (update.type == Animation::ComboUpdateType::StepChanged) {
        m_player.comboStep = update.stepIndex;
        BeginComboStep(update.move);
    } else if (update.type == Animation::ComboUpdateType::Finished) {
        m_player.comboStep = 0;
    }
}

void SurvivalController::BeginComboStep(Animation::ComboMove move) {
    // The combo window is an authored natural completion/cancel point. Move
    // the graph back through its base state before submitting the next clip so
    // a same-family attack does not look like a stale restart.
    m_playerAnimationGraph.CompleteCurrent(false);
    switch (move) {
        case Animation::ComboMove::KnightLightTwo:
            BeginPlayerAction(PlayerCombatAction::Basic,
                              PlayerAnimation::BasicAttack,
                              0.66f, 30.0f / 66.0f);
            m_player.basicCooldown = Cooldown(0.18f);
            break;
        case Animation::ComboMove::KnightLightThree:
            BeginPlayerAction(PlayerCombatAction::Basic,
                              PlayerAnimation::BasicAttack,
                              0.78f, 38.0f / 74.0f);
            m_player.basicCooldown = Cooldown(0.18f);
            break;
        case Animation::ComboMove::MageFrostLink:
            BeginPlayerAction(PlayerCombatAction::SkillOne,
                              PlayerAnimation::SkillOne,
                              0.95f, 50.0f / 78.0f);
            m_player.skillOneCooldown = Cooldown(7.0f);
            break;
        case Animation::ComboMove::MageGravityLink:
            BeginPlayerAction(PlayerCombatAction::SkillTwo,
                              PlayerAnimation::SkillTwo,
                              1.05f, 40.0f / 78.0f);
            m_player.skillTwoCooldown = Cooldown(10.0f);
            break;
        case Animation::ComboMove::KnightLightOne:
        case Animation::ComboMove::MageArcaneBolt:
        case Animation::ComboMove::None:
            break;
    }
}

void SurvivalController::OnPlayerAnimationEvent(
    const Animation::EventOccurrence& occurrence, void* user) noexcept {
    if (user == nullptr || occurrence.event == nullptr) return;
    static_cast<SurvivalController*>(user)->HandlePlayerAnimationEvent(
        *occurrence.event);
}

void SurvivalController::HandlePlayerAnimationEvent(
    const Animation::FrameEvent& event) {
    using Animation::EventId;
    using Animation::EventType;
    if (event.type == EventType::HitboxOn) {
        m_player.actionHitboxActive = true;
        return;
    }
    if (event.type == EventType::HitboxOff) {
        m_player.actionHitboxActive = false;
        return;
    }

    const bool dashCommit = event.type == EventType::Vfx
        && ((event.id == EventId::KnightDash && event.value == 1)
            || event.id == EventId::MageDash);
    const bool gameplayContact = event.type == EventType::Projectile
        || dashCommit
        || (event.type == EventType::Vfx
            && event.id != EventId::KnightDash
            && event.id != EventId::MageDash);
    if (!gameplayContact || m_player.actionContactTriggered) return;
    m_player.actionContactTriggered = true;
    ++m_player.contactSerial;
    ResolvePlayerActionContact();
}

void SurvivalController::UpdatePlayerAnimationGraph() {
    const auto local = Animation::WorldToLocalVelocity(
        m_player.velocity.x, m_player.velocity.z,
        m_player.facing.x, m_player.facing.z);
    const auto weights = Animation::SolveLocomotionBlend(
        local, std::max(0.1f, m_player.moveSpeed));
    m_player.locomotionBlend = {weights.idle, weights.forward, weights.backward,
                                weights.left, weights.right,
                                weights.normalizedSpeed};

    if (m_player.combatAction == PlayerCombatAction::None
        && m_player.animation != PlayerAnimation::Hurt
        && m_player.animation != PlayerAnimation::Death) {
        const Animation::State target = weights.normalizedSpeed > 0.02f
            ? Animation::State::Locomotion : Animation::State::Idle;
        if (target != m_playerAnimationGraph.Current()) {
            m_playerAnimationGraph.Submit({target,
                                           Animation::TransitionReason::Locomotion,
                                           ++m_animationGraphSerial}, 1.0f);
        }
    }

    const bool mage = m_player.character == CharacterId::MagicCaster;
    const bool cast = mage
        && m_player.combatAction != PlayerCombatAction::None
        && m_player.combatAction != PlayerCombatAction::Dash;
    const float castLayerWeight = cast
        ? (weights.normalizedSpeed > 0.02f ? 0.84f : 1.0f) : 0.0f;
    const auto layer = Animation::ResolveUpperBodyLayer({
        m_playerAnimationGraph.Current(),
        mage && m_player.combatAction == PlayerCombatAction::None,
        cast,
        0.72f,
        castLayerWeight
    });
    m_player.upperBodyMask = layer.mask;
    m_player.upperBodyLayerWeight = layer.weight;
}

void SurvivalController::ApplyPlayerRootMotion(float previousProgress,
                                                float currentProgress) {
    if (m_player.combatAction == PlayerCombatAction::None) return;
    const Animation::ClipLibrary& library =
        m_player.character == CharacterId::Knight
            ? m_knightClipLibrary : m_mageClipLibrary;
    const char* clipId = nullptr;
    if (m_player.combatAction == PlayerCombatAction::Dash)
        clipId = m_player.character == CharacterId::Knight
            ? "knight.dash" : "mage.dash";
    else if (m_player.combatAction == PlayerCombatAction::SkillTwo
             && m_player.character == CharacterId::Knight)
        clipId = "knight.skill_two";
    if (clipId == nullptr) return;
    const Animation::ClipDefinition* clip = library.Find(clipId);
    if (clip == nullptr) return;
    const auto delta = Animation::SampleRootMotionDelta(
        clip->rootMotion, previousProgress, currentProgress,
        m_playerActionFacing.x, m_playerActionFacing.z);
    if (std::abs(delta.worldX) < 0.0001f
        && std::abs(delta.worldZ) < 0.0001f) return;

    const Vector3 previous = m_player.position;
    Vector3 desired{previous.x + delta.worldX, previous.y,
                    previous.z + delta.worldZ};
    desired = ClampToArena(desired, kArenaHalfExtent, 0.45f);
    desired.y = previous.y;
    desired = delta.requiresCollisionSweep
        ? StopBeforeArenaPillar(previous, desired, 0.45f)
        : ResolveArenaPillars(desired, 0.45f);
    m_player.position = desired;

    if (m_player.character != CharacterId::Knight
        || m_player.combatAction != PlayerCombatAction::SkillTwo
        || !m_player.actionHitboxActive) return;
    const float width = 1.1f * m_player.areaMultiplier
        * (GetUpgradeStack(UpgradeId::Juggernaut) > 0 ? 1.35f : 1.0f);
    for (std::size_t i = 0; i < m_enemies.size(); ++i) {
        EnemyState& enemy = m_enemies[i];
        if (!enemy.active || enemy.dying
            || enemy.lastPlayerActionHitSerial == m_player.actionSerial
            || DistancePointToSegmentXZ(enemy.position, previous, desired)
                > width + CollisionRadius(enemy)) continue;
        enemy.lastPlayerActionHitSerial = m_player.actionSerial;
        DamageEnemy(i, 32.0f * m_player.damageMultiplier,
                    {m_playerActionFacing.x * 4.0f, 0.0f,
                     m_playerActionFacing.z * 4.0f});
    }
}

void SurvivalController::UpdatePlayerRuntimeIk(float dt) {
    const Vector3 forward = NormalizeXZ(m_player.facing);
    const Vector3 right{forward.z, 0.0f, -forward.x};
    const float stride = m_player.animation == PlayerAnimation::Run
        ? std::sin(m_player.animationTime * 8.72664626f) * 0.22f : 0.0f;
    const Vector3 leftFoot{
        m_player.position.x - right.x * 0.17f + forward.x * stride,
        m_player.position.y + 0.04f,
        m_player.position.z - right.z * 0.17f + forward.z * stride};
    const Vector3 rightFoot{
        m_player.position.x + right.x * 0.17f - forward.x * stride,
        m_player.position.y + 0.04f,
        m_player.position.z + right.z * 0.17f - forward.z * stride};
    RuntimeIK::FootIkSettings settings;
    RuntimeIK::FootIkInput input;
    input.pelvisPosition = IkVector({m_player.position.x,
                                     m_player.position.y + 0.92f,
                                     m_player.position.z});
    input.leftAnimatedFoot = IkVector(leftFoot);
    input.rightAnimatedFoot = IkVector(rightFoot);
    input.deltaSeconds = dt;
    input.weight = m_player.animation == PlayerAnimation::Death
        || !m_player.grounded ? 0.0f : 1.0f;
    const auto feet = RuntimeIK::SolveFeetAndPelvis(
        settings, input, &ProbeArenaFloor, nullptr, m_playerFootIkState);

    RuntimeIK::AimInput aimInput;
    aimInput.origin = IkVector({m_player.position.x,
                                m_player.position.y + 1.28f,
                                m_player.position.z});
    aimInput.characterForward = IkVector(m_player.facing);
    aimInput.desiredWorldPoint = IkVector({m_aimPoint.x, 1.05f, m_aimPoint.z});
    aimInput.deltaSeconds = dt;
    RuntimeIK::AimSettings aimSettings;
    const auto aim = RuntimeIK::SolveAimTarget(
        aimSettings, aimInput, m_playerAimIkState);

    RuntimeIK::HandTargetSettings handSettings;
    if (m_player.character == CharacterId::Knight) {
        handSettings.mainHandForward = 0.42f;
        handSettings.supportHandForward = 0.66f;
        handSettings.supportHandSide = 0.14f;
    } else {
        handSettings.mainHandHeight = 0.04f;
        handSettings.supportHandHeight = 0.10f;
        handSettings.weaponAimDistance = 2.4f;
    }
    RuntimeIK::HandTargetInput handInput;
    handInput.chestPosition = IkVector({m_player.position.x,
                                        m_player.position.y + 1.34f,
                                        m_player.position.z});
    handInput.aimDirection = aim.direction;
    const auto hands = RuntimeIK::BuildHandTargets(handSettings, handInput);

    m_player.runtimeIk.aimTarget = RayVector(aim.target);
    m_player.runtimeIk.mainHandTarget = RayVector(hands.mainHand);
    m_player.runtimeIk.supportHandTarget = RayVector(hands.supportHand);
    m_player.runtimeIk.mainElbowPole = RayVector(hands.mainElbowPole);
    m_player.runtimeIk.supportElbowPole = RayVector(hands.supportElbowPole);
    m_player.runtimeIk.leftFootTarget = RayVector(feet.left.target);
    m_player.runtimeIk.rightFootTarget = RayVector(feet.right.target);
    m_player.runtimeIk.leftFootNormal = RayVector(feet.left.normal);
    m_player.runtimeIk.rightFootNormal = RayVector(feet.right.normal);
    m_player.runtimeIk.pelvisOffset = feet.pelvisOffset;
    m_player.runtimeIk.aimYawRadians = aim.yawRadians;
    m_player.runtimeIk.aimPitchRadians = aim.pitchRadians;
    m_player.runtimeIk.leftFootWeight = feet.left.weight;
    m_player.runtimeIk.rightFootWeight = feet.right.weight;
}

void SurvivalController::ResolvePlayerActionContact() {
    const Vector3 facing = m_playerActionFacing;
    switch (m_player.combatAction) {
        case PlayerCombatAction::Basic: {
            if (m_player.character == CharacterId::MagicCaster) {
                m_player.facing = facing;
                SpawnArcBolt();
                m_attackFxTimer = 0.28f;
                EmitCombatFeedback(CombatCue::MagicBoltRelease, m_player.position, facing,
                                   0.75f * m_player.projectileScale, 0.42f,
                                   0.0f, 0.06f);
                SoundManager::GetInstance().PlaySoundAt(
                    "survival_mage_arc_bolt", m_player.position,
                    m_player.position, 18.0f);
                break;
            }

            m_attackFxTimer = 0.30f;
            const Vector3 center{
                m_player.position.x + facing.x * 1.35f,
                0.0f,
                m_player.position.z + facing.z * 1.35f
            };
            const float comboDamage[] = {1.0f, 1.1f, 1.6f};
            const std::size_t comboIndex = std::min<std::size_t>(
                m_player.comboStep, std::size(comboDamage) - 1u);
            const float arcDot = comboIndex == 2u ? -0.18f : -0.25f;
            int hitCount = 0;
            for (std::size_t i = 0; i < m_enemies.size(); ++i) {
                if (!m_enemies[i].active || m_enemies[i].dying) continue;
                const Vector3 relative{
                    m_enemies[i].position.x - m_player.position.x, 0.0f,
                    m_enemies[i].position.z - m_player.position.z
                };
                const Vector3 normal = NormalizeXZ(relative);
                if (DistanceXZ(center, m_enemies[i].position)
                    <= 2.1f * m_player.areaMultiplier + CollisionRadius(m_enemies[i])
                    && DotXZ(normal, facing) > arcDot
                    && HasArenaLineOfSight(m_player.position, m_enemies[i].position)) {
                    DamageEnemy(i, 24.0f * comboDamage[comboIndex]
                                   * m_player.damageMultiplier,
                                {facing.x * 2.5f, 0.0f, facing.z * 2.5f});
                    ++hitCount;
                }
            }
            EmitCombatFeedback(CombatCue::KnightSlash, center, facing,
                               2.1f * m_player.areaMultiplier,
                               hitCount > 0 ? 0.78f : 0.48f,
                               hitCount > 0 ? 0.045f : 0.0f,
                               hitCount > 0 ? 0.10f : 0.05f);
            SoundManager::GetInstance().PlaySoundAt(
                "survival_knight_violet_edge", center,
                m_player.position, 18.0f);
            break;
        }

        case PlayerCombatAction::SkillOne: {
            if (m_player.character == CharacterId::Knight) {
                m_player.guardTimer = m_player.royalBulwark ? 1.0f : 0.65f;
                m_skillFxCenter = m_player.position;
                m_skillFxRadius = 1.5f;
                m_skillFxTimer = m_player.guardTimer;
                EmitCombatFeedback(CombatCue::KnightGuard, m_player.position, facing,
                                   m_skillFxRadius, 0.60f, 0.0f, 0.08f);
                SoundManager::GetInstance().PlaySoundAt(
                    "survival_knight_aegis_counter", m_player.position,
                    m_player.position, 18.0f);
                break;
            }

            m_skillFxCenter = m_player.position;
            m_skillFxRadius = 4.0f * m_player.areaMultiplier;
            m_skillFxTimer = 0.80f;
            int hitCount = 0;
            for (std::size_t i = 0; i < m_enemies.size(); ++i) {
                if (!m_enemies[i].active || m_enemies[i].dying
                    || DistanceXZ(m_player.position, m_enemies[i].position)
                        > m_skillFxRadius + CollisionRadius(m_enemies[i])
                    || !HasArenaLineOfSight(m_player.position, m_enemies[i].position)) continue;
                m_enemies[i].slowTimer = 3.0f;
                DamageEnemy(i, 22.0f * m_player.damageMultiplier, {});
                ++hitCount;
            }
            EmitCombatFeedback(CombatCue::MagicFrostNova, m_player.position, facing,
                               m_skillFxRadius, hitCount > 0 ? 0.82f : 0.62f,
                               hitCount > 0 ? 0.035f : 0.0f, 0.12f);
            SoundManager::GetInstance().PlaySoundAt(
                "survival_mage_frost_ring", m_player.position,
                m_player.position, 22.0f);
            break;
        }

        case PlayerCombatAction::SkillTwo: {
            if (m_player.character == CharacterId::MagicCaster) {
                Vector3 target = ClampToArena(m_playerActionAim, kArenaHalfExtent, 1.0f);
                target = StopBeforeArenaPillar(m_player.position, target, 0.25f);
                m_gravityWellCenter = target;
                m_gravityWellTimer = 4.0f;
                m_skillFxCenter = target;
                m_skillFxRadius = 5.0f * m_player.areaMultiplier;
                m_skillFxTimer = 1.0f;
                EmitCombatFeedback(CombatCue::MagicGravityWell, target, facing,
                                   m_skillFxRadius, 0.86f, 0.025f, 0.20f);
                SoundManager::GetInstance().PlaySoundAt(
                    "survival_mage_gravity_well", target,
                    m_player.position, 24.0f);
                break;
            }

            // Travel is sampled incrementally from the authored Rush root
            // curve. Damage uses the swept path already covered, never a
            // second code-side teleport.
            const Vector3 start = m_playerActionStart;
            const Vector3 end = m_player.position;
            const float rushWidth = 1.1f * m_player.areaMultiplier
                                  * (GetUpgradeStack(UpgradeId::Juggernaut) > 0
                                      ? 1.35f : 1.0f);
            int hitCount = 0;
            for (std::size_t i = 0; i < m_enemies.size(); ++i) {
                if (!m_enemies[i].active || m_enemies[i].dying) continue;
                if (m_enemies[i].lastPlayerActionHitSerial
                    == m_player.actionSerial) continue;
                if (DistancePointToSegmentXZ(m_enemies[i].position, start, end)
                    <= rushWidth + CollisionRadius(m_enemies[i])) {
                    m_enemies[i].lastPlayerActionHitSerial = m_player.actionSerial;
                    DamageEnemy(i, 32.0f * m_player.damageMultiplier,
                                {facing.x * 4.0f, 0.0f, facing.z * 4.0f});
                    ++hitCount;
                }
            }
            m_player.rushTimer = 0.45f;
            m_player.invulnerableTimer = std::max(m_player.invulnerableTimer, 0.22f);
            m_skillFxCenter = end;
            m_skillFxRadius = 2.0f;
            m_skillFxTimer = 0.55f;
            EmitCombatFeedback(CombatCue::KnightRush, end, facing,
                               rushWidth, hitCount > 0 ? 0.95f : 0.70f,
                               hitCount > 0 ? 0.065f : 0.0f, 0.16f);
            SoundManager::GetInstance().PlaySoundAt(
                "survival_knight_shield_rush", end,
                m_player.position, 22.0f);
            break;
        }

        case PlayerCombatAction::Ultimate: {
            const bool knight = m_player.character == CharacterId::Knight;
            m_skillFxCenter = m_player.position;
            m_skillFxRadius = (knight ? 5.5f : 8.0f) * m_player.areaMultiplier;
            m_skillFxTimer = knight ? 1.15f : 1.30f;
            const float damage = (knight ? 85.0f : 65.0f) * m_player.damageMultiplier;
            int hitCount = 0;
            for (std::size_t i = 0; i < m_enemies.size(); ++i) {
                if (!m_enemies[i].active || m_enemies[i].dying
                    || DistanceXZ(m_player.position, m_enemies[i].position)
                        > m_skillFxRadius + CollisionRadius(m_enemies[i])
                    || !HasArenaLineOfSight(m_player.position, m_enemies[i].position)) continue;
                DamageEnemy(i, damage,
                            NormalizeXZ({m_enemies[i].position.x - m_player.position.x,
                                         0.0f,
                                         m_enemies[i].position.z - m_player.position.z}));
                ++hitCount;
            }
            EmitCombatFeedback(knight ? CombatCue::KnightUltimate
                                      : CombatCue::MagicUltimate,
                               m_player.position, facing, m_skillFxRadius,
                               hitCount > 0 ? 1.0f : 0.86f,
                               hitCount > 0 ? 0.085f : 0.035f,
                               knight ? 0.34f : 0.38f);
            SoundManager::GetInstance().PlaySoundAt(
                knight ? "survival_knight_bastion_breaker"
                       : "survival_mage_astral_tempest",
                m_player.position, m_player.position, 30.0f);
            break;
        }

        case PlayerCombatAction::Dash: {
            const Vector3 start = m_playerActionStart;
            m_player.invulnerableTimer = std::max(m_player.invulnerableTimer, 0.14f);
            m_attackFxTimer = 0.24f;
            EmitCombatFeedback(CombatCue::DashBurst, start, facing,
                               DistanceXZ(start, m_player.position), 0.62f,
                               0.0f, 0.07f);
            SoundManager::GetInstance().PlaySoundAt(
                m_player.character == CharacterId::Knight
                    ? "survival_knight_steel_step"
                    : "survival_mage_phase_blink",
                start, m_player.position, 18.0f);
            break;
        }

        case PlayerCombatAction::None: break;
    }
}

void SurvivalController::CancelPlayerAction() {
    m_player.combatAction = PlayerCombatAction::None;
    m_player.actionContactNormalized = 0.0f;
    m_player.actionContactTriggered = false;
    m_player.actionHitboxActive = false;
    m_playerEventTrackActive = false;
    m_playerEventCursor.Stop();
    m_previousActionProgress = 0.0f;
}

void SurvivalController::EmitCombatFeedback(CombatCue cue, Vector3 origin,
                                            Vector3 direction, float radius,
                                            float intensity,
                                            float hitStopSeconds,
                                            float cameraShakeSeconds) {
    m_combatFeedback.cue = cue;
    ++m_combatFeedback.serial;
    m_combatFeedback.origin = origin;
    m_combatFeedback.direction = NormalizeXZ(direction);
    if (LengthXZ(m_combatFeedback.direction) <= 0.1f)
        m_combatFeedback.direction = {0.0f, 0.0f, 1.0f};
    m_combatFeedback.radius = std::max(0.0f, radius);
    m_combatFeedback.intensity = std::clamp(intensity, 0.0f, 1.0f);
    m_hitStopTimer = std::max(m_hitStopTimer, std::max(0.0f, hitStopSeconds));
    if (!m_reducedMotion) {
        m_cameraShakeTimer = std::max(m_cameraShakeTimer,
                                      std::max(0.0f, cameraShakeSeconds));
        m_cameraShakeIntensity = std::max(m_cameraShakeIntensity,
                                          m_combatFeedback.intensity);
    }
}

void SurvivalController::UpdateProjectiles(float dt) {
    for (std::size_t i = 0; i < m_projectiles.size(); ++i) {
        ProjectileState& projectile = m_projectiles[i];
        if (!projectile.active) continue;
        if (projectile.hostile && m_player.eventHorizon && m_gravityWellTimer > 0.0f
            && DistanceXZ(projectile.position, m_gravityWellCenter) <= 5.5f) {
            ReleaseProjectile(i);
            continue;
        }
        const Vector3 previousPosition = projectile.position;
        const Vector3 nextPosition{
            projectile.position.x + projectile.velocity.x * dt,
            projectile.position.y + projectile.velocity.y * dt,
            projectile.position.z + projectile.velocity.z * dt
        };
        if (SegmentIntersectsArenaPillar(previousPosition, nextPosition, projectile.radius)) {
            ReleaseProjectile(i);
            continue;
        }
        projectile.position = nextPosition;
        projectile.lifetime -= dt;
        if (projectile.lifetime <= 0.0f ||
            std::abs(projectile.position.x) > kArenaHalfExtent ||
            std::abs(projectile.position.z) > kArenaHalfExtent) {
            ReleaseProjectile(i);
            continue;
        }

        if (projectile.hostile) {
            const float hitRadius = projectile.radius + 0.45f;
            if (DistanceXZ(projectile.position, m_player.position) <= hitRadius) {
                DamagePlayer(projectile.damage);
                ReleaseProjectile(i);
                if (m_phase == Phase::RunFailed) return;
            }
            continue;
        }

        bool hit = false;
        Vector3 hitOrigin{};
        float hitRadius = 0.0f;
        Vector3 hitDirection{};
        for (std::size_t enemyIndex = 0; enemyIndex < m_enemies.size(); ++enemyIndex) {
            if (!m_enemies[enemyIndex].active || m_enemies[enemyIndex].dying) continue;
            const float enemyRadius = CollisionRadius(m_enemies[enemyIndex]);
            if (DistanceXZ(projectile.position, m_enemies[enemyIndex].position) > projectile.radius + enemyRadius) continue;

            const Vector3 impact = projectile.position;
            const float damage = projectile.damage;
            const float splashRadius = projectile.splashRadius;
            hitOrigin = impact;
            hitRadius = std::max(projectile.radius * 2.4f, splashRadius);
            hitDirection = NormalizeXZ(projectile.velocity);
            for (std::size_t splashIndex = 0; splashIndex < m_enemies.size(); ++splashIndex) {
                if (!m_enemies[splashIndex].active || m_enemies[splashIndex].dying
                    || DistanceXZ(impact, m_enemies[splashIndex].position)
                        > splashRadius + CollisionRadius(m_enemies[splashIndex])) continue;
                DamageEnemy(splashIndex, damage, NormalizeXZ(projectile.velocity));
            }
            hit = true;
            break;
        }
        if (hit) {
            EmitCombatFeedback(CombatCue::MagicProjectileImpact, hitOrigin, hitDirection,
                               hitRadius, 0.58f, 0.025f, 0.10f);
            if (projectile.remainingPierce > 0) {
                --projectile.remainingPierce;
                const Vector3 direction = Vector3Normalize(projectile.velocity);
                projectile.position = Vector3Add(projectile.position, Vector3Scale(direction, 0.9f));
            } else {
                ReleaseProjectile(i);
            }
        }
    }
}

void SurvivalController::DamagePlayer(float damage) {
    if (m_player.invulnerableTimer > 0.0f || m_phase == Phase::RunFailed) return;
    const float guardMultiplier = m_player.guardTimer > 0.0f ? 0.20f : 1.0f;
    float incoming = std::max(0.0f, damage) * guardMultiplier
                   * (1.0f - m_player.damageReduction);
    // Preserve reaction time at late waves: one resolved hit can be severe,
    // but never deletes a full-health build before the player can respond.
    incoming = std::min(incoming, m_player.maxHp * 0.35f);
    if (m_player.shield > 0.0f) {
        const float absorbed = std::min(m_player.shield, incoming);
        m_player.shield -= absorbed;
        incoming -= absorbed;
    }
    m_player.hp -= incoming;
    m_damageTaken += static_cast<int>(std::round(incoming));
    m_player.invulnerableTimer = 0.72f;
    SetPlayerAnimation(PlayerAnimation::Hurt, 0.32f);
    if (m_player.guardTimer > 0.0f)
        m_player.ultimateCharge = std::min(100.0f, m_player.ultimateCharge + 18.0f);
    SoundManager::GetInstance().PlaySound("player_hurt");
    if (GetUpgradeStack(UpgradeId::EmergencyBarrier) > 0
        && m_player.hp > 0.0f && m_player.hp <= m_player.maxHp * 0.30f
        && m_player.barrierCooldown <= 0.0f) {
        m_player.shield = std::max(m_player.shield, m_player.maxHp * 0.20f);
        m_player.barrierCooldown = 30.0f;
        SoundManager::GetInstance().PlaySound("checkpoint_activate");
    }
    if (m_player.hp <= 0.0f && m_player.secondWindAvailable) {
        m_player.secondWindAvailable = false;
        m_player.hp = m_player.maxHp * 0.35f;
        m_player.invulnerableTimer = 1.5f;
        SoundManager::GetInstance().PlaySound("checkpoint_activate");
        return;
    }
    if (m_player.hp <= 0.0f) {
        m_player.hp = 0.0f;
        SetPlayerAnimation(PlayerAnimation::Death, -1.0f);
        SetPhase(Phase::RunFailed);
        SoundManager::GetInstance().PlaySound("player_die");
    }
}

void SurvivalController::SetPlayerAnimation(PlayerAnimation animation, float duration) {
    if (animation == PlayerAnimation::Hurt || animation == PlayerAnimation::Death) {
        const auto reason = animation == PlayerAnimation::Death
            ? Animation::TransitionReason::Death
            : Animation::TransitionReason::HitReaction;
        m_playerAnimationGraph.Submit({GraphState(animation), reason,
                                       ++m_animationGraphSerial},
                                      m_player.animationDuration > 0.0f
                                          ? std::clamp(m_player.animationTime
                                                       / m_player.animationDuration,
                                                       0.0f, 1.0f)
                                          : 1.0f);
        CancelPlayerAction();
        m_playerCombo.Cancel();
        m_player.comboStep = 0;
    }
    if (animation != m_player.animation) {
        m_player.previousAnimation = m_player.animation;
        m_player.previousAnimationTime = m_player.animationTime;
        m_player.animationBlend = 0.0f;
    }
    m_player.animation = animation;
    m_player.animationTime = 0.0f;
    m_player.animationDuration = duration;
}

void SurvivalController::DamageEnemy(std::size_t index, float damage, Vector3 knockback) {
    if (index >= m_enemies.size() || !m_enemies[index].active) return;
    EnemyState& enemy = m_enemies[index];
    if (enemy.dying) return;
    float finalDamage = std::max(0.0f, damage);
    if (enemy.hp > 0.0f && enemy.hp <= enemy.maxHp * 0.25f)
        finalDamage *= 1.0f + m_player.executionBonus;
    enemy.hp -= finalDamage;
    enemy.hitFlash = 1.0f;
    enemy.position.x += knockback.x * 0.08f;
    enemy.position.z += knockback.z * 0.08f;
    m_player.ultimateCharge = std::min(100.0f, m_player.ultimateCharge + finalDamage * 0.22f);
    SoundManager::GetInstance().PlaySound("enemy_hurt");
    if (finalDamage > 0.5f && GetUpgradeStack(UpgradeId::EmberEdge) > 0
        && NextRandom01() < 0.20f) {
        enemy.burnTimer = 3.0f;
        enemy.burnDps = 4.0f * m_player.damageMultiplier;
    }
    if (!m_chainResolving && finalDamage > 0.5f && GetUpgradeStack(UpgradeId::ChainSpark) > 0) {
        ++m_chainHitCounter;
        if (m_chainHitCounter >= 6) {
            m_chainHitCounter = 0;
            m_chainResolving = true;
            int chained = 0;
            for (std::size_t otherIndex = 0; otherIndex < m_enemies.size() && chained < 3; ++otherIndex) {
                if (otherIndex == index || !m_enemies[otherIndex].active
                    || m_enemies[otherIndex].dying) continue;
                if (DistanceXZ(enemy.position, m_enemies[otherIndex].position) > 6.0f) continue;
                DamageEnemy(otherIndex, 14.0f * m_player.damageMultiplier, {});
                ++chained;
            }
            m_chainResolving = false;
        }
    }
    if (enemy.hp <= 0.0f) {
        if (enemy.archetype == EnemyArchetype::VoidSovereign && enemy.bossPhase < 3) {
            ++enemy.bossPhase;
            const float phaseBaseHp = enemy.bossPhase == 2 ? 6000.0f : 8000.0f;
            enemy.hp = enemy.maxHp = phaseBaseHp * HpScale();
            enemy.scale = enemy.bossPhase == 2 ? 4.8f : 7.5f;
            enemy.moveSpeed = enemy.bossPhase == 2 ? 4.1f : 5.2f;
            enemy.action = EnemyAction::None;
            enemy.attackCooldown = 1.2f;
            enemy.specialCooldown = 3.5f;
            enemy.phaseTransitionTimer = 1.8f;
            SoundManager::GetInstance().PlaySound("boss_phase");
            return;
        }
        const bool boss = IsBoss(enemy.archetype);
        ++m_kills;
        if (boss) ++m_bossesKilled;
        m_score += boss ? 10000 * (m_wave / 10) : (int)std::round(10.0f * (1.0f + 0.04f * (m_wave - 1)));
        enemy.hp = 0.0f;
        enemy.dying = true;
        enemy.deathTimer = boss ? 1.55f : 0.95f;
        enemy.action = EnemyAction::None;
        enemy.velocity = {};
        SoundManager::GetInstance().PlaySound(boss ? "boss_phase" : "enemy_death");
    }
}

void SurvivalController::UpdateCursorCapture() {
    const bool gameplayPhase = m_phase == Phase::PreWave
        || m_phase == Phase::Combat || m_phase == Phase::WaveClear;
    const bool shouldCapture = gameplayPhase && !m_paused && !m_showRecords;
    if (shouldCapture == m_mouseCaptured || !IsWindowReady()) return;
    if (shouldCapture) DisableCursor();
    else EnableCursor();
    m_mouseCaptured = shouldCapture;
}

std::size_t SurvivalController::FindCameraTarget(bool hardLock) const {
    const Vector3 from{m_player.position.x, 0.8f, m_player.position.z};
    Vector3 view = NormalizeXZ({m_camera.target.x - m_camera.position.x,
                                0.0f,
                                m_camera.target.z - m_camera.position.z});
    if (LengthXZ(view) < 0.01f) view = m_player.facing;

    std::size_t best = kEnemyCapacity;
    float bestScore = std::numeric_limits<float>::max();
    for (std::size_t index = 0; index < m_enemies.size(); ++index) {
        const EnemyState& enemy = m_enemies[index];
        if (!enemy.active || enemy.dying) continue;
        const float distance = DistanceXZ(from, enemy.position);
        if (distance > (hardLock ? 31.0f : 15.0f)) continue;
        if (!HasArenaLineOfSight(from, enemy.position)) continue;
        const Vector3 direction = NormalizeXZ({enemy.position.x - from.x, 0.0f,
                                               enemy.position.z - from.z});
        const float facing = DotXZ(view, direction);
        if (facing < (hardLock ? -0.18f : 0.60f)) continue;
        float score = distance + (1.0f - facing) * (hardLock ? 7.0f : 15.0f);
        if (IsBoss(enemy.archetype)) score -= hardLock ? 6.0f : 2.0f;
        if (score < bestScore) {
            bestScore = score;
            best = index;
        }
    }
    return best;
}

bool SurvivalController::GetLockedTargetPosition(Vector3& position) const {
    if (!m_targetLock || m_lockedEnemyIndex >= m_enemies.size()) return false;
    const EnemyState& enemy = m_enemies[m_lockedEnemyIndex];
    if (!enemy.active || enemy.dying) return false;
    position = enemy.position;
    return true;
}

Vector3 SurvivalController::ResolveCameraCollision(Vector3 target,
                                                   Vector3 desired) const {
    const Vector3 travel{desired.x - target.x, desired.y - target.y,
                         desired.z - target.z};
    float fraction = 1.0f;
    constexpr float kWallLimit = kArenaHalfExtent - 0.42f;
    const auto limitAxis = [&fraction](float start, float delta, float limit) {
        if (std::abs(delta) < 0.0001f) return;
        const float end = start + delta;
        if (end > limit) fraction = std::min(fraction, (limit - start) / delta);
        else if (end < -limit) fraction = std::min(fraction, (-limit - start) / delta);
    };
    limitAxis(target.x, travel.x, kWallLimit);
    limitAxis(target.z, travel.z, kWallLimit);
    fraction = std::clamp(fraction, 0.08f, 1.0f);
    Vector3 resolved{target.x + travel.x * fraction,
                     target.y + travel.y * fraction,
                     target.z + travel.z * fraction};
    resolved = StopBeforeArenaPillar(target, resolved, 0.38f);
    resolved.y = std::clamp(resolved.y, 1.65f, 13.0f);
    return resolved;
}

void SurvivalController::UpdateCamera(float frameDt) {
    if (m_phase == Phase::CharacterSelect) return;

    Vector3 playerFocus{m_player.position.x,
                        m_player.position.y + 1.12f,
                        m_player.position.z};
    Vector3 desiredTarget = playerFocus;
    float desiredDistance = m_cameraUserDistance;
    bool locked = false;
    if (m_targetLock && m_lockedEnemyIndex < m_enemies.size()) {
        const EnemyState& enemy = m_enemies[m_lockedEnemyIndex];
        if (enemy.active && !enemy.dying) {
            locked = true;
            const Vector3 enemyFocus{enemy.position.x,
                                     IsBoss(enemy.archetype) ? 2.1f : 1.05f,
                                     enemy.position.z};
            const float separation = DistanceXZ(playerFocus, enemyFocus);
            const float focusWeight = std::clamp(0.24f + separation * 0.006f,
                                                 0.24f, 0.38f);
            desiredTarget = Vector3Lerp(playerFocus, enemyFocus, focusWeight);
            desiredDistance += std::clamp(separation * 0.12f, 0.45f, 3.0f);
            if (IsBoss(enemy.archetype)) desiredDistance += 0.75f;
            const Vector3 toEnemy = NormalizeXZ({enemy.position.x - m_player.position.x,
                                                  0.0f,
                                                  enemy.position.z - m_player.position.z});
            const float lockYaw = std::atan2(-toEnemy.x, -toEnemy.z);
            const float yawBlend = 1.0f - std::exp(-3.4f * frameDt);
            m_cameraYaw += ShortestAngleDelta(m_cameraYaw, lockYaw) * yawBlend;
        }
    }
    if (!locked && m_player.combatAction != PlayerCombatAction::None) {
        const Vector3 actionFocus{m_playerActionAim.x,
                                  m_player.position.y + 1.0f,
                                  m_playerActionAim.z};
        desiredTarget = Vector3Lerp(playerFocus, actionFocus, 0.10f);
    }
    if (m_player.combatAction == PlayerCombatAction::Ultimate)
        desiredDistance += 1.35f;

    desiredDistance = std::clamp(desiredDistance, 2.15f, 17.5f);
    const float distanceBlend = 1.0f - std::exp(-5.5f * frameDt);
    m_cameraDistance += (desiredDistance - m_cameraDistance) * distanceBlend;
    const float horizontal = std::cos(m_cameraPitch) * m_cameraDistance;
    Vector3 desiredPosition{
        desiredTarget.x + std::sin(m_cameraYaw) * horizontal,
        desiredTarget.y + std::sin(m_cameraPitch) * m_cameraDistance,
        desiredTarget.z + std::cos(m_cameraYaw) * horizontal
    };
    desiredPosition = ResolveCameraCollision(playerFocus, desiredPosition);

    const float targetBlend = 1.0f - std::exp(-10.0f * frameDt);
    const float positionBlend = 1.0f - std::exp(-12.0f * frameDt);
    m_camera.target = Vector3Lerp(m_camera.target, desiredTarget, targetBlend);
    m_camera.position = Vector3Lerp(m_camera.position, desiredPosition,
                                    positionBlend);
    const float desiredFovy = locked ? 55.0f
        : (m_player.combatAction == PlayerCombatAction::Ultimate ? 57.0f
            : (m_cameraUserDistance < 3.2f ? 56.0f : 52.0f));
    m_camera.fovy += (desiredFovy - m_camera.fovy) * distanceBlend;
}

float SurvivalController::NextRandom01() {
    m_rngState ^= m_rngState << 13;
    m_rngState ^= m_rngState >> 17;
    m_rngState ^= m_rngState << 5;
    return (m_rngState & 0x00FFFFFFu) / 16777216.0f;
}

float SurvivalController::NextUpgradeRandom01() {
    m_upgradeRngState ^= m_upgradeRngState << 13;
    m_upgradeRngState ^= m_upgradeRngState >> 17;
    m_upgradeRngState ^= m_upgradeRngState << 5;
    return (m_upgradeRngState & 0x00FFFFFFu) / 16777216.0f;
}

float SurvivalController::HpScale() const {
    const float t = (float)(m_wave - 1);
    return m_balance.enemyHpMultiplier
        * (1.0f + m_balance.hpLinear * t + m_balance.hpQuadratic * t * t);
}

float SurvivalController::DamageScale() const {
    const float t = (float)(m_wave - 1);
    return m_balance.enemyDamageMultiplier
        * (1.0f + m_balance.damageLinear * t + m_balance.damageQuadratic * t * t);
}

float SurvivalController::Cooldown(float baseSeconds) const {
    return std::max(0.08f, baseSeconds * m_player.cooldownMultiplier);
}

void SurvivalController::SetPhase(Phase phase, float timer) {
    const bool enteringResult = (phase == Phase::RunFailed || phase == Phase::RunVictory)
        && m_phase != Phase::RunFailed && m_phase != Phase::RunVictory;
    m_phase = phase;
    m_phaseTimer = timer;
    if (enteringResult) m_selectedResultAction = 0;
    if (enteringResult) FinalizeRun(phase == Phase::RunVictory);
}

void SurvivalController::FinalizeRun(bool victory) {
    if (m_runRecorded || m_runId.empty()) return;
    RunResultInput result;
    result.runId = m_runId;
    result.character = m_player.character;
    result.configVersion = m_balanceVersion;
    result.highestWave = m_wave;
    result.score = m_score;
    result.survivalMs = std::max(0, static_cast<int>(std::round(m_runTime * 1000.0f)));
    result.kills = m_kills;
    result.bossesKilled = m_bossesKilled;
    result.damageTaken = m_damageTaken;
    result.peakEnemies = m_peakEnemies;
    result.peakProjectiles = m_peakProjectiles;
    result.droppedTicks = m_droppedTicks;
    result.averageFrameMs = m_averageFrameMs;
    result.peakFrameMs = m_peakFrameMs;
    result.victory = victory;
    SurvivalRunService::GetInstance().FinalizeRun(result);
    m_runRecorded = true;
}

void SurvivalController::Render() const {
    SurvivalView::GetInstance().Render(*this);
}

void SurvivalController::Shutdown() {
    if (m_initialized && !m_runRecorded && !m_runId.empty()
        && m_phase != Phase::CharacterSelect) {
        FinalizeRun(false);
    }
    m_returnToMenu = false;
    m_paused = false;
    if (m_mouseCaptured && IsWindowReady()) EnableCursor();
    m_mouseCaptured = false;
    auto& save = SaveManager::GetInstance();
    save.SetSurvivalHighContrast(m_highContrast);
    save.SetSurvivalReducedMotion(m_reducedMotion);
    save.SetSurvivalUiScale(m_uiScale);
    for (auto& enemy : m_enemies) enemy.active = false;
    for (auto& projectile : m_projectiles) projectile.active = false;
}

} // namespace Survival3D
