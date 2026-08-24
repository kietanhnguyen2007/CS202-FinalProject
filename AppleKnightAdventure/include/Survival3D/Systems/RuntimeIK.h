#pragma once

// Deliberately independent from raylib. The view/animation adapter converts
// between this Vec3 and its engine vector type at the integration boundary.

namespace Survival3D::RuntimeIK {

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

constexpr Vec3 operator+(Vec3 a, Vec3 b) noexcept {
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}
constexpr Vec3 operator-(Vec3 a, Vec3 b) noexcept {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}
constexpr Vec3 operator-(Vec3 value) noexcept {
    return {-value.x, -value.y, -value.z};
}
constexpr Vec3 operator*(Vec3 value, float scalar) noexcept {
    return {value.x * scalar, value.y * scalar, value.z * scalar};
}
constexpr Vec3 operator*(float scalar, Vec3 value) noexcept {
    return value * scalar;
}
constexpr Vec3 operator/(Vec3 value, float scalar) noexcept {
    return {value.x / scalar, value.y / scalar, value.z / scalar};
}

float Dot(Vec3 a, Vec3 b) noexcept;
Vec3 Cross(Vec3 a, Vec3 b) noexcept;
float LengthSquared(Vec3 value) noexcept;
float Length(Vec3 value) noexcept;
Vec3 NormalizeOr(Vec3 value, Vec3 fallback) noexcept;
Vec3 Lerp(Vec3 from, Vec3 to, float alpha) noexcept;

struct GroundRay {
    Vec3 origin{};
    Vec3 direction{0.0f, -1.0f, 0.0f};
    float maxDistance = 0.0f;
};

struct GroundHit {
    Vec3 point{};
    Vec3 normal{0.0f, 1.0f, 0.0f};
    float distance = 0.0f;
};

using GroundProbe = bool (*)(const GroundRay& ray,
                             GroundHit& hit,
                             void* user) noexcept;

struct FootIkSettings {
    Vec3 up{0.0f, 1.0f, 0.0f};
    float probeHeight = 0.65f;
    float probeDepth = 1.25f;
    float soleOffset = 0.045f;
    float maxStepUp = 0.45f;
    float maxStepDown = 0.70f;
    float pelvisMaxUp = 0.20f;
    float pelvisMaxDown = 0.55f;
    float minimumGroundDot = 0.45f;
    float footPositionSharpness = 24.0f;
    float footNormalSharpness = 18.0f;
    float pelvisSharpness = 16.0f;

    bool IsValid() const noexcept;
};

struct FootIkInput {
    Vec3 pelvisPosition{};
    Vec3 leftAnimatedFoot{};
    Vec3 rightAnimatedFoot{};
    float deltaSeconds = 0.0f;
    float weight = 1.0f;
    bool snap = false;
};

struct FootContact {
    Vec3 target{};
    Vec3 normal{0.0f, 1.0f, 0.0f};
    float verticalOffset = 0.0f;
    float weight = 0.0f;
    bool grounded = false;
};

struct FootIkOutput {
    Vec3 pelvisPosition{};
    float pelvisOffset = 0.0f;
    FootContact left{};
    FootContact right{};
};

struct FootIkState {
    Vec3 leftTarget{};
    Vec3 rightTarget{};
    Vec3 leftNormal{0.0f, 1.0f, 0.0f};
    Vec3 rightNormal{0.0f, 1.0f, 0.0f};
    float leftWeight = 0.0f;
    float rightWeight = 0.0f;
    float pelvisOffset = 0.0f;
    bool initialized = false;
};

// Performs exactly two probe callbacks and no allocation. Invalid settings or
// a null probe return the authored pose with zero IK weight.
FootIkOutput SolveFeetAndPelvis(const FootIkSettings& settings,
                                const FootIkInput& input,
                                GroundProbe probe,
                                void* user,
                                FootIkState& state) noexcept;

struct AimSettings {
    float maxYawRadians = 1.40f;
    float minPitchRadians = -0.75f;
    float maxPitchRadians = 1.05f;
    float targetDistance = 12.0f;
    float directionSharpness = 22.0f;

    bool IsValid() const noexcept;
};

struct AimInput {
    Vec3 origin{};
    Vec3 characterForward{0.0f, 0.0f, 1.0f};
    Vec3 desiredWorldPoint{0.0f, 0.0f, 1.0f};
    Vec3 up{0.0f, 1.0f, 0.0f};
    float deltaSeconds = 0.0f;
    bool snap = false;
};

struct AimState {
    Vec3 direction{0.0f, 0.0f, 1.0f};
    bool initialized = false;
};

struct AimOutput {
    Vec3 direction{0.0f, 0.0f, 1.0f};
    Vec3 target{};
    float yawRadians = 0.0f;
    float pitchRadians = 0.0f;
    bool clamped = false;
};

// Clamps desired aim in character-local yaw/pitch space and optionally damps
// it. The target remains at targetDistance, making it suitable for head/chest
// look-at, staff pointing, or an upper-body aim layer.
AimOutput SolveAimTarget(const AimSettings& settings,
                         const AimInput& input,
                         AimState& state) noexcept;

struct HandTargetSettings {
    float mainHandForward = 0.52f;
    float supportHandForward = 0.76f;
    float mainHandSide = 0.24f;
    float supportHandSide = 0.20f;
    float mainHandHeight = -0.08f;
    float supportHandHeight = -0.02f;
    float elbowForward = 0.24f;
    float elbowSide = 0.48f;
    float elbowDrop = 0.18f;
    float weaponAimDistance = 1.8f;

    bool IsValid() const noexcept;
};

struct HandTargetInput {
    Vec3 chestPosition{};
    Vec3 aimDirection{0.0f, 0.0f, 1.0f};
    Vec3 up{0.0f, 1.0f, 0.0f};
    bool rightHanded = true;
};

struct HandTargets {
    Vec3 mainHand{};
    Vec3 supportHand{};
    Vec3 mainElbowPole{};
    Vec3 supportElbowPole{};
    Vec3 weaponAimTarget{};
};

// Produces weapon/staff hand goals plus stable outward elbow-pole targets.
// Mirroring handedness swaps the side of every goal without changing aim.
HandTargets BuildHandTargets(const HandTargetSettings& settings,
                             const HandTargetInput& input) noexcept;

} // namespace Survival3D::RuntimeIK
