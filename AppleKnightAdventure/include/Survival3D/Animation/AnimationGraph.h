#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Survival3D::Animation {

// This module deliberately owns no raylib types.  The controller converts its
// Vector2/Vector3 values at the boundary, which keeps the animation graph
// deterministic and independently testable.
enum class ActorClass : std::uint8_t {
    Hero,
    Enemy,
    Boss
};

enum class State : std::uint8_t {
    Idle,
    Locomotion,
    Aim,
    BasicAttack,
    SkillOne,
    SkillTwo,
    Ultimate,
    Dash,
    Special,
    Hurt,
    PhaseTransition,
    Death
};

enum class TransitionReason : std::uint8_t {
    Locomotion,
    Gameplay,
    HitReaction,
    PhaseChange,
    Death,
    NaturalCompletion
};

enum class TransitionStatus : std::uint8_t {
    Accepted,
    NoFreshRequest,
    RejectedStale,
    RejectedRedundant,
    RejectedRole,
    RejectedLocked,
    RejectedTerminal
};

struct TransitionRequest {
    State target = State::Idle;
    TransitionReason reason = TransitionReason::Gameplay;
    // Requests are consumed monotonically. A caller should use its simulation
    // tick or action serial so a held key cannot restart an active attack.
    std::uint64_t serial = 0;
};

struct TransitionResult {
    TransitionStatus status = TransitionStatus::NoFreshRequest;
    State previous = State::Idle;
    State current = State::Idle;
    std::uint64_t acceptedSerial = 0;

    bool Accepted() const noexcept { return status == TransitionStatus::Accepted; }
};

class StateMachine {
public:
    explicit StateMachine(ActorClass actorClass = ActorClass::Hero) noexcept;

    void Reset(State initial = State::Idle) noexcept;
    State Current() const noexcept { return m_current; }
    ActorClass Class() const noexcept { return m_actorClass; }
    std::uint64_t StateGeneration() const noexcept { return m_stateGeneration; }
    std::uint64_t LastRequestSerial() const noexcept { return m_lastRequestSerial; }

    // Evaluates all fresh requests in a canonical order (reason, state
    // priority, serial, enum value), so the result does not depend on the order
    // in which input systems appended requests during the fixed simulation tick.
    TransitionResult SubmitBest(const std::vector<TransitionRequest>& requests,
                                float currentNormalizedProgress) noexcept;
    TransitionResult Submit(const TransitionRequest& request,
                            float currentNormalizedProgress) noexcept;

    // Called once a non-looping clip reaches its end. Death is terminal; every
    // other state resolves to Aim, Locomotion, or Idle deterministically.
    TransitionResult CompleteCurrent(bool locomotionRequested,
                                     bool aimRequested = false) noexcept;

    bool CanTransition(State target, TransitionReason reason,
                       float currentNormalizedProgress) const noexcept;

    static int StatePriority(State state) noexcept;
    static bool IsBaseState(State state) noexcept;
    static bool IsActionState(State state) noexcept;

private:
    TransitionStatus Evaluate(State target, TransitionReason reason,
                              float progress) const noexcept;
    void Accept(State target) noexcept;

    ActorClass m_actorClass = ActorClass::Hero;
    State m_current = State::Idle;
    std::uint64_t m_stateGeneration = 0;
    std::uint64_t m_lastRequestSerial = 0;
};

struct LocalVelocity2D {
    float right = 0.0f;
    float forward = 0.0f;
};

struct LocomotionWeights {
    float idle = 1.0f;
    float forward = 0.0f;
    float backward = 0.0f;
    float left = 0.0f;
    float right = 0.0f;
    float normalizedSpeed = 0.0f;

    float Sum() const noexcept {
        return idle + forward + backward + left + right;
    }
};

// Converts world X/Z velocity to the actor's local right/forward axes.
LocalVelocity2D WorldToLocalVelocity(float worldX, float worldZ,
                                     float facingX, float facingZ) noexcept;

// A five-point 2D blend tree. Directional weights use L1 angular blending;
// idle-to-motion uses speed magnitude. The five pose weights always sum to 1.
LocomotionWeights SolveLocomotionBlend(LocalVelocity2D velocity,
                                       float fullSpeed,
                                       float deadZone = 0.05f) noexcept;

enum class SemanticBone : std::uint8_t {
    Root,
    Hips,
    Spine,
    Chest,
    Head,
    UpperArmLeft,
    ForearmLeft,
    HandLeft,
    UpperArmRight,
    ForearmRight,
    HandRight,
    ThighLeft,
    ShinLeft,
    ThighRight,
    ShinRight,
    WeaponSocketRight,
    Count
};

using BoneMask = std::uint32_t;

constexpr BoneMask BoneBit(SemanticBone bone) noexcept {
    return BoneMask{1u} << static_cast<std::uint8_t>(bone);
}

bool MaskContains(BoneMask mask, SemanticBone bone) noexcept;
BoneMask AimUpperBodyMask() noexcept;
BoneMask CastUpperBodyMask() noexcept;

enum class LayerMode : std::uint8_t {
    None,
    Additive,
    Override
};

struct UpperBodyLayerRequest {
    State baseState = State::Idle;
    bool aimActive = false;
    bool castActive = false;
    float aimWeight = 1.0f;
    float castWeight = 1.0f;
};

struct UpperBodyLayer {
    LayerMode mode = LayerMode::None;
    BoneMask mask = 0;
    float weight = 0.0f;
    // Allows the runtime to choose its aim or cast clip without inferring that
    // choice again from the bone mask.
    bool usesCastPose = false;
};

// Cast has precedence over aim. Hurt, phase transition, death, and dash
// suppress upper-body overlays so their authored full-body silhouettes remain
// intact. Masks never include root, hips, or either leg.
UpperBodyLayer ResolveUpperBodyLayer(const UpperBodyLayerRequest& request) noexcept;

enum class RootMotionMode : std::uint8_t {
    None,
    Dash,
    Rush,
    Blink
};

struct RootMotionProfile {
    RootMotionMode mode = RootMotionMode::None;
    float forwardDistance = 0.0f;
    float rightDistance = 0.0f;
    float startNormalized = 0.0f;
    float endNormalized = 1.0f;
    float triggerNormalized = 0.5f; // Blink only.
    bool requiresCollisionSweep = true;
};

struct RootMotionDelta {
    float localRight = 0.0f;
    float localForward = 0.0f;
    float worldX = 0.0f;
    float worldZ = 0.0f;
    bool instantaneous = false;
    bool requiresCollisionSweep = false;
};

// Samples an incremental displacement. Dash/Rush use a smoothstep distance
// curve and Blink emits its full delta exactly once when the trigger is crossed.
// The graph never changes actor position: the controller must collision-sweep
// this delta before applying it.
RootMotionDelta SampleRootMotionDelta(const RootMotionProfile& profile,
                                      float previousNormalized,
                                      float currentNormalized,
                                      float facingX,
                                      float facingZ) noexcept;

enum class ClipEventType : std::uint8_t {
    FootstepLeft,
    FootstepRight,
    ActionContact,
    ProjectileRelease,
    PhaseCommit,
    RootMotionBegin,
    RootMotionEnd,
    HurtPeak,
    DeathSettled
};

struct EventDefinition {
    std::string id;
    ClipEventType type = ClipEventType::ActionContact;
    int frame = 0;
    float value = 0.0f;
};

struct ClipDefinition {
    std::string id;
    State state = State::Idle;
    int firstFrame = 0;
    int lastFrame = 0;
    float sampleRate = 60.0f;
    bool looping = false;
    RootMotionProfile rootMotion{};
    std::vector<EventDefinition> events;

    int FrameCount() const noexcept { return lastFrame - firstFrame + 1; }
    float DurationSeconds() const noexcept;
};

class ClipLibrary {
public:
    bool Add(ClipDefinition clip, std::string* error = nullptr);
    const ClipDefinition* Find(const std::string& id) const noexcept;
    const ClipDefinition* FindFirst(State state) const noexcept;
    const std::vector<ClipDefinition>& Clips() const noexcept { return m_clips; }
    std::vector<std::string> Validate() const;

private:
    std::vector<ClipDefinition> m_clips;
};

// Returns events in playback order for (previousFrame, currentFrame]. Set
// wrapped=true when a looping clip crossed lastFrame -> firstFrame.
std::vector<EventDefinition> CollectCrossedEvents(const ClipDefinition& clip,
                                                  float previousFrame,
                                                  float currentFrame,
                                                  bool wrapped = false);

enum class HeroStyle : std::uint8_t {
    Knight,
    MagicCaster
};

// Built-in C++ production definitions mirror the current 655-frame Step 6 GLB
// contract. They are ordinary data: future JSON loading can populate the same
// ClipLibrary API without changing the graph or requiring Python at runtime.
ClipLibrary BuildStep6HeroClipLibrary(HeroStyle style);
ClipLibrary BuildStep6NonHeroClipLibrary(ActorClass actorClass);

} // namespace Survival3D::Animation
