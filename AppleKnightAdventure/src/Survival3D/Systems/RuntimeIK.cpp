#include "Survival3D/Systems/RuntimeIK.h"

#include <algorithm>
#include <cmath>

namespace Survival3D::RuntimeIK {
namespace {

constexpr float kEpsilon = 1.0e-6f;

float Clamp01(float value) noexcept {
    return std::clamp(value, 0.0f, 1.0f);
}

bool IsFinite(float value) noexcept {
    return std::isfinite(value);
}

bool IsFinite(Vec3 value) noexcept {
    return IsFinite(value.x) && IsFinite(value.y) && IsFinite(value.z);
}

float DampAlpha(float sharpness, float deltaSeconds, bool snap) noexcept {
    if (snap || deltaSeconds <= 0.0f) return 1.0f;
    return Clamp01(1.0f - std::exp(-sharpness * deltaSeconds));
}

float LerpFloat(float from, float to, float alpha) noexcept {
    return from + (to - from) * alpha;
}

Vec3 SafeUp(Vec3 up) noexcept {
    return NormalizeOr(up, {0.0f, 1.0f, 0.0f});
}

Vec3 PerpendicularTo(Vec3 direction) noexcept {
    const Vec3 axis = std::abs(direction.y) < 0.90f
        ? Vec3{0.0f, 1.0f, 0.0f} : Vec3{1.0f, 0.0f, 0.0f};
    return NormalizeOr(Cross(axis, direction), {1.0f, 0.0f, 0.0f});
}

struct RawFootContact {
    Vec3 target{};
    Vec3 normal{0.0f, 1.0f, 0.0f};
    float verticalOffset = 0.0f;
    bool grounded = false;
};

RawFootContact ProbeFoot(Vec3 animatedFoot,
                        Vec3 up,
                        const FootIkSettings& settings,
                        GroundProbe probe,
                        void* user) noexcept {
    RawFootContact contact{};
    contact.target = animatedFoot;
    contact.normal = up;

    const GroundRay ray{
        animatedFoot + up * settings.probeHeight,
        -up,
        settings.probeHeight + settings.probeDepth
    };
    GroundHit hit{};
    if (!probe(ray, hit, user) || !IsFinite(hit.point) || !IsFinite(hit.normal))
        return contact;

    const float travel = Dot(hit.point - ray.origin, ray.direction);
    if (travel < -0.001f || travel > ray.maxDistance + 0.001f) return contact;

    const Vec3 normal = NormalizeOr(hit.normal, up);
    if (Dot(normal, up) < settings.minimumGroundDot) return contact;

    Vec3 target = hit.point + normal * settings.soleOffset;
    const float rawVertical = Dot(target - animatedFoot, up);
    const float clampedVertical = std::clamp(rawVertical,
                                             -settings.maxStepDown,
                                             settings.maxStepUp);
    target = target + up * (clampedVertical - rawVertical);
    contact.target = target;
    contact.normal = normal;
    contact.verticalOffset = clampedVertical;
    contact.grounded = true;
    return contact;
}

void ResetFootState(const FootIkInput& input, Vec3 up,
                    FootIkState& state) noexcept {
    state.leftTarget = input.leftAnimatedFoot;
    state.rightTarget = input.rightAnimatedFoot;
    state.leftNormal = up;
    state.rightNormal = up;
    state.leftWeight = 0.0f;
    state.rightWeight = 0.0f;
    state.pelvisOffset = 0.0f;
    state.initialized = true;
}

} // namespace

float Dot(Vec3 a, Vec3 b) noexcept {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3 Cross(Vec3 a, Vec3 b) noexcept {
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

float LengthSquared(Vec3 value) noexcept {
    return Dot(value, value);
}

float Length(Vec3 value) noexcept {
    return std::sqrt(LengthSquared(value));
}

Vec3 NormalizeOr(Vec3 value, Vec3 fallback) noexcept {
    const float lengthSquared = LengthSquared(value);
    if (!IsFinite(lengthSquared) || lengthSquared <= kEpsilon * kEpsilon) {
        const float fallbackSquared = LengthSquared(fallback);
        if (!IsFinite(fallbackSquared) || fallbackSquared <= kEpsilon * kEpsilon)
            return {0.0f, 0.0f, 1.0f};
        return fallback / std::sqrt(fallbackSquared);
    }
    return value / std::sqrt(lengthSquared);
}

Vec3 Lerp(Vec3 from, Vec3 to, float alpha) noexcept {
    alpha = Clamp01(alpha);
    return from + (to - from) * alpha;
}

bool FootIkSettings::IsValid() const noexcept {
    return IsFinite(up) && LengthSquared(up) > kEpsilon * kEpsilon
        && IsFinite(probeHeight) && probeHeight >= 0.0f
        && IsFinite(probeDepth) && probeDepth > 0.0f
        && IsFinite(soleOffset) && soleOffset >= 0.0f
        && IsFinite(maxStepUp) && maxStepUp >= 0.0f
        && IsFinite(maxStepDown) && maxStepDown >= 0.0f
        && IsFinite(pelvisMaxUp) && pelvisMaxUp >= 0.0f
        && IsFinite(pelvisMaxDown) && pelvisMaxDown >= 0.0f
        && IsFinite(minimumGroundDot) && minimumGroundDot >= 0.0f
        && minimumGroundDot <= 1.0f
        && IsFinite(footPositionSharpness) && footPositionSharpness > 0.0f
        && IsFinite(footNormalSharpness) && footNormalSharpness > 0.0f
        && IsFinite(pelvisSharpness) && pelvisSharpness > 0.0f;
}

FootIkOutput SolveFeetAndPelvis(const FootIkSettings& settings,
                                const FootIkInput& input,
                                GroundProbe probe,
                                void* user,
                                FootIkState& state) noexcept {
    FootIkOutput output{};
    output.pelvisPosition = input.pelvisPosition;
    output.left.target = input.leftAnimatedFoot;
    output.right.target = input.rightAnimatedFoot;

    const Vec3 up = SafeUp(settings.up);
    output.left.normal = up;
    output.right.normal = up;
    if (!settings.IsValid() || probe == nullptr
        || !IsFinite(input.pelvisPosition)
        || !IsFinite(input.leftAnimatedFoot)
        || !IsFinite(input.rightAnimatedFoot)) {
        ResetFootState(input, up, state);
        return output;
    }

    const RawFootContact left = ProbeFoot(input.leftAnimatedFoot, up,
                                          settings, probe, user);
    const RawFootContact right = ProbeFoot(input.rightAnimatedFoot, up,
                                           settings, probe, user);
    const bool wasInitialized = state.initialized;
    if (!state.initialized) ResetFootState(input, up, state);

    const bool snap = input.snap || !wasInitialized;
    const float positionAlpha = DampAlpha(settings.footPositionSharpness,
                                          input.deltaSeconds, snap);
    const float normalAlpha = DampAlpha(settings.footNormalSharpness,
                                        input.deltaSeconds, snap);
    const float pelvisAlpha = DampAlpha(settings.pelvisSharpness,
                                        input.deltaSeconds, snap);

    state.leftTarget = Lerp(state.leftTarget, left.target, positionAlpha);
    state.rightTarget = Lerp(state.rightTarget, right.target, positionAlpha);
    state.leftNormal = NormalizeOr(Lerp(state.leftNormal, left.normal, normalAlpha), up);
    state.rightNormal = NormalizeOr(Lerp(state.rightNormal, right.normal, normalAlpha), up);
    state.leftWeight = LerpFloat(state.leftWeight, left.grounded ? 1.0f : 0.0f,
                                 positionAlpha);
    state.rightWeight = LerpFloat(state.rightWeight, right.grounded ? 1.0f : 0.0f,
                                  positionAlpha);

    float desiredPelvis = 0.0f;
    if (left.grounded && right.grounded)
        desiredPelvis = std::min(left.verticalOffset, right.verticalOffset);
    else if (left.grounded)
        desiredPelvis = left.verticalOffset;
    else if (right.grounded)
        desiredPelvis = right.verticalOffset;
    desiredPelvis = std::clamp(desiredPelvis,
                               -settings.pelvisMaxDown,
                               settings.pelvisMaxUp);
    state.pelvisOffset = LerpFloat(state.pelvisOffset, desiredPelvis, pelvisAlpha);

    const float globalWeight = Clamp01(input.weight);
    output.pelvisOffset = state.pelvisOffset * globalWeight;
    output.pelvisPosition = input.pelvisPosition + up * output.pelvisOffset;

    output.left.weight = state.leftWeight * globalWeight;
    output.left.target = Lerp(input.leftAnimatedFoot, state.leftTarget,
                              output.left.weight);
    output.left.normal = NormalizeOr(Lerp(up, state.leftNormal, output.left.weight), up);
    output.left.verticalOffset = Dot(output.left.target - input.leftAnimatedFoot, up);
    output.left.grounded = left.grounded;

    output.right.weight = state.rightWeight * globalWeight;
    output.right.target = Lerp(input.rightAnimatedFoot, state.rightTarget,
                               output.right.weight);
    output.right.normal = NormalizeOr(Lerp(up, state.rightNormal, output.right.weight), up);
    output.right.verticalOffset = Dot(output.right.target - input.rightAnimatedFoot, up);
    output.right.grounded = right.grounded;
    return output;
}

bool AimSettings::IsValid() const noexcept {
    return IsFinite(maxYawRadians) && maxYawRadians >= 0.0f
        && maxYawRadians <= 3.14159265f
        && IsFinite(minPitchRadians) && IsFinite(maxPitchRadians)
        && minPitchRadians >= -1.57079632f
        && maxPitchRadians <= 1.57079632f
        && minPitchRadians <= maxPitchRadians
        && IsFinite(targetDistance) && targetDistance > 0.0f
        && IsFinite(directionSharpness) && directionSharpness > 0.0f;
}

AimOutput SolveAimTarget(const AimSettings& settings,
                         const AimInput& input,
                         AimState& state) noexcept {
    AimOutput output{};
    const Vec3 up = SafeUp(input.up);
    Vec3 forward = input.characterForward - up * Dot(input.characterForward, up);
    forward = NormalizeOr(forward, PerpendicularTo(up));
    const Vec3 right = NormalizeOr(Cross(up, forward), PerpendicularTo(up));

    if (!settings.IsValid() || !IsFinite(input.origin)
        || !IsFinite(input.desiredWorldPoint)) {
        state.direction = forward;
        state.initialized = true;
        output.direction = forward;
        output.target = input.origin + forward;
        return output;
    }

    const Vec3 desired = NormalizeOr(input.desiredWorldPoint - input.origin, forward);
    const float rawPitch = std::asin(std::clamp(Dot(desired, up), -1.0f, 1.0f));
    const Vec3 desiredPlanar = NormalizeOr(desired - up * Dot(desired, up), forward);
    const float rawYaw = std::atan2(Dot(desiredPlanar, right),
                                    Dot(desiredPlanar, forward));
    const float yaw = std::clamp(rawYaw, -settings.maxYawRadians,
                                 settings.maxYawRadians);
    const float pitch = std::clamp(rawPitch, settings.minPitchRadians,
                                   settings.maxPitchRadians);
    const float cosPitch = std::cos(pitch);
    const Vec3 clampedDirection = NormalizeOr(
        forward * (std::cos(yaw) * cosPitch)
        + right * (std::sin(yaw) * cosPitch)
        + up * std::sin(pitch), forward);

    if (!state.initialized || input.snap) {
        state.direction = clampedDirection;
        state.initialized = true;
    } else {
        const float alpha = DampAlpha(settings.directionSharpness,
                                      input.deltaSeconds, false);
        state.direction = NormalizeOr(Lerp(state.direction, clampedDirection, alpha),
                                      clampedDirection);
    }

    output.direction = state.direction;
    output.target = input.origin + output.direction * settings.targetDistance;
    const float finalPitch = std::asin(std::clamp(Dot(output.direction, up), -1.0f, 1.0f));
    const Vec3 finalPlanar = NormalizeOr(output.direction
                                        - up * Dot(output.direction, up), forward);
    output.yawRadians = std::atan2(Dot(finalPlanar, right),
                                   Dot(finalPlanar, forward));
    output.pitchRadians = finalPitch;
    output.clamped = std::abs(rawYaw - yaw) > 1.0e-4f
                  || std::abs(rawPitch - pitch) > 1.0e-4f;
    return output;
}

bool HandTargetSettings::IsValid() const noexcept {
    return IsFinite(mainHandForward) && mainHandForward >= 0.0f
        && IsFinite(supportHandForward) && supportHandForward >= 0.0f
        && IsFinite(mainHandSide) && mainHandSide >= 0.0f
        && IsFinite(supportHandSide) && supportHandSide >= 0.0f
        && IsFinite(mainHandHeight) && IsFinite(supportHandHeight)
        && IsFinite(elbowForward) && elbowForward >= 0.0f
        && IsFinite(elbowSide) && elbowSide >= 0.0f
        && IsFinite(elbowDrop) && elbowDrop >= 0.0f
        && IsFinite(weaponAimDistance) && weaponAimDistance > 0.0f;
}

HandTargets BuildHandTargets(const HandTargetSettings& settings,
                             const HandTargetInput& input) noexcept {
    HandTargets output{};
    const Vec3 up = SafeUp(input.up);
    const Vec3 aim = NormalizeOr(input.aimDirection, {0.0f, 0.0f, 1.0f});
    const Vec3 right = NormalizeOr(Cross(up, aim), PerpendicularTo(up));
    const float dominantSide = input.rightHanded ? 1.0f : -1.0f;
    if (!settings.IsValid() || !IsFinite(input.chestPosition)) {
        output.mainHand = input.chestPosition;
        output.supportHand = input.chestPosition;
        output.mainElbowPole = input.chestPosition;
        output.supportElbowPole = input.chestPosition;
        output.weaponAimTarget = input.chestPosition + aim;
        return output;
    }

    output.mainHand = input.chestPosition
        + aim * settings.mainHandForward
        + right * (settings.mainHandSide * dominantSide)
        + up * settings.mainHandHeight;
    output.supportHand = input.chestPosition
        + aim * settings.supportHandForward
        - right * (settings.supportHandSide * dominantSide)
        + up * settings.supportHandHeight;
    output.mainElbowPole = input.chestPosition
        + aim * settings.elbowForward
        + right * (settings.elbowSide * dominantSide)
        - up * settings.elbowDrop;
    output.supportElbowPole = input.chestPosition
        + aim * settings.elbowForward
        - right * (settings.elbowSide * dominantSide)
        - up * settings.elbowDrop;
    output.weaponAimTarget = input.chestPosition + aim * settings.weaponAimDistance;
    return output;
}

} // namespace Survival3D::RuntimeIK
