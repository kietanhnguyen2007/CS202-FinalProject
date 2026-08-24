#include "Survival3D/Animation/AnimationGraph.h"

#include <algorithm>
#include <cmath>
#include <set>
#include <tuple>
#include <utility>

namespace Survival3D::Animation {
namespace {

constexpr float kEpsilon = 0.0001f;

float Clamp01(float value) noexcept {
    return std::clamp(value, 0.0f, 1.0f);
}

int ReasonPriority(TransitionReason reason) noexcept {
    switch (reason) {
        case TransitionReason::Death: return 600;
        case TransitionReason::PhaseChange: return 500;
        case TransitionReason::HitReaction: return 400;
        case TransitionReason::Gameplay: return 300;
        case TransitionReason::NaturalCompletion: return 200;
        case TransitionReason::Locomotion: return 100;
    }
    return 0;
}

bool IsSuppressedUpperBodyState(State state) noexcept {
    return state == State::Dash || state == State::Hurt
        || state == State::PhaseTransition || state == State::Death;
}

bool CanHeroCancel(State current, State target, float progress) noexcept {
    if (!StateMachine::IsActionState(target)) return false;

    switch (current) {
        case State::BasicAttack:
            if (target == State::Dash) return progress >= 0.45f;
            if (target == State::SkillOne || target == State::SkillTwo)
                return progress >= 0.68f;
            if (target == State::Ultimate) return progress >= 0.74f;
            return false;
        case State::SkillOne:
            if (target == State::Dash) return progress >= 0.72f;
            if (target == State::SkillTwo || target == State::Ultimate)
                return progress >= 0.82f;
            return false;
        case State::SkillTwo:
            if (target == State::Dash) return progress >= 0.82f;
            if (target == State::Ultimate) return progress >= 0.88f;
            return false;
        case State::Dash:
            return progress >= 0.88f && target != State::Dash;
        case State::Special:
            return progress >= 0.85f
                && (target == State::Dash || target == State::Ultimate);
        case State::Ultimate:
            return false;
        default:
            return false;
    }
}

float SmoothStep(float value) noexcept {
    value = Clamp01(value);
    return value * value * (3.0f - 2.0f * value);
}

BoneMask MakeMask(std::initializer_list<SemanticBone> bones) noexcept {
    BoneMask result = 0;
    for (const SemanticBone bone : bones) result |= BoneBit(bone);
    return result;
}

bool RootProfileValid(const RootMotionProfile& profile) noexcept {
    if (!std::isfinite(profile.forwardDistance)
        || !std::isfinite(profile.rightDistance)) return false;
    if (profile.mode == RootMotionMode::None) return true;
    if (profile.mode == RootMotionMode::Blink) {
        return std::isfinite(profile.triggerNormalized)
            && profile.triggerNormalized >= 0.0f
            && profile.triggerNormalized <= 1.0f;
    }
    return std::isfinite(profile.startNormalized)
        && std::isfinite(profile.endNormalized)
        && profile.startNormalized >= 0.0f
        && profile.endNormalized <= 1.0f
        && profile.endNormalized > profile.startNormalized;
}

std::vector<std::string> ValidateClip(const ClipDefinition& clip) {
    std::vector<std::string> errors;
    if (clip.id.empty()) errors.emplace_back("clip id is empty");
    if (clip.firstFrame < 0 || clip.lastFrame < clip.firstFrame)
        errors.emplace_back(clip.id + ": invalid frame range");
    if (!std::isfinite(clip.sampleRate) || clip.sampleRate <= 0.0f)
        errors.emplace_back(clip.id + ": sampleRate must be positive");
    if (!RootProfileValid(clip.rootMotion))
        errors.emplace_back(clip.id + ": invalid root-motion profile");

    std::set<std::string> eventIds;
    int previousFrame = clip.firstFrame;
    for (std::size_t index = 0; index < clip.events.size(); ++index) {
        const EventDefinition& event = clip.events[index];
        if (event.id.empty()) errors.emplace_back(clip.id + ": empty event id");
        if (!eventIds.insert(event.id).second)
            errors.emplace_back(clip.id + ": duplicate event id " + event.id);
        if (event.frame < clip.firstFrame || event.frame > clip.lastFrame)
            errors.emplace_back(clip.id + ": event outside clip: " + event.id);
        if (index > 0 && event.frame < previousFrame)
            errors.emplace_back(clip.id + ": events are not frame-sorted");
        previousFrame = event.frame;
    }
    return errors;
}

ClipDefinition MakeClip(std::string id, State state, int first, int last,
                        bool looping, std::vector<EventDefinition> events = {},
                        RootMotionProfile rootMotion = {}) {
    ClipDefinition clip;
    clip.id = std::move(id);
    clip.state = state;
    clip.firstFrame = first;
    clip.lastFrame = last;
    clip.sampleRate = 60.0f;
    clip.looping = looping;
    clip.rootMotion = rootMotion;
    clip.events = std::move(events);
    return clip;
}

void AddKnownValid(ClipLibrary& library, ClipDefinition clip) {
    // Definitions in this translation unit are compile-time authored data. The
    // public Add API remains fallible for designer/runtime-loaded definitions.
    std::string ignored;
    library.Add(std::move(clip), &ignored);
}

} // namespace

StateMachine::StateMachine(ActorClass actorClass) noexcept
    : m_actorClass(actorClass) {}

void StateMachine::Reset(State initial) noexcept {
    m_current = initial;
    m_stateGeneration = 0;
    m_lastRequestSerial = 0;
}

int StateMachine::StatePriority(State state) noexcept {
    switch (state) {
        case State::Death: return 1000;
        case State::PhaseTransition: return 900;
        case State::Hurt: return 800;
        case State::Ultimate: return 700;
        case State::SkillTwo: return 620;
        case State::SkillOne: return 600;
        case State::Special: return 580;
        case State::Dash: return 560;
        case State::BasicAttack: return 500;
        case State::Aim: return 100;
        case State::Locomotion: return 50;
        case State::Idle: return 0;
    }
    return 0;
}

bool StateMachine::IsBaseState(State state) noexcept {
    return state == State::Idle || state == State::Locomotion
        || state == State::Aim;
}

bool StateMachine::IsActionState(State state) noexcept {
    return state == State::BasicAttack || state == State::SkillOne
        || state == State::SkillTwo || state == State::Ultimate
        || state == State::Dash || state == State::Special;
}

TransitionStatus StateMachine::Evaluate(State target, TransitionReason reason,
                                        float progress) const noexcept {
    progress = Clamp01(progress);

    if (target == m_current) return TransitionStatus::RejectedRedundant;
    if (m_current == State::Death) return TransitionStatus::RejectedTerminal;

    if (target == State::Death) {
        return reason == TransitionReason::Death
            ? TransitionStatus::Accepted : TransitionStatus::RejectedRole;
    }
    if (target == State::PhaseTransition) {
        if (m_actorClass != ActorClass::Boss
            || reason != TransitionReason::PhaseChange)
            return TransitionStatus::RejectedRole;
        return TransitionStatus::Accepted;
    }
    if (target == State::Hurt) {
        if (reason != TransitionReason::HitReaction)
            return TransitionStatus::RejectedRole;
        return m_current == State::PhaseTransition
            ? TransitionStatus::RejectedLocked : TransitionStatus::Accepted;
    }

    if (reason == TransitionReason::Death
        || reason == TransitionReason::PhaseChange
        || reason == TransitionReason::HitReaction)
        return TransitionStatus::RejectedRole;

    if (m_current == State::PhaseTransition)
        return TransitionStatus::RejectedLocked;

    if (m_current == State::Hurt) {
        if (reason == TransitionReason::NaturalCompletion
            && IsBaseState(target) && progress >= 0.90f)
            return TransitionStatus::Accepted;
        return TransitionStatus::RejectedLocked;
    }

    if (IsBaseState(m_current)) return TransitionStatus::Accepted;

    if (IsActionState(m_current)) {
        if (reason == TransitionReason::NaturalCompletion
            && IsBaseState(target) && progress >= 0.98f)
            return TransitionStatus::Accepted;

        if (IsActionState(target)) {
            if (m_actorClass == ActorClass::Hero)
                return CanHeroCancel(m_current, target, progress)
                    ? TransitionStatus::Accepted : TransitionStatus::RejectedLocked;
            // Enemy and boss attacks are committed. They may select their next
            // action only from the final two percent of recovery.
            return progress >= 0.98f
                ? TransitionStatus::Accepted : TransitionStatus::RejectedLocked;
        }
        return TransitionStatus::RejectedLocked;
    }

    return TransitionStatus::RejectedLocked;
}

bool StateMachine::CanTransition(State target, TransitionReason reason,
                                 float currentNormalizedProgress) const noexcept {
    return Evaluate(target, reason, currentNormalizedProgress)
        == TransitionStatus::Accepted;
}

void StateMachine::Accept(State target) noexcept {
    m_current = target;
    ++m_stateGeneration;
}

TransitionResult StateMachine::SubmitBest(
    const std::vector<TransitionRequest>& requests,
    float currentNormalizedProgress) noexcept {
    TransitionResult result;
    result.previous = m_current;
    result.current = m_current;

    std::vector<const TransitionRequest*> fresh;
    std::uint64_t greatestSeen = m_lastRequestSerial;
    for (const TransitionRequest& request : requests) {
        greatestSeen = std::max(greatestSeen, request.serial);
        if (request.serial > m_lastRequestSerial) fresh.push_back(&request);
    }
    m_lastRequestSerial = greatestSeen;

    if (fresh.empty()) {
        result.status = requests.empty()
            ? TransitionStatus::NoFreshRequest : TransitionStatus::RejectedStale;
        return result;
    }

    std::sort(fresh.begin(), fresh.end(), [](const TransitionRequest* left,
                                             const TransitionRequest* right) {
        const auto leftRank = std::make_tuple(
            ReasonPriority(left->reason), StateMachine::StatePriority(left->target),
            left->serial, -static_cast<int>(left->target));
        const auto rightRank = std::make_tuple(
            ReasonPriority(right->reason), StateMachine::StatePriority(right->target),
            right->serial, -static_cast<int>(right->target));
        return leftRank > rightRank;
    });

    TransitionStatus strongestRejection = TransitionStatus::RejectedLocked;
    bool haveRejection = false;
    for (const TransitionRequest* request : fresh) {
        const TransitionStatus status = Evaluate(
            request->target, request->reason, currentNormalizedProgress);
        if (!haveRejection) {
            strongestRejection = status;
            haveRejection = true;
        }
        if (status != TransitionStatus::Accepted) continue;

        Accept(request->target);
        result.status = TransitionStatus::Accepted;
        result.current = m_current;
        result.acceptedSerial = request->serial;
        return result;
    }

    result.status = strongestRejection;
    return result;
}

TransitionResult StateMachine::Submit(const TransitionRequest& request,
                                      float currentNormalizedProgress) noexcept {
    TransitionResult result;
    result.previous = m_current;
    result.current = m_current;
    if (request.serial <= m_lastRequestSerial) {
        result.status = TransitionStatus::RejectedStale;
        return result;
    }

    m_lastRequestSerial = request.serial;
    result.status = Evaluate(request.target, request.reason,
                             currentNormalizedProgress);
    if (result.status != TransitionStatus::Accepted) return result;

    Accept(request.target);
    result.current = m_current;
    result.acceptedSerial = request.serial;
    return result;
}

TransitionResult StateMachine::CompleteCurrent(bool locomotionRequested,
                                               bool aimRequested) noexcept {
    TransitionResult result;
    result.previous = m_current;
    result.current = m_current;
    if (m_current == State::Death) {
        result.status = TransitionStatus::RejectedTerminal;
        return result;
    }

    const State target = aimRequested ? State::Aim
        : (locomotionRequested ? State::Locomotion : State::Idle);
    if (target == m_current) {
        result.status = TransitionStatus::RejectedRedundant;
        return result;
    }
    Accept(target);
    result.status = TransitionStatus::Accepted;
    result.current = target;
    return result;
}

LocalVelocity2D WorldToLocalVelocity(float worldX, float worldZ,
                                     float facingX, float facingZ) noexcept {
    const float facingLength = std::hypot(facingX, facingZ);
    if (facingLength <= kEpsilon) return {};
    facingX /= facingLength;
    facingZ /= facingLength;
    // Local right is a clockwise 90-degree rotation of forward in X/Z.
    const float rightX = facingZ;
    const float rightZ = -facingX;
    return {
        worldX * rightX + worldZ * rightZ,
        worldX * facingX + worldZ * facingZ
    };
}

LocomotionWeights SolveLocomotionBlend(LocalVelocity2D velocity,
                                       float fullSpeed,
                                       float deadZone) noexcept {
    LocomotionWeights result;
    const float speed = std::hypot(velocity.right, velocity.forward);
    fullSpeed = std::max(fullSpeed, kEpsilon);
    deadZone = std::clamp(deadZone, 0.0f, fullSpeed - kEpsilon);
    if (speed <= deadZone) return result;

    result.normalizedSpeed = Clamp01((speed - deadZone) / (fullSpeed - deadZone));
    result.idle = 1.0f - result.normalizedSpeed;

    const float directionalDenominator = std::abs(velocity.right)
                                       + std::abs(velocity.forward);
    if (directionalDenominator <= kEpsilon) return result;
    const float scale = result.normalizedSpeed / directionalDenominator;
    result.forward = std::max(0.0f, velocity.forward) * scale;
    result.backward = std::max(0.0f, -velocity.forward) * scale;
    result.left = std::max(0.0f, -velocity.right) * scale;
    result.right = std::max(0.0f, velocity.right) * scale;

    // Repair the final floating-point ulp on idle so consumers may treat the
    // weights as a normalized convex combination without renormalizing.
    result.idle += 1.0f - result.Sum();
    return result;
}

bool MaskContains(BoneMask mask, SemanticBone bone) noexcept {
    return (mask & BoneBit(bone)) != 0;
}

BoneMask AimUpperBodyMask() noexcept {
    return MakeMask({
        SemanticBone::Spine, SemanticBone::Chest, SemanticBone::Head,
        SemanticBone::UpperArmLeft, SemanticBone::ForearmLeft,
        SemanticBone::HandLeft, SemanticBone::UpperArmRight,
        SemanticBone::ForearmRight, SemanticBone::HandRight,
        SemanticBone::WeaponSocketRight
    });
}

BoneMask CastUpperBodyMask() noexcept {
    return MakeMask({
        SemanticBone::Spine, SemanticBone::Chest, SemanticBone::Head,
        SemanticBone::UpperArmLeft, SemanticBone::ForearmLeft,
        SemanticBone::HandLeft, SemanticBone::UpperArmRight,
        SemanticBone::ForearmRight, SemanticBone::HandRight,
        SemanticBone::WeaponSocketRight
    });
}

UpperBodyLayer ResolveUpperBodyLayer(
    const UpperBodyLayerRequest& request) noexcept {
    UpperBodyLayer result;
    if (IsSuppressedUpperBodyState(request.baseState)) return result;

    if (request.castActive) {
        result.mode = LayerMode::Override;
        result.mask = CastUpperBodyMask();
        result.weight = Clamp01(request.castWeight);
        result.usesCastPose = true;
    } else if (request.aimActive) {
        result.mode = LayerMode::Additive;
        result.mask = AimUpperBodyMask();
        result.weight = Clamp01(request.aimWeight);
    }
    if (result.weight <= kEpsilon) {
        result.mode = LayerMode::None;
        result.mask = 0;
        result.usesCastPose = false;
    }
    return result;
}

RootMotionDelta SampleRootMotionDelta(const RootMotionProfile& profile,
                                      float previousNormalized,
                                      float currentNormalized,
                                      float facingX,
                                      float facingZ) noexcept {
    RootMotionDelta result;
    if (profile.mode == RootMotionMode::None
        || currentNormalized <= previousNormalized + kEpsilon
        || !RootProfileValid(profile)) return result;

    float forwardDelta = 0.0f;
    float rightDelta = 0.0f;
    if (profile.mode == RootMotionMode::Blink) {
        const bool crossed = previousNormalized < profile.triggerNormalized
            && currentNormalized >= profile.triggerNormalized;
        if (!crossed) return result;
        forwardDelta = profile.forwardDistance;
        rightDelta = profile.rightDistance;
        result.instantaneous = true;
    } else {
        const float width = profile.endNormalized - profile.startNormalized;
        const float previousCurve = SmoothStep(
            (previousNormalized - profile.startNormalized) / width);
        const float currentCurve = SmoothStep(
            (currentNormalized - profile.startNormalized) / width);
        const float curveDelta = std::max(0.0f, currentCurve - previousCurve);
        forwardDelta = profile.forwardDistance * curveDelta;
        rightDelta = profile.rightDistance * curveDelta;
    }

    if (std::abs(forwardDelta) <= kEpsilon
        && std::abs(rightDelta) <= kEpsilon) return result;

    float facingLength = std::hypot(facingX, facingZ);
    if (facingLength <= kEpsilon) {
        facingX = 0.0f;
        facingZ = 1.0f;
    } else {
        facingX /= facingLength;
        facingZ /= facingLength;
    }
    const float rightX = facingZ;
    const float rightZ = -facingX;
    result.localForward = forwardDelta;
    result.localRight = rightDelta;
    result.worldX = facingX * forwardDelta + rightX * rightDelta;
    result.worldZ = facingZ * forwardDelta + rightZ * rightDelta;
    result.requiresCollisionSweep = profile.requiresCollisionSweep;
    return result;
}

float ClipDefinition::DurationSeconds() const noexcept {
    return sampleRate > 0.0f
        ? static_cast<float>(std::max(0, lastFrame - firstFrame)) / sampleRate
        : 0.0f;
}

bool ClipLibrary::Add(ClipDefinition clip, std::string* error) {
    if (Find(clip.id) != nullptr) {
        if (error) *error = "duplicate clip id " + clip.id;
        return false;
    }

    std::stable_sort(clip.events.begin(), clip.events.end(),
        [](const EventDefinition& left, const EventDefinition& right) {
            return std::tie(left.frame, left.type, left.id)
                 < std::tie(right.frame, right.type, right.id);
        });
    const std::vector<std::string> errors = ValidateClip(clip);
    if (!errors.empty()) {
        if (error) *error = errors.front();
        return false;
    }
    m_clips.push_back(std::move(clip));
    return true;
}

const ClipDefinition* ClipLibrary::Find(const std::string& id) const noexcept {
    const auto found = std::find_if(m_clips.begin(), m_clips.end(),
        [&id](const ClipDefinition& clip) { return clip.id == id; });
    return found == m_clips.end() ? nullptr : &*found;
}

const ClipDefinition* ClipLibrary::FindFirst(State state) const noexcept {
    const auto found = std::find_if(m_clips.begin(), m_clips.end(),
        [state](const ClipDefinition& clip) { return clip.state == state; });
    return found == m_clips.end() ? nullptr : &*found;
}

std::vector<std::string> ClipLibrary::Validate() const {
    std::vector<std::string> errors;
    std::set<std::string> ids;
    for (const ClipDefinition& clip : m_clips) {
        if (!ids.insert(clip.id).second)
            errors.emplace_back("duplicate clip id " + clip.id);
        const std::vector<std::string> clipErrors = ValidateClip(clip);
        errors.insert(errors.end(), clipErrors.begin(), clipErrors.end());
    }
    return errors;
}

std::vector<EventDefinition> CollectCrossedEvents(const ClipDefinition& clip,
                                                  float previousFrame,
                                                  float currentFrame,
                                                  bool wrapped) {
    std::vector<EventDefinition> result;
    if (!wrapped && currentFrame + kEpsilon < previousFrame) return result;

    const auto appendRange = [&result, &clip](float after, float through,
                                              bool includeAfter) {
        for (const EventDefinition& event : clip.events) {
            const bool pastStart = includeAfter
                ? static_cast<float>(event.frame) + kEpsilon >= after
                : static_cast<float>(event.frame) > after + kEpsilon;
            if (pastStart
                && static_cast<float>(event.frame) <= through + kEpsilon)
                result.push_back(event);
        }
    };

    if (wrapped) {
        appendRange(previousFrame, static_cast<float>(clip.lastFrame), false);
        appendRange(static_cast<float>(clip.firstFrame), currentFrame, true);
    } else {
        appendRange(previousFrame, currentFrame, false);
    }
    return result;
}

ClipLibrary BuildStep6HeroClipLibrary(HeroStyle style) {
    ClipLibrary library;
    const std::string prefix = style == HeroStyle::Knight ? "knight." : "mage.";
    AddKnownValid(library, MakeClip(prefix + "idle", State::Idle, 0, 94, true));
    AddKnownValid(library, MakeClip(prefix + "run", State::Locomotion, 96, 158, true, {
        {"footstep_left_a", ClipEventType::FootstepLeft, 104, 0.0f},
        {"footstep_right_a", ClipEventType::FootstepRight, 120, 0.0f},
        {"footstep_left_b", ClipEventType::FootstepLeft, 135, 0.0f},
        {"footstep_right_b", ClipEventType::FootstepRight, 151, 0.0f}
    }));

    AddKnownValid(library, MakeClip(prefix + "basic", State::BasicAttack,
        160, 222, false, {{
            style == HeroStyle::Knight ? "sword_contact" : "bolt_release",
            style == HeroStyle::Knight ? ClipEventType::ActionContact
                                       : ClipEventType::ProjectileRelease,
            188, 1.0f
        }}));
    AddKnownValid(library, MakeClip(prefix + "skill_one", State::SkillOne,
        224, 302, false, {{"skill_one_contact", ClipEventType::ActionContact, 274, 1.0f}}));

    RootMotionProfile skillTwoMotion;
    std::vector<EventDefinition> skillTwoEvents{
        {"skill_two_contact", ClipEventType::ActionContact, 344, 1.0f}
    };
    if (style == HeroStyle::Knight) {
        skillTwoMotion = {RootMotionMode::Rush, 7.0f, 0.0f,
                          20.0f / 78.0f, 60.0f / 78.0f, 0.5f, true};
        skillTwoEvents.push_back(
            {"rush_begin", ClipEventType::RootMotionBegin, 324, 0.0f});
        skillTwoEvents.push_back(
            {"rush_end", ClipEventType::RootMotionEnd, 364, 0.0f});
    }
    AddKnownValid(library, MakeClip(prefix + "skill_two", State::SkillTwo,
        304, 382, false, std::move(skillTwoEvents), skillTwoMotion));

    AddKnownValid(library, MakeClip(prefix + "ultimate", State::Ultimate,
        384, 478, false,
        {{"ultimate_contact", ClipEventType::ActionContact, 448, 1.0f}}));

    RootMotionProfile dashMotion;
    std::vector<EventDefinition> dashEvents;
    if (style == HeroStyle::Knight) {
        dashMotion = {RootMotionMode::Dash, 4.0f, 0.0f,
                      12.0f / 46.0f, 38.0f / 46.0f, 0.5f, true};
        dashEvents = {
            {"dash_begin", ClipEventType::RootMotionBegin, 492, 0.0f},
            {"dash_contact", ClipEventType::ActionContact, 505, 1.0f},
            {"dash_end", ClipEventType::RootMotionEnd, 518, 0.0f}
        };
    } else {
        dashMotion = {RootMotionMode::Blink, 4.0f, 0.0f,
                      0.0f, 1.0f, 25.0f / 46.0f, true};
        dashEvents = {
            {"blink_begin", ClipEventType::RootMotionBegin, 492, 0.0f},
            {"blink_commit", ClipEventType::ActionContact, 505, 1.0f},
            {"blink_end", ClipEventType::RootMotionEnd, 518, 0.0f}
        };
    }
    AddKnownValid(library, MakeClip(prefix + "dash", State::Dash,
        480, 526, false, std::move(dashEvents), dashMotion));
    AddKnownValid(library, MakeClip(prefix + "hurt", State::Hurt,
        528, 558, false, {{"hurt_peak", ClipEventType::HurtPeak, 540, 1.0f}}));
    AddKnownValid(library, MakeClip(prefix + "death", State::Death,
        560, 654, false,
        {{"death_settled", ClipEventType::DeathSettled, 638, 1.0f}}));
    return library;
}

ClipLibrary BuildStep6NonHeroClipLibrary(ActorClass actorClass) {
    ClipLibrary library;
    const bool boss = actorClass == ActorClass::Boss;
    const std::string prefix = boss ? "boss." : "enemy.";
    AddKnownValid(library, MakeClip(prefix + "idle", State::Idle, 0, 94, true));
    AddKnownValid(library, MakeClip(prefix + "run", State::Locomotion, 96, 158, true, {
        {"footstep_left", ClipEventType::FootstepLeft, 111, 0.0f},
        {"footstep_right", ClipEventType::FootstepRight, 143, 0.0f}
    }));
    AddKnownValid(library, MakeClip(prefix + "basic", State::BasicAttack,
        160, 222, false,
        {{"basic_contact", ClipEventType::ActionContact, 190, 1.0f}}));
    AddKnownValid(library, MakeClip(prefix + "skill_one", State::SkillOne,
        224, 302, false,
        {{"skill_one_contact", ClipEventType::ActionContact, 274, 1.0f}}));
    AddKnownValid(library, MakeClip(prefix + "skill_two", State::SkillTwo,
        304, 382, false,
        {{"skill_two_contact", ClipEventType::ActionContact, 348, 1.0f}}));
    AddKnownValid(library, MakeClip(prefix + "ultimate", State::Ultimate,
        384, 478, false, {{
            boss ? "phase_commit" : "ultimate_contact",
            boss ? ClipEventType::PhaseCommit : ClipEventType::ActionContact,
            448, 1.0f
        }}));
    AddKnownValid(library, MakeClip(prefix + "special", State::Special,
        480, 526, false,
        {{"special_contact", ClipEventType::ActionContact, 505, 1.0f}}));
    AddKnownValid(library, MakeClip(prefix + "hurt", State::Hurt,
        528, 558, false, {{"hurt_peak", ClipEventType::HurtPeak, 540, 1.0f}}));
    AddKnownValid(library, MakeClip(prefix + "death", State::Death,
        560, 654, false,
        {{"death_settled", ClipEventType::DeathSettled, 638, 1.0f}}));
    return library;
}

} // namespace Survival3D::Animation
