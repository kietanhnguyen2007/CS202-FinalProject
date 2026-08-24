#include "Survival3D/View/SurvivalView.h"
#include "Survival3D/Controller/SurvivalController.h"
#include "Survival3D/Model/SurvivalTypes.h"
#include "Survival3D/Systems/SurvivalRunService.h"
#include "Survival3D/Vfx/SkillVfxPackages.h"
#include "raymath.h"
#include "rlgl.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iterator>
#include <sstream>
#include <vector>

namespace Survival3D {
namespace {

void DrawPanel(Rectangle panel, Color border) {
    DrawRectangleRounded({panel.x + 5.0f, panel.y + 7.0f, panel.width, panel.height},
                         0.08f, 10, Color{0, 0, 0, 130});
    DrawRectangleRounded(panel, 0.08f, 10, Color{15, 10, 29, 238});
    DrawRectangleRoundedLinesEx(panel, 0.08f, 10, 2.0f, border);
}

Vector2 Measure(Font font, const char* text, float size) {
    return MeasureTextEx(font, text, size, 0.6f);
}

float LengthXZ(Vector3 value) {
    return std::sqrt(value.x * value.x + value.z * value.z);
}

bool IsBoss(EnemyArchetype archetype) {
    return archetype == EnemyArchetype::BroodWarden
        || archetype == EnemyArchetype::HexeyeArtillerist
        || archetype == EnemyArchetype::IronrootColossus
        || archetype == EnemyArchetype::EclipseChimera
        || archetype == EnemyArchetype::VoidSovereign
        || archetype == EnemyArchetype::BossPrototype;
}

const char* BossName(EnemyArchetype archetype) {
    if (archetype == EnemyArchetype::BroodWarden) return "BROOD WARDEN";
    if (archetype == EnemyArchetype::HexeyeArtillerist) return "HEXEYE ARTILLERIST";
    if (archetype == EnemyArchetype::IronrootColossus) return "IRONROOT COLOSSUS";
    if (archetype == EnemyArchetype::EclipseChimera) return "ECLIPSE CHIMERA";
    if (archetype == EnemyArchetype::VoidSovereign) return "VOID SOVEREIGN";
    return "RIFT GUARDIAN  -  CONTENT PREVIEW";
}

const char* RarityName(UpgradeRarity rarity) {
    switch (rarity) {
        case UpgradeRarity::Rare: return "RARE";
        case UpgradeRarity::Epic: return "EPIC";
        case UpgradeRarity::Legendary: return "LEGENDARY";
        default: return "COMMON";
    }
}

std::vector<std::string> WrapLines(Font font, const std::string& text,
                                   float fontSize, float maxWidth) {
    std::vector<std::string> lines;
    std::istringstream words(text);
    std::string word;
    std::string line;
    while (words >> word) {
        const std::string candidate = line.empty() ? word : line + " " + word;
        if (!line.empty() && MeasureTextEx(font, candidate.c_str(), fontSize, 0.2f).x > maxWidth) {
            lines.push_back(line);
            line = word;
        } else {
            line = candidate;
        }
    }
    if (!line.empty()) lines.push_back(line);
    return lines;
}

void DrawWrappedCentered(Font font, const std::string& text, Rectangle bounds,
                         float preferredSize, Color color) {
    float size = preferredSize;
    std::vector<std::string> lines;
    for (;;) {
        lines = WrapLines(font, text, size, bounds.width);
        const float totalHeight = lines.size() * size * 1.35f;
        if ((totalHeight <= bounds.height && lines.size() <= 3) || size <= 10.0f) break;
        size -= 1.0f;
    }
    const float lineHeight = size * 1.35f;
    const float startY = bounds.y + (bounds.height - lineHeight * lines.size()) * 0.5f;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        const Vector2 measured = MeasureTextEx(font, lines[i].c_str(), size, 0.2f);
        DrawTextEx(font, lines[i].c_str(),
                   {bounds.x + (bounds.width - measured.x) * 0.5f,
                    startY + i * lineHeight}, size, 0.2f, color);
    }
}

float AnimationProgress(const PlayerState& player) {
    if (player.animationDuration <= 0.0f) return 0.0f;
    return std::clamp(player.animationTime / player.animationDuration, 0.0f, 1.0f);
}

void DrawLowPolyLimb(Vector3 start, Vector3 joint, Vector3 end,
                     float radius, Color color, Color jointColor) {
    DrawCylinderEx(start, joint, radius, radius * 0.88f, 6, color);
    DrawSphere(joint, radius * 1.08f, jointColor);
    DrawCylinderEx(joint, end, radius * 0.88f, radius * 0.72f, 6, color);
}

struct AnimationClip {
    float first;
    float last;
};

constexpr AnimationClip kAnimationClips[] = {
    {0.0f, 94.0f},     // idle
    {96.0f, 158.0f},   // run
    {160.0f, 222.0f},  // basic
    {224.0f, 302.0f},  // skill one
    {304.0f, 382.0f},  // skill two
    {384.0f, 478.0f},  // ultimate / phase
    {480.0f, 526.0f},  // dash / special
    {528.0f, 558.0f},  // hurt
    {560.0f, 654.0f}   // death
};

constexpr std::size_t kHexArcherModelSlot = 3u;
constexpr std::size_t kHexArcherBowSlot = 2u;

enum SkillModelSlot : std::size_t {
    KnightVioletEdge = 0,
    KnightAegisCounter,
    KnightShieldRush,
    KnightBastionBreaker,
    KnightSteelStep,
    MageArcBolt,
    MageFrostRing,
    MageGravityWell,
    MageAstralTempest,
    MagePhaseBlink,
    SkillModelCount
};

Vfx::SkillPackage SkillPackageForCue(CombatCue cue,
                                     CharacterId character,
                                     bool& valid) {
    valid = true;
    switch (cue) {
        case CombatCue::KnightSlash:
            return Vfx::SkillPackage::KnightVioletEdge;
        case CombatCue::KnightGuard:
            return Vfx::SkillPackage::KnightAegisCounter;
        case CombatCue::KnightRush:
            return Vfx::SkillPackage::KnightShieldRush;
        case CombatCue::KnightUltimate:
            return Vfx::SkillPackage::KnightBastionBreaker;
        case CombatCue::MagicBoltRelease:
            return Vfx::SkillPackage::MageArcBolt;
        case CombatCue::MagicFrostNova:
            return Vfx::SkillPackage::MageFrostRing;
        case CombatCue::MagicGravityWell:
            return Vfx::SkillPackage::MageGravityWell;
        case CombatCue::MagicUltimate:
            return Vfx::SkillPackage::MageAstralTempest;
        case CombatCue::DashBurst:
            return character == CharacterId::Knight
                ? Vfx::SkillPackage::KnightSteelStep
                : Vfx::SkillPackage::MagePhaseBlink;
        case CombatCue::MagicProjectileImpact:
        case CombatCue::None:
            valid = false;
            return Vfx::SkillPackage::KnightVioletEdge;
    }
    valid = false;
    return Vfx::SkillPackage::KnightVioletEdge;
}

float SkillCueBaseRadius(CombatCue cue, CharacterId character) {
    switch (cue) {
        case CombatCue::KnightSlash: return 2.10f;
        case CombatCue::KnightGuard: return 1.50f;
        case CombatCue::KnightRush: return 1.10f;
        case CombatCue::KnightUltimate: return 5.50f;
        case CombatCue::MagicBoltRelease: return 0.75f;
        case CombatCue::MagicFrostNova: return 4.00f;
        case CombatCue::MagicGravityWell: return 5.00f;
        case CombatCue::MagicUltimate: return 8.00f;
        case CombatCue::DashBurst:
            return character == CharacterId::Knight ? 3.20f : 3.00f;
        case CombatCue::MagicProjectileImpact:
        case CombatCue::None: return 1.0f;
    }
    return 1.0f;
}

float SkillCueHeight(CombatCue cue) {
    switch (cue) {
        case CombatCue::KnightSlash: return 0.78f;
        case CombatCue::KnightGuard: return 1.02f;
        case CombatCue::MagicBoltRelease: return 1.32f;
        case CombatCue::KnightRush: return 0.30f;
        default: return 0.10f;
    }
}

struct SkillVfxPalette {
    Color primary{};
    Color accent{};
};

SkillVfxPalette PaletteForPackage(Vfx::PackageId package,
                                  bool highContrast) {
    if (highContrast) return {WHITE, YELLOW};
    const auto is = [package](Vfx::SkillPackage skill) {
        return package == Vfx::GetSkillPackage(skill).id;
    };
    if (is(Vfx::SkillPackage::KnightVioletEdge))
        return {Color{220, 124, 255, 255}, Color{255, 230, 255, 255}};
    if (is(Vfx::SkillPackage::KnightAegisCounter))
        return {Color{167, 91, 246, 255}, Color{255, 205, 92, 255}};
    if (is(Vfx::SkillPackage::KnightShieldRush))
        return {Color{194, 113, 255, 255}, Color{255, 177, 70, 255}};
    if (is(Vfx::SkillPackage::KnightBastionBreaker))
        return {Color{204, 113, 255, 255}, Color{255, 225, 116, 255}};
    if (is(Vfx::SkillPackage::KnightSteelStep))
        return {Color{178, 166, 208, 255}, Color{229, 198, 255, 255}};
    if (is(Vfx::SkillPackage::MageArcBolt))
        return {Color{71, 215, 255, 255}, Color{181, 241, 255, 255}};
    if (is(Vfx::SkillPackage::MageFrostRing))
        return {Color{113, 226, 255, 255}, Color{231, 252, 255, 255}};
    if (is(Vfx::SkillPackage::MageGravityWell))
        return {Color{104, 94, 255, 255}, Color{212, 105, 255, 255}};
    if (is(Vfx::SkillPackage::MageAstralTempest))
        return {Color{99, 218, 255, 255}, Color{225, 121, 255, 255}};
    return {Color{87, 213, 255, 255}, Color{180, 101, 255, 255}};
}

// Step 7 exports one independently editable GLB action per gameplay state.
// Keeping the names in C++ makes a missing or renamed production clip fail the
// runtime contract immediately instead of silently sampling a master timeline.
constexpr const char* kActorClipNames[10][16] = {
    {
        "idle", "walk_forward", "walk_backward", "run_forward",
        "run_backward", "strafe_left", "strafe_right", "basic_01",
        "basic_02", "basic_03", "skill_one", "skill_two", "ultimate",
        "dash", "hurt", "death"
    },
    {
        "idle", "walk_forward", "walk_backward", "run_forward",
        "run_backward", "strafe_left", "strafe_right", "basic_01",
        "basic_02", "basic_03", "skill_one", "skill_two", "ultimate",
        "dash", "hurt", "death"
    },
    {
        "idle", "run_forward", "basic_01", "skill_one", "skill_two",
        "ultimate", "special", "hurt", "death"
    },
    {
        "idle", "run_forward", "basic_01", "skill_one", "skill_two",
        "ultimate", "special", "hurt", "death"
    },
    {
        "idle", "run_forward", "basic_01", "skill_one", "skill_two",
        "ultimate", "special", "hurt", "death"
    },
    {
        "idle", "run_forward", "basic_01", "skill_one", "skill_two",
        "ultimate", "special", "hurt", "death"
    },
    {
        "idle", "run_forward", "basic_01", "skill_one", "skill_two",
        "ultimate", "special", "hurt", "death"
    },
    {
        "idle", "run_forward", "basic_01", "skill_one", "skill_two",
        "ultimate", "special", "hurt", "death"
    },
    {
        "idle", "run_forward", "basic_01", "skill_one", "skill_two",
        "ultimate", "special", "hurt", "death"
    },
    {
        "idle", "run_forward", "basic_01", "skill_one", "skill_two",
        "ultimate", "special", "hurt", "death"
    }
};

constexpr int kActorClipCounts[10] = {16, 16, 9, 9, 9, 9, 9, 9, 9, 9};

const char* HeroActionClip(CharacterId character, PlayerAnimation animation,
                           std::uint8_t comboStep) {
    const std::size_t slot = character == CharacterId::Knight ? 0u : 1u;
    switch (animation) {
        case PlayerAnimation::Idle: return kActorClipNames[slot][0];
        case PlayerAnimation::Run: return kActorClipNames[slot][3];
        case PlayerAnimation::BasicAttack:
            return kActorClipNames[slot][7 + std::min<int>(comboStep, 2)];
        case PlayerAnimation::SkillOne: return kActorClipNames[slot][10];
        case PlayerAnimation::SkillTwo: return kActorClipNames[slot][11];
        case PlayerAnimation::Ultimate: return kActorClipNames[slot][12];
        case PlayerAnimation::Dash: return kActorClipNames[slot][13];
        case PlayerAnimation::Hurt: return kActorClipNames[slot][14];
        case PlayerAnimation::Death: return kActorClipNames[slot][15];
    }
    return kActorClipNames[slot][0];
}

const char* EnemyActionClip(std::size_t slot, EnemyAnimation animation) {
    const int index = std::clamp(static_cast<int>(animation), 0, 8);
    return kActorClipNames[std::min<std::size_t>(slot, 9u)][index];
}

std::size_t ModelSlot(CharacterId character) {
    return character == CharacterId::Knight ? 0u : 1u;
}

int WeaponModelSlot(WeaponId weapon) {
    switch (weapon) {
        case WeaponId::KnightGreatsword: return 0;
        case WeaponId::MagicCasterStaff: return 1;
        case WeaponId::None: break;
    }
    return -1;
}

int FindBoneIndex(const Model& model, const char* name) {
    if (model.skeleton.bones == nullptr) return -1;
    for (int i = 0; i < model.skeleton.boneCount; ++i) {
        if (TextIsEqual(model.skeleton.bones[i].name, name)) return i;
    }
    return -1;
}

int FindWeaponSocketBone(const Model& model) {
    for (const char* name : {"weapon_socket.R", "weapon.R", "hand.R"}) {
        const int index = FindBoneIndex(model, name);
        if (index >= 0) return index;
    }
    return -1;
}

Matrix TransformMatrix(const Transform& transform) {
    return MatrixMultiply(
        MatrixMultiply(MatrixScale(transform.scale.x, transform.scale.y, transform.scale.z),
                       QuaternionToMatrix(transform.rotation)),
        MatrixTranslate(transform.translation.x,
                        transform.translation.y,
                        transform.translation.z));
}

Matrix ActorMatrix(Vector3 position, Vector3 facing, float scale) {
    const float facingAngle = std::atan2(facing.x, facing.z);
    return MatrixMultiply(
        MatrixMultiply(MatrixScale(scale, scale, scale), MatrixRotateY(facingAngle)),
        MatrixTranslate(position.x, position.y, position.z));
}

Matrix SocketWorldMatrix(const Model& model, int socketBone,
                         Vector3 position, Vector3 facing, float scale) {
    return MatrixMultiply(TransformMatrix(model.currentPose[socketBone]),
                          ActorMatrix(position, facing, scale));
}

void DrawFallbackHexArcherBow(Matrix socketWorld, Color tint) {
    const Color limb = tint.r > 250 && tint.g > 220
        ? tint : Color{112, 229, 71, 255};
    const Color crystal = tint.r > 250 && tint.g > 220
        ? tint : Color{185, 255, 96, 255};

    rlPushMatrix();
    rlMultMatrixf(MatrixToFloat(socketWorld));
    const Vector3 grip{0.0f, 0.0f, 0.0f};
    const Vector3 upper{0.0f, 0.62f, 0.03f};
    const Vector3 lower{0.0f, -0.62f, 0.03f};
    const Vector3 upperCurve{0.10f, 0.34f, 0.0f};
    const Vector3 lowerCurve{0.10f, -0.34f, 0.0f};
    DrawCylinderEx(grip, upperCurve, 0.028f, 0.035f, 6, limb);
    DrawCylinderEx(upperCurve, upper, 0.035f, 0.018f, 6, limb);
    DrawCylinderEx(grip, lowerCurve, 0.028f, 0.035f, 6, limb);
    DrawCylinderEx(lowerCurve, lower, 0.035f, 0.018f, 6, limb);
    DrawLine3D(upper, lower, crystal);
    DrawSphere(grip, 0.055f, crystal);
    rlPopMatrix();
}

std::size_t ModelSlot(EnemyArchetype archetype) {
    switch (archetype) {
        case EnemyArchetype::Riftling: return 2u;
        case EnemyArchetype::HexArcher: return 3u;
        case EnemyArchetype::ObsidianBrute: return 4u;
        case EnemyArchetype::BroodWarden: return 5u;
        case EnemyArchetype::HexeyeArtillerist: return 6u;
        case EnemyArchetype::IronrootColossus: return 7u;
        case EnemyArchetype::EclipseChimera: return 8u;
        case EnemyArchetype::VoidSovereign: return 9u;
        case EnemyArchetype::BossPrototype: return 4u;
    }
    return 2u;
}

float LoopFrame(AnimationClip clip, float clock, float speed = 1.0f) {
    const float length = clip.last - clip.first + 1.0f;
    return clip.first + std::fmod(std::max(0.0f, clock) * 60.0f * speed, length);
}

float ProgressFrame(AnimationClip clip, float progress) {
    return clip.first + (clip.last - clip.first)
         * std::clamp(progress, 0.0f, 1.0f);
}

float PlayerModelFrame(const PlayerState& player) {
    const int state = static_cast<int>(player.animation);
    const AnimationClip clip = kAnimationClips[std::clamp(state, 0, 8)];
    if (player.animation == PlayerAnimation::Idle)
        return LoopFrame(clip, player.animationTime, 0.72f);
    if (player.animation == PlayerAnimation::Run)
        return LoopFrame(clip, player.animationTime, 1.0f);
    if (player.animation == PlayerAnimation::Death)
        return ProgressFrame(clip, player.animationTime / 1.50f);
    const float progress = player.animationDuration > 0.0f
        ? player.animationTime / player.animationDuration : 0.0f;
    return ProgressFrame(clip, progress);
}

float PlayerPreviousModelFrame(const PlayerState& player) {
    const int state = std::clamp(static_cast<int>(player.previousAnimation), 0, 8);
    return LoopFrame(kAnimationClips[state], player.previousAnimationTime,
                     player.previousAnimation == PlayerAnimation::Run ? 1.0f : 0.72f);
}

float EnemyAnimationFrame(EnemyAnimation animation, float animationTime,
                          const EnemyState& enemy, float time) {
    const int state = std::clamp(static_cast<int>(animation), 0, 8);
    if (animation == EnemyAnimation::Death) {
        const float duration = IsBoss(enemy.archetype) ? 1.55f : 0.95f;
        return ProgressFrame(kAnimationClips[8], 1.0f - enemy.deathTimer / duration);
    }
    if (animation == EnemyAnimation::Hurt)
        return ProgressFrame(kAnimationClips[7], animationTime / 0.20f);

    if (animation == EnemyAnimation::Ultimate) {
        float duration = 1.20f;
        if (enemy.archetype == EnemyArchetype::EclipseChimera) duration = 1.00f;
        if (enemy.archetype == EnemyArchetype::VoidSovereign) duration = 1.80f;
        return ProgressFrame(kAnimationClips[5], animationTime / duration);
    }

    if (animation == EnemyAnimation::BasicAttack
        || animation == EnemyAnimation::SkillOne
        || animation == EnemyAnimation::SkillTwo) {
        float duration = 0.60f;
        switch (enemy.archetype) {
            case EnemyArchetype::Riftling: duration = 0.42f; break;
            case EnemyArchetype::HexArcher: duration = 0.50f; break;
            case EnemyArchetype::ObsidianBrute: duration = 0.80f; break;
            case EnemyArchetype::BroodWarden: duration = 0.60f; break;
            case EnemyArchetype::HexeyeArtillerist: duration = 0.90f; break;
            case EnemyArchetype::IronrootColossus:
                duration = enemy.bossPhase == 1 ? 1.05f : 0.82f;
                break;
            case EnemyArchetype::EclipseChimera:
                duration = animation == EnemyAnimation::SkillOne ? 0.85f : 0.62f;
                break;
            case EnemyArchetype::VoidSovereign:
                if (animation == EnemyAnimation::SkillOne)
                    duration = enemy.bossPhase == 2 ? 0.75f : 0.68f;
                else
                    duration = enemy.bossPhase == 1 ? 0.52f : 0.62f;
                break;
            case EnemyArchetype::BossPrototype: duration = 0.80f; break;
        }

        // Authored contact poses: Basic 190, SkillOne 274, SkillTwo 348.
        // The gameplay timer reaches zero at this pose; the following blend
        // supplies recovery without looping the strike a second time.
        float contactFrame = 190.0f;
        if (animation == EnemyAnimation::SkillOne) contactFrame = 274.0f;
        if (animation == EnemyAnimation::SkillTwo) contactFrame = 348.0f;
        const AnimationClip windup{kAnimationClips[state].first, contactFrame};
        return ProgressFrame(windup, animationTime / duration);
    }

    const float speed = animation == EnemyAnimation::Run
        ? (enemy.archetype == EnemyArchetype::Riftling ? 1.24f : 0.82f)
        : (animation == EnemyAnimation::Idle ? 0.66f : 0.88f);
    return LoopFrame(kAnimationClips[state], animationTime + time * 0.001f, speed);
}

float EnemyModelFrame(const EnemyState& enemy, float time) {
    return EnemyAnimationFrame(enemy.visualAnimation, enemy.animationTime, enemy, time);
}

float EnemyModelScale(const EnemyState& enemy) {
    if (enemy.archetype == EnemyArchetype::VoidSovereign)
        return enemy.bossPhase == 1 ? 0.50f : (enemy.bossPhase == 2 ? 1.0f : 1.5625f);
    if (enemy.archetype == EnemyArchetype::BossPrototype) return 1.35f;
    return 1.0f;
}

void EnemyBarMetrics(EnemyArchetype archetype, float& height, float& width) {
    switch (archetype) {
        case EnemyArchetype::Riftling: height = 1.25f; width = 0.95f; break;
        case EnemyArchetype::HexArcher: height = 2.15f; width = 1.10f; break;
        case EnemyArchetype::ObsidianBrute: height = 3.55f; width = 2.10f; break;
        case EnemyArchetype::BroodWarden: height = 3.40f; width = 3.80f; break;
        case EnemyArchetype::HexeyeArtillerist: height = 3.25f; width = 2.40f; break;
        case EnemyArchetype::IronrootColossus: height = 5.15f; width = 5.00f; break;
        case EnemyArchetype::EclipseChimera: height = 4.65f; width = 6.00f; break;
        case EnemyArchetype::VoidSovereign: height = 4.80f; width = 3.00f; break;
        case EnemyArchetype::BossPrototype: height = 3.80f; width = 3.20f; break;
    }
}

constexpr float kPi = 3.14159265358979323846f;

float Saturate(float value) {
    return std::clamp(value, 0.0f, 1.0f);
}

float SmoothPulse(float value) {
    value = Saturate(value);
    return std::sin(value * kPi);
}

Vector3 HorizontalFacing(Vector3 facing) {
    facing.y = 0.0f;
    const float length = LengthXZ(facing);
    if (length < 0.001f) return {0.0f, 0.0f, 1.0f};
    return {facing.x / length, 0.0f, facing.z / length};
}

Vector3 FacingRight(Vector3 facing) {
    facing = HorizontalFacing(facing);
    return {facing.z, 0.0f, -facing.x};
}

Vector3 CombatPoint(Vector3 origin, Vector3 right, Vector3 forward,
                    float side, float height, float depth) {
    return {
        origin.x + right.x * side + forward.x * depth,
        origin.y + height,
        origin.z + right.z * side + forward.z * depth
    };
}

void DrawGlowSegment(Vector3 start, Vector3 end, float radius, Color color,
                     float intensity = 1.0f) {
    intensity = Saturate(intensity);
    DrawCylinderEx(start, end, radius * 2.8f, radius * 2.2f, 6,
                   Fade(color, 0.12f * intensity));
    DrawCylinderEx(start, end, radius * 1.45f, radius * 1.15f, 6,
                   Fade(color, 0.36f * intensity));
    DrawCylinderEx(start, end, radius, radius * 0.72f, 6,
                   Fade(RAYWHITE, 0.76f * intensity));
}

Vector3 ArcPoint(Vector3 center, Vector3 right, Vector3 forward,
                 float radius, float angle, float verticalArc) {
    return {
        center.x + right.x * std::sin(angle) * radius
                 + forward.x * std::cos(angle) * radius,
        center.y + std::sin((angle + kPi * 0.5f) * 0.65f) * verticalArc,
        center.z + right.z * std::sin(angle) * radius
                 + forward.z * std::cos(angle) * radius
    };
}

void DrawSlashTrail(Vector3 center, Vector3 facing, float radius,
                    float tailAngle, float headAngle, float verticalArc,
                    Color color, int segments, float intensity = 1.0f) {
    const Vector3 forward = HorizontalFacing(facing);
    const Vector3 right = FacingRight(forward);
    segments = std::max(3, segments);
    Vector3 previous = ArcPoint(center, right, forward, radius, tailAngle, verticalArc);
    for (int i = 1; i <= segments; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(segments);
        const float angle = tailAngle + (headAngle - tailAngle) * t;
        const Vector3 current = ArcPoint(center, right, forward, radius, angle, verticalArc);
        DrawGlowSegment(previous, current, 0.035f + 0.035f * t, color,
                        intensity * (0.22f + t * 0.78f));
        previous = current;
    }
    DrawSphere(previous, 0.09f + 0.05f * intensity, Fade(RAYWHITE, 0.84f * intensity));
}

void DrawRuneRing(Vector3 center, Vector3 facing, float radius, float rotation,
                  Color color, int segments = 28, float thickness = 0.018f) {
    const Vector3 right = FacingRight(facing);
    const Vector3 up{0.0f, 1.0f, 0.0f};
    Vector3 previous = CombatPoint(center, right, up,
                                   std::cos(rotation) * radius,
                                   std::sin(rotation) * radius, 0.0f);
    for (int i = 1; i <= segments; ++i) {
        const float angle = rotation + static_cast<float>(i) * 2.0f * kPi
                          / static_cast<float>(segments);
        const Vector3 current = CombatPoint(center, right, up,
                                            std::cos(angle) * radius,
                                            std::sin(angle) * radius, 0.0f);
        DrawCylinderEx(previous, current, thickness * 2.1f, thickness * 2.1f, 5,
                       Fade(color, 0.22f));
        DrawCylinderEx(previous, current, thickness, thickness, 5, color);
        if ((i % 4) == 0) {
            const Vector3 tick = CombatPoint(center, right, up,
                std::cos(angle) * radius * 1.16f,
                std::sin(angle) * radius * 1.16f, 0.0f);
            DrawGlowSegment(current, tick, thickness * 0.70f, color, 0.70f);
        }
        previous = current;
    }
}

void DrawGroundGlyph(Vector3 center, float radius, float rotation,
                     Color color, int spokes = 8) {
    center.y = std::max(center.y, 0.055f);
    DrawCircle3D(center, radius, {1.0f, 0.0f, 0.0f}, 90.0f, Fade(color, 0.82f));
    DrawCircle3D({center.x, center.y + 0.012f, center.z}, radius * 0.72f,
                 {1.0f, 0.0f, 0.0f}, 90.0f, Fade(color, 0.48f));
    for (int i = 0; i < spokes; ++i) {
        const float angle = rotation + 2.0f * kPi * static_cast<float>(i)
                          / static_cast<float>(spokes);
        const Vector3 inner{center.x + std::cos(angle) * radius * 0.46f,
                            center.y + 0.018f,
                            center.z + std::sin(angle) * radius * 0.46f};
        const Vector3 outer{center.x + std::cos(angle) * radius,
                            center.y + 0.018f,
                            center.z + std::sin(angle) * radius};
        DrawGlowSegment(inner, outer, 0.014f, color, 0.62f);
    }
}

void DrawVfxQuad(Texture2D texture, Vector3 center, Vector3 axisU, Vector3 axisV,
                 float halfWidth, float halfHeight, Color tint) {
    if (texture.id == 0 || halfWidth <= 0.0f || halfHeight <= 0.0f) return;
    axisU = Vector3Normalize(axisU);
    axisV = Vector3Normalize(axisV);
    const Vector3 u = Vector3Scale(axisU, halfWidth);
    const Vector3 v = Vector3Scale(axisV, halfHeight);
    const Vector3 bottomLeft = Vector3Subtract(Vector3Subtract(center, u), v);
    const Vector3 bottomRight = Vector3Add(Vector3Subtract(center, v), u);
    const Vector3 topRight = Vector3Add(Vector3Add(center, u), v);
    const Vector3 topLeft = Vector3Add(Vector3Subtract(center, u), v);

    rlDisableBackfaceCulling();
    rlDisableDepthMask();
    rlSetTexture(texture.id);
    rlBegin(RL_QUADS);
        rlColor4ub(tint.r, tint.g, tint.b, tint.a);
        rlTexCoord2f(0.0f, 1.0f); rlVertex3f(bottomLeft.x, bottomLeft.y, bottomLeft.z);
        rlTexCoord2f(1.0f, 1.0f); rlVertex3f(bottomRight.x, bottomRight.y, bottomRight.z);
        rlTexCoord2f(1.0f, 0.0f); rlVertex3f(topRight.x, topRight.y, topRight.z);
        rlTexCoord2f(0.0f, 0.0f); rlVertex3f(topLeft.x, topLeft.y, topLeft.z);
    rlEnd();
    rlSetTexture(0);
    rlEnableDepthMask();
    rlEnableBackfaceCulling();
}

void VerticalQuadAxes(Vector3 facing, float rotation, Vector3& axisU, Vector3& axisV) {
    const Vector3 right = FacingRight(facing);
    const Vector3 up{0.0f, 1.0f, 0.0f};
    const float c = std::cos(rotation);
    const float s = std::sin(rotation);
    axisU = Vector3Add(Vector3Scale(right, c), Vector3Scale(up, s));
    axisV = Vector3Add(Vector3Scale(right, -s), Vector3Scale(up, c));
}

void DrawImpactBurst(Vector3 center, Color color, float phase,
                     float scale, int rays = 8) {
    phase = Saturate(phase);
    const float expansion = 0.18f + phase * 0.85f;
    const float opacity = 1.0f - phase;
    DrawSphere(center, scale * (0.16f + phase * 0.08f),
               Fade(RAYWHITE, opacity * 0.72f));
    for (int i = 0; i < rays; ++i) {
        const float angle = 2.0f * kPi * static_cast<float>(i) / static_cast<float>(rays)
                          + phase * 0.35f;
        const float lift = ((i % 3) - 1) * 0.20f;
        const Vector3 start{center.x + std::cos(angle) * scale * 0.10f,
                            center.y + lift * scale * 0.20f,
                            center.z + std::sin(angle) * scale * 0.10f};
        const Vector3 end{center.x + std::cos(angle) * scale * expansion,
                          center.y + lift * scale + 0.08f + phase * 0.25f,
                          center.z + std::sin(angle) * scale * expansion};
        DrawGlowSegment(start, end, 0.018f * scale, color, opacity);
    }
}

void ResetBladeTrail(BladeTrailState& trail, std::uint32_t actionSerial,
                     std::uint32_t contactSerial) {
    trail.count = 0;
    trail.actionSerial = actionSerial;
    trail.contactSerial = contactSerial;
    trail.contactPoint = {};
    trail.contactTime = -1.0;
}

void UpdateBladeTrail(const PlayerState& player, const WeaponWorldPose& weaponPose,
                      BladeTrailState& trail, bool reducedMotion) {
    if (player.animation != PlayerAnimation::BasicAttack || !weaponPose.valid) {
        if (trail.count > 0)
            ResetBladeTrail(trail, player.actionSerial, player.contactSerial);
        return;
    }

    if (trail.actionSerial != player.actionSerial) {
        ResetBladeTrail(trail, player.actionSerial, player.contactSerial);
        // A long render hitch can skip directly over the fixed-step contact.
        // Leave the serial pending so the burst is still emitted once.
        if (player.actionContactTriggered && player.contactSerial > 0)
            trail.contactSerial = player.contactSerial - 1;
    }

    // Animation time freezes on the authored contact pose during hit-stop,
    // keeping the blade silhouette frozen with it instead of fading by wall time.
    const double now = std::max(0.0, static_cast<double>(player.animationTime));
    if (trail.contactSerial != player.contactSerial) {
        trail.contactSerial = player.contactSerial;
        trail.contactPoint = weaponPose.bladeTip;
        trail.contactTime = now;
    }

    if (reducedMotion) {
        // Do not retain hidden history that could pop into view if the option is
        // changed halfway through an attack.
        trail.count = 0;
        return;
    }

    constexpr double kTrailLifetime = 0.22;
    int retained = 0;
    for (int i = 0; i < trail.count; ++i) {
        if (now - trail.samples[i].time <= kTrailLifetime)
            trail.samples[retained++] = trail.samples[i];
    }
    trail.count = retained;

    if (trail.count > 0) {
        const BladeTrailSample& latest = trail.samples[trail.count - 1];
        const float jump = std::max(Vector3Distance(latest.root, weaponPose.bladeRoot),
                                    Vector3Distance(latest.tip, weaponPose.bladeTip));
        if (jump > 2.5f) trail.count = 0;
    }

    const bool moved = trail.count == 0
        || Vector3Distance(trail.samples[trail.count - 1].root, weaponPose.bladeRoot) > 0.018f
        || Vector3Distance(trail.samples[trail.count - 1].tip, weaponPose.bladeTip) > 0.018f;
    const auto appendSample = [&trail](const BladeTrailSample& sample) {
        if (trail.count == static_cast<int>(trail.samples.size())) {
            for (std::size_t i = 1; i < trail.samples.size(); ++i)
                trail.samples[i - 1] = trail.samples[i];
            --trail.count;
        }
        trail.samples[trail.count++] = sample;
    };

    if (trail.count == 0) {
        appendSample({weaponPose.bladeRoot, weaponPose.bladeTip, now});
    } else if (moved) {
        const BladeTrailSample previous = trail.samples[trail.count - 1];
        const double span = now - previous.time;
        if (span > 0.00001) {
            constexpr double kSampleInterval = 1.0 / 120.0;
            const int steps = std::clamp(
                static_cast<int>(std::ceil(span / kSampleInterval)), 1, 12);
            for (int step = 1; step <= steps; ++step) {
                const float t = static_cast<float>(step) / static_cast<float>(steps);
                appendSample({
                    Vector3Lerp(previous.root, weaponPose.bladeRoot, t),
                    Vector3Lerp(previous.tip, weaponPose.bladeTip, t),
                    previous.time + span * t
                });
            }
        }
    }
}

void DrawBladeRibbon(const PlayerState& player, const WeaponWorldPose& weaponPose,
                     BladeTrailState& trail, Texture2D texture,
                     bool reducedMotion, Color color) {
    UpdateBladeTrail(player, weaponPose, trail, reducedMotion);
    if (player.animation != PlayerAnimation::BasicAttack || !weaponPose.valid) return;

    const float progress = AnimationProgress(player);
    const float contact = std::clamp(player.actionContactNormalized, 0.08f, 0.92f);
    const float phaseT = progress <= contact
        ? Saturate(progress / contact)
        : Saturate((progress - contact) / (1.0f - contact));
    const float smoothPhase = phaseT * phaseT * (3.0f - 2.0f * phaseT);
    const float phaseEnvelope = progress <= contact
        ? 0.12f + 0.88f * smoothPhase
        : 1.0f - 0.90f * smoothPhase;

    if (reducedMotion) {
        DrawGlowSegment(weaponPose.bladeRoot, weaponPose.bladeTip,
                        0.026f, color, 0.82f * phaseEnvelope);
    } else if (trail.count >= 2) {
        const double now = std::max(0.0, static_cast<double>(player.animationTime));
        constexpr double kTrailLifetime = 0.22;
        if (texture.id != 0) {
            rlDisableBackfaceCulling();
            rlDisableDepthMask();
            BeginBlendMode(BLEND_ADDITIVE);
            rlSetTexture(texture.id);
            rlBegin(RL_QUADS);
            for (int i = 0; i + 1 < trail.count; ++i) {
                const BladeTrailSample& a = trail.samples[i];
                const BladeTrailSample& b = trail.samples[i + 1];
                const float age = static_cast<float>((now - b.time) / kTrailLifetime);
                const float alpha = std::clamp(1.0f - age, 0.0f, 1.0f)
                                  * (0.30f + 0.70f * (i + 1.0f) / trail.count)
                                  * phaseEnvelope;
                const unsigned char opacity = static_cast<unsigned char>(
                    std::clamp(alpha, 0.0f, 1.0f) * 235.0f);
                const float u0 = static_cast<float>(i) / (trail.count - 1.0f);
                const float u1 = static_cast<float>(i + 1) / (trail.count - 1.0f);
                rlColor4ub(color.r, color.g, color.b, opacity);
                rlTexCoord2f(u0, 1.0f); rlVertex3f(a.root.x, a.root.y, a.root.z);
                rlTexCoord2f(u1, 1.0f); rlVertex3f(b.root.x, b.root.y, b.root.z);
                rlTexCoord2f(u1, 0.0f); rlVertex3f(b.tip.x, b.tip.y, b.tip.z);
                rlTexCoord2f(u0, 0.0f); rlVertex3f(a.tip.x, a.tip.y, a.tip.z);
            }
            rlEnd();
            rlSetTexture(0);
            EndBlendMode();
            rlEnableDepthMask();
            rlEnableBackfaceCulling();
        } else {
            // Texture failure still leaves a full-width swept surface rather
            // than degrading to a single tip line.
            rlDisableBackfaceCulling();
            rlDisableDepthMask();
            BeginBlendMode(BLEND_ADDITIVE);
            for (int i = 0; i + 1 < trail.count; ++i) {
                const BladeTrailSample& a = trail.samples[i];
                const BladeTrailSample& b = trail.samples[i + 1];
                const float age = static_cast<float>((now - b.time) / kTrailLifetime);
                const float alpha = std::clamp(1.0f - age, 0.0f, 1.0f)
                                  * (0.18f + 0.42f * (i + 1.0f) / trail.count)
                                  * phaseEnvelope;
                const Color tint = Fade(color, alpha);
                DrawTriangle3D(a.root, b.root, b.tip, tint);
                DrawTriangle3D(a.root, b.tip, a.tip, tint);
            }
            EndBlendMode();
            rlEnableDepthMask();
            rlEnableBackfaceCulling();
        }
        for (int i = 0; i + 1 < trail.count; ++i) {
            const float strength = (i + 1.0f) / trail.count;
            DrawGlowSegment(trail.samples[i].tip, trail.samples[i + 1].tip,
                            0.014f + 0.018f * strength, color,
                            (0.28f + 0.55f * strength) * phaseEnvelope);
        }
    }

    const double burstAge = static_cast<double>(player.animationTime)
                          - trail.contactTime;
    if (trail.contactTime >= 0.0 && burstAge >= 0.0 && burstAge <= 0.13) {
        DrawImpactBurst(trail.contactPoint, color,
                        static_cast<float>(burstAge / 0.13),
                        reducedMotion ? 0.42f : 0.72f,
                        reducedMotion ? 4 : 9);
    }
}

void DrawDashTexture(Texture2D texture, const PlayerState& player,
                     float progress, Color tint, bool reducedMotion) {
    if (texture.id == 0) return;
    const Vector3 facing = HorizontalFacing(player.facing);
    const Vector3 right = FacingRight(facing);
    const Vector3 up{0.0f, 1.0f, 0.0f};
    const float fade = std::clamp(1.0f - progress * 0.78f, 0.0f, 1.0f)
                     * (reducedMotion ? 0.42f : 0.72f);
    const Vector3 center{player.position.x - facing.x * 1.25f,
                         player.position.y + 0.82f,
                         player.position.z - facing.z * 1.25f};
    DrawVfxQuad(texture, center, facing, up,
                1.85f, 0.58f, Fade(tint, fade));
    if (!reducedMotion) {
        DrawVfxQuad(texture,
                    {center.x, player.position.y + 0.10f, center.z}, facing, right,
                    1.95f, 0.52f, Fade(tint, fade * 0.58f));
    }
}

void DrawDashAfterimage(const PlayerState& player, Color color,
                        float progress, bool reducedMotion) {
    const Vector3 forward = HorizontalFacing(player.facing);
    const Vector3 right = FacingRight(forward);
    const int echoes = reducedMotion ? 2 : 5;
    for (int echo = 1; echo <= echoes; ++echo) {
        const float distance = 0.55f + echo * 0.55f;
        const float alpha = (1.0f - static_cast<float>(echo) / (echoes + 1.0f))
                          * (1.0f - progress * 0.45f);
        const Vector3 base{player.position.x - forward.x * distance,
                           player.position.y,
                           player.position.z - forward.z * distance};
        DrawCapsule({base.x, base.y + 0.42f, base.z},
                    {base.x, base.y + 1.58f, base.z},
                    0.25f, 7, 5, Fade(color, alpha * 0.20f));
        for (int side : {-1, 1}) {
            const Vector3 start = CombatPoint(base, right, forward,
                                              side * 0.34f, 0.35f, 0.0f);
            const Vector3 end = CombatPoint(base, right, forward,
                                            side * 0.52f, 1.40f, -0.32f);
            DrawGlowSegment(start, end, 0.022f, color, alpha);
        }
    }
}

void DrawKnightCombatVfx(const PlayerState& player, bool reducedMotion,
                         bool highContrast, const WeaponWorldPose& weaponPose,
                         BladeTrailState& bladeTrail, Texture2D swordArcTexture,
                         Texture2D guardTexture, Texture2D rushTexture,
                         Texture2D stormTexture, Texture2D dashTexture) {
    const float progress = AnimationProgress(player);
    const float time = static_cast<float>(GetTime());
    const Vector3 facing = HorizontalFacing(player.facing);
    const Vector3 right = FacingRight(facing);
    const Color violet = highContrast ? Color{255, 255, 255, 255}
                                      : Color{215, 112, 255, 255};
    const Color gold = highContrast ? Color{255, 235, 48, 255}
                                    : Color{255, 218, 111, 255};

    if (player.animation != PlayerAnimation::BasicAttack && bladeTrail.count > 0)
        ResetBladeTrail(bladeTrail, player.actionSerial, player.contactSerial);

    switch (player.animation) {
        case PlayerAnimation::BasicAttack: {
            DrawBladeRibbon(player, weaponPose, bladeTrail, swordArcTexture,
                            reducedMotion, violet);
            if (!weaponPose.valid) {
                const float strike = Saturate((progress - 0.14f) / 0.72f);
                const float intensity = SmoothPulse(strike);
                const float head = -2.15f + strike * 4.25f;
                DrawSlashTrail({player.position.x + facing.x * 0.34f, 1.05f,
                                player.position.z + facing.z * 0.34f},
                               facing, 1.78f, head - 0.82f, head, 0.34f,
                               violet, reducedMotion ? 7 : 13, intensity);
                DrawSlashTrail({player.position.x + facing.x * 0.40f, 1.07f,
                                player.position.z + facing.z * 0.40f},
                               facing, 1.52f, head - 0.58f, head, 0.25f,
                               gold, reducedMotion ? 5 : 9, intensity * 0.72f);
            }
            break;
        }
        case PlayerAnimation::SkillOne: {
            const float charge = SmoothPulse(progress);
            const float radius = 0.82f + charge * 0.24f;
            DrawGroundGlyph(player.position, 1.25f + charge * 0.22f,
                            time * 0.8f, violet, 6);
            DrawRuneRing({player.position.x, 1.02f, player.position.z}, facing,
                         radius, time * 1.6f, gold, reducedMotion ? 16 : 28, 0.024f);
            // The old wire sphere resembled a debug hitbox. Keep the same
            // gameplay guard radius, but present it as authored rune arcs.
            DrawRuneRing({player.position.x, 1.02f, player.position.z}, facing,
                         radius * 0.84f, -time * 1.05f, violet,
                         reducedMotion ? 10 : 18, 0.015f);
            if (guardTexture.id != 0) {
                DrawVfxQuad(guardTexture,
                            {player.position.x + facing.x * 0.48f,
                             1.06f,
                             player.position.z + facing.z * 0.48f},
                            right, {0.0f, 1.0f, 0.0f},
                            radius * 1.03f, radius * 1.03f,
                            Fade(WHITE, (0.40f + charge * 0.42f)
                                        * (reducedMotion ? 0.68f : 1.0f)));
            }
            for (int i = 0; i < (reducedMotion ? 4 : 8); ++i) {
                const float angle = time * 1.2f + i * 2.0f * kPi
                                  / (reducedMotion ? 4.0f : 8.0f);
                DrawSphere({player.position.x + std::cos(angle) * radius,
                            0.32f + (i % 3) * 0.32f,
                            player.position.z + std::sin(angle) * radius},
                           0.045f, Fade(gold, 0.72f));
            }
            break;
        }
        case PlayerAnimation::SkillTwo: {
            DrawDashAfterimage(player, violet, progress, reducedMotion);
            DrawDashTexture(dashTexture, player, progress, violet, reducedMotion);
            if (rushTexture.id != 0) {
                const float release = Saturate((progress - 0.18f) / 0.82f);
                const Vector3 rushCenter{
                    player.position.x + facing.x * (0.28f + release * 0.82f),
                    0.105f,
                    player.position.z + facing.z * (0.28f + release * 0.82f)
                };
                DrawVfxQuad(rushTexture, rushCenter, facing, right,
                            1.85f + release * 1.15f,
                            0.70f + release * 0.34f,
                            Fade(WHITE, (0.38f + 0.40f * (1.0f - progress))
                                        * (reducedMotion ? 0.62f : 1.0f)));
            }
            const int lanes = reducedMotion ? 3 : 7;
            for (int lane = -lanes / 2; lane <= lanes / 2; ++lane) {
                const float side = static_cast<float>(lane) * 0.22f;
                const Vector3 end = CombatPoint(player.position, right, facing,
                                                side, 0.10f, 0.45f);
                const Vector3 start = CombatPoint(player.position, right, facing,
                                                  side, 0.10f,
                                                  -5.5f * (1.0f - progress * 0.30f));
                DrawGlowSegment(start, end, 0.025f, lane == 0 ? gold : violet,
                                0.42f + 0.45f * (1.0f - progress));
            }
            DrawSlashTrail({player.position.x + facing.x * 0.50f, 0.88f,
                            player.position.z + facing.z * 0.50f},
                           facing, 1.38f, -1.15f, 1.15f, 0.18f, gold,
                           reducedMotion ? 7 : 12, 1.0f - progress * 0.35f);
            break;
        }
        case PlayerAnimation::Ultimate: {
            const float charge = Saturate(progress / 0.38f);
            const float release = Saturate((progress - 0.30f) / 0.70f);
            DrawGroundGlyph(player.position, 1.2f + charge * 2.0f,
                            time * 1.35f, gold, reducedMotion ? 6 : 12);
            if (stormTexture.id != 0) {
                const float spin = time * 1.55f;
                const Vector3 axisU{std::cos(spin), 0.0f, std::sin(spin)};
                const Vector3 axisV{-std::sin(spin), 0.0f, std::cos(spin)};
                const float size = 1.25f + charge * 2.15f;
                DrawVfxQuad(stormTexture,
                            {player.position.x, 0.10f, player.position.z},
                            axisU, axisV, size, size,
                            Fade(WHITE, (0.28f + release * 0.58f)
                                        * (reducedMotion ? 0.62f : 1.0f)));
            }
            DrawCylinderEx({player.position.x, 0.08f, player.position.z},
                           {player.position.x, 2.4f + charge * 2.8f, player.position.z},
                           0.18f + charge * 0.13f, 0.06f,
                           reducedMotion ? 8 : 12, Fade(violet, 0.20f + charge * 0.28f));
            const int blades = reducedMotion ? 3 : 7;
            for (int blade = 0; blade < blades; ++blade) {
                const float spin = time * 4.0f + blade * 2.0f * kPi / blades;
                const Vector3 direction{std::cos(spin), 0.0f, std::sin(spin)};
                DrawSlashTrail({player.position.x, 0.80f + (blade % 3) * 0.35f,
                                player.position.z}, direction,
                               2.0f + release * 3.2f, -0.78f, 0.78f,
                               0.20f, blade % 2 == 0 ? gold : violet,
                               reducedMotion ? 5 : 9, 0.35f + release * 0.65f);
            }
            if (release > 0.05f)
                DrawImpactBurst({player.position.x, 0.22f, player.position.z},
                                violet, release, 2.7f, reducedMotion ? 6 : 12);
            break;
        }
        case PlayerAnimation::Dash:
            DrawDashAfterimage(player, violet, progress, reducedMotion);
            DrawDashTexture(dashTexture, player, progress, violet, reducedMotion);
            DrawGroundGlyph(player.position, 0.55f + (1.0f - progress) * 0.45f,
                            time * 1.8f, gold, 4);
            break;
        default: break;
    }
}

void DrawMageCombatVfx(const PlayerState& player, bool reducedMotion,
                       bool highContrast, Texture2D sigilTexture,
                       Texture2D boltTexture, Texture2D frostTexture,
                       Texture2D gravityTexture, Texture2D ultimateTexture,
                       Texture2D dashTexture) {
    const float progress = AnimationProgress(player);
    const float time = static_cast<float>(GetTime());
    const Vector3 facing = HorizontalFacing(player.facing);
    const Vector3 right = FacingRight(facing);
    const Color cyan = highContrast ? Color{255, 255, 255, 255}
                                    : Color{77, 224, 255, 255};
    const Color azure = highContrast ? Color{255, 236, 58, 255}
                                     : Color{74, 112, 255, 255};
    const Color violet = highContrast ? Color{255, 255, 255, 255}
                                      : Color{186, 98, 255, 255};
    const Vector3 focus = CombatPoint(player.position, right, facing, 0.34f, 1.42f, 0.62f);

    switch (player.animation) {
        case PlayerAnimation::BasicAttack: {
            const float pulse = SmoothPulse(progress);
            DrawRuneRing(focus, facing, 0.28f + pulse * 0.32f,
                         time * 3.6f, cyan, reducedMotion ? 16 : 28, 0.020f);
            DrawSphere(focus, 0.09f + pulse * 0.13f, Fade(RAYWHITE, 0.72f));
            const Vector3 muzzle = CombatPoint(focus, right, facing, 0.0f, 0.0f,
                                               0.38f + progress * 0.45f);
            DrawGlowSegment(focus, muzzle, 0.035f, azure, 0.65f + pulse * 0.35f);
            if (boltTexture.id != 0) {
                const float length = 0.34f + progress * 0.54f;
                DrawVfxQuad(boltTexture, muzzle, facing, {0.0f, 1.0f, 0.0f},
                            length, 0.24f + pulse * 0.09f,
                            Fade(WHITE, (0.38f + pulse * 0.52f)
                                        * (reducedMotion ? 0.70f : 1.0f)));
                if (!reducedMotion)
                    DrawVfxQuad(boltTexture, muzzle, facing, right,
                                length, 0.22f + pulse * 0.07f,
                                Fade(WHITE, 0.48f + pulse * 0.32f));
            }
            if (sigilTexture.id != 0) {
                Vector3 quadU{};
                Vector3 quadV{};
                VerticalQuadAxes(facing, time * 0.35f, quadU, quadV);
                DrawVfxQuad(sigilTexture, focus, quadU, quadV,
                            0.42f + pulse * 0.16f, 0.42f + pulse * 0.16f,
                            Fade(WHITE, pulse * (reducedMotion ? 0.46f : 0.68f)));
            }
            break;
        }
        case PlayerAnimation::SkillOne: {
            const float release = Saturate((progress - 0.18f) / 0.82f);
            const float radius = 0.9f + release * 3.35f;
            DrawGroundGlyph(player.position, radius, -time * 0.9f, cyan,
                            reducedMotion ? 6 : 12);
            DrawCircle3D({player.position.x, 0.075f, player.position.z},
                         radius * 0.83f, {1.0f, 0.0f, 0.0f}, 90.0f,
                         Fade(azure, 0.70f * (1.0f - release * 0.42f)));
            const int spikes = reducedMotion ? 6 : 14;
            for (int i = 0; i < spikes; ++i) {
                const float angle = i * 2.0f * kPi / spikes + time * 0.18f;
                const Vector3 base{player.position.x + std::cos(angle) * radius * 0.82f,
                                   0.08f,
                                   player.position.z + std::sin(angle) * radius * 0.82f};
                const Vector3 tip{player.position.x + std::cos(angle) * radius,
                                  0.35f + (i % 3) * 0.16f,
                                  player.position.z + std::sin(angle) * radius};
                DrawGlowSegment(base, tip, 0.025f, i % 2 ? cyan : violet,
                                0.55f + 0.30f * (1.0f - release));
            }
            DrawRuneRing(focus, facing, 0.40f + release * 0.16f,
                         -time * 2.4f, violet, reducedMotion ? 14 : 24, 0.018f);
            if (frostTexture.id != 0) {
                const float spin = -time * 0.28f;
                const Vector3 axisU{std::cos(spin), 0.0f, std::sin(spin)};
                const Vector3 axisV{-std::sin(spin), 0.0f, std::cos(spin)};
                DrawVfxQuad(frostTexture,
                            {player.position.x, 0.085f, player.position.z},
                            axisU, axisV, radius * 0.78f, radius * 0.78f,
                            Fade(WHITE, (0.42f + release * 0.24f)
                                        * (reducedMotion ? 0.72f : 1.0f)));
            }
            if (sigilTexture.id != 0) {
                Vector3 quadU{};
                Vector3 quadV{};
                VerticalQuadAxes(facing, -time * 0.35f, quadU, quadV);
                DrawVfxQuad(sigilTexture, focus, quadU, quadV,
                            0.36f + release * 0.16f,
                            0.36f + release * 0.16f,
                            Fade(WHITE, 0.34f + release * 0.24f));
            }
            break;
        }
        case PlayerAnimation::SkillTwo: {
            const float charge = SmoothPulse(progress);
            DrawRuneRing(focus, facing, 0.52f + charge * 0.20f,
                         time * 2.9f, violet, reducedMotion ? 16 : 30, 0.025f);
            DrawRuneRing(focus, facing, 0.31f + charge * 0.12f,
                         -time * 4.1f, cyan, reducedMotion ? 12 : 22, 0.015f);
            DrawSphere(focus, 0.13f + charge * 0.18f, Fade(azure, 0.82f));
            if (gravityTexture.id != 0) {
                // The vortex artwork is authored top-facing, so preview it as
                // a world-space portal under the cast instead of a vertical card.
                const float spin = -time * 1.20f;
                const Vector3 quadU{std::cos(spin), 0.0f, std::sin(spin)};
                const Vector3 quadV{-std::sin(spin), 0.0f, std::cos(spin)};
                const Vector3 preview{
                    player.position.x + facing.x * (0.70f + charge * 0.52f),
                    0.092f,
                    player.position.z + facing.z * (0.70f + charge * 0.52f)
                };
                DrawVfxQuad(gravityTexture, preview, quadU, quadV,
                            0.58f + charge * 0.36f,
                            0.58f + charge * 0.36f,
                            Fade(WHITE, (0.42f + charge * 0.42f)
                                        * (reducedMotion ? 0.66f : 1.0f)));
            }
            if (sigilTexture.id != 0) {
                Vector3 quadU{};
                Vector3 quadV{};
                VerticalQuadAxes(facing, -time * 0.45f, quadU, quadV);
                DrawVfxQuad(sigilTexture, focus, quadU, quadV,
                            0.58f + charge * 0.18f, 0.58f + charge * 0.18f,
                            Fade(WHITE, (0.48f + charge * 0.26f)
                                        * (reducedMotion ? 0.70f : 1.0f)));
            }
            const int motes = reducedMotion ? 3 : 7;
            for (int i = 0; i < motes; ++i) {
                const float angle = time * (1.8f + i * 0.05f) + i * 2.0f * kPi / motes;
                const Vector3 mote = CombatPoint(focus, right, {0.0f, 1.0f, 0.0f},
                    std::cos(angle) * (0.42f + charge * 0.25f),
                    std::sin(angle) * (0.42f + charge * 0.25f), 0.0f);
                DrawSphere(mote, 0.045f + 0.02f * charge,
                           Fade(i % 2 ? cyan : violet, 0.76f));
            }
            break;
        }
        case PlayerAnimation::Ultimate: {
            const float charge = Saturate(progress / 0.42f);
            const float release = Saturate((progress - 0.34f) / 0.66f);
            DrawGroundGlyph(player.position, 1.5f + charge * 4.3f,
                            -time * 1.3f, violet, reducedMotion ? 8 : 16);
            DrawGroundGlyph(player.position, 0.9f + charge * 2.4f,
                            time * 2.1f, cyan, reducedMotion ? 6 : 12);
            if (ultimateTexture.id != 0) {
                const float sigilRadius = 1.45f + charge * 3.35f;
                const float spin = -time * (0.72f + release * 0.86f);
                const Vector3 axisU{std::cos(spin), 0.0f, std::sin(spin)};
                const Vector3 axisV{-std::sin(spin), 0.0f, std::cos(spin)};
                DrawVfxQuad(ultimateTexture,
                            {player.position.x, 0.088f, player.position.z},
                            axisU, axisV, sigilRadius, sigilRadius,
                            Fade(WHITE, (0.36f + release * 0.38f)
                                        * (reducedMotion ? 0.70f : 1.0f)));
            }
            if (sigilTexture.id != 0) {
                Vector3 quadU{};
                Vector3 quadV{};
                VerticalQuadAxes(facing, time * 0.34f, quadU, quadV);
                DrawVfxQuad(sigilTexture, focus, quadU, quadV,
                            0.54f + charge * 0.28f,
                            0.54f + charge * 0.28f,
                            Fade(WHITE, (0.26f + charge * 0.30f)
                                        * (reducedMotion ? 0.62f : 1.0f)));
            }
            DrawCylinderEx({player.position.x, 0.06f, player.position.z},
                           {player.position.x, 5.5f, player.position.z},
                           0.33f + release * 0.36f, 0.08f,
                           reducedMotion ? 8 : 14, Fade(azure, 0.25f + release * 0.32f));
            const int orbs = reducedMotion ? 4 : 9;
            for (int i = 0; i < orbs; ++i) {
                const float angle = time * (2.0f + release * 3.0f)
                                  + i * 2.0f * kPi / orbs;
                const float orbit = 1.2f + charge * 2.8f + (i % 2) * 0.42f;
                const Vector3 orb{player.position.x + std::cos(angle) * orbit,
                                  0.42f + (i % 3) * 0.52f + charge * 0.8f,
                                  player.position.z + std::sin(angle) * orbit};
                DrawSphere(orb, 0.09f + release * 0.08f,
                           Fade(i % 2 ? cyan : violet, 0.82f));
                if (!reducedMotion)
                    DrawGlowSegment(orb, {player.position.x, 1.15f, player.position.z},
                                    0.014f, azure, 0.30f + release * 0.35f);
            }
            if (release > 0.05f)
                DrawImpactBurst({player.position.x, 0.25f, player.position.z},
                                cyan, release, 3.6f, reducedMotion ? 6 : 14);
            break;
        }
        case PlayerAnimation::Dash:
            DrawDashAfterimage(player, cyan, progress, reducedMotion);
            DrawDashTexture(dashTexture, player, progress, cyan, reducedMotion);
            DrawRuneRing({player.position.x, 0.82f, player.position.z}, facing,
                         0.62f + (1.0f - progress) * 0.32f,
                         time * 3.5f, violet, reducedMotion ? 14 : 24, 0.018f);
            break;
        default: break;
    }
}

void DrawPlayerCombatVfx(const PlayerState& player, bool reducedMotion,
                         bool highContrast, const WeaponWorldPose& weaponPose,
                         BladeTrailState& bladeTrail,
                         Texture2D knightSwordArcTexture,
                         Texture2D knightGuardTexture,
                         Texture2D knightRushTexture,
                         Texture2D knightStormTexture,
                         Texture2D dashTexture,
                         Texture2D mageSigilTexture,
                         Texture2D mageBoltTexture,
                         Texture2D mageFrostTexture,
                         Texture2D mageGravityTexture,
                         Texture2D mageUltimateTexture) {
    if (player.animation == PlayerAnimation::Hurt) {
        const Color impact = highContrast ? Color{255, 255, 255, 255}
                                          : Color{255, 78, 112, 255};
        DrawImpactBurst({player.position.x, 1.08f, player.position.z}, impact,
                        AnimationProgress(player), 0.95f,
                        reducedMotion ? 4 : 9);
    }
    if (player.character == CharacterId::Knight)
        DrawKnightCombatVfx(player, reducedMotion, highContrast, weaponPose,
                            bladeTrail, knightSwordArcTexture, knightGuardTexture,
                            knightRushTexture, knightStormTexture, dashTexture);
    else
        DrawMageCombatVfx(player, reducedMotion, highContrast, mageSigilTexture,
                          mageBoltTexture, mageFrostTexture,
                          mageGravityTexture, mageUltimateTexture, dashTexture);
}

void DrawProjectileVfx(const ProjectileState& projectile, float time,
                       bool reducedMotion, bool highContrast,
                       Texture2D mageBoltTexture) {
    const Color core = highContrast ? Color{255, 255, 255, 255}
        : (projectile.hostile ? Color{163, 255, 76, 255}
                              : Color{68, 218, 255, 255});
    const Color accent = highContrast ? Color{255, 236, 50, 255}
        : (projectile.hostile ? Color{255, 108, 85, 255}
                              : Color{178, 105, 255, 255});
    Vector3 direction = Vector3Normalize(projectile.velocity);
    if (Vector3LengthSqr(direction) < 0.001f) direction = {0.0f, 0.0f, 1.0f};
    if (!projectile.hostile && mageBoltTexture.id != 0) {
        Vector3 side = Vector3CrossProduct({0.0f, 1.0f, 0.0f}, direction);
        if (Vector3LengthSqr(side) < 0.001f) side = {1.0f, 0.0f, 0.0f};
        side = Vector3Normalize(side);
        DrawVfxQuad(mageBoltTexture, projectile.position,
                    direction, {0.0f, 1.0f, 0.0f},
                    0.72f, 0.30f, Fade(WHITE, 0.88f));
        if (!reducedMotion)
            DrawVfxQuad(mageBoltTexture, projectile.position,
                        direction, side, 0.72f, 0.26f,
                        Fade(WHITE, 0.62f));
    }
    const int segments = reducedMotion ? 2 : 6;
    const float trailLength = projectile.hostile ? 0.95f : 1.45f;
    Vector3 previous = projectile.position;
    for (int i = 1; i <= segments; ++i) {
        const float t = static_cast<float>(i) / segments;
        const Vector3 current = Vector3Subtract(projectile.position,
            Vector3Scale(direction, trailLength * t));
        DrawGlowSegment(previous, current,
                        projectile.radius * (0.28f - t * 0.14f),
                        i % 2 ? core : accent, 0.85f - t * 0.65f);
        previous = current;
    }
    DrawSphere(projectile.position, projectile.radius * 2.25f, Fade(core, 0.18f));
    DrawSphere(projectile.position, projectile.radius * 1.30f, core);
    DrawSphere(projectile.position, projectile.radius * 0.58f, RAYWHITE);
    if (!projectile.hostile) {
        const Vector3 right = FacingRight(direction);
        for (int i = 0; i < (reducedMotion ? 1 : 3); ++i) {
            const float angle = time * 11.0f + i * 2.0f * kPi / 3.0f;
            const Vector3 bead{
                projectile.position.x + right.x * std::cos(angle) * projectile.radius * 2.1f,
                projectile.position.y + std::sin(angle) * projectile.radius * 2.1f,
                projectile.position.z + right.z * std::cos(angle) * projectile.radius * 2.1f
            };
            DrawSphere(bead, projectile.radius * 0.28f, accent);
        }
    }
}

void DrawEnemyHitVfx(const EnemyState& enemy, bool reducedMotion,
                     bool highContrast) {
    if (enemy.hitFlash <= 0.0f) return;
    float height = 1.0f;
    float width = 1.0f;
    EnemyBarMetrics(enemy.archetype, height, width);
    if (enemy.archetype == EnemyArchetype::VoidSovereign) {
        height *= EnemyModelScale(enemy);
        width *= EnemyModelScale(enemy);
    }
    const float phase = 1.0f - Saturate(enemy.hitFlash);
    const Color burstColor = highContrast ? Color{255, 255, 255, 255}
        : (enemy.burnTimer > 0.0f ? Color{255, 116, 49, 255}
                                  : Color{232, 160, 255, 255});
    DrawImpactBurst({enemy.position.x, height * 0.52f, enemy.position.z},
                    burstColor,
                    phase, std::clamp(width * 0.48f, 0.50f, 2.20f),
                    reducedMotion ? 4 : (IsBoss(enemy.archetype) ? 12 : 8));
}

Camera3D CombatCamera(const SurvivalController& controller, float time) {
    Camera3D camera = controller.GetCamera();
    if (controller.IsReducedMotion()) return camera;

    const PlayerState& player = controller.GetPlayer();
    const float progress = AnimationProgress(player);
    float amplitude = 0.0f;
    if (player.animation == PlayerAnimation::BasicAttack)
        amplitude = (player.character == CharacterId::Knight ? 0.095f : 0.035f)
                  * SmoothPulse(Saturate((progress - 0.28f) / 0.55f));
    else if (player.animation == PlayerAnimation::SkillOne)
        amplitude = (player.character == CharacterId::Knight ? 0.075f : 0.105f)
                  * SmoothPulse(progress);
    else if (player.animation == PlayerAnimation::SkillTwo)
        amplitude = (player.character == CharacterId::Knight ? 0.15f : 0.12f)
                  * SmoothPulse(progress);
    else if (player.animation == PlayerAnimation::Ultimate)
        amplitude = (player.character == CharacterId::Knight ? 0.26f : 0.22f)
                  * SmoothPulse(Saturate((progress - 0.20f) / 0.72f));
    else if (player.animation == PlayerAnimation::Dash)
        amplitude = 0.06f * (1.0f - progress);
    else if (player.animation == PlayerAnimation::Hurt)
        amplitude = 0.18f * (1.0f - progress);

    // Controller feedback is emitted exactly at the authored contact frame.
    // Blend it with the softer anticipation motion above so the strongest
    // camera impulse happens on impact rather than throughout the whole clip.
    if (controller.GetCameraShakeTimer() > 0.0f)
        amplitude = std::max(amplitude,
                             controller.GetCameraShakeIntensity() * 0.24f);

    float strongestHit = 0.0f;
    for (const EnemyState& enemy : controller.GetEnemies())
        if (enemy.active) strongestHit = std::max(strongestHit, enemy.hitFlash);
    amplitude = std::max(amplitude, strongestHit * 0.045f);
    if (amplitude <= 0.001f) return camera;

    Vector3 view = Vector3Subtract(camera.target, camera.position);
    view.y = 0.0f;
    view = HorizontalFacing(view);
    const Vector3 right = FacingRight(view);
    const float horizontal = std::sin(time * 91.7f) * amplitude;
    const float vertical = std::sin(time * 117.1f + 1.2f) * amplitude * 0.58f;
    const Vector3 offset{right.x * horizontal, vertical, right.z * horizontal};
    camera.position = Vector3Add(camera.position, offset);
    camera.target = Vector3Add(camera.target, Vector3Scale(offset, 0.35f));
    return camera;
}

} // namespace

SurvivalView& SurvivalView::GetInstance() {
    static SurvivalView instance;
    return instance;
}

bool SurvivalView::Init() {
    if (m_initialized) return true;
    m_font = LoadFont("assets/fonts/game_font.ttf");
    const auto loadVfx = [](Texture2D& texture, const char* path) {
        if (!FileExists(path)) {
            TraceLog(LOG_WARNING,
                     "SURVIVAL3D: Combat VFX missing; using procedural fallback: %s",
                     path);
            return;
        }
        texture = LoadTexture(path);
        if (texture.id != 0)
            SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);
    };
    loadVfx(m_knightSwordArcTexture,
            "assets/survival3d/textures/vfx/knight_sword_arc_v2.png");
    loadVfx(m_knightGuardTexture,
            "assets/survival3d/textures/vfx/knight_guard_crest_v1.png");
    loadVfx(m_knightRushTexture,
            "assets/survival3d/textures/vfx/knight_shield_rush_impact_v1.png");
    loadVfx(m_knightStormTexture,
            "assets/survival3d/textures/vfx/knight_blade_storm_v1.png");
    loadVfx(m_dashStreakTexture,
            "assets/survival3d/textures/vfx/hero_dash_streak_v1.png");
    loadVfx(m_mageSigilTexture,
            "assets/survival3d/textures/vfx/mage_arcane_sigil_v1.png");
    loadVfx(m_mageBoltTexture,
            "assets/survival3d/textures/vfx/mage_arcane_bolt_v1.png");
    loadVfx(m_mageFrostTexture,
            "assets/survival3d/textures/vfx/mage_frost_nova_v1.png");
    loadVfx(m_mageGravityTexture,
            "assets/survival3d/textures/vfx/mage_gravity_vortex_v1.png");
    loadVfx(m_mageUltimateTexture,
            "assets/survival3d/textures/vfx/mage_astral_tempest_v1.png");
    loadVfx(m_arenaBackdropTexture,
            "assets/survival3d/textures/environment/aegis_rift_void_panorama_v1.png");
    loadVfx(m_upgradeCardFrameTexture,
            "assets/survival3d/textures/ui/aegis_rift_upgrade_card_v1.png");
    loadVfx(m_resultLedgerFrameTexture,
            "assets/survival3d/textures/ui/aegis_rift_result_ledger_v1.png");
    const char* arenaModelPath =
        "assets/survival3d/models/environment/aegis_rift_arena_v1.glb";
    if (FileExists(arenaModelPath)) {
        m_arenaEnvironmentModel = LoadModel(arenaModelPath);
        m_arenaEnvironmentReady = m_arenaEnvironmentModel.meshCount > 0;
        if (m_arenaEnvironmentReady)
            TraceLog(LOG_INFO, "SURVIVAL3D: Loaded Aegis Rift arena: %s",
                     arenaModelPath);
    } else {
        TraceLog(LOG_WARNING,
                 "SURVIVAL3D: Aegis Rift arena missing; using fallback geometry: %s",
                 arenaModelPath);
    }
    const char* modelPaths[] = {
        "assets/survival3d/models/step7/actors/knight_named_actions.glb",
        "assets/survival3d/models/step7/actors/magic_caster_named_actions.glb",
        "assets/survival3d/models/step7/actors/riftling_named_actions.glb",
        "assets/survival3d/models/step7/actors/hex_archer_named_actions.glb",
        "assets/survival3d/models/step7/actors/obsidian_brute_named_actions.glb",
        "assets/survival3d/models/step7/actors/brood_warden_named_actions.glb",
        "assets/survival3d/models/step7/actors/hexeye_artillerist_named_actions.glb",
        "assets/survival3d/models/step7/actors/ironroot_colossus_named_actions.glb",
        "assets/survival3d/models/step7/actors/eclipse_chimera_named_actions.glb",
        "assets/survival3d/models/step7/actors/void_sovereign_named_actions.glb"
    };
    for (std::size_t i = 0; i < m_animatedModels.size(); ++i)
        LoadAnimatedModel(i, modelPaths[i]);
    const char* weaponPaths[] = {
        "assets/survival3d/models/weapons/knight_greatsword.glb",
        "assets/survival3d/models/weapons/magic_caster_staff.glb",
        "assets/survival3d/models/enemies/hex_archer_bow.glb"
    };
    for (std::size_t i = 0; i < m_weaponModels.size(); ++i)
        LoadWeaponModel(i, weaponPaths[i]);
    const char* skillModelPaths[] = {
        "assets/survival3d/models/skills/knight_violet_edge.glb",
        "assets/survival3d/models/skills/knight_aegis_counter_v2.glb",
        "assets/survival3d/models/skills/knight_shield_rush_v2.glb",
        "assets/survival3d/models/skills/knight_bastion_breaker_v2.glb",
        "assets/survival3d/models/skills/knight_steel_step.glb",
        "assets/survival3d/models/skills/mage_arc_bolt_v2.glb",
        "assets/survival3d/models/skills/mage_frost_ring.glb",
        "assets/survival3d/models/skills/mage_gravity_well.glb",
        "assets/survival3d/models/skills/mage_astral_tempest.glb",
        "assets/survival3d/models/skills/mage_phase_blink.glb"
    };
    static_assert(std::size(skillModelPaths) == SkillModelCount,
                  "Every hero ability must have one non-VFX model asset");
    for (std::size_t i = 0; i < m_skillModels.size(); ++i)
        LoadSkillModel(i, skillModelPaths[i]);
    m_skillVfxRuntime.Reset();
    for (const Vfx::Package& package : Vfx::SkillPackages()) {
        if (!m_skillVfxRuntime.RegisterPackage(package))
            TraceLog(LOG_WARNING, "SURVIVAL3D: Invalid VFX package: %s",
                     package.debugName != nullptr ? package.debugName : "<unnamed>");
    }
    m_lastSkillVfxCueSerial = 0u;
    m_skillParticles = {};
    m_skillParticleCursor = 0u;
    m_skillParticleSerial = 1u;
    m_initialized = true;
    return true;
}

void SurvivalView::Shutdown() {
    if (!m_initialized) return;
    for (Texture2D* texture : {
            &m_knightSwordArcTexture, &m_knightGuardTexture,
            &m_knightRushTexture, &m_knightStormTexture,
            &m_dashStreakTexture,
            &m_mageSigilTexture, &m_mageBoltTexture,
            &m_mageFrostTexture, &m_mageGravityTexture,
            &m_mageUltimateTexture, &m_arenaBackdropTexture,
            &m_upgradeCardFrameTexture, &m_resultLedgerFrameTexture}) {
        if (texture->id != 0) UnloadTexture(*texture);
        *texture = {};
    }
    if (m_font.texture.id != 0) UnloadFont(m_font);
    UnloadSkillModels();
    UnloadWeaponModels();
    UnloadAnimatedModels();
    if (m_arenaEnvironmentReady || m_arenaEnvironmentModel.meshCount > 0)
        UnloadModel(m_arenaEnvironmentModel);
    m_arenaEnvironmentModel = {};
    m_arenaEnvironmentReady = false;
    m_knightBasicTrail = {};
    m_skillVfxRuntime.StopAll();
    m_lastSkillVfxCueSerial = 0u;
    m_skillParticles = {};
    m_skillParticleCursor = 0u;
    m_skillParticleSerial = 1u;
    m_font = {};
    m_initialized = false;
}

bool SurvivalView::LoadAnimatedModel(std::size_t slot, const char* path) {
    if (slot >= m_animatedModels.size()) return false;
    if (!FileExists(path)) {
        TraceLog(LOG_WARNING, "SURVIVAL3D: Animated model missing, using fallback: %s", path);
        return false;
    }
    AnimatedModelAsset& asset = m_animatedModels[slot];
    asset.model = LoadModel(path);
    asset.animations = LoadModelAnimations(path, &asset.animationCount);
    asset.ready = asset.model.meshCount > 0 && asset.animations != nullptr
               && asset.animationCount >= kActorClipCounts[slot];
    for (int action = 0; asset.ready && action < kActorClipCounts[slot]; ++action) {
        bool found = false;
        for (int animation = 0; animation < asset.animationCount; ++animation) {
            if (TextIsEqual(asset.animations[animation].name,
                            kActorClipNames[slot][action])
                && IsModelAnimationValid(asset.model, asset.animations[animation])) {
                found = true;
                break;
            }
        }
        if (!found) {
            TraceLog(LOG_WARNING, "SURVIVAL3D: Required named action missing: %s :: %s",
                     path, kActorClipNames[slot][action]);
            asset.ready = false;
        }
    }
    if (asset.ready) asset.weaponSocketBone = FindWeaponSocketBone(asset.model);
    if (!asset.ready)
        TraceLog(LOG_WARNING, "SURVIVAL3D: Invalid animated model: %s", path);
    else {
        TraceLog(LOG_INFO, "SURVIVAL3D: Loaded %s (%i bones, %i named actions)", path,
                 asset.model.skeleton.boneCount, asset.animationCount);
        if ((slot < 2 || slot == kHexArcherModelSlot) && asset.weaponSocketBone < 0)
            TraceLog(LOG_WARNING, "SURVIVAL3D: Modular actor has no weapon socket joint: %s", path);
    }
    return asset.ready;
}

bool SurvivalView::LoadWeaponModel(std::size_t slot, const char* path) {
    if (slot >= m_weaponModels.size()) return false;
    if (!FileExists(path)) {
        TraceLog(LOG_WARNING, "SURVIVAL3D: Optional weapon model missing: %s", path);
        return false;
    }
    WeaponModelAsset& asset = m_weaponModels[slot];
    asset.model = LoadModel(path);
    asset.ready = asset.model.meshCount > 0;
    if (asset.ready) {
        bool haveBounds = false;
        BoundingBox combined{};
        for (int meshIndex = 0; meshIndex < asset.model.meshCount; ++meshIndex) {
            const BoundingBox bounds = GetMeshBoundingBox(asset.model.meshes[meshIndex]);
            if (!haveBounds) {
                combined = bounds;
                haveBounds = true;
            } else {
                combined.min.x = std::min(combined.min.x, bounds.min.x);
                combined.min.y = std::min(combined.min.y, bounds.min.y);
                combined.min.z = std::min(combined.min.z, bounds.min.z);
                combined.max.x = std::max(combined.max.x, bounds.max.x);
                combined.max.y = std::max(combined.max.y, bounds.max.y);
                combined.max.z = std::max(combined.max.z, bounds.max.z);
            }
        }
        if (haveBounds) {
            const Vector3 extent = Vector3Subtract(combined.max, combined.min);
            int axis = 0;
            if (extent.y > extent.x && extent.y >= extent.z) axis = 1;
            else if (extent.z > extent.x && extent.z > extent.y) axis = 2;
            const float low = axis == 0 ? combined.min.x
                            : (axis == 1 ? combined.min.y : combined.min.z);
            const float high = axis == 0 ? combined.max.x
                             : (axis == 1 ? combined.max.y : combined.max.z);
            const float tipCoordinate = std::abs(high) >= std::abs(low) ? high : low;
            const float span = axis == 0 ? extent.x : (axis == 1 ? extent.y : extent.z);
            if (span > 0.45f && std::abs(tipCoordinate) > 0.30f) {
                if (axis == 0) {
                    asset.bladeRootLocal.x = tipCoordinate * 0.14f;
                    asset.bladeTipLocal.x = tipCoordinate * 0.96f;
                } else if (axis == 1) {
                    asset.bladeRootLocal.y = tipCoordinate * 0.14f;
                    asset.bladeTipLocal.y = tipCoordinate * 0.96f;
                } else {
                    asset.bladeRootLocal.z = tipCoordinate * 0.14f;
                    asset.bladeTipLocal.z = tipCoordinate * 0.96f;
                }
                asset.bladeSegmentValid = true;
            }
        }
    }
    if (!asset.ready)
        TraceLog(LOG_WARNING, "SURVIVAL3D: Invalid weapon model: %s", path);
    else
        TraceLog(LOG_INFO, "SURVIVAL3D: Loaded weapon model: %s", path);
    return asset.ready;
}

bool SurvivalView::LoadSkillModel(std::size_t slot, const char* path) {
    if (slot >= m_skillModels.size()) return false;
    if (!FileExists(path)) {
        TraceLog(LOG_WARNING,
                 "SURVIVAL3D: Required 3D skill asset missing; using procedural fallback: %s",
                 path);
        return false;
    }
    SkillModelAsset& asset = m_skillModels[slot];
    asset.model = LoadModel(path);
    asset.animations = LoadModelAnimations(path, &asset.animationCount);
    for (int animation = 0; animation < asset.animationCount; ++animation) {
        if (TextIsEqual(asset.animations[animation].name, "Activate")
            && IsModelAnimationValid(asset.model, asset.animations[animation])) {
            asset.activateAnimation = animation;
            break;
        }
    }
    asset.ready = asset.model.meshCount > 0 && asset.activateAnimation >= 0;
    if (asset.ready)
        TraceLog(LOG_INFO, "SURVIVAL3D: Loaded 3D skill asset: %s", path);
    else
        TraceLog(LOG_WARNING, "SURVIVAL3D: Invalid 3D skill asset: %s", path);
    return asset.ready;
}

void SurvivalView::UnloadAnimatedModels() {
    for (AnimatedModelAsset& asset : m_animatedModels) {
        if (asset.animations) UnloadModelAnimations(asset.animations, asset.animationCount);
        if (asset.model.meshCount > 0) UnloadModel(asset.model);
        asset = {};
    }
}

void SurvivalView::UnloadWeaponModels() {
    for (WeaponModelAsset& asset : m_weaponModels) {
        if (asset.model.meshCount > 0) UnloadModel(asset.model);
        asset = {};
    }
}

void SurvivalView::UnloadSkillModels() {
    for (SkillModelAsset& asset : m_skillModels) {
        if (asset.animations)
            UnloadModelAnimations(asset.animations, asset.animationCount);
        if (asset.model.meshCount > 0) UnloadModel(asset.model);
        asset = {};
    }
}

void SurvivalView::RenderSkillGeometry(const SurvivalController& controller,
                                       float time) const {
    const PlayerState& player = controller.GetPlayer();
    const Vector3 forward = HorizontalFacing(player.facing);
    const Vector3 right = FacingRight(forward);
    const float facingDegrees = std::atan2(forward.x, forward.z) * 57.2957795f;
    const float progress = AnimationProgress(player);

    const auto draw = [this](std::size_t slot, Vector3 position,
                             Vector3 axis, float angle, float scale,
                             float animationPhase, Color tint = WHITE) {
        if (slot >= m_skillModels.size() || !m_skillModels[slot].ready) return;
        const SkillModelAsset& asset = m_skillModels[slot];
        const ModelAnimation& animation = asset.animations[asset.activateAnimation];
        const float frame = std::clamp(animationPhase, 0.0f, 1.0f)
                          * static_cast<float>(std::max(0, animation.keyframeCount - 1));
        UpdateModelAnimation(asset.model, animation, frame);
        DrawModelEx(asset.model, position, axis, angle,
                    {scale, scale, scale}, tint);
    };

    // Actor-attached geometry is displayed through the complete authored
    // animation window. World-field geometry below survives for the gameplay
    // lifetime of the ability.
    if (player.character == CharacterId::Knight) {
        // J/Basic already has the real sword, a socket-sampled ribbon and the
        // Violet Edge VFX package. The extra crescent GLB duplicated the blade
        // at some camera angles, so it is intentionally not rendered.
        if (player.animation == PlayerAnimation::SkillOne
            || player.guardTimer > 0.0f) {
            draw(KnightAegisCounter,
                 {player.position.x + forward.x * 0.72f, 1.05f,
                  player.position.z + forward.z * 0.72f},
                 {0.0f, 1.0f, 0.0f}, facingDegrees, 1.0f, progress);
        } else if (player.animation == PlayerAnimation::SkillTwo
                   || player.rushTimer > 0.0f) {
            draw(KnightShieldRush,
                 {player.position.x + forward.x * 0.92f, 0.72f,
                  player.position.z + forward.z * 0.92f},
                 {0.0f, 1.0f, 0.0f}, facingDegrees, 1.0f, progress);
        } else if (player.animation == PlayerAnimation::Dash) {
            draw(KnightSteelStep,
                 {player.position.x - forward.x * 0.78f, 0.04f,
                  player.position.z - forward.z * 0.78f},
                 {0.0f, 1.0f, 0.0f}, facingDegrees, 0.92f, progress,
                 Fade(WHITE, 0.72f));
        }
        if (controller.GetSkillFxTimer() > 0.0f
            && controller.GetSkillFxRadius() > 4.5f) {
            const Vector3 center = controller.GetSkillFxCenter();
            draw(KnightBastionBreaker, {center.x, 0.055f, center.z},
                 {0.0f, 1.0f, 0.0f}, time * 24.0f,
                 std::max(0.75f, controller.GetSkillFxRadius() / 5.5f),
                 1.0f - Saturate(controller.GetSkillFxTimer() / 0.70f));
        }
    } else {
        if (player.animation == PlayerAnimation::BasicAttack) {
            draw(MageArcBolt,
                 {player.position.x + forward.x * (0.68f + progress * 0.55f)
                                      + right.x * 0.22f,
                  1.34f,
                  player.position.z + forward.z * (0.68f + progress * 0.55f)
                                      + right.z * 0.22f},
                 {0.0f, 1.0f, 0.0f}, facingDegrees, 0.62f, progress);
        } else if (player.animation == PlayerAnimation::Dash) {
            draw(MagePhaseBlink,
                 {player.position.x - forward.x * 0.86f, 0.04f,
                  player.position.z - forward.z * 0.86f},
                 {0.0f, 1.0f, 0.0f}, facingDegrees + time * 35.0f,
                 0.94f, progress, Fade(WHITE, 0.72f));
        }

        if (controller.GetGravityWellTimer() > 0.0f) {
            const Vector3 center = controller.GetGravityWellCenter();
            const float pulse = controller.IsReducedMotion()
                ? 1.0f : 0.92f + std::sin(time * 4.0f) * 0.08f;
            draw(MageGravityWell, {center.x, 0.42f, center.z},
                 {0.0f, 1.0f, 0.0f}, -time * 42.0f, pulse,
                 std::fmod(std::max(time, 0.0f) * 0.65f, 1.0f));
        }
        if (controller.GetSkillFxTimer() > 0.0f) {
            const Vector3 center = controller.GetSkillFxCenter();
            const float radius = controller.GetSkillFxRadius();
            if (radius > 6.25f) {
                draw(MageAstralTempest, {center.x, 0.06f, center.z},
                     {0.0f, 1.0f, 0.0f}, time * 28.0f,
                     std::max(0.8f, radius / 8.0f),
                     1.0f - Saturate(controller.GetSkillFxTimer() / 0.82f));
            } else if (controller.GetGravityWellTimer() <= 0.0f
                       && radius > 2.5f) {
                draw(MageFrostRing, {center.x, 0.055f, center.z},
                     {0.0f, 1.0f, 0.0f}, -time * 18.0f,
                     std::max(0.8f, radius / 4.0f),
                     1.0f - Saturate(controller.GetSkillFxTimer() / 0.48f));
            }
        }
    }

    // Arc Bolt remains a real mesh for its entire projectile lifetime; the
    // texture trail rendered elsewhere is presentation layered on top of it.
    for (const ProjectileState& projectile : controller.GetProjectiles()) {
        if (!projectile.active || projectile.hostile) continue;
        Vector3 direction = HorizontalFacing(projectile.velocity);
        const float angle = std::atan2(direction.x, direction.z) * 57.2957795f;
        draw(MageArcBolt, projectile.position, {0.0f, 1.0f, 0.0f},
             angle + time * 120.0f, std::max(0.45f, projectile.radius * 2.6f),
             std::fmod(std::max(time, 0.0f) * 1.8f, 1.0f));
    }
}

WeaponWorldPose SurvivalView::DrawAnimatedModel(std::size_t slot, Vector3 position,
                                                Vector3 facing,
                                                const AnimationPlayback& poseA,
                                                const AnimationPlayback& poseB,
                                                float poseBlend,
                                                float scale, Color tint,
                                                WeaponId weapon) const {
    WeaponWorldPose weaponPose{};
    if (slot >= m_animatedModels.size()) return weaponPose;
    const AnimatedModelAsset& asset = m_animatedModels[slot];
    if (!asset.ready) return weaponPose;

    const auto findAnimation = [&asset](const char* name) -> const ModelAnimation* {
        if (name != nullptr) {
            for (int index = 0; index < asset.animationCount; ++index) {
                if (TextIsEqual(asset.animations[index].name, name))
                    return &asset.animations[index];
            }
        }
        return asset.animationCount > 0 ? &asset.animations[0] : nullptr;
    };
    const auto sampleFrame = [](const ModelAnimation& animation,
                                const AnimationPlayback& playback) {
        const float last = static_cast<float>(
            std::max(0, animation.keyframeCount - 1));
        if (!playback.looping)
            return std::clamp(playback.position, 0.0f, 1.0f) * last;
        const float count = static_cast<float>(
            std::max(1, animation.keyframeCount));
        return std::fmod(std::max(0.0f, playback.position)
                         * 60.0f * std::max(0.0f, playback.speed), count);
    };

    const ModelAnimation* animationA = findAnimation(poseA.clipName);
    const ModelAnimation* animationB = findAnimation(poseB.clipName);
    if (animationA == nullptr || animationB == nullptr) return weaponPose;
    const float frameA = sampleFrame(*animationA, poseA);
    const float frameB = sampleFrame(*animationB, poseB);
    poseBlend = std::clamp(poseBlend, 0.0f, 1.0f);
    if (poseBlend > 0.001f && poseBlend < 0.999f) {
        UpdateModelAnimationEx(asset.model, *animationA, frameA,
                               *animationB, frameB, poseBlend);
    } else if (poseBlend >= 0.999f) {
        UpdateModelAnimation(asset.model, *animationB, frameB);
    } else {
        UpdateModelAnimation(asset.model, *animationA, frameA);
    }
    const float facingAngle = std::atan2(facing.x, facing.z) * 57.2957795f;
    DrawModelEx(asset.model, position, {0.0f, 1.0f, 0.0f}, facingAngle,
                {scale, scale, scale}, tint);

    int weaponSlot = WeaponModelSlot(weapon);
    if (slot == kHexArcherModelSlot)
        weaponSlot = static_cast<int>(kHexArcherBowSlot);
    if (weaponSlot < 0
        || static_cast<std::size_t>(weaponSlot) >= m_weaponModels.size()
        || asset.weaponSocketBone < 0
        || asset.weaponSocketBone >= asset.model.skeleton.boneCount
        || asset.model.currentPose == nullptr) return weaponPose;
    const Matrix socketWorld = SocketWorldMatrix(asset.model, asset.weaponSocketBone,
                                                 position, facing, scale);
    const WeaponModelAsset& weaponAsset = m_weaponModels[(std::size_t)weaponSlot];
    if (!weaponAsset.ready) {
        if (slot == kHexArcherModelSlot)
            DrawFallbackHexArcherBow(socketWorld, tint);
        return weaponPose;
    }

    Model drawWeapon = weaponAsset.model;
    Matrix weaponWorld = MatrixMultiply(
        MatrixMultiply(
            drawWeapon.transform, weaponAsset.gripOffset),
        socketWorld);
    if (weapon == WeaponId::KnightGreatsword
        && weaponAsset.bladeSegmentValid) {
        const auto rotateAroundGrip = [&weaponWorld](Vector3 grip,
                                                      Vector3 currentBlade,
                                                      Vector3 desiredBlade) {
            if (Vector3LengthSqr(currentBlade) < 0.0001f
                || Vector3LengthSqr(desiredBlade) < 0.0001f) return;
            const Quaternion correction = QuaternionFromVector3ToVector3(
                Vector3Normalize(currentBlade),
                Vector3Normalize(desiredBlade));
            const Matrix aroundGrip = MatrixMultiply(
                MatrixMultiply(
                    MatrixTranslate(-grip.x, -grip.y, -grip.z),
                    QuaternionToMatrix(correction)),
                MatrixTranslate(grip.x, grip.y, grip.z));
            weaponWorld = MatrixMultiply(weaponWorld, aroundGrip);
        };

        Vector3 grip = Vector3Transform({0.0f, 0.0f, 0.0f}, weaponWorld);
        Vector3 tip = Vector3Transform(weaponAsset.bladeTipLocal, weaponWorld);
        Vector3 gripOutward{grip.x - position.x, 0.0f,
                            grip.z - position.z};
        const float gripRadius = Vector3Length(gripOutward);
        float gripOutwardLength = gripRadius;
        if (gripOutwardLength < 0.08f) {
            gripOutward = {facing.z, 0.0f, -facing.x};
            gripOutwardLength = std::max(0.001f,
                                         Vector3Length(gripOutward));
        }
        gripOutward = Vector3Scale(gripOutward,
                                   1.0f / gripOutwardLength);
        constexpr float kMinimumGripRadius = 0.42f;
        if (gripRadius < kMinimumGripRadius) {
            const float push = kMinimumGripRadius - gripRadius;
            weaponWorld = MatrixMultiply(
                weaponWorld,
                MatrixTranslate(gripOutward.x * push, 0.0f,
                                gripOutward.z * push));
            grip = Vector3Transform({0.0f, 0.0f, 0.0f}, weaponWorld);
            tip = Vector3Transform(weaponAsset.bladeTipLocal, weaponWorld);
        }
        constexpr float kBladeFloorClearance = 0.085f;
        if (tip.y < kBladeFloorClearance) {
            const Vector3 blade = Vector3Subtract(tip, grip);
            const float bladeLength = Vector3Length(blade);
            if (bladeLength > 0.20f) {
                Vector3 horizontal{blade.x, 0.0f, blade.z};
                float horizontalLength = Vector3Length(horizontal);
                if (horizontalLength < 0.001f) {
                    horizontal = {facing.x, 0.0f, facing.z};
                    horizontalLength = std::max(0.001f,
                                                Vector3Length(horizontal));
                }
                horizontal = Vector3Scale(horizontal,
                                           1.0f / horizontalLength);
                const float desiredVertical = std::clamp(
                    kBladeFloorClearance - grip.y,
                    -bladeLength * 0.94f, bladeLength * 0.94f);
                const float desiredHorizontal = std::sqrt(std::max(
                    0.0f, bladeLength * bladeLength
                        - desiredVertical * desiredVertical));
                const Vector3 desiredBlade{
                    horizontal.x * desiredHorizontal,
                    desiredVertical,
                    horizontal.z * desiredHorizontal
                };
                rotateAroundGrip(grip, blade, desiredBlade);
            }
        }

        // Camera-independent body-space guard. The blade is allowed to follow
        // every authored swing, but its horizontal direction may never point
        // back through the torso from the right-hand grip. This keeps the
        // weapon visibly outside the Knight from every orbit-camera angle.
        grip = Vector3Transform({0.0f, 0.0f, 0.0f}, weaponWorld);
        tip = Vector3Transform(weaponAsset.bladeTipLocal, weaponWorld);
        const Vector3 blade = Vector3Subtract(tip, grip);
        Vector3 bladeHorizontal{blade.x, 0.0f, blade.z};
        const float bladeHorizontalLength = Vector3Length(bladeHorizontal);
        if (bladeHorizontalLength > 0.015f) {
            const Vector3 bladeDirection = Vector3Scale(
                bladeHorizontal, 1.0f / bladeHorizontalLength);
            Vector3 outward{grip.x - position.x, 0.0f,
                            grip.z - position.z};
            float outwardLength = Vector3Length(outward);
            if (outwardLength < 0.08f) {
                outward = {facing.z, 0.0f, -facing.x};
                outwardLength = std::max(0.001f, Vector3Length(outward));
            }
            outward = Vector3Scale(outward, 1.0f / outwardLength);
            const bool stableHold = poseB.clipName != nullptr
                && (TextIsEqual(poseB.clipName, "idle")
                    || TextIsEqual(poseB.clipName, "walk_forward")
                    || TextIsEqual(poseB.clipName, "walk_backward")
                    || TextIsEqual(poseB.clipName, "run_forward")
                    || TextIsEqual(poseB.clipName, "run_backward")
                    || TextIsEqual(poseB.clipName, "strafe_left")
                    || TextIsEqual(poseB.clipName, "strafe_right"));
            constexpr float kMinimumOutwardDot = 0.62f;
            const float outwardDot = Vector3DotProduct(bladeDirection,
                                                       outward);
            if (stableHold || outwardDot < kMinimumOutwardDot) {
                Vector3 tangent = Vector3Subtract(
                    bladeDirection, Vector3Scale(outward, outwardDot));
                float tangentLength = Vector3Length(tangent);
                if (tangentLength < 0.001f) {
                    tangent = {facing.x, 0.0f, facing.z};
                    tangentLength = std::max(0.001f,
                                             Vector3Length(tangent));
                }
                tangent = Vector3Scale(tangent, 1.0f / tangentLength);
                const float outwardWeight = stableHold
                    ? 1.0f : kMinimumOutwardDot;
                const float tangentWeight = std::sqrt(
                    1.0f - outwardWeight * outwardWeight);
                const Vector3 desiredDirection = Vector3Add(
                    Vector3Scale(outward, outwardWeight),
                    Vector3Scale(tangent, tangentWeight));
                const Vector3 desiredBlade{
                    desiredDirection.x * bladeHorizontalLength,
                    blade.y,
                    desiredDirection.z * bladeHorizontalLength
                };
                rotateAroundGrip(grip, blade, desiredBlade);
            }
        }
    }
    drawWeapon.transform = weaponWorld;
    if (weaponAsset.bladeSegmentValid) {
        weaponPose.valid = true;
        weaponPose.grip = Vector3Transform({0.0f, 0.0f, 0.0f}, weaponWorld);
        weaponPose.bladeRoot = Vector3Transform(weaponAsset.bladeRootLocal, weaponWorld);
        weaponPose.bladeTip = Vector3Transform(weaponAsset.bladeTipLocal, weaponWorld);
    }
    DrawModel(drawWeapon, {0.0f, 0.0f, 0.0f}, 1.0f, tint);
    return weaponPose;
}

void SurvivalView::UpdateSkillVfxRuntime(
    const SurvivalController& controller, float frameDt) const {
    const CombatFeedbackState& feedback = controller.GetCombatFeedback();
    if (feedback.serial != 0u && feedback.serial != m_lastSkillVfxCueSerial) {
        m_lastSkillVfxCueSerial = feedback.serial;
        bool valid = false;
        const Vfx::SkillPackage skill = SkillPackageForCue(
            feedback.cue, controller.GetPlayer().character, valid);
        if (valid) {
            const float baseRadius = SkillCueBaseRadius(
                feedback.cue, controller.GetPlayer().character);
            Vfx::SpawnParameters parameters;
            parameters.position = {
                feedback.origin.x,
                feedback.origin.y + SkillCueHeight(feedback.cue),
                feedback.origin.z
            };
            parameters.forward = {
                feedback.direction.x, feedback.direction.y,
                feedback.direction.z
            };
            parameters.surfaceNormal = {0.0f, 1.0f, 0.0f};
            parameters.scale = std::clamp(
                feedback.radius / std::max(0.1f, baseRadius), 0.60f, 1.85f);
            parameters.intensityScale = std::clamp(
                0.45f + feedback.intensity * 0.70f, 0.45f, 1.15f);
            (void)m_skillVfxRuntime.Spawn(
                Vfx::GetSkillPackage(skill).id, parameters);
        }
    }

    // Clamp presentation time after a breakpoint or window drag. Gameplay
    // stays fixed-step in the controller; this only prevents a visual package
    // from skipping every layer in one render frame.
    const float safeDt = std::clamp(frameDt, 1.0f / 240.0f, 1.0f / 20.0f);
    UpdateSkillParticlePool(safeDt);
    const Vfx::FrameOutput& output = m_skillVfxRuntime.Update(safeDt);
    for (std::uint16_t index = 0; index < output.sampleCount; ++index) {
        const Vfx::LayerSample& sample = output.samples[index];
        if (sample.resolvedComponent != Vfx::Component::SecondaryParticles
            || !sample.beganThisFrame) continue;
        SpawnSkillParticleBurst(
            sample.package,
            {sample.position.x, sample.position.y, sample.position.z},
            {sample.forward.x, sample.forward.y, sample.forward.z},
            sample.radius, sample.intensity,
            controller.IsReducedMotion() ? 6 : 18);
    }
    for (std::uint16_t index = 0; index < output.cueCount; ++index) {
        const Vfx::TriggerCue& cue = output.cues[index];
        if (cue.resolvedComponent != Vfx::Component::Impact
            && cue.resolvedComponent != Vfx::Component::SecondaryParticles) continue;
        SpawnSkillParticleBurst(
            cue.package, {cue.position.x, cue.position.y, cue.position.z},
            {cue.forward.x, cue.forward.y, cue.forward.z},
            cue.radius, cue.intensity,
            controller.IsReducedMotion() ? 5 : 14);
    }
}

void SurvivalView::SpawnSkillParticleBurst(Vfx::PackageId package,
                                           Vector3 position,
                                           Vector3 forward, float radius,
                                           float intensity, int count) const {
    const SkillVfxPalette palette = PaletteForPackage(package, false);
    forward = HorizontalFacing(forward);
    const Vector3 right = FacingRight(forward);
    count = std::clamp(count, 0, 32);
    radius = std::max(0.15f, radius);
    intensity = std::clamp(intensity, 0.10f, 1.25f);
    for (int index = 0; index < count; ++index) {
        SkillParticle3D& particle = m_skillParticles[m_skillParticleCursor];
        m_skillParticleCursor = static_cast<std::uint16_t>(
            (m_skillParticleCursor + 1u) % m_skillParticles.size());

        std::uint32_t hash = (m_skillParticleSerial++ * 747796405u)
                           ^ (package * 2891336453u)
                           ^ static_cast<std::uint32_t>(index * 97 + 11);
        hash ^= hash >> 16;
        const float randomA = (hash & 0xFFFFu) / 65535.0f;
        hash = hash * 1664525u + 1013904223u;
        const float randomB = (hash & 0xFFFFu) / 65535.0f;
        hash = hash * 1664525u + 1013904223u;
        const float randomC = (hash & 0xFFFFu) / 65535.0f;
        const float angle = randomA * kPi * 2.0f;
        const float radialSpeed = (1.0f + randomB * 2.4f)
                                * std::clamp(radius * 0.42f, 0.65f, 3.4f);
        const Vector3 radial{
            std::cos(angle) * radialSpeed,
            1.1f + randomC * 2.6f + intensity * 0.7f,
            std::sin(angle) * radialSpeed
        };
        const float forwardBias = (randomB - 0.35f) * radius * 0.55f;
        const float sideBias = (randomC - 0.5f) * radius * 0.28f;
        particle.position = {
            position.x + right.x * sideBias,
            position.y + 0.04f + randomA * 0.14f,
            position.z + right.z * sideBias
        };
        particle.velocity = {
            radial.x + forward.x * forwardBias,
            radial.y,
            radial.z + forward.z * forwardBias
        };
        particle.color = (index & 1) ? palette.primary : palette.accent;
        particle.age = 0.0f;
        particle.lifetime = 0.32f + randomB * 0.48f;
        particle.size = (0.022f + randomC * 0.055f)
                      * (0.70f + intensity * 0.46f);
        particle.active = true;
    }
}

void SurvivalView::UpdateSkillParticlePool(float frameDt) const {
    for (SkillParticle3D& particle : m_skillParticles) {
        if (!particle.active) continue;
        particle.age += frameDt;
        if (particle.age >= particle.lifetime) {
            particle.active = false;
            continue;
        }
        particle.position = Vector3Add(
            particle.position, Vector3Scale(particle.velocity, frameDt));
        particle.velocity.y -= 4.2f * frameDt;
        const float damping = std::exp(-1.8f * frameDt);
        particle.velocity.x *= damping;
        particle.velocity.z *= damping;
    }
}

void SurvivalView::RenderSkillParticlePool(bool reducedMotion,
                                           bool highContrast) const {
    int rendered = 0;
    const int renderBudget = reducedMotion ? 160 : 512;
    for (const SkillParticle3D& particle : m_skillParticles) {
        if (!particle.active || rendered >= renderBudget) continue;
        ++rendered;
        const float remaining = 1.0f - particle.age
                              / std::max(0.001f, particle.lifetime);
        const Color color = highContrast
            ? (rendered & 1 ? WHITE : YELLOW) : particle.color;
        const Vector3 tail = Vector3Subtract(
            particle.position,
            Vector3Scale(particle.velocity, reducedMotion ? 0.012f : 0.028f));
        if (!reducedMotion)
            DrawGlowSegment(tail, particle.position,
                            particle.size * 0.38f, color,
                            remaining * 0.62f);
        DrawSphere(particle.position,
                   particle.size * (0.55f + remaining * 0.55f),
                   Fade(color, std::clamp(remaining * 0.90f, 0.05f, 0.90f)));
    }
}

void SurvivalView::RenderSkillVfxRuntime(bool reducedMotion,
                                         bool highContrast) const {
    const Vfx::FrameOutput& output = m_skillVfxRuntime.GetFrameOutput();
    const float time = static_cast<float>(GetTime());

    for (std::uint16_t index = 0; index < output.sampleCount; ++index) {
        const Vfx::LayerSample& sample = output.samples[index];
        const SkillVfxPalette palette = PaletteForPackage(sample.package,
                                                          highContrast);
        const Vector3 origin{sample.position.x, sample.position.y,
                             sample.position.z};
        const Vector3 forward = HorizontalFacing(
            {sample.forward.x, sample.forward.y, sample.forward.z});
        const Vector3 right = FacingRight(forward);
        const float strength = std::clamp(sample.intensity, 0.0f, 1.25f);
        const float radius = std::max(0.08f, sample.radius);

        switch (sample.resolvedComponent) {
            case Vfx::Component::SecondaryParticles: {
                // Burst particles live in the fixed-capacity pool below, so
                // their lifetime is independent of frame rate and no heap
                // allocation occurs during combat.
                break;
            }
            case Vfx::Component::Trail: {
                const int segments = reducedMotion ? 2 : 5;
                for (int segment = 0; segment < segments; ++segment) {
                    const float t0 = segment / static_cast<float>(segments);
                    const float t1 = (segment + 1) / static_cast<float>(segments);
                    const float wave0 = std::sin(time * 9.0f + segment * 1.7f)
                                      * radius * 0.055f;
                    const float wave1 = std::sin(time * 9.0f + (segment + 1) * 1.7f)
                                      * radius * 0.055f;
                    const Vector3 start{
                        origin.x - forward.x * radius * t0 + right.x * wave0,
                        origin.y + 0.05f + t0 * 0.10f,
                        origin.z - forward.z * radius * t0 + right.z * wave0
                    };
                    const Vector3 end{
                        origin.x - forward.x * radius * t1 + right.x * wave1,
                        origin.y + 0.05f + t1 * 0.10f,
                        origin.z - forward.z * radius * t1 + right.z * wave1
                    };
                    DrawGlowSegment(start, end,
                                    0.012f + strength * 0.025f,
                                    palette.primary,
                                    strength * (1.0f - t0) * 0.78f);
                }
                break;
            }
            case Vfx::Component::Glow: {
                if (radius > 2.0f) {
                    DrawCircle3D({origin.x, 0.115f, origin.z}, radius,
                                 {1.0f, 0.0f, 0.0f}, 90.0f,
                                 Fade(palette.primary,
                                      std::clamp(strength * 0.55f, 0.08f, 0.72f)));
                    if (!reducedMotion)
                        DrawGroundGlyph({origin.x, 0.11f, origin.z}, radius * 0.72f,
                                        time * 0.65f, palette.accent, 8);
                } else {
                    // Never visualize a gameplay volume with wire geometry:
                    // this glow is a compact emissive core plus an offset rune
                    // halo, while collision stays controller-only.
                    DrawSphere(origin, radius * (0.30f + strength * 0.10f),
                               Fade(palette.primary,
                                    std::clamp(strength * 0.22f, 0.05f, 0.28f)));
                    DrawRuneRing(origin, forward,
                                 radius * (0.48f + strength * 0.10f),
                                 time * 1.7f, palette.accent,
                                 reducedMotion ? 8 : 14, 0.012f);
                }
                break;
            }
            case Vfx::Component::Impact:
                DrawImpactBurst(origin, palette.accent,
                                sample.normalizedTime, radius,
                                reducedMotion ? 4 : 11);
                break;
            case Vfx::Component::MainShape:
                // Authored GLB geometry is drawn by RenderSkillGeometry.
                break;
            case Vfx::Component::Distortion:
            case Vfx::Component::Light:
            case Vfx::Component::SpatialSound:
            case Vfx::Component::CameraShake:
            case Vfx::Component::None:
                break;
        }
    }

    RenderSkillParticlePool(reducedMotion, highContrast);

    for (std::uint16_t index = 0; index < output.cueCount; ++index) {
        const Vfx::TriggerCue& cue = output.cues[index];
        if (cue.resolvedComponent != Vfx::Component::Impact) continue;
        const SkillVfxPalette palette = PaletteForPackage(cue.package,
                                                          highContrast);
        DrawImpactBurst({cue.position.x, cue.position.y, cue.position.z},
                        palette.accent, 0.08f, std::max(0.12f, cue.radius),
                        reducedMotion ? 4 : 12);
    }
}

void SurvivalView::DrawArenaBackdrop(float time, bool reducedMotion) const {
    const float sw = static_cast<float>(GetScreenWidth());
    const float sh = static_cast<float>(GetScreenHeight());
    if (m_arenaBackdropTexture.id == 0 || sw <= 0.0f || sh <= 0.0f) {
        DrawRectangleGradientV(0, 0, static_cast<int>(sw), static_cast<int>(sh),
                               Color{10, 5, 29, 255}, Color{2, 3, 12, 255});
        return;
    }

    const float textureW = static_cast<float>(m_arenaBackdropTexture.width);
    const float textureH = static_cast<float>(m_arenaBackdropTexture.height);
    const float screenAspect = sw / std::max(1.0f, sh);
    float sourceH = textureH;
    float sourceW = sourceH * screenAspect;
    if (sourceW > textureW) {
        sourceW = textureW;
        sourceH = sourceW / screenAspect;
    }
    // Deliberately crop a little more than aspect-fill so the panorama can
    // drift without exposing an edge. Reduced Motion keeps a fixed crop.
    sourceW *= 0.88f;
    sourceH *= 0.92f;
    const float maxX = std::max(0.0f, textureW - sourceW);
    const float maxY = std::max(0.0f, textureH - sourceH);
    const float drift = reducedMotion ? 0.5f : 0.5f + std::sin(time * 0.045f) * 0.42f;
    const float lift = reducedMotion ? 0.52f : 0.52f + std::sin(time * 0.031f + 1.8f) * 0.12f;
    const Rectangle source{maxX * drift, maxY * lift, sourceW, sourceH};
    DrawTexturePro(m_arenaBackdropTexture, source, {0, 0, sw, sh}, {}, 0.0f, WHITE);

    // A second, faint counter-moving crop creates depth without a video codec
    // or per-frame image allocation.
    if (!reducedMotion) {
        const Rectangle hazeSource{maxX * (1.0f - drift), maxY * 0.44f,
                                   sourceW, sourceH};
        DrawTexturePro(m_arenaBackdropTexture, hazeSource,
                       {-sw * 0.015f, -sh * 0.015f, sw * 1.03f, sh * 1.03f},
                       {}, 0.0f, Fade(Color{152, 111, 255, 255}, 0.13f));
    }

    // Deterministic near-star layer. It scrolls faster than the panorama and
    // therefore reads as an animated void even while gameplay is paused.
    const int moteCount = reducedMotion ? 18 : 48;
    for (int index = 0; index < moteCount; ++index) {
        std::uint32_t hash = 0x9E3779B9u * static_cast<std::uint32_t>(index + 7);
        hash ^= hash >> 16;
        const float seedX = (hash & 0xFFFFu) / 65535.0f;
        hash = hash * 1664525u + 1013904223u;
        const float seedY = (hash & 0xFFFFu) / 65535.0f;
        const float speed = 4.0f + (index % 7) * 1.7f;
        float x = seedX * sw;
        if (!reducedMotion) x = std::fmod(x + time * speed, sw + 24.0f) - 12.0f;
        const float y = seedY * sh * 0.82f;
        const float pulse = reducedMotion ? 0.55f
            : 0.45f + 0.32f * std::sin(time * (0.8f + (index % 5) * 0.11f)
                                       + index * 1.73f);
        DrawCircleV({x, y}, 0.7f + (index % 4) * 0.32f,
                    Fade(index % 3 == 0 ? Color{171, 102, 255, 255}
                                        : Color{128, 224, 255, 255},
                         std::clamp(pulse, 0.12f, 0.78f)));
    }
    DrawRectangleGradientV(0, 0, static_cast<int>(sw), static_cast<int>(sh),
                           Fade(BLACK, 0.02f), Fade(Color{3, 2, 14, 255}, 0.38f));
}

void SurvivalView::RenderArena(float time, bool reducedMotion,
                               bool showroom) const {
    if (m_arenaEnvironmentReady) {
        DrawModel(m_arenaEnvironmentModel, {0.0f, 0.0f, 0.0f}, 1.0f, WHITE);
    } else {
        DrawPlane({0.0f, -0.04f, 0.0f}, {42.0f, 42.0f}, Color{22, 18, 35, 255});
        const Color wall{54, 36, 76, 255};
        DrawCube({0.0f, 0.65f, -20.6f}, 42.0f, 1.3f, 1.2f, wall);
        DrawCube({0.0f, 0.65f, 20.6f}, 42.0f, 1.3f, 1.2f, wall);
        DrawCube({-20.6f, 0.65f, 0.0f}, 1.2f, 1.3f, 42.0f, wall);
        DrawCube({20.6f, 0.65f, 0.0f}, 1.2f, 1.3f, 42.0f, wall);
    }

    const float pulse = reducedMotion ? 0.72f : 0.62f + std::sin(time * 2.4f) * 0.16f;
    const Color violet{190, 86, 255, 255};
    const Color cyan{80, 218, 255, 255};
    DrawCircle3D({0.0f, 0.13f, 0.0f}, 4.18f, {1, 0, 0}, 90.0f,
                 Fade(violet, pulse));
    DrawCircle3D({0.0f, 0.14f, 0.0f}, 9.40f, {1, 0, 0}, 90.0f,
                 Fade(cyan, pulse * 0.58f));

    const Vector3 portals[] = {
        {-18.4f, 0.06f, 0.0f}, {18.4f, 0.06f, 0.0f},
        {0.0f, 0.06f, -18.4f}, {0.0f, 0.06f, 18.4f}
    };
    for (int index = 0; index < 4; ++index) {
        const Vector3 portal = portals[index];
        const float ring = 1.22f + (reducedMotion ? 0.0f
            : std::sin(time * 2.9f + index) * 0.12f);
        DrawCircle3D(portal, ring, {1, 0, 0}, 90.0f, Fade(violet, pulse));
        DrawCircle3D({portal.x, portal.y + 0.01f, portal.z}, ring * 0.68f,
                     {1, 0, 0}, 90.0f, Fade(cyan, pulse * 0.85f));
        if (!reducedMotion && !showroom) {
            for (int mote = 0; mote < 4; ++mote) {
                const float angle = time * (0.7f + mote * 0.12f)
                                  + mote * kPi * 0.5f + index;
                DrawSphere({portal.x + std::cos(angle) * ring * 0.72f,
                            0.20f + mote * 0.12f,
                            portal.z + std::sin(angle) * ring * 0.72f},
                           0.035f, Fade(mote & 1 ? violet : cyan, 0.74f));
            }
        }
    }

    const int arenaMotes = reducedMotion || showroom ? 10 : 28;
    for (int index = 0; index < arenaMotes; ++index) {
        const float angle = index * 2.3999632f + (reducedMotion ? 0.0f : time * 0.08f);
        const float radius = 5.5f + (index % 9) * 1.25f;
        const float y = 0.25f + (index % 6) * 0.31f
                      + (reducedMotion ? 0.0f : std::sin(time * 0.7f + index) * 0.12f);
        DrawSphere({std::cos(angle) * radius, y, std::sin(angle) * radius},
                   0.025f + (index % 3) * 0.012f,
                   Fade(index & 1 ? violet : cyan, 0.58f));
    }
}

void SurvivalView::RenderPlayer(const PlayerState& player, bool reducedMotion,
                                bool highContrast) const {
    const float flash = player.invulnerableTimer > 0.0f
        ? 0.55f + 0.45f * std::sin((float)GetTime() * 35.0f) : 0.0f;

    const float shadowScale = std::clamp(1.0f - player.position.y * 0.17f,
                                         0.48f, 1.0f);
    DrawCircle3D({player.position.x, 0.035f, player.position.z},
                 0.48f * shadowScale, {1.0f, 0.0f, 0.0f}, 90.0f,
                 Fade(BLACK, 0.42f * shadowScale));

    const std::size_t modelSlot = ModelSlot(player.character);
    if (m_animatedModels[modelSlot].ready) {
        const Color tint = flash > 0.5f ? Color{245, 225, 255, 255} : WHITE;
        Vector3 renderPosition = player.position;
        renderPosition.y += player.runtimeIk.pelvisOffset;

        AnimationPlayback poseA{};
        AnimationPlayback poseB{};
        float poseBlend = 1.0f;
        const auto chooseLocomotion = [&](float clock,
                                          AnimationPlayback& first,
                                          AnimationPlayback& second,
                                          float& blend) {
            const bool walk = player.locomotionBlend.normalizedSpeed > 0.001f
                           && player.locomotionBlend.normalizedSpeed < 0.62f;
            const float weights[5] = {
                player.locomotionBlend.idle,
                player.locomotionBlend.forward,
                player.locomotionBlend.backward,
                player.locomotionBlend.left,
                player.locomotionBlend.right
            };
            const char* names[5] = {
                kActorClipNames[modelSlot][0],
                kActorClipNames[modelSlot][walk ? 1 : 3],
                kActorClipNames[modelSlot][walk ? 2 : 4],
                kActorClipNames[modelSlot][5],
                kActorClipNames[modelSlot][6]
            };
            int best = 0;
            int runnerUp = 0;
            for (int index = 1; index < 5; ++index) {
                if (weights[index] > weights[best]) {
                    runnerUp = best;
                    best = index;
                } else if (runnerUp == best || weights[index] > weights[runnerUp]) {
                    runnerUp = index;
                }
            }
            const float sum = weights[best] + weights[runnerUp];
            first = {names[best], clock, 1.0f, true};
            second = {names[runnerUp], clock, 1.0f, true};
            blend = sum > 0.0001f ? weights[runnerUp] / sum : 0.0f;
        };

        if (player.animation == PlayerAnimation::Idle
            || player.animation == PlayerAnimation::Run) {
            chooseLocomotion(player.animationTime, poseA, poseB, poseBlend);
        } else {
            const float progress = AnimationProgress(player);
            poseB = {HeroActionClip(player.character, player.animation,
                                    player.comboStep),
                     progress, 1.0f, false};
            if (player.previousAnimation == PlayerAnimation::Idle
                || player.previousAnimation == PlayerAnimation::Run) {
                AnimationPlayback ignored{};
                float ignoredBlend = 0.0f;
                chooseLocomotion(player.previousAnimationTime, poseA,
                                 ignored, ignoredBlend);
            } else {
                const std::uint8_t previousCombo = player.comboStep > 0
                    ? static_cast<std::uint8_t>(player.comboStep - 1) : 0;
                poseA = {HeroActionClip(player.character,
                                        player.previousAnimation,
                                        previousCombo),
                         std::clamp(player.previousAnimationTime
                                    / std::max(0.10f, player.animationDuration),
                                    0.0f, 1.0f),
                         1.0f, false};
            }
            poseBlend = player.animationBlend;
        }

        const WeaponWorldPose weaponPose = DrawAnimatedModel(
            modelSlot, renderPosition, player.facing, poseA, poseB, poseBlend,
            1.0f, tint, player.equippedWeapon);
        DrawLine3D({player.position.x, 0.08f, player.position.z},
                   {player.position.x + player.facing.x * 1.4f, 0.08f,
                    player.position.z + player.facing.z * 1.4f},
                   Color{255, 226, 113, 220});
        DrawPlayerCombatVfx(player, reducedMotion, highContrast,
                            weaponPose, m_knightBasicTrail,
                            m_knightSwordArcTexture, m_knightGuardTexture,
                            m_knightRushTexture, m_knightStormTexture,
                            m_dashStreakTexture,
                            m_mageSigilTexture, m_mageBoltTexture,
                            m_mageFrostTexture, m_mageGravityTexture,
                            m_mageUltimateTexture);
        return;
    }

    const float progress = AnimationProgress(player);
    const float motionScale = reducedMotion ? 0.35f : 1.0f;
    const float idleTime = player.animationTime > 0.0f
        ? player.animationTime : (float)GetTime();
    float stride = player.animation == PlayerAnimation::Run
        ? std::sin(idleTime * 11.0f) * 0.24f * motionScale : 0.0f;
    float bob = player.animation == PlayerAnimation::Run
        ? std::abs(std::sin(idleTime * 11.0f)) * 0.055f * motionScale
        : std::sin(idleTime * 2.4f) * 0.018f * motionScale;
    float bodyLean = player.animation == PlayerAnimation::Run ? 7.0f : 0.0f;
    float rightArmReach = -stride * 0.70f;
    float leftArmReach = stride * 0.70f;
    float weaponPitch = 0.0f;
    float castLift = 0.0f;
    if (player.animation == PlayerAnimation::BasicAttack) {
        const float swing = std::sin(progress * 3.14159265f);
        weaponPitch = -115.0f * swing;
        rightArmReach = 0.52f * swing;
        bodyLean += 10.0f * swing;
    } else if (player.animation == PlayerAnimation::SkillOne) {
        castLift = std::sin(progress * 3.14159265f);
        weaponPitch = -42.0f * castLift;
    } else if (player.animation == PlayerAnimation::SkillTwo) {
        castLift = std::sin(progress * 3.14159265f);
        weaponPitch = -75.0f * castLift;
        bodyLean = -8.0f * castLift;
    } else if (player.animation == PlayerAnimation::Ultimate) {
        castLift = std::sin(std::min(1.0f, progress * 1.35f) * 3.14159265f * 0.5f);
        weaponPitch = -145.0f * castLift;
        bob += 0.12f * castLift * motionScale;
    } else if (player.animation == PlayerAnimation::Dash) {
        bodyLean = 28.0f;
        stride = 0.35f;
    } else if (player.animation == PlayerAnimation::Hurt) {
        bodyLean = -18.0f * std::sin(progress * 3.14159265f);
    }
    const float deathTilt = player.animation == PlayerAnimation::Death
        ? std::min(82.0f, player.animationTime * 180.0f + 58.0f) : 0.0f;

    const float facingAngle = std::atan2(player.facing.x, player.facing.z)
                            * 57.2957795f;
    rlPushMatrix();
    rlTranslatef(player.position.x, player.position.y + bob, player.position.z);
    rlRotatef(facingAngle, 0.0f, 1.0f, 0.0f);
    rlRotatef(deathTilt, 0.0f, 0.0f, 1.0f);
    rlRotatef(bodyLean, 1.0f, 0.0f, 0.0f);

    const Vector3 leftHip{-0.20f, 0.82f, 0.0f};
    const Vector3 rightHip{0.20f, 0.82f, 0.0f};
    const Vector3 leftKnee{-0.22f, 0.45f, stride};
    const Vector3 rightKnee{0.22f, 0.45f, -stride};
    const Vector3 leftFoot{-0.22f, 0.08f, -stride * 0.45f + 0.04f};
    const Vector3 rightFoot{0.22f, 0.08f, stride * 0.45f + 0.04f};

    if (player.character == CharacterId::Knight) {
        const Color armor = flash > 0.5f ? Color{238, 214, 255, 255}
                                         : Color{54, 27, 76, 255};
        const Color plate{89, 42, 126, 255};
        const Color edge{151, 82, 194, 255};
        const Color crystal{215, 112, 255, 255};
        const Color skin{238, 184, 162, 255};

        DrawLowPolyLimb(leftHip, leftKnee, leftFoot, 0.14f, armor, plate);
        DrawLowPolyLimb(rightHip, rightKnee, rightFoot, 0.14f, armor, plate);
        DrawCube({leftFoot.x, 0.09f, leftFoot.z + 0.07f}, 0.29f, 0.19f, 0.44f, plate);
        DrawCube({rightFoot.x, 0.09f, rightFoot.z + 0.07f}, 0.29f, 0.19f, 0.44f, plate);
        DrawCube({0.0f, 1.12f, 0.0f}, 0.76f, 0.62f, 0.43f, armor);
        DrawCube({0.0f, 1.34f, 0.04f}, 0.88f, 0.20f, 0.49f, plate);
        DrawCube({0.0f, 0.82f, 0.01f}, 0.70f, 0.13f, 0.40f, edge);
        DrawSphere({-0.45f, 1.35f, 0.0f}, 0.23f, plate);
        DrawSphere({0.45f, 1.35f, 0.0f}, 0.23f, plate);

        const Vector3 leftShoulder{-0.44f, 1.33f, 0.0f};
        const Vector3 rightShoulder{0.44f, 1.33f, 0.0f};
        const Vector3 leftElbow{-0.52f, 1.02f, leftArmReach};
        const Vector3 rightElbow{0.52f, 1.02f, rightArmReach + castLift * 0.22f};
        const Vector3 leftHand{-0.50f, 0.76f, leftArmReach * 0.55f + castLift * 0.38f};
        const Vector3 rightHand{0.54f, 0.76f + castLift * 0.30f,
                                rightArmReach * 0.55f + castLift * 0.52f};
        DrawLowPolyLimb(leftShoulder, leftElbow, leftHand, 0.13f, armor, edge);
        DrawLowPolyLimb(rightShoulder, rightElbow, rightHand, 0.13f, armor, edge);
        DrawSphere(leftHand, 0.12f, Color{34, 25, 40, 255});
        DrawSphere(rightHand, 0.12f, Color{34, 25, 40, 255});

        DrawSphere({0.0f, 1.70f, 0.0f}, 0.30f, skin);
        DrawSphere({0.0f, 1.78f, -0.08f}, 0.32f, Color{107, 48, 145, 255});
        DrawSphere({0.0f, 1.69f, 0.22f}, 0.25f, skin);
        DrawSphere({-0.10f, 1.73f, 0.43f}, 0.035f, Color{70, 32, 91, 255});
        DrawSphere({0.10f, 1.73f, 0.43f}, 0.035f, Color{70, 32, 91, 255});
        DrawCapsule({0.0f, 1.88f, -0.20f}, {0.0f, 2.18f, -0.45f},
                    0.17f, 6, 6, Color{129, 60, 172, 255});
        DrawCapsule({0.0f, 2.13f, -0.43f}, {0.0f, 1.62f, -0.62f},
                    0.16f, 6, 6, Color{119, 52, 164, 255});
        for (float x : {-0.16f, 0.0f, 0.16f})
            DrawSphere({x, 1.18f, 0.25f}, 0.065f, crystal);

        if (player.equippedWeapon == WeaponId::KnightGreatsword) {
            rlPushMatrix();
            rlTranslatef(rightHand.x, rightHand.y, rightHand.z);
            rlRotatef(weaponPitch, 1.0f, 0.0f, 0.0f);
            DrawCylinder({0.0f, -0.04f, 0.0f}, 0.055f, 0.055f, 0.34f, 6,
                         Color{35, 24, 42, 255});
            DrawCube({0.0f, 0.31f, 0.0f}, 0.52f, 0.10f, 0.13f, edge);
            DrawCube({0.0f, 0.92f, 0.0f}, 0.30f, 1.18f, 0.105f, crystal);
            DrawCubeWires({0.0f, 0.92f, 0.0f}, 0.30f, 1.18f, 0.105f,
                          Color{255, 209, 255, 255});
            DrawSphere({0.0f, 1.52f, 0.0f}, 0.12f, Color{236, 154, 255, 255});
            rlPopMatrix();
        }

        if (player.guardTimer > 0.0f)
            DrawSphere({0.0f, 1.02f, 0.62f}, 0.48f,
                       Fade(Color{223, 147, 255, 255}, 0.16f));
    } else {
        const Color blue = flash > 0.5f ? Color{222, 245, 255, 255}
                                        : Color{30, 87, 178, 255};
        const Color deepBlue{20, 53, 124, 255};
        const Color cyan{60, 200, 255, 255};
        const Color whiteCloth{224, 235, 242, 255};
        const Color skin{239, 190, 166, 255};
        const Color leather{40, 37, 50, 255};

        DrawLowPolyLimb(leftHip, leftKnee, leftFoot, 0.12f, leather, deepBlue);
        DrawLowPolyLimb(rightHip, rightKnee, rightFoot, 0.12f, leather, deepBlue);
        DrawCube({leftFoot.x, 0.09f, leftFoot.z + 0.06f}, 0.25f, 0.18f, 0.38f, leather);
        DrawCube({rightFoot.x, 0.09f, rightFoot.z + 0.06f}, 0.25f, 0.18f, 0.38f, leather);
        DrawCylinder({0.0f, 0.70f, -0.10f}, 0.46f, 0.29f, 0.92f, 6, deepBlue);
        DrawCube({0.0f, 1.12f, 0.0f}, 0.70f, 0.64f, 0.38f, blue);
        DrawCube({0.0f, 1.12f, 0.205f}, 0.28f, 0.60f, 0.035f, whiteCloth);
        DrawCube({0.0f, 0.82f, 0.0f}, 0.72f, 0.11f, 0.40f, leather);
        DrawSphere({0.0f, 1.70f, 0.0f}, 0.29f, skin);
        DrawSphere({0.0f, 1.82f, -0.05f}, 0.31f, Color{39, 133, 235, 255});
        DrawSphere({0.0f, 1.69f, 0.21f}, 0.245f, skin);
        DrawSphere({-0.095f, 1.72f, 0.42f}, 0.032f, Color{21, 62, 122, 255});
        DrawSphere({0.095f, 1.72f, 0.42f}, 0.032f, Color{21, 62, 122, 255});
        DrawCylinderEx({-0.30f, 1.54f, -0.06f}, {0.0f, 1.98f, -0.22f},
                       0.13f, 0.08f, 6, deepBlue);
        DrawCylinderEx({0.30f, 1.54f, -0.06f}, {0.0f, 1.98f, -0.22f},
                       0.13f, 0.08f, 6, deepBlue);

        const Vector3 leftShoulder{-0.39f, 1.34f, 0.0f};
        const Vector3 rightShoulder{0.39f, 1.34f, 0.0f};
        const Vector3 leftElbow{-0.48f, 1.05f + castLift * 0.22f,
                                leftArmReach + castLift * 0.30f};
        const Vector3 rightElbow{0.48f, 1.06f + castLift * 0.30f,
                                 rightArmReach + castLift * 0.34f};
        const Vector3 leftHand{-0.46f, 0.79f + castLift * 0.48f,
                               leftArmReach * 0.50f + castLift * 0.55f};
        const Vector3 rightHand{0.51f, 0.80f + castLift * 0.38f,
                                rightArmReach * 0.50f + castLift * 0.50f};
        DrawLowPolyLimb(leftShoulder, leftElbow, leftHand, 0.105f, blue, whiteCloth);
        DrawLowPolyLimb(rightShoulder, rightElbow, rightHand, 0.105f, blue, whiteCloth);
        DrawSphere(leftHand, 0.105f, skin);
        DrawSphere(rightHand, 0.105f, skin);

        if (player.equippedWeapon == WeaponId::MagicCasterStaff) {
            rlPushMatrix();
            rlTranslatef(rightHand.x, rightHand.y - 0.52f, rightHand.z);
            rlRotatef(weaponPitch * 0.40f, 1.0f, 0.0f, 0.0f);
            DrawCylinder({0.0f, 0.0f, 0.0f}, 0.055f, 0.055f, 1.72f, 7,
                         Color{107, 64, 39, 255});
            DrawSphere({0.0f, 1.79f, 0.0f}, 0.23f, Color{104, 63, 37, 255});
            DrawSphere({0.0f, 1.79f, 0.0f}, 0.15f, cyan);
            DrawSphere({0.0f, 1.79f, 0.0f}, 0.16f + castLift * 0.05f,
                       Fade(Color{176, 239, 255, 255}, 0.28f));
            rlPopMatrix();
        }

        if (castLift > 0.05f)
            DrawSphere(leftHand, 0.10f + castLift * 0.07f,
                       Fade(Color{88, 220, 255, 255}, 0.32f));
    }

    rlPopMatrix();

    DrawLine3D({player.position.x, 0.08f, player.position.z},
               {player.position.x + player.facing.x * 1.4f, 0.08f,
                player.position.z + player.facing.z * 1.4f},
               Color{255, 226, 113, 220});
    DrawPlayerCombatVfx(player, reducedMotion, highContrast,
                        {}, m_knightBasicTrail,
                        m_knightSwordArcTexture, m_knightGuardTexture,
                        m_knightRushTexture, m_knightStormTexture,
                        m_dashStreakTexture,
                        m_mageSigilTexture, m_mageBoltTexture,
                        m_mageFrostTexture, m_mageGravityTexture,
                        m_mageUltimateTexture);
}

void SurvivalView::RenderEnemy(const EnemyState& enemy, float time, bool reducedMotion,
                               bool lowDetail, bool highContrast) const {
    const bool boss = IsBoss(enemy.archetype);
    const bool floating = enemy.archetype == EnemyArchetype::HexeyeArtillerist;
    const float bob = floating
        ? 0.35f + (reducedMotion ? 0.0f
            : std::sin(time * 2.0f + enemy.position.x) * 0.07f)
        : 0.0f;
    const bool flash = enemy.hitFlash > 0.0f;
    if (lowDetail && !boss) {
        Color color = Color{184, 58, 52, 255};
        float radius = 0.42f;
        if (enemy.archetype == EnemyArchetype::HexArcher) color = Color{35, 150, 104, 255};
        if (enemy.archetype == EnemyArchetype::ObsidianBrute) {
            color = Color{64, 61, 72, 255};
            radius = 0.90f;
        }
        const float pulse = reducedMotion ? 1.0f : 1.0f + std::sin(time * 6.0f) * 0.04f;
        DrawSphere({enemy.position.x, radius + 0.15f, enemy.position.z}, radius * pulse,
                   flash ? RAYWHITE : color);
        DrawEnemyHitVfx(enemy, reducedMotion, highContrast);
        return;
    }
    const std::size_t modelSlot = ModelSlot(enemy.archetype);
    if (m_animatedModels[modelSlot].ready) {
        const Color tint = flash ? Color{255, 232, 255, 255} : WHITE;
        const auto playback = [&](EnemyAnimation animation,
                                  float animationTime) {
            const int state = std::clamp(static_cast<int>(animation), 0, 8);
            const AnimationClip clip = kAnimationClips[state];
            const float legacyFrame = EnemyAnimationFrame(
                animation, animationTime, enemy, time);
            const float denominator = std::max(1.0f, clip.last - clip.first);
            return AnimationPlayback{
                EnemyActionClip(modelSlot, animation),
                std::clamp((legacyFrame - clip.first) / denominator,
                           0.0f, 1.0f),
                1.0f,
                false
            };
        };
        const AnimationPlayback previous = playback(
            enemy.previousVisualAnimation, enemy.previousAnimationTime);
        const AnimationPlayback current = playback(
            enemy.visualAnimation, enemy.animationTime);
        DrawAnimatedModel(modelSlot, {enemy.position.x, bob, enemy.position.z},
                          enemy.facing, previous, current,
                          enemy.animationBlend, EnemyModelScale(enemy), tint);
        if (boss) {
            Color phaseColor{219, 83, 255, 255};
            Color phaseAccent{255, 177, 74, 255};
            if (enemy.archetype == EnemyArchetype::HexeyeArtillerist) {
                phaseColor = Color{87, 236, 210, 255};
                phaseAccent = Color{181, 255, 99, 255};
            } else if (enemy.archetype == EnemyArchetype::IronrootColossus) {
                phaseColor = Color{255, 151, 52, 255};
                phaseAccent = Color{255, 224, 118, 255};
            } else if (enemy.archetype == EnemyArchetype::EclipseChimera) {
                const bool solar = enemy.bossPhase == 1;
                phaseColor = solar ? Color{255, 144, 45, 255}
                                   : Color{102, 116, 255, 255};
                phaseAccent = solar ? Color{255, 229, 121, 255}
                                    : Color{210, 126, 255, 255};
            } else if (enemy.archetype == EnemyArchetype::VoidSovereign) {
                phaseColor = enemy.bossPhase == 1 ? Color{159, 72, 255, 255}
                    : (enemy.bossPhase == 2 ? Color{79, 211, 255, 255}
                                            : Color{255, 76, 159, 255});
                phaseAccent = enemy.bossPhase == 3
                    ? Color{255, 214, 110, 255} : Color{221, 168, 255, 255};
            }
            const float auraRadius = std::clamp(
                1.35f * EnemyModelScale(enemy), 1.30f, 7.20f);
            const float auraPulse = reducedMotion ? 0.68f
                : 0.55f + std::sin(time * 3.2f + enemy.bossPhase) * 0.14f;
            DrawCircle3D({enemy.position.x, 0.10f, enemy.position.z},
                         auraRadius, {1, 0, 0}, 90.0f,
                         Fade(phaseColor, auraPulse));
            DrawCircle3D({enemy.position.x, 0.115f, enemy.position.z},
                         auraRadius * 0.72f, {1, 0, 0}, 90.0f,
                         Fade(phaseAccent, auraPulse * 0.74f));
            if (enemy.specialFxTimer > 0.0f) {
                DrawGroundGlyph({enemy.position.x, 0.12f, enemy.position.z},
                                auraRadius * 1.18f,
                                reducedMotion ? 0.0f : time * 0.55f,
                                phaseColor, 8 + enemy.bossPhase * 2);
            }
            if (enemy.phaseTransitionTimer > 0.0f) {
                const float transition = std::clamp(enemy.phaseTransitionTimer
                                                    / 1.8f, 0.0f, 1.0f);
                for (int ring = 0; ring < enemy.bossPhase + 1; ++ring) {
                    const float transitionRadius = auraRadius
                        * (0.62f + ring * 0.22f)
                        * (1.0f + (1.0f - transition) * 0.22f);
                    DrawCircle3D({enemy.position.x,
                                  0.16f + ring * 0.05f,
                                  enemy.position.z},
                                 transitionRadius, {1, 0, 0}, 90.0f,
                                 Fade(ring & 1 ? phaseAccent : phaseColor,
                                      0.30f + transition * 0.48f));
                }
            }
        }
        float height = 1.0f;
        float barWidth = 1.0f;
        EnemyBarMetrics(enemy.archetype, height, barWidth);
        if (enemy.archetype == EnemyArchetype::VoidSovereign) {
            height *= EnemyModelScale(enemy);
            barWidth *= EnemyModelScale(enemy);
        }
        const float ratio = enemy.maxHp > 0.0f
            ? std::clamp(enemy.hp / enemy.maxHp, 0.0f, 1.0f) : 0.0f;
        const float barY = height + (boss ? 0.85f : 0.42f) + bob;
        DrawCube({enemy.position.x, barY, enemy.position.z},
                 barWidth, 0.07f, 0.08f, Color{32, 18, 27, 255});
        DrawCube({enemy.position.x - barWidth * (1.0f - ratio) * 0.5f,
                  barY + 0.01f, enemy.position.z - 0.01f},
                 std::max(0.01f, barWidth * ratio), 0.08f, 0.09f,
                 boss ? Color{255, 89, 71, 255} : Color{116, 225, 121, 255});
        DrawEnemyHitVfx(enemy, reducedMotion, highContrast);
        return;
    }
    float height = 0.95f;
    float barWidth = 0.85f;

    if (enemy.archetype == EnemyArchetype::Riftling) {
        height = 1.05f;
        barWidth = 0.90f;
        const Color hide = flash ? Color{255, 235, 255, 255} : Color{68, 31, 91, 255};
        const Color plate{93, 43, 119, 255};
        const Color claw{244, 83, 42, 255};
        const float moving = LengthXZ(enemy.velocity) > 0.1f ? 1.0f : 0.25f;
        const float gait = std::sin(time * 15.0f + enemy.position.x * 0.7f)
                         * 0.18f * moving;
        DrawCapsule({enemy.position.x, 0.48f + bob, enemy.position.z + 0.28f},
                    {enemy.position.x, 0.73f + bob, enemy.position.z - 0.22f},
                    0.38f, 7, 6, hide);
        DrawSphere({enemy.position.x, 0.80f + bob, enemy.position.z - 0.46f},
                   0.31f, plate);
        DrawSphere({enemy.position.x, 0.73f + bob, enemy.position.z - 0.73f},
                   0.13f, Color{255, 102, 45, 255});
        DrawSphere({enemy.position.x - 0.10f, 0.84f + bob,
                    enemy.position.z - 0.70f}, 0.045f, Color{255, 174, 44, 255});
        DrawSphere({enemy.position.x + 0.10f, 0.84f + bob,
                    enemy.position.z - 0.70f}, 0.045f, Color{255, 174, 44, 255});
        for (int pair = 0; pair < 2; ++pair) {
            const float z = enemy.position.z + (pair == 0 ? -0.18f : 0.34f);
            const float step = pair == 0 ? gait : -gait;
            for (int side = -1; side <= 1; side += 2) {
                const Vector3 hip{enemy.position.x + side * 0.27f, 0.50f + bob, z};
                const Vector3 knee{enemy.position.x + side * 0.43f, 0.27f + bob,
                                   z + step * side};
                const Vector3 foot{enemy.position.x + side * 0.50f, 0.07f,
                                   z - 0.12f + step * side};
                DrawCylinderEx(hip, knee, 0.10f, 0.075f, 6, hide);
                DrawCylinderEx(knee, foot, 0.075f, 0.045f, 6, plate);
                DrawSphere(foot, 0.075f, claw);
            }
        }
        for (int spike = 0; spike < 3; ++spike)
            DrawCylinderEx({enemy.position.x, 0.80f + bob,
                            enemy.position.z + 0.05f + spike * 0.18f},
                           {enemy.position.x, 1.10f + bob - spike * 0.05f,
                            enemy.position.z + 0.10f + spike * 0.18f},
                           0.09f, 0.01f, 5, plate);
    } else if (enemy.archetype == EnemyArchetype::HexArcher) {
        height = 1.75f;
        barWidth = 1.0f;
        const Color robe = flash ? Color{240, 255, 238, 255} : Color{24, 73, 58, 255};
        const Color armor{35, 51, 45, 255};
        const Color hex{145, 255, 64, 255};
        const bool aiming = enemy.action == EnemyAction::RangedShot;
        const float draw = aiming ? std::clamp(1.0f - enemy.actionTimer / 0.50f, 0.0f, 1.0f) : 0.0f;
        const float walk = LengthXZ(enemy.velocity) > 0.1f
            ? std::sin(time * 8.5f + enemy.position.z) * 0.11f : 0.0f;
        DrawCylinder({enemy.position.x, 0.12f + bob, enemy.position.z},
                     0.40f, 0.27f, 1.18f, 7, robe);
        DrawCylinderEx({enemy.position.x - 0.17f, 0.46f + bob, enemy.position.z},
                       {enemy.position.x - 0.18f, 0.08f, enemy.position.z + walk},
                       0.10f, 0.08f, 6, armor);
        DrawCylinderEx({enemy.position.x + 0.17f, 0.46f + bob, enemy.position.z},
                       {enemy.position.x + 0.18f, 0.08f, enemy.position.z - walk},
                       0.10f, 0.08f, 6, armor);
        DrawSphere({enemy.position.x, 1.52f + bob, enemy.position.z}, 0.32f, armor);
        DrawCylinderEx({enemy.position.x - 0.28f, 1.58f + bob, enemy.position.z + 0.02f},
                       {enemy.position.x, 1.88f + bob, enemy.position.z + 0.06f},
                       0.15f, 0.04f, 6, robe);
        DrawCylinderEx({enemy.position.x + 0.28f, 1.58f + bob, enemy.position.z + 0.02f},
                       {enemy.position.x, 1.88f + bob, enemy.position.z + 0.06f},
                       0.15f, 0.04f, 6, robe);
        DrawSphere({enemy.position.x, 1.57f + bob, enemy.position.z - 0.30f},
                   0.095f + draw * 0.025f, hex);
        const Vector3 bowTop{enemy.position.x + 0.48f, 1.58f + bob, enemy.position.z - 0.18f};
        const Vector3 bowGrip{enemy.position.x + 0.54f, 1.18f + bob, enemy.position.z - 0.20f};
        const Vector3 bowBottom{enemy.position.x + 0.48f, 0.78f + bob, enemy.position.z - 0.18f};
        DrawCylinderEx(bowTop, bowGrip, 0.035f, 0.035f, 5, hex);
        DrawCylinderEx(bowGrip, bowBottom, 0.035f, 0.035f, 5, hex);
        DrawLine3D(bowTop, {enemy.position.x + 0.18f - draw * 0.18f,
                            1.18f + bob, enemy.position.z - 0.24f}, hex);
        DrawLine3D(bowBottom, {enemy.position.x + 0.18f - draw * 0.18f,
                               1.18f + bob, enemy.position.z - 0.24f}, hex);
        DrawCylinderEx({enemy.position.x - 0.36f, 1.28f + bob, enemy.position.z},
                       {enemy.position.x + 0.18f - draw * 0.18f, 1.18f + bob,
                        enemy.position.z - 0.24f}, 0.08f, 0.06f, 6, robe);
    } else if (enemy.archetype == EnemyArchetype::ObsidianBrute) {
        height = 2.65f;
        barWidth = 1.8f;
        const Color stone = flash ? Color{255, 245, 215, 255} : Color{49, 48, 61, 255};
        const Color darkStone{34, 34, 43, 255};
        const Color core{255, 186, 43, 255};
        const float stomp = LengthXZ(enemy.velocity) > 0.1f
            ? std::abs(std::sin(time * 4.2f + enemy.position.x)) * 0.08f : 0.0f;
        const float slam = enemy.action == EnemyAction::GroundSlam
            ? std::clamp(1.0f - enemy.actionTimer / 0.80f, 0.0f, 1.0f) : 0.0f;
        DrawCube({enemy.position.x, 1.33f + bob + stomp, enemy.position.z},
                 1.38f, 1.82f, 1.02f, stone);
        DrawCube({enemy.position.x, 2.05f + bob + stomp, enemy.position.z},
                 1.62f, 0.56f, 1.18f, darkStone);
        DrawSphere({enemy.position.x, 2.53f + bob + stomp, enemy.position.z},
                   0.48f, darkStone);
        DrawSphere({enemy.position.x, 1.43f + bob + stomp, enemy.position.z - 0.55f},
                   0.28f + slam * 0.06f, core);
        DrawCircle3D({enemy.position.x, 1.43f + bob + stomp,
                      enemy.position.z - 0.57f}, 0.38f + slam * 0.10f,
                     {0.0f, 0.0f, 1.0f}, 0.0f,
                     Color{255, 222, 102, 190});
        for (int side = -1; side <= 1; side += 2) {
            const float armX = enemy.position.x + side * 0.94f;
            const Vector3 shoulder{armX, 1.95f + bob + stomp, enemy.position.z};
            const Vector3 elbow{enemy.position.x + side * (1.10f - slam * 0.18f),
                                1.35f + bob + stomp + slam * 0.68f,
                                enemy.position.z - slam * 0.36f};
            const Vector3 fist{enemy.position.x + side * (1.12f - slam * 0.30f),
                               0.62f + bob + stomp + slam * 1.10f,
                               enemy.position.z - 0.16f - slam * 0.70f};
            DrawCylinderEx(shoulder, elbow, 0.34f, 0.30f, 7, stone);
            DrawCylinderEx(elbow, fist, 0.38f, 0.32f, 7, darkStone);
            DrawSphere(fist, 0.43f, stone);
            DrawCube({enemy.position.x + side * 0.43f, 0.38f + stomp,
                      enemy.position.z}, 0.60f, 0.72f, 0.78f, darkStone);
        }
    } else if (enemy.archetype == EnemyArchetype::BroodWarden) {
        height = 3.25f;
        barWidth = 3.2f;
        const Color shell = flash ? Color{255, 235, 255, 255} : Color{93, 38, 127, 255};
        DrawCapsule({enemy.position.x, 0.85f + bob, enemy.position.z + 0.55f},
                    {enemy.position.x, 1.35f + bob, enemy.position.z - 0.75f},
                    1.20f, 12, 12, shell);
        DrawSphere({enemy.position.x, 1.65f + bob, enemy.position.z - 1.02f},
                   0.72f, Color{143, 55, 151, 255});
        DrawSphere({enemy.position.x, 1.28f + bob, enemy.position.z + 1.10f},
                   0.78f, Color{196, 55, 91, 255});
        for (int side = -1; side <= 1; side += 2) {
            DrawCylinderEx({enemy.position.x + side * 0.65f, 0.55f, enemy.position.z - 0.4f},
                           {enemy.position.x + side * 1.55f, 0.15f, enemy.position.z - 1.0f},
                           0.16f, 0.08f, 7, shell);
            DrawCylinderEx({enemy.position.x + side * 0.65f, 0.55f, enemy.position.z + 0.4f},
                           {enemy.position.x + side * 1.55f, 0.15f, enemy.position.z + 1.0f},
                           0.16f, 0.08f, 7, shell);
        }
    } else if (enemy.archetype == EnemyArchetype::HexeyeArtillerist) {
        height = 4.0f;
        barWidth = 3.6f;
        const float hover = 2.25f + bob * 2.0f;
        const Color shell = flash ? Color{235, 255, 255, 255} : Color{37, 86, 122, 255};
        DrawSphere({enemy.position.x, hover, enemy.position.z}, 1.38f, shell);
        DrawSphere({enemy.position.x, hover, enemy.position.z - 1.18f}, 0.52f,
                   Color{80, 232, 210, 255});
        DrawSphere({enemy.position.x, hover, enemy.position.z - 1.60f}, 0.18f,
                   Color{224, 255, 120, 255});
        for (int ring = 0; ring < 3; ++ring)
            DrawCircle3D({enemy.position.x, hover, enemy.position.z}, 1.65f + ring * 0.24f,
                         {1.0f, 0.0f, 0.0f}, time * (35.0f + ring * 12.0f),
                         Color{70, 218, 222, (unsigned char)(210 - ring * 40)});
    } else if (enemy.archetype == EnemyArchetype::IronrootColossus) {
        height = 5.2f;
        barWidth = 4.4f;
        const Color bark = flash ? Color{255, 246, 202, 255} : Color{62, 67, 54, 255};
        DrawCube({enemy.position.x, 2.30f + bob, enemy.position.z}, 2.45f, 3.65f, 1.75f, bark);
        DrawSphere({enemy.position.x, 4.50f + bob, enemy.position.z}, 1.02f,
                   Color{72, 77, 62, 255});
        DrawCylinderEx({enemy.position.x - 1.35f, 0.45f, enemy.position.z},
                       {enemy.position.x - 1.72f, 3.25f, enemy.position.z},
                       0.52f, 0.72f, 9, bark);
        DrawCylinderEx({enemy.position.x + 1.35f, 0.45f, enemy.position.z},
                       {enemy.position.x + 1.72f, 3.25f, enemy.position.z},
                       0.52f, 0.72f, 9, bark);
        DrawSphere({enemy.position.x, 2.10f + bob, enemy.position.z - 0.94f},
                   0.42f, Color{255, 194, 65, 255});
        DrawCircle3D({enemy.position.x, 2.10f + bob,
                      enemy.position.z - 0.96f}, 0.63f,
                     {0.0f, 0.0f, 1.0f}, 0.0f,
                     Color{255, 226, 112, 185});
    } else if (enemy.archetype == EnemyArchetype::EclipseChimera) {
        height = 5.8f;
        barWidth = 4.8f;
        const bool solar = enemy.bossPhase == 1;
        const Color body = flash ? Color{255, 245, 235, 255}
                                 : (solar ? Color{135, 61, 44, 255}
                                          : Color{47, 55, 126, 255});
        DrawCapsule({enemy.position.x, 1.05f + bob, enemy.position.z + 0.9f},
                    {enemy.position.x, 2.4f + bob, enemy.position.z - 0.7f},
                    1.48f, 14, 12, body);
        DrawSphere({enemy.position.x, 3.45f + bob, enemy.position.z - 1.0f},
                   1.05f, body);
        DrawSphere({enemy.position.x - 0.48f, 3.55f + bob, enemy.position.z - 1.82f},
                   0.28f, solar ? Color{255, 189, 53, 255} : Color{111, 178, 255, 255});
        DrawSphere({enemy.position.x + 0.48f, 3.55f + bob, enemy.position.z - 1.82f},
                   0.28f, solar ? Color{255, 91, 52, 255} : Color{197, 107, 255, 255});
        for (int side = -1; side <= 1; side += 2)
            DrawCylinderEx({enemy.position.x + side * 0.9f, 1.45f, enemy.position.z},
                           {enemy.position.x + side * 2.25f, 2.35f, enemy.position.z + 0.2f},
                           0.28f, 0.10f, 8, body);
    } else if (enemy.archetype == EnemyArchetype::VoidSovereign) {
        const float phaseScale = enemy.bossPhase == 1 ? 1.0f : (enemy.bossPhase == 2 ? 1.55f : 2.05f);
        height = 2.9f * phaseScale;
        barWidth = 3.8f + enemy.bossPhase * 0.55f;
        const Color voidBody = flash ? Color{255, 235, 255, 255}
                                     : Color{53, 24, (unsigned char)(92 + enemy.bossPhase * 28), 255};
        DrawCapsule({enemy.position.x, 0.55f * phaseScale + bob, enemy.position.z},
                    {enemy.position.x, 1.9f * phaseScale + bob, enemy.position.z},
                    0.52f * phaseScale, 14, 12, voidBody);
        DrawSphere({enemy.position.x, 2.35f * phaseScale + bob, enemy.position.z},
                   0.43f * phaseScale, Color{113, 49, 178, 255});
        DrawSphere({enemy.position.x, 1.45f * phaseScale + bob, enemy.position.z - 0.55f * phaseScale},
                   0.20f * phaseScale, Color{231, 104, 255, 255});
        for (int ring = 0; ring < enemy.bossPhase; ++ring)
            DrawCircle3D({enemy.position.x, 1.35f * phaseScale + bob, enemy.position.z},
                         0.95f * phaseScale + ring * 0.30f,
                         {0.0f, 1.0f, 0.0f}, time * (45.0f + ring * 18.0f),
                         Color{184, 92, 255, (unsigned char)(220 - ring * 45)});
    } else {
        const bool previewBoss = enemy.archetype == EnemyArchetype::BossPrototype;
        const float radius = previewBoss ? 1.15f : 0.34f;
        height = previewBoss ? 2.9f : 0.75f;
        barWidth = previewBoss ? 2.8f : 0.8f;
        const Color body = flash ? Color{255, 241, 220, 255}
                                 : (previewBoss ? Color{126, 42, 55, 255}
                                                : Color{184, 58, 52, 255});
        DrawCapsule({enemy.position.x, radius + bob, enemy.position.z},
                    {enemy.position.x, height + bob, enemy.position.z},
                    radius, previewBoss ? 12 : 8, previewBoss ? 10 : 6, body);
        DrawSphere({enemy.position.x, height + radius * 0.55f + bob, enemy.position.z},
                   radius * 0.62f, previewBoss ? Color{221, 86, 67, 255}
                                              : Color{246, 105, 63, 255});
        DrawSphere({enemy.position.x - radius * 0.24f, height + radius * 0.67f + bob,
                    enemy.position.z - radius * 0.53f}, radius * 0.10f,
                   Color{255, 222, 82, 255});
        DrawSphere({enemy.position.x + radius * 0.24f, height + radius * 0.67f + bob,
                    enemy.position.z - radius * 0.53f}, radius * 0.10f,
                   Color{255, 222, 82, 255});
    }

    const float ratio = enemy.maxHp > 0.0f ? std::clamp(enemy.hp / enemy.maxHp, 0.0f, 1.0f) : 0.0f;
    const float barY = height + (boss ? 0.85f : 0.42f) + bob;
    DrawCube({enemy.position.x, barY, enemy.position.z},
             barWidth, 0.07f, 0.08f, Color{32, 18, 27, 255});
    DrawCube({enemy.position.x - barWidth * (1.0f - ratio) * 0.5f,
              barY + 0.01f, enemy.position.z - 0.01f},
             std::max(0.01f, barWidth * ratio), 0.08f, 0.09f,
             boss ? Color{255, 89, 71, 255} : Color{116, 225, 121, 255});
    DrawEnemyHitVfx(enemy, reducedMotion, highContrast);
}

void SurvivalView::Render(const SurvivalController& controller) const {
    ClearBackground(Color{7, 5, 16, 255});
    const float time = static_cast<float>(GetTime());
    DrawArenaBackdrop(time, controller.IsReducedMotion());

    if (controller.GetPhase() == Phase::CharacterSelect) {
        Camera3D showroomCamera{};
        showroomCamera.position = {0.0f, 4.35f, 11.6f};
        showroomCamera.target = {0.0f, 1.15f, 0.0f};
        showroomCamera.up = {0.0f, 1.0f, 0.0f};
        showroomCamera.fovy = 38.0f;
        showroomCamera.projection = CAMERA_PERSPECTIVE;
        BeginMode3D(showroomCamera);
        RenderArena(time, controller.IsReducedMotion(), true);

        const int selected = controller.GetSelectedCharacter();
        for (int index = 0; index < 2; ++index) {
            const float x = index == 0 ? -2.55f : 2.55f;
            const bool active = selected == index;
            const Color accent = index == 0 ? Color{208, 112, 255, 255}
                                            : Color{76, 220, 255, 255};
            DrawCylinder({x, 0.0f, 0.0f}, active ? 1.52f : 1.30f,
                         active ? 1.68f : 1.42f, active ? 0.30f : 0.22f,
                         24, Color{28, 22, 51, 255});
            DrawCylinderWires({x, 0.0f, 0.0f}, active ? 1.52f : 1.30f,
                              active ? 1.68f : 1.42f, active ? 0.30f : 0.22f,
                              24, Fade(accent, active ? 0.92f : 0.34f));
            DrawCircle3D({x, active ? 0.32f : 0.24f, 0.0f},
                         active ? 1.22f : 1.02f, {1, 0, 0}, 90.0f,
                         Fade(accent, active ? 0.90f : 0.30f));
            if (active) {
                const float beacon = controller.IsReducedMotion()
                    ? 0.15f : 0.12f + 0.04f * std::sin(time * 2.0f);
                DrawCylinder({x, 0.24f, 0.0f}, 0.82f, 1.16f, 3.9f, 24,
                             Fade(accent, beacon));
            }
        }
        PlayerState knight;
        knight.character = CharacterId::Knight;
        knight.equippedWeapon = DefaultWeaponFor(knight.character);
        knight.position = {-2.55f, controller.GetSelectedCharacter() == 0 ? 0.31f : 0.23f, 0.0f};
        knight.facing = {0.0f, 0.0f, 1.0f};
        knight.animationTime = time;
        RenderPlayer(knight, controller.IsReducedMotion(), controller.IsHighContrast());
        PlayerState caster;
        caster.character = CharacterId::MagicCaster;
        caster.equippedWeapon = DefaultWeaponFor(caster.character);
        caster.position = {2.55f, controller.GetSelectedCharacter() == 1 ? 0.31f : 0.23f, 0.0f};
        caster.facing = {0.0f, 0.0f, 1.0f};
        caster.animationTime = time;
        RenderPlayer(caster, controller.IsReducedMotion(), controller.IsHighContrast());
        EndMode3D();
        RenderCharacterSelect(controller);
        if (controller.IsRecordsVisible()) RenderRecords(controller);
        return;
    }

    UpdateSkillVfxRuntime(controller, GetFrameTime());
    const Camera3D renderCamera = CombatCamera(controller, time);
    BeginMode3D(renderCamera);
    RenderArena(time, controller.IsReducedMotion());
    RenderPlayer(controller.GetPlayer(), controller.IsReducedMotion(),
                 controller.IsHighContrast());
    for (const EnemyState& enemy : controller.GetEnemies()) {
        if (!enemy.active) continue;
        const float playerDistance = LengthXZ({enemy.position.x - controller.GetPlayer().position.x,
                                               0.0f,
                                               enemy.position.z - controller.GetPlayer().position.z});
        RenderEnemy(enemy, time, controller.IsReducedMotion(), playerDistance > 21.0f,
                    controller.IsHighContrast());
        const float pulse = controller.IsReducedMotion() ? 1.0f
            : 0.65f + 0.35f * std::sin(time * 12.0f);
        const Color danger = controller.IsHighContrast() ? Color{255, 255, 255, 255}
                                                         : Color{255, 168, 55, 255};
        if (enemy.action == EnemyAction::RangedShot) {
            DrawLine3D({enemy.position.x, 0.14f, enemy.position.z},
                       {controller.GetPlayer().position.x, 0.14f,
                        controller.GetPlayer().position.z},
                       Fade(controller.IsHighContrast() ? Color{255, 255, 0, 255}
                                                        : Color{124, 255, 125, 255}, pulse));
        } else if (enemy.action == EnemyAction::GroundSlam) {
            float radius = 4.0f;
            if (enemy.archetype == EnemyArchetype::IronrootColossus) radius = 5.2f;
            if (enemy.archetype == EnemyArchetype::VoidSovereign) radius = 5.5f;
            DrawCircle3D({enemy.position.x, 0.08f, enemy.position.z}, radius,
                         {1.0f, 0.0f, 0.0f}, 90.0f,
                         Fade(danger, pulse));
        } else if (enemy.action == EnemyAction::ClawSweep) {
            const float radius = enemy.archetype == EnemyArchetype::EclipseChimera
                ? 4.8f : 3.8f;
            DrawCircle3D({enemy.position.x, 0.09f, enemy.position.z}, radius,
                         {1.0f, 0.0f, 0.0f}, 90.0f,
                         Fade(controller.IsHighContrast() ? Color{255, 255, 255, 255}
                                                          : Color{255, 72, 102, 255}, pulse));
        } else if (enemy.action == EnemyAction::TargetingVolley) {
            const Vector3 target = controller.GetPlayer().position;
            DrawCircle3D({target.x, 0.10f, target.z}, 1.35f,
                         {1.0f, 0.0f, 0.0f}, 90.0f,
                         Fade(controller.IsHighContrast() ? Color{255, 255, 0, 255}
                                                          : Color{91, 244, 232, 255}, pulse));
            DrawLine3D({enemy.position.x, 2.5f, enemy.position.z},
                       {target.x, 0.10f, target.z},
                       Fade(Color{91, 244, 232, 255}, pulse * 0.8f));
        }
    }

    for (const ProjectileState& projectile : controller.GetProjectiles()) {
        if (!projectile.active) continue;
        DrawProjectileVfx(projectile, time, controller.IsReducedMotion(),
                          controller.IsHighContrast(), m_mageBoltTexture);
    }
    RenderSkillGeometry(controller, time);
    RenderSkillVfxRuntime(controller.IsReducedMotion(),
                          controller.IsHighContrast());

    if (controller.GetAttackFxTimer() > 0.0f) {
        const PlayerState& player = controller.GetPlayer();
        const float duration = player.animation == PlayerAnimation::Dash
            ? 0.16f : (player.character == CharacterId::Knight ? 0.22f : 0.18f);
        const float phase = 1.0f - Saturate(controller.GetAttackFxTimer() / duration);
        const Vector3 center{
            player.position.x + player.facing.x * (player.character == CharacterId::Knight
                                                    ? 1.45f : 0.92f),
            player.character == CharacterId::Knight ? 0.72f : 1.05f,
            player.position.z + player.facing.z * (player.character == CharacterId::Knight
                                                    ? 1.45f : 0.92f)
        };
        const Color color = controller.IsHighContrast()
            ? Color{255, 255, 255, 255}
            : (player.character == CharacterId::Knight
                ? Color{220, 124, 255, 255} : Color{72, 224, 255, 255});
        DrawImpactBurst(center, color, phase,
                        player.character == CharacterId::Knight ? 1.15f : 0.62f,
                        controller.IsReducedMotion() ? 4 : 9);
    }
    if (controller.GetSkillFxTimer() > 0.0f) {
        const PlayerState& player = controller.GetPlayer();
        const Vector3 center = controller.GetSkillFxCenter();
        const float radius = controller.GetSkillFxRadius();
        const bool ultimate = player.animation == PlayerAnimation::Ultimate
                           || radius > 6.25f;
        const bool gravity = player.character == CharacterId::MagicCaster
                          && controller.GetGravityWellTimer() > 0.0f
                          && !ultimate;
        const bool rush = player.character == CharacterId::Knight
                       && (player.rushTimer > 0.0f || radius <= 2.10f)
                       && player.guardTimer <= 0.0f && !ultimate;
        const float nominalLife = ultimate ? 1.10f
            : (gravity ? 0.80f : (rush ? 0.45f : 0.65f));
        const float phase = 1.0f - Saturate(controller.GetSkillFxTimer() / nominalLife);
        const Color primary = controller.IsHighContrast()
            ? Color{255, 255, 255, 255}
            : (player.character == CharacterId::Knight
                ? Color{225, 132, 255, 255} : Color{91, 225, 255, 255});
        const Color accent = player.character == CharacterId::Knight
            ? Color{255, 217, 103, 255} : Color{155, 92, 255, 255};

        if (rush) {
            DrawImpactBurst({center.x, 0.30f, center.z}, primary, phase, 1.65f,
                            controller.IsReducedMotion() ? 5 : 11);
            DrawGroundGlyph(center, radius * (0.42f + phase * 0.58f),
                            time * 1.7f, accent, controller.IsReducedMotion() ? 4 : 8);
            if (m_knightRushTexture.id != 0) {
                const Vector3 facing = HorizontalFacing(player.facing);
                const Vector3 right = FacingRight(facing);
                DrawVfxQuad(m_knightRushTexture,
                            {center.x, 0.115f, center.z}, facing, right,
                            2.55f + phase * 0.55f,
                            0.86f + phase * 0.24f,
                            Fade(WHITE, (0.84f - phase * 0.42f)
                                        * (controller.IsReducedMotion() ? 0.64f : 1.0f)));
            }
        } else {
            const float expansion = ultimate
                ? (0.22f + phase * 0.78f)
                : (0.58f + phase * 0.42f);
            DrawGroundGlyph(center, std::max(0.35f, radius * expansion),
                            (player.character == CharacterId::Knight ? 1.0f : -1.0f)
                                * time * (ultimate ? 1.4f : 0.8f),
                            primary, controller.IsReducedMotion() ? 6 : (ultimate ? 16 : 10));
            DrawCircle3D({center.x, 0.09f, center.z},
                         std::max(0.25f, radius * expansion * 0.82f),
                         {1.0f, 0.0f, 0.0f}, 90.0f,
                         Fade(accent, 0.76f * (1.0f - phase * 0.45f)));
            if (ultimate && phase > 0.12f)
                DrawImpactBurst({center.x, 0.24f, center.z}, accent, phase,
                                radius * 0.45f, controller.IsReducedMotion() ? 6 : 14);

            // These are textured planes embedded in world space, not screen-space
            // pictures. Their rotation, scale and opacity are driven each frame,
            // while the procedural geometry above gives them depth and impact.
            Texture2D fieldTexture{};
            if (player.character == CharacterId::Knight && ultimate)
                fieldTexture = m_knightStormTexture;
            else if (player.character == CharacterId::MagicCaster)
                fieldTexture = gravity ? m_mageGravityTexture
                                       : (ultimate ? m_mageUltimateTexture
                                                   : m_mageFrostTexture);
            if (fieldTexture.id != 0) {
                const float spin = time * (gravity ? -1.85f : (ultimate ? 0.82f : -0.42f));
                const Vector3 axisU{std::cos(spin), 0.0f, std::sin(spin)};
                const Vector3 axisV{-std::sin(spin), 0.0f, std::cos(spin)};
                const float textureRadius = std::max(0.42f, radius * expansion);
                DrawVfxQuad(fieldTexture, {center.x, 0.105f, center.z},
                            axisU, axisV, textureRadius, textureRadius,
                            Fade(WHITE, (0.30f + 0.42f * (1.0f - phase))
                                        * (controller.IsReducedMotion() ? 0.66f : 1.0f)));
            }
        }
    }
    if (controller.GetGravityWellTimer() > 0.0f) {
        const Vector3 center = controller.GetGravityWellCenter();
        const Color gravityPrimary = controller.IsHighContrast()
            ? Color{255, 255, 255, 255} : Color{91, 94, 255, 255};
        const Color gravityAccent = controller.IsHighContrast()
            ? Color{255, 235, 48, 255} : Color{193, 97, 255, 255};
        const int ringCount = controller.IsReducedMotion() ? 3 : 6;
        if (m_mageGravityTexture.id != 0) {
            const float spin = -time * (controller.IsReducedMotion() ? 0.48f : 1.35f);
            const Vector3 axisU{std::cos(spin), 0.0f, std::sin(spin)};
            const Vector3 axisV{-std::sin(spin), 0.0f, std::cos(spin)};
            const float breathe = controller.IsReducedMotion()
                ? 4.15f : 4.15f + std::sin(time * 4.2f) * 0.22f;
            DrawVfxQuad(m_mageGravityTexture, {center.x, 0.105f, center.z},
                        axisU, axisV, breathe, breathe,
                        Fade(WHITE, controller.IsReducedMotion() ? 0.42f : 0.66f));
        }
        for (int ring = 0; ring < ringCount; ++ring) {
            const float pulse = std::fmod(time * 2.2f + ring / static_cast<float>(ringCount),
                                          1.0f);
            DrawCircle3D({center.x, 0.09f + ring * 0.01f, center.z},
                         4.8f - pulse * 3.9f, {1.0f, 0.0f, 0.0f}, 90.0f,
                         Fade(gravityPrimary, (80.0f + pulse * 155.0f) / 255.0f));
        }
        const int motes = controller.IsReducedMotion() ? 4 : 10;
        for (int mote = 0; mote < motes; ++mote) {
            const float angle = time * (2.0f + mote * 0.06f)
                              + mote * 2.0f * kPi / motes;
            const float orbit = 0.75f + (mote % 4) * 0.62f;
            const Vector3 orb{center.x + std::cos(angle) * orbit,
                              0.30f + (mote % 3) * 0.34f,
                              center.z + std::sin(angle) * orbit};
            DrawSphere(orb, 0.07f + (mote % 2) * 0.025f,
                       Fade(mote % 2 ? gravityPrimary : gravityAccent, 0.86f));
            if (!controller.IsReducedMotion())
                DrawGlowSegment(orb, {center.x, 0.42f, center.z}, 0.012f,
                                gravityPrimary, 0.38f);
        }
        DrawSphere({center.x, 0.45f, center.z}, 0.42f,
                   controller.IsHighContrast() ? BLACK : Color{58, 27, 112, 245});
        DrawCircle3D({center.x, 0.46f, center.z}, 0.68f,
                     {1.0f, 0.0f, 0.0f}, 90.0f,
                     Fade(gravityAccent, 0.75f));
        DrawCircle3D({center.x, 0.47f, center.z}, 0.47f,
                     {1.0f, 0.0f, 0.0f}, 90.0f,
                     Fade(gravityPrimary, 0.62f));
    }
    EndMode3D();

    Vector3 lockedTarget{};
    if (controller.GetLockedTargetPosition(lockedTarget)) {
        lockedTarget.y = 1.55f;
        const Vector2 screenTarget = GetWorldToScreen(lockedTarget, renderCamera);
        const float pulse = controller.IsReducedMotion()
            ? 1.0f : 0.88f + std::sin(time * 6.0f) * 0.12f;
        const float radius = 18.0f * pulse;
        const Color lockColor = controller.IsHighContrast()
            ? Color{255, 255, 0, 255} : Color{255, 205, 86, 255};
        DrawCircleLinesV(screenTarget, radius, lockColor);
        DrawLineEx({screenTarget.x - radius - 8.0f, screenTarget.y},
                   {screenTarget.x - radius + 3.0f, screenTarget.y}, 2.0f,
                   lockColor);
        DrawLineEx({screenTarget.x + radius - 3.0f, screenTarget.y},
                   {screenTarget.x + radius + 8.0f, screenTarget.y}, 2.0f,
                   lockColor);
        const Vector2 labelSize = Measure(m_font, "LOCK", 9.0f);
        DrawTextEx(m_font, "LOCK",
                   {screenTarget.x - labelSize.x * 0.5f,
                    screenTarget.y + radius + 7.0f},
                   9.0f, 0.15f, lockColor);
    }

    const PlayerState& combatPlayer = controller.GetPlayer();
    if (controller.GetHitStopTimer() > 0.0f && !controller.IsReducedMotion()) {
        const float contactFlash = Saturate(controller.GetHitStopTimer() / 0.085f);
        const Color contactColor = combatPlayer.character == CharacterId::Knight
            ? Color{235, 192, 255, 255} : Color{157, 232, 255, 255};
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(),
                      Fade(contactColor, contactFlash * 0.045f));
    }
    if (combatPlayer.animation == PlayerAnimation::Hurt) {
        const float hurtFade = 1.0f - AnimationProgress(combatPlayer);
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(),
                      Fade(Color{255, 38, 75, 255}, 0.10f * hurtFade));
    } else if (combatPlayer.animation == PlayerAnimation::Ultimate
               && !controller.IsReducedMotion()) {
        const float flash = SmoothPulse(Saturate((AnimationProgress(combatPlayer) - 0.24f)
                                                / 0.34f));
        if (flash > 0.01f)
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(),
                          Fade(combatPlayer.character == CharacterId::Knight
                                   ? Color{245, 198, 255, 255}
                                   : Color{132, 225, 255, 255},
                               flash * 0.055f));
    }

    RenderHUD(controller);
    if (controller.GetPhase() == Phase::UpgradeChoice) RenderUpgradeChoice(controller);
    if (controller.GetPhase() == Phase::RunFailed || controller.GetPhase() == Phase::RunVictory)
        RenderResult(controller);
    if (controller.IsRecordsVisible()) RenderRecords(controller);
    if (controller.IsPerformanceVisible()) RenderPerformance(controller);

    if (controller.IsPaused()) {
        const float sw = static_cast<float>(GetScreenWidth());
        const float sh = static_cast<float>(GetScreenHeight());
        const float scale = std::clamp(std::min(sw / 1280.0f, sh / 720.0f),
                                       0.68f, 1.55f);
        DrawRectangleGradientV(0, 0, (int)sw, (int)sh,
                               Color{3, 3, 12, 185}, Color{2, 1, 8, 238});
        const Rectangle panel{sw * 0.20f, sh * 0.20f,
                              sw * 0.60f, sh * 0.60f};
        const Color heroAccent = controller.GetPlayer().character == CharacterId::Knight
            ? Color{207, 126, 255, 255} : Color{79, 221, 255, 255};
        if (m_resultLedgerFrameTexture.id != 0) {
            const Rectangle source{0.0f, 0.0f,
                                   (float)m_resultLedgerFrameTexture.width,
                                   (float)m_resultLedgerFrameTexture.height};
            DrawTexturePro(m_resultLedgerFrameTexture, source, panel,
                           {0.0f, 0.0f}, 0.0f,
                           Color{205, 198, 218, 250});
        } else {
            DrawPanel(panel, heroAccent);
        }
        const Rectangle inner{panel.x + panel.width * 0.095f,
                              panel.y + panel.height * 0.15f,
                              panel.width * 0.81f,
                              panel.height * 0.68f};
        DrawRectangleRec(inner, Color{7, 7, 18, 208});
        DrawRectangleLinesEx(inner, 1.5f * scale,
                             Fade(heroAccent, 0.58f));

        DrawCentered("RIFT PAUSED", panel.y + panel.height * 0.175f,
                     34.0f * scale, Color{255, 224, 143, 255});
        char pauseStatus[128];
        std::snprintf(pauseStatus, sizeof(pauseStatus),
                      "WAVE %d / 50     ENEMIES %d     SCORE %d",
                      controller.GetWave(), controller.GetActiveEnemyCount(),
                      controller.GetScore());
        DrawCentered(pauseStatus, panel.y + panel.height * 0.295f,
                     12.0f * scale, Color{177, 202, 223, 255});
        DrawLineEx({inner.x + inner.width * 0.12f,
                    panel.y + panel.height * 0.375f},
                   {inner.x + inner.width * 0.88f,
                    panel.y + panel.height * 0.375f},
                   1.5f * scale, Fade(heroAccent, 0.48f));

        const float buttonW = panel.width * 0.31f;
        const float buttonH = panel.height * 0.145f;
        const float buttonY = panel.y + panel.height * 0.53f;
        const Rectangle buttons[2] = {
            {panel.x + panel.width * 0.17f, buttonY, buttonW, buttonH},
            {panel.x + panel.width * 0.52f, buttonY, buttonW, buttonH}
        };
        const char* labels[2] = {"RESUME", "RETURN TO MENU"};
        const char* hints[2] = {"ESC", "Q"};
        const Vector2 mouse = GetMousePosition();
        for (int index = 0; index < 2; ++index) {
            const bool hovered = CheckCollisionPointRec(mouse, buttons[index]);
            DrawRectangleRec({buttons[index].x + 4.0f * scale,
                              buttons[index].y + 5.0f * scale,
                              buttons[index].width, buttons[index].height},
                             Color{0, 0, 0, 150});
            DrawRectangleRec(buttons[index], hovered
                ? Fade(heroAccent, 0.42f) : Color{24, 21, 40, 245});
            DrawRectangleLinesEx(buttons[index],
                                 hovered ? 2.5f * scale : 1.2f * scale,
                                 hovered ? Color{255, 220, 128, 255}
                                         : Fade(heroAccent, 0.66f));
            for (const Vector2 rivet : std::array<Vector2, 4>{
                    Vector2{buttons[index].x + 6.0f * scale,
                            buttons[index].y + 6.0f * scale},
                    Vector2{buttons[index].x + buttons[index].width - 6.0f * scale,
                            buttons[index].y + 6.0f * scale},
                    Vector2{buttons[index].x + 6.0f * scale,
                            buttons[index].y + buttons[index].height - 6.0f * scale},
                    Vector2{buttons[index].x + buttons[index].width - 6.0f * scale,
                            buttons[index].y + buttons[index].height - 6.0f * scale}}) {
                DrawCircleV(rivet, 2.0f * scale, Color{211, 158, 75, 220});
            }
            const Vector2 labelSize = Measure(m_font, labels[index], 13.0f * scale);
            DrawTextEx(m_font, labels[index],
                       {buttons[index].x
                            + (buttons[index].width - labelSize.x) * 0.5f,
                        buttons[index].y + 12.0f * scale},
                       13.0f * scale, 0.25f,
                       hovered ? Color{255, 237, 185, 255} : RAYWHITE);
            const Vector2 hintSize = Measure(m_font, hints[index], 8.5f * scale);
            DrawTextEx(m_font, hints[index],
                       {buttons[index].x
                            + (buttons[index].width - hintSize.x) * 0.5f,
                        buttons[index].y + 36.0f * scale},
                       8.5f * scale, 0.1f, Color{151, 143, 171, 255});
        }

        DrawCentered("SPACE  JUMP     L  DASH     MOUSE  CAMERA     WHEEL  ZOOM",
                     panel.y + panel.height * 0.735f, 10.0f * scale,
                     Color{173, 203, 221, 255});
        DrawCentered("F3  PERFORMANCE     F4  CONTRAST     F5  REDUCED MOTION     F6  UI SCALE",
                     panel.y + panel.height * 0.795f, 8.5f * scale,
                     Color{137, 129, 158, 255});
    }
}

void SurvivalView::RenderHUD(const SurvivalController& controller) const {
    const float sw = (float)GetScreenWidth();
    const float sh = (float)GetScreenHeight();
    const float scale = std::clamp(std::min(sw / 1280.0f, sh / 720.0f)
                                   * controller.GetUiScale(), 0.65f, 1.65f);
    const PlayerState& player = controller.GetPlayer();

    const Rectangle playerPanel{18.0f * scale, 18.0f * scale, 310.0f * scale, 96.0f * scale};
    DrawPanel(playerPanel, player.character == CharacterId::Knight
        ? Color{196, 121, 235, 255} : Color{92, 209, 255, 255});
    const char* hero = player.character == CharacterId::Knight ? "KNIGHT" : "MAGIC CASTER";
    DrawTextEx(m_font, hero, {playerPanel.x + 16 * scale, playerPanel.y + 12 * scale},
               18 * scale, 0.5f, Color{255, 232, 181, 255});

    const Rectangle hpBack{playerPanel.x + 16 * scale, playerPanel.y + 42 * scale,
                           278 * scale, 18 * scale};
    DrawRectangleRounded(hpBack, 0.35f, 8, Color{40, 19, 31, 255});
    const float hpRatio = player.maxHp > 0.0f ? std::clamp(player.hp / player.maxHp, 0.0f, 1.0f) : 0.0f;
    DrawRectangleRounded({hpBack.x, hpBack.y, hpBack.width * hpRatio, hpBack.height},
                         0.35f, 8, Color{213, 66, 92, 255});
    if (player.shield > 0.0f) {
        const float shieldRatio = std::clamp(player.shield / player.maxHp, 0.0f, 1.0f);
        DrawRectangleRounded({hpBack.x, hpBack.y, hpBack.width * shieldRatio, 4 * scale},
                             0.35f, 8, Color{74, 213, 255, 255});
    }
    char text[128];
    std::snprintf(text, sizeof(text), "HP %.0f / %.0f", player.hp, player.maxHp);
    DrawTextEx(m_font, text, {hpBack.x + 7 * scale, hpBack.y + 1 * scale},
               12 * scale, 0.2f, WHITE);

    const Rectangle ultBack{hpBack.x, hpBack.y + 25 * scale, hpBack.width, 10 * scale};
    DrawRectangleRec(ultBack, Color{30, 29, 62, 255});
    DrawRectangleRec({ultBack.x, ultBack.y,
                      ultBack.width * std::clamp(player.ultimateCharge / 100.0f, 0.0f, 1.0f),
                      ultBack.height}, Color{103, 194, 255, 255});
    if (player.shield > 0.0f) {
        std::snprintf(text, sizeof(text), "SHIELD %.0f", player.shield);
        DrawTextEx(m_font, text, {playerPanel.x + 205 * scale, playerPanel.y + 13 * scale},
                   10 * scale, 0.2f, Color{103, 225, 255, 255});
    }

    const Rectangle wavePanel{sw * 0.5f - 125 * scale, 18 * scale, 250 * scale, 66 * scale};
    DrawPanel(wavePanel, Color{237, 188, 78, 255});
    std::snprintf(text, sizeof(text), "WAVE %d / 50", controller.GetWave());
    const Vector2 waveMeasure = Measure(m_font, text, 24 * scale);
    DrawTextEx(m_font, text, {wavePanel.x + (wavePanel.width - waveMeasure.x) * 0.5f,
                              wavePanel.y + 10 * scale}, 24 * scale, 0.6f,
               Color{255, 225, 139, 255});
    std::snprintf(text, sizeof(text), "ENEMIES  %d + %d", controller.GetActiveEnemyCount(),
                  controller.GetRemainingSpawnCount());
    const Vector2 enemyMeasure = Measure(m_font, text, 12 * scale);
    DrawTextEx(m_font, text, {wavePanel.x + (wavePanel.width - enemyMeasure.x) * 0.5f,
                              wavePanel.y + 41 * scale}, 12 * scale, 0.2f,
               Color{211, 200, 227, 255});

    const Rectangle radar{sw - 178 * scale, 18 * scale, 160 * scale, 160 * scale};
    DrawPanel(radar, Color{112, 96, 153, 255});
    const Vector2 radarCenter{radar.x + radar.width * 0.5f, radar.y + radar.height * 0.55f};
    DrawCircleV(radarCenter, 58 * scale, Color{9, 13, 24, 245});
    DrawCircleLinesV(radarCenter, 58 * scale, Color{96, 126, 153, 230});
    for (const EnemyState& enemy : controller.GetEnemies()) {
        if (!enemy.active) continue;
        const Vector3 relative{enemy.position.x - player.position.x, 0.0f,
                               enemy.position.z - player.position.z};
        const float length = LengthXZ(relative);
        const float factor = length > 20.0f ? 20.0f / length : 1.0f;
        const Vector2 dot{radarCenter.x + relative.x * factor * 2.7f * scale,
                          radarCenter.y + relative.z * factor * 2.7f * scale};
        DrawCircleV(dot, IsBoss(enemy.archetype) ? 5 * scale : 2.5f * scale,
                    IsBoss(enemy.archetype)
                        ? Color{255, 190, 65, 255} : Color{240, 83, 76, 255});
    }
    DrawCircleV(radarCenter, 4 * scale, Color{86, 217, 255, 255});

    const float skillW = 126 * scale;
    const float skillGap = 8 * scale;
    const float skillStart = sw * 0.5f - (skillW * 4 + skillGap * 3) * 0.5f;
    const float skillY = sh - 70 * scale;
    const char* labels[4] = {"J  BASIC", "K  SKILL 1", "U  SKILL 2", "H  ULTIMATE"};
    const float cooldowns[4] = {player.basicCooldown, player.skillOneCooldown,
                                player.skillTwoCooldown, player.ultimateCharge >= 100.0f ? 0.0f : -1.0f};
    for (int i = 0; i < 4; ++i) {
        const Rectangle box{skillStart + i * (skillW + skillGap), skillY, skillW, 48 * scale};
        DrawRectangleRounded(box, 0.18f, 8, Color{22, 15, 38, 235});
        DrawRectangleRoundedLinesEx(box, 0.18f, 8, 1.5f,
                                    cooldowns[i] == 0.0f ? Color{239, 195, 85, 255}
                                                         : Color{103, 83, 133, 255});
        DrawTextEx(m_font, labels[i], {box.x + 9 * scale, box.y + 8 * scale},
                   12 * scale, 0.2f, RAYWHITE);
        if (cooldowns[i] > 0.0f) {
            std::snprintf(text, sizeof(text), "%.1fs", cooldowns[i]);
            DrawTextEx(m_font, text, {box.x + 9 * scale, box.y + 27 * scale},
                       10 * scale, 0.2f, Color{207, 166, 227, 255});
        } else if (cooldowns[i] < 0.0f) {
            std::snprintf(text, sizeof(text), "%.0f%%", player.ultimateCharge);
            DrawTextEx(m_font, text, {box.x + 9 * scale, box.y + 27 * scale},
                       10 * scale, 0.2f, Color{105, 204, 255, 255});
        } else {
            DrawTextEx(m_font, "READY", {box.x + 9 * scale, box.y + 27 * scale},
                       10 * scale, 0.2f, Color{112, 230, 159, 255});
        }
    }

    if (controller.GetPhase() == Phase::PreWave) {
        std::snprintf(text, sizeof(text), "WAVE %d STARTS IN %d", controller.GetWave(),
                      std::max(1, (int)std::ceil(controller.GetPhaseTimer())));
        DrawCentered(text, sh * 0.30f, 32 * scale, Color{255, 225, 135, 255});
    } else if (controller.GetPhase() == Phase::WaveClear) {
        DrawCentered("WAVE CLEAR", sh * 0.31f, 38 * scale, Color{119, 238, 174, 255});
    }

    if (controller.GetWave() % 10 == 0 && controller.GetActiveEnemyCount() > 0) {
        const EnemyState* boss = nullptr;
        for (const EnemyState& enemy : controller.GetEnemies())
            if (enemy.active && IsBoss(enemy.archetype)) { boss = &enemy; break; }
        if (boss) {
            const Rectangle bossBack{sw * 0.20f, sh * 0.16f, sw * 0.60f, 20 * scale};
            DrawRectangleRounded(bossBack, 0.3f, 8, Color{40, 15, 21, 240});
            DrawRectangleRounded({bossBack.x, bossBack.y,
                                  bossBack.width * std::clamp(boss->hp / boss->maxHp, 0.0f, 1.0f),
                                  bossBack.height}, 0.3f, 8, Color{209, 52, 67, 255});
            char bossTitle[128];
            if (boss->archetype == EnemyArchetype::EclipseChimera) {
                std::snprintf(bossTitle, sizeof(bossTitle), "%s    %s STANCE",
                              BossName(boss->archetype), boss->bossPhase == 1 ? "SOLAR" : "LUNAR");
            } else {
                const int phaseCount = boss->archetype == EnemyArchetype::VoidSovereign ? 3 : 2;
                std::snprintf(bossTitle, sizeof(bossTitle), "%s    PHASE %d / %d",
                              BossName(boss->archetype), boss->bossPhase, phaseCount);
            }
            DrawCentered(bossTitle, bossBack.y - 24 * scale, 15 * scale,
                         Color{255, 194, 135, 255});
        }
    }

    const std::string version = "BALANCE " + controller.GetBalanceVersion();
    DrawTextEx(m_font, version.c_str(), {18 * scale, sh - 22 * scale},
               9 * scale, 0.1f, Color{123, 109, 151, 190});

    const char* cameraHelp = controller.IsTargetLockActive()
        ? "SPACE JUMP  |  L DASH  |  WHEEL ZOOM  |  T / MMB UNLOCK"
        : "SPACE JUMP  |  L DASH  |  WHEEL ZOOM  |  T / MMB LOCK";
    const Vector2 cameraHelpSize = Measure(m_font, cameraHelp, 9.0f * scale);
    DrawTextEx(m_font, cameraHelp,
               {sw - cameraHelpSize.x - 18.0f * scale,
                sh - 22.0f * scale},
               9.0f * scale, 0.1f,
               controller.IsTargetLockActive()
                   ? Color{255, 205, 86, 220}
                   : Color{138, 153, 181, 190});
}

void SurvivalView::RenderCharacterSelect(const SurvivalController& controller) const {
    const float sw = (float)GetScreenWidth();
    const float sh = (float)GetScreenHeight();
    const float scale = std::clamp(std::min(sw / 1280.0f, sh / 720.0f),
                                   0.68f, 1.55f);
    DrawRectangleGradientV(0, 0, (int)sw, (int)sh,
                           Fade(Color{5, 3, 16, 255}, 0.30f),
                           Fade(Color{3, 2, 12, 255}, 0.74f));

    const Rectangle titlePanel{sw * 0.245f, sh * 0.035f,
                               sw * 0.510f, sh * 0.145f};
    DrawPanel(titlePanel, Color{198, 140, 255, 230});
    DrawRectangleGradientH((int)(titlePanel.x + titlePanel.width * 0.12f),
                           (int)(titlePanel.y + titlePanel.height - 5.0f * scale),
                           (int)(titlePanel.width * 0.76f), (int)(2.0f * scale),
                           Fade(Color{98, 215, 255, 255}, 0.05f),
                           Fade(Color{255, 192, 86, 255}, 0.92f));
    DrawCentered("AEGIS RIFT", titlePanel.y + 15.0f * scale,
                 34.0f * scale, Color{255, 224, 139, 255});
    DrawCentered("CHOOSE YOUR CHAMPION", titlePanel.y + 61.0f * scale,
                 13.0f * scale, Color{198, 216, 235, 255});

    const Rectangle cards[2] = {
        {sw * 0.035f, sh * 0.48f, sw * 0.275f, sh * 0.32f},
        {sw * 0.690f, sh * 0.48f, sw * 0.275f, sh * 0.32f}
    };
    const char* names[2] = {"KNIGHT", "MAGIC CASTER"};
    const char* roles[2] = {"VANGUARD / MELEE", "ARCANIST / CONTROL"};
    const char* descriptions[2] = {
        "Break the swarm with steel, guard, and relentless forward pressure.",
        "Control the battlefield with ranged spells, frost, and astral force."
    };
    const char* abilities[2][5] = {
        {"VIOLET EDGE", "AEGIS COUNTER", "SHIELD RUSH", "BASTION BREAKER", "STEEL STEP"},
        {"ARC BOLT", "FROST RING", "GRAVITY WELL", "ASTRAL TEMPEST", "PHASE BLINK"}
    };
    const char* keys[5] = {"J", "K", "U", "H", "L"};
    for (int i = 0; i < 2; ++i) {
        const bool selected = controller.GetSelectedCharacter() == i;
        const Color accent = i == 0 ? Color{205, 114, 255, 255}
                                    : Color{72, 216, 255, 255};
        DrawPanel(cards[i], selected ? Color{255, 209, 92, 255}
                                     : Fade(accent, 0.62f));
        DrawRectangleRec({cards[i].x, cards[i].y, 5.0f * scale, cards[i].height},
                         Fade(accent, selected ? 0.92f : 0.42f));
        const Vector2 nameSize = Measure(m_font, names[i], 24.0f * scale);
        DrawTextEx(m_font, names[i], {cards[i].x + (cards[i].width - nameSize.x) * 0.5f,
                                      cards[i].y + 14.0f * scale},
                   24.0f * scale, 0.6f,
                   selected ? Color{255, 231, 151, 255} : RAYWHITE);
        const Vector2 roleSize = Measure(m_font, roles[i], 11.0f * scale);
        DrawTextEx(m_font, roles[i], {cards[i].x + (cards[i].width - roleSize.x) * 0.5f,
                                      cards[i].y + 46.0f * scale},
                   11.0f * scale, 0.2f, accent);
        DrawLineEx({cards[i].x + 16.0f * scale, cards[i].y + 67.0f * scale},
                   {cards[i].x + cards[i].width - 16.0f * scale,
                    cards[i].y + 67.0f * scale}, 1.0f * scale,
                   Fade(accent, 0.38f));
        DrawWrappedCentered(m_font, descriptions[i],
                            {cards[i].x + 18.0f * scale,
                             cards[i].y + 73.0f * scale,
                             cards[i].width - 36.0f * scale,
                             45.0f * scale},
                            10.5f * scale, Color{205, 216, 231, 255});
        for (int skill = 0; skill < 5; ++skill) {
            const int column = skill % 2;
            const int row = skill / 2;
            const float cellW = (cards[i].width - 34.0f * scale) * 0.5f;
            const float x = cards[i].x + 16.0f * scale + column * cellW;
            const float y = cards[i].y + 127.0f * scale + row * 25.0f * scale;
            const Rectangle keyBox{x, y, 19.0f * scale, 18.0f * scale};
            DrawRectangleRounded(keyBox, 0.25f, 6,
                                 Fade(accent, selected ? 0.62f : 0.30f));
            DrawRectangleRoundedLinesEx(keyBox, 0.25f, 6, 1.0f, accent);
            DrawTextEx(m_font, keys[skill],
                       {x + 5.0f * scale, y + 2.0f * scale},
                       10.0f * scale, 0.1f, WHITE);
            DrawTextEx(m_font, abilities[i][skill],
                       {x + 25.0f * scale, y + 3.0f * scale},
                       8.4f * scale, 0.1f, Color{222, 211, 233, 255});
        }
        const char* state = selected ? "SELECTED" : "HOVER OR CLICK TO SELECT";
        const Vector2 stateSize = Measure(m_font, state, 10.0f * scale);
        DrawTextEx(m_font, state,
                   {cards[i].x + (cards[i].width - stateSize.x) * 0.5f,
                    cards[i].y + cards[i].height - 23.0f * scale},
                   10.0f * scale, 0.2f,
                   selected ? Color{126, 239, 181, 255}
                            : Color{145, 132, 169, 255});
    }

    const Rectangle deploy{sw * 0.385f, sh * 0.835f, sw * 0.230f, sh * 0.082f};
    const bool deployHover = CheckCollisionPointRec(GetMousePosition(), deploy);
    const Color deployAccent = controller.GetSelectedCharacter() == 0
        ? Color{210, 118, 255, 255} : Color{70, 219, 255, 255};
    DrawRectangleRounded({deploy.x + 4.0f * scale, deploy.y + 6.0f * scale,
                          deploy.width, deploy.height}, 0.28f, 10,
                         Color{0, 0, 0, 145});
    DrawRectangleRounded(deploy, 0.28f, 10,
                         deployHover ? Fade(deployAccent, 0.82f)
                                     : Color{28, 21, 49, 248});
    DrawRectangleRoundedLinesEx(deploy, 0.28f, 10, 2.0f * scale,
                                deployHover ? Color{255, 228, 143, 255}
                                            : deployAccent);
    const char* deployText = controller.GetSelectedCharacter() == 0
        ? "DEPLOY KNIGHT" : "DEPLOY MAGIC CASTER";
    const Vector2 deploySize = Measure(m_font, deployText, 15.0f * scale);
    DrawTextEx(m_font, deployText,
               {deploy.x + (deploy.width - deploySize.x) * 0.5f,
                deploy.y + (deploy.height - 15.0f * scale) * 0.43f},
               15.0f * scale, 0.3f, Color{255, 236, 184, 255});

    DrawCentered("A / D OR ARROWS  SELECT     ENTER  DEPLOY     TAB  RECORDS     ESC  RETURN",
                 sh * 0.945f, 11.0f * scale, Color{203, 190, 221, 255});
}

void SurvivalView::RenderUpgradeChoice(const SurvivalController& controller) const {
    const float sw = (float)GetScreenWidth();
    const float sh = (float)GetScreenHeight();
    DrawRectangle(0, 0, (int)sw, (int)sh, Color{4, 2, 12, 220});
    DrawCentered("CHOOSE ONE UPGRADE", sh * 0.13f, 38.0f, Color{255, 218, 111, 255});
    DrawCentered("THE RIFT IS FROZEN WHILE YOU DECIDE", sh * 0.21f, 15.0f,
                 Color{181, 169, 205, 255});

    const float cardW = sw * 0.25f;
    const float gap = sw * 0.025f;
    const float startX = (sw - cardW * 3.0f - gap * 2.0f) * 0.5f;
    const auto& options = controller.GetUpgradeOptions();
    for (int i = 0; i < 3; ++i) {
        const Rectangle card{startX + i * (cardW + gap), sh * 0.32f, cardW, sh * 0.40f};
        const bool selected = controller.GetSelectedUpgrade() == i;
        DrawRectangleRounded({card.x + 7.0f, card.y + 9.0f,
                              card.width, card.height},
                             0.08f, 6, Color{0, 0, 0, 175});
        if (selected) {
            DrawRectangleRoundedLinesEx({card.x - 5.0f, card.y - 5.0f,
                                         card.width + 10.0f,
                                         card.height + 10.0f},
                                        0.08f, 6, 3.0f,
                                        Fade(options[i].accent, 0.76f));
        }
        if (m_upgradeCardFrameTexture.id != 0) {
            const Rectangle source{0.0f, 0.0f,
                                   (float)m_upgradeCardFrameTexture.width,
                                   (float)m_upgradeCardFrameTexture.height};
            DrawTexturePro(m_upgradeCardFrameTexture, source, card,
                           {0.0f, 0.0f}, 0.0f,
                           selected ? WHITE : Color{185, 179, 196, 238});
        } else {
            DrawPanel(card, selected ? options[i].accent
                                     : Color{103, 84, 135, 255});
        }
        const Vector2 iconCenter{card.x + card.width * 0.5f,
                                 card.y + card.height * 0.245f};
        DrawPoly(iconCenter, 6, card.height * 0.084f, 30.0f,
                 Fade(Color{11, 11, 22, 255}, 0.96f));
        DrawPolyLinesEx(iconCenter, 6, card.height * 0.098f, 30.0f,
                        selected ? 3.0f : 1.5f, options[i].accent);
        DrawCircleV(iconCenter, card.height * 0.044f,
                    Fade(options[i].accent, selected ? 0.92f : 0.65f));
        const Rectangle nameBounds{card.x + card.width * 0.15f,
                                   card.y + card.height * 0.385f,
                                   card.width * 0.70f,
                                   card.height * 0.16f};
        DrawWrappedCentered(m_font, options[i].name, nameBounds, 22.0f,
                            selected ? Color{255, 238, 181, 255} : RAYWHITE);
        DrawLineEx({card.x + card.width * 0.20f,
                    card.y + card.height * 0.555f},
                   {card.x + card.width * 0.80f,
                    card.y + card.height * 0.555f},
                   1.5f, Fade(options[i].accent, 0.56f));
        const Rectangle descBounds{card.x + card.width * 0.16f,
                                   card.y + card.height * 0.585f,
                                   card.width * 0.68f,
                                   card.height * 0.19f};
        DrawWrappedCentered(m_font, options[i].description, descBounds, 14.0f,
                            Color{198, 211, 226, 255});
        char key[8];
        std::snprintf(key, sizeof(key), "%d", i + 1);
        const Vector2 keyCenter{card.x + card.width * 0.145f,
                                card.y + card.height * 0.115f};
        DrawCircleV(keyCenter, 15.0f, Color{13, 10, 24, 240});
        DrawCircleLinesV(keyCenter, 15.0f, options[i].accent);
        const Vector2 keySize = Measure(m_font, key, 16.0f);
        DrawTextEx(m_font, key, {keyCenter.x - keySize.x * 0.5f,
                                 keyCenter.y - 8.0f},
                   16.0f, 0.2f, options[i].accent);
        const char* rarity = RarityName(options[i].rarity);
        const Vector2 raritySize = MeasureTextEx(m_font, rarity, 11.0f, 0.2f);
        DrawTextEx(m_font, rarity,
                   {card.x + card.width * 0.855f - raritySize.x,
                    card.y + card.height * 0.09f},
                   11.0f, 0.2f, options[i].accent);
        char stack[32];
        const int currentStack = controller.GetUpgradeStack(options[i].upgrade);
        std::snprintf(stack, sizeof(stack), "STACK %d / %d",
                      currentStack + 1, options[i].maxStacks);
        const Vector2 stackSize = MeasureTextEx(m_font, stack, 11.0f, 0.2f);
        DrawTextEx(m_font, stack,
                   {card.x + (card.width - stackSize.x) * 0.5f,
                    card.y + card.height * 0.825f},
                   11.0f, 0.2f, Color{164, 150, 184, 255});
    }
    DrawCentered("1 / 2 / 3 OR CLICK   |   ENTER TO CONFIRM", sh * 0.80f, 15.0f,
                 Color{207, 190, 225, 255});
}

void SurvivalView::RenderResult(const SurvivalController& controller) const {
    const float sw = (float)GetScreenWidth();
    const float sh = (float)GetScreenHeight();
    const float scale = std::clamp(std::min(sw / 1280.0f, sh / 720.0f),
                                   0.68f, 1.55f);
    DrawRectangleGradientV(0, 0, (int)sw, (int)sh,
                           Color{3, 2, 10, 182}, Color{5, 2, 15, 238});
    const bool victory = controller.GetPhase() == Phase::RunVictory;
    const Color accent = victory ? Color{255, 208, 87, 255}
                                 : Color{236, 92, 132, 255};
    const Rectangle panel{sw * 0.13f, sh * 0.075f, sw * 0.74f, sh * 0.82f};
    if (m_resultLedgerFrameTexture.id != 0) {
        const Rectangle source{0.0f, 0.0f,
                               (float)m_resultLedgerFrameTexture.width,
                               (float)m_resultLedgerFrameTexture.height};
        DrawTexturePro(m_resultLedgerFrameTexture, source, panel,
                       {0.0f, 0.0f}, 0.0f, WHITE);
        DrawRectangleRec({panel.x + panel.width * 0.095f,
                          panel.y + panel.height * 0.13f,
                          panel.width * 0.81f,
                          panel.height * 0.70f},
                         Fade(Color{6, 5, 14, 255}, 0.24f));
    } else {
        DrawPanel(panel, accent);
    }
    DrawRectangleGradientH((int)(panel.x + panel.width * 0.10f),
                           (int)(panel.y + 82.0f * scale),
                           (int)(panel.width * 0.80f), (int)(2.0f * scale),
                           Fade(accent, 0.04f), Fade(accent, 0.82f));
    DrawCentered(victory ? "RIFT CONQUERED" : "RUN ENDED",
                 panel.y + 19.0f * scale, 36.0f * scale,
                 victory ? Color{255, 224, 139, 255}
                         : Color{255, 155, 171, 255});
    const char* hero = controller.GetPlayer().character == CharacterId::Knight
        ? "KNIGHT" : "MAGIC CASTER";
    char text[160];
    std::snprintf(text, sizeof(text), "%s  /  AEGIS RIFT EXPEDITION", hero);
    DrawCentered(text, panel.y + 62.0f * scale, 11.0f * scale,
                 Color{184, 202, 224, 255});

    const int minutes = (int)controller.GetRunTime() / 60;
    const int seconds = (int)controller.GetRunTime() % 60;
    char values[6][64];
    std::snprintf(values[0], sizeof(values[0]), "%d / 50", controller.GetWave());
    std::snprintf(values[1], sizeof(values[1]), "%d", controller.GetScore());
    std::snprintf(values[2], sizeof(values[2]), "%02d:%02d", minutes, seconds);
    std::snprintf(values[3], sizeof(values[3]), "%d", controller.GetKills());
    std::snprintf(values[4], sizeof(values[4]), "%d", controller.GetBossesKilled());
    std::snprintf(values[5], sizeof(values[5]), "%d", controller.GetDamageTaken());
    const char* labels[6] = {
        "HIGHEST WAVE", "RIFT SCORE", "SURVIVAL TIME",
        "ENEMIES DEFEATED", "BOSSES DEFEATED", "DAMAGE TAKEN"
    };
    const float gridX = panel.x + panel.width * 0.075f;
    const float gridY = panel.y + 112.0f * scale;
    const float gridGap = 12.0f * scale;
    const float tileW = (panel.width * 0.85f - gridGap * 2.0f) / 3.0f;
    const float tileH = 92.0f * scale;
    for (int index = 0; index < 6; ++index) {
        const int row = index / 3;
        const int column = index % 3;
        const Rectangle tile{gridX + column * (tileW + gridGap),
                             gridY + row * (tileH + gridGap), tileW, tileH};
        DrawRectangleRec(tile, index == 1 ? Color{42, 31, 65, 238}
                                          : Color{18, 16, 31, 228});
        DrawRectangleLinesEx(tile, 1.2f * scale,
                             Fade(index == 1 ? accent
                                             : Color{130, 111, 158, 255},
                                  index == 1 ? 0.85f : 0.55f));
        for (const Vector2 rivet : std::array<Vector2, 4>{
                 Vector2{tile.x + 6.0f * scale, tile.y + 6.0f * scale},
                 Vector2{tile.x + tile.width - 6.0f * scale,
                         tile.y + 6.0f * scale},
                 Vector2{tile.x + 6.0f * scale,
                         tile.y + tile.height - 6.0f * scale},
                 Vector2{tile.x + tile.width - 6.0f * scale,
                         tile.y + tile.height - 6.0f * scale}}) {
            DrawCircleV(rivet, 2.1f * scale, Color{123, 91, 61, 230});
            DrawCircleV({rivet.x - 0.5f * scale, rivet.y - 0.5f * scale},
                        0.7f * scale, Color{230, 189, 117, 210});
        }
        const Vector2 labelSize = Measure(m_font, labels[index], 9.5f * scale);
        DrawTextEx(m_font, labels[index],
                   {tile.x + (tile.width - labelSize.x) * 0.5f,
                    tile.y + 14.0f * scale},
                   9.5f * scale, 0.15f, Color{155, 146, 176, 255});
        const Vector2 valueSize = Measure(m_font, values[index], 24.0f * scale);
        DrawTextEx(m_font, values[index],
                   {tile.x + (tile.width - valueSize.x) * 0.5f,
                    tile.y + 43.0f * scale},
                   24.0f * scale, 0.25f,
                   index == 1 ? Color{255, 229, 154, 255} : RAYWHITE);
    }

    const auto& service = SurvivalRunService::GetInstance();
    const Rectangle reward{panel.x + panel.width * 0.18f,
                           panel.y + 332.0f * scale,
                           panel.width * 0.64f, 58.0f * scale};
    DrawRectangleRounded(reward, 0.22f, 8, Color{20, 39, 43, 240});
    DrawRectangleRoundedLinesEx(reward, 0.22f, 8, 1.4f * scale,
                                Color{91, 222, 174, 230});
    DrawCircleV({reward.x + 31.0f * scale, reward.y + reward.height * 0.5f},
                13.0f * scale, Color{255, 194, 63, 255});
    DrawCircleLinesV({reward.x + 31.0f * scale, reward.y + reward.height * 0.5f},
                     16.0f * scale, Color{255, 228, 128, 230});
    std::snprintf(text, sizeof(text), "EXPEDITION REWARD   +%d COINS",
                  service.GetLastCoinReward());
    DrawTextEx(m_font, text,
               {reward.x + 57.0f * scale, reward.y + 12.0f * scale},
               14.0f * scale, 0.2f, Color{141, 239, 194, 255});
    DrawTextEx(m_font, service.GetSyncLabel().c_str(),
               {reward.x + 57.0f * scale, reward.y + 34.0f * scale},
               9.0f * scale, 0.1f, Color{144, 176, 183, 255});

    const float buttonW = sw * 0.18f;
    const float buttonGap = sw * 0.025f;
    const float startX = (sw - buttonW * 3.0f - buttonGap * 2.0f) * 0.5f;
    const float buttonY = sh * 0.785f;
    const float buttonH = sh * 0.082f;
    const char* buttonLabels[3] = {"RETRY", "RIFT RECORDS", "RETURN TO MENU"};
    const char* buttonHints[3] = {"R", "TAB", "ESC"};
    for (int index = 0; index < 3; ++index) {
        const bool selected = controller.GetSelectedResultAction() == index;
        const Rectangle button{startX + index * (buttonW + buttonGap),
                               buttonY, buttonW, buttonH};
        DrawRectangleRounded({button.x + 3.0f * scale, button.y + 5.0f * scale,
                              button.width, button.height},
                             0.24f, 8, Color{0, 0, 0, 145});
        DrawRectangleRounded(button, 0.24f, 8,
                             selected ? Fade(accent, 0.44f)
                                      : Color{28, 21, 45, 248});
        DrawRectangleRoundedLinesEx(button, 0.24f, 8,
                                    selected ? 2.0f * scale : 1.0f * scale,
                                    selected ? accent
                                             : Color{108, 92, 132, 230});
        const Vector2 labelSize = Measure(m_font, buttonLabels[index], 12.0f * scale);
        DrawTextEx(m_font, buttonLabels[index],
                   {button.x + (button.width - labelSize.x) * 0.5f,
                    button.y + 13.0f * scale},
                   12.0f * scale, 0.2f,
                   selected ? Color{255, 235, 183, 255} : RAYWHITE);
        const Vector2 hintSize = Measure(m_font, buttonHints[index], 8.0f * scale);
        DrawTextEx(m_font, buttonHints[index],
                   {button.x + (button.width - hintSize.x) * 0.5f,
                    button.y + 36.0f * scale},
                   8.0f * scale, 0.1f, Color{150, 139, 169, 255});
    }
    DrawCentered("A / D OR ARROWS  SELECT     ENTER  CONFIRM",
                 panel.y + panel.height - 17.0f * scale,
                 9.5f * scale, Color{178, 164, 197, 255});
}

void SurvivalView::RenderRecords(const SurvivalController& controller) const {
    (void)controller;
    const float sw = (float)GetScreenWidth();
    const float sh = (float)GetScreenHeight();
    DrawRectangle(0, 0, (int)sw, (int)sh, Color{3, 2, 10, 235});
    const Rectangle panel{sw * 0.13f, sh * 0.10f, sw * 0.74f, sh * 0.78f};
    DrawPanel(panel, Color{181, 121, 239, 255});
    DrawCentered("RIFT RECORDS", panel.y + panel.height * 0.07f, 36.0f,
                 Color{255, 218, 112, 255});

    const auto& service = SurvivalRunService::GetInstance();
    const std::string status = service.GetSyncLabel() + "  |  PENDING "
        + std::to_string(service.GetPendingCount());
    DrawCentered(status.c_str(), panel.y + panel.height * 0.16f, 13.0f,
                 service.GetPendingCount() > 0 ? Color{255, 181, 105, 255}
                                               : Color{105, 226, 173, 255});
    DrawCentered(service.GetLeaderboardLabel().c_str(), panel.y + panel.height * 0.205f,
                 12.0f, service.HasRemoteLeaderboard()
                     ? Color{113, 214, 255, 255} : Color{184, 166, 204, 255});

    const float left = panel.x + panel.width * 0.08f;
    const float top = panel.y + panel.height * 0.275f;
    DrawTextEx(m_font, "RANK", {left, top}, 13.0f, 0.2f, Color{151, 133, 177, 255});
    DrawTextEx(m_font, "PLAYER", {left + panel.width * 0.10f, top}, 13.0f, 0.2f,
               Color{151, 133, 177, 255});
    DrawTextEx(m_font, "HERO", {left + panel.width * 0.30f, top}, 13.0f, 0.2f,
               Color{151, 133, 177, 255});
    DrawTextEx(m_font, "WAVE", {left + panel.width * 0.43f, top}, 13.0f, 0.2f,
               Color{151, 133, 177, 255});
    DrawTextEx(m_font, "SCORE", {left + panel.width * 0.54f, top}, 13.0f, 0.2f,
               Color{151, 133, 177, 255});
    DrawTextEx(m_font, "TIME", {left + panel.width * 0.72f, top}, 13.0f, 0.2f,
               Color{151, 133, 177, 255});

    const auto& runs = service.HasRemoteLeaderboard()
        ? service.GetRemoteTopRuns() : service.GetTopRuns();
    if (runs.empty()) {
        DrawCentered("NO RUNS YET - ENTER THE RIFT", panel.y + panel.height * 0.51f,
                     18.0f, Color{175, 158, 197, 255});
    }
    for (std::size_t i = 0; i < runs.size(); ++i) {
        const SurvivalRunRecord& run = runs[i];
        const float y = top + 42.0f + i * panel.height * 0.105f;
        const Rectangle row{left - 14.0f, y - 10.0f, panel.width * 0.84f, panel.height * 0.082f};
        DrawRectangleRounded(row, 0.18f, 8, i == 0 ? Color{57, 39, 74, 245}
                                                    : Color{28, 21, 42, 225});
        char cell[64];
        std::snprintf(cell, sizeof(cell), "#%d", (int)i + 1);
        DrawTextEx(m_font, cell, {left, y}, 16.0f, 0.2f,
                   i == 0 ? Color{255, 205, 83, 255} : RAYWHITE);
        std::string playerName = run.playerName.empty() ? "Player" : run.playerName;
        if (playerName.size() > 12) playerName = playerName.substr(0, 11) + ".";
        DrawTextEx(m_font, playerName.c_str(), {left + panel.width * 0.10f, y},
                   14.0f, 0.2f, Color{226, 215, 238, 255});
        DrawTextEx(m_font, run.characterId == "knight" ? "KNIGHT" : "CASTER",
                   {left + panel.width * 0.30f, y}, 14.0f, 0.2f,
                   run.characterId == "knight" ? Color{210, 139, 246, 255}
                                                : Color{91, 211, 255, 255});
        std::snprintf(cell, sizeof(cell), "%d", run.highestWave);
        DrawTextEx(m_font, cell, {left + panel.width * 0.43f, y}, 15.0f, 0.2f, RAYWHITE);
        std::snprintf(cell, sizeof(cell), "%d", run.score);
        DrawTextEx(m_font, cell, {left + panel.width * 0.54f, y}, 15.0f, 0.2f, RAYWHITE);
        const int seconds = run.survivalMs / 1000;
        std::snprintf(cell, sizeof(cell), "%02d:%02d", seconds / 60, seconds % 60);
        DrawTextEx(m_font, cell, {left + panel.width * 0.72f, y}, 15.0f, 0.2f,
                   Color{188, 211, 231, 255});
    }
    DrawCentered("TAB / ESC  CLOSE", panel.y + panel.height * 0.92f, 14.0f,
                 Color{208, 190, 225, 255});
}

void SurvivalView::RenderPerformance(const SurvivalController& controller) const {
    const float sw = (float)GetScreenWidth();
    const float sh = (float)GetScreenHeight();
    const Rectangle panel{sw - 248.0f, sh - 186.0f, 230.0f, 166.0f};
    DrawRectangleRounded(panel, 0.08f, 8, Color{5, 10, 16, 225});
    DrawRectangleRoundedLinesEx(panel, 0.08f, 8, 1.5f, Color{76, 213, 164, 230});
    DrawTextEx(m_font, "PERFORMANCE", {panel.x + 14.0f, panel.y + 11.0f},
               14.0f, 0.2f, Color{114, 235, 183, 255});
    char line[96];
    std::snprintf(line, sizeof(line), "FPS %d   AVG %.2f ms", GetFPS(),
                  controller.GetAverageFrameMs());
    DrawTextEx(m_font, line, {panel.x + 14.0f, panel.y + 40.0f}, 12.0f, 0.2f, RAYWHITE);
    std::snprintf(line, sizeof(line), "PEAK FRAME %.2f ms", controller.GetPeakFrameMs());
    DrawTextEx(m_font, line, {panel.x + 14.0f, panel.y + 63.0f}, 12.0f, 0.2f,
               Color{190, 207, 225, 255});
    std::snprintf(line, sizeof(line), "ENEMY %d  PEAK %d", controller.GetActiveEnemyCount(),
                  controller.GetPeakEnemyCount());
    DrawTextEx(m_font, line, {panel.x + 14.0f, panel.y + 86.0f}, 12.0f, 0.2f,
               Color{190, 207, 225, 255});
    std::snprintf(line, sizeof(line), "PROJECTILES %d", controller.GetActiveProjectileCount());
    DrawTextEx(m_font, line, {panel.x + 14.0f, panel.y + 109.0f}, 12.0f, 0.2f,
               Color{190, 207, 225, 255});
    std::snprintf(line, sizeof(line), "DROPPED TICKS %d", controller.GetDroppedTicks());
    DrawTextEx(m_font, line, {panel.x + 14.0f, panel.y + 132.0f}, 12.0f, 0.2f,
               controller.GetDroppedTicks() > 0 ? Color{255, 159, 92, 255}
                                                : Color{112, 228, 166, 255});
}

void SurvivalView::DrawCentered(const char* text, float y, float size, Color color) const {
    const Vector2 measured = Measure(m_font, text, size);
    DrawTextEx(m_font, text, {(GetScreenWidth() - measured.x) * 0.5f, y},
               size, 0.6f, color);
}

} // namespace Survival3D
