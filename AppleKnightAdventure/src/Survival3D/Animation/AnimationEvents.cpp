#include "Survival3D/Animation/AnimationEvents.h"

#include <limits>

namespace Survival3D::Animation {
namespace {

constexpr std::uint16_t PhaseValue(PhaseId phase) noexcept {
    return static_cast<std::uint16_t>(phase);
}

constexpr FrameEvent kKnightRunEvents[] = {
    {0, EventType::Phase, EventId::None, PhaseValue(PhaseId::Locomotion)},
    {14, EventType::Footstep, EventId::LeftFoot, 0},
    {45, EventType::Footstep, EventId::RightFoot, 0}
};

constexpr FrameEvent kKnightBasicEvents[] = {
    {0, EventType::Phase, EventId::None, PhaseValue(PhaseId::Windup)},
    {24, EventType::Phase, EventId::None, PhaseValue(PhaseId::Active)},
    {25, EventType::HitboxOn, EventId::KnightSword, 0},
    {28, EventType::Vfx, EventId::KnightSword, 0},
    {28, EventType::Audio, EventId::KnightSword, 0},
    {35, EventType::HitboxOff, EventId::KnightSword, 0},
    {36, EventType::Phase, EventId::None, PhaseValue(PhaseId::Recovery)}
};

constexpr FrameEvent kKnightSkillOneEvents[] = {
    {0, EventType::Phase, EventId::None, PhaseValue(PhaseId::Windup)},
    {46, EventType::HitboxOn, EventId::KnightGuard, 0},
    {50, EventType::Phase, EventId::None, PhaseValue(PhaseId::Active)},
    {50, EventType::Vfx, EventId::KnightGuard, 0},
    {50, EventType::Audio, EventId::KnightGuard, 0},
    {64, EventType::HitboxOff, EventId::KnightGuard, 0},
    {65, EventType::Phase, EventId::None, PhaseValue(PhaseId::Recovery)}
};

constexpr FrameEvent kKnightSkillTwoEvents[] = {
    {0, EventType::Phase, EventId::None, PhaseValue(PhaseId::Windup)},
    {35, EventType::Phase, EventId::None, PhaseValue(PhaseId::Active)},
    {36, EventType::HitboxOn, EventId::KnightRush, 0},
    {40, EventType::Vfx, EventId::KnightRush, 0},
    {40, EventType::Audio, EventId::KnightRush, 0},
    {46, EventType::HitboxOff, EventId::KnightRush, 0},
    {47, EventType::Phase, EventId::None, PhaseValue(PhaseId::Recovery)}
};

constexpr FrameEvent kKnightUltimateEvents[] = {
    {0, EventType::Phase, EventId::None, PhaseValue(PhaseId::Windup)},
    {58, EventType::Phase, EventId::None, PhaseValue(PhaseId::Active)},
    {60, EventType::HitboxOn, EventId::KnightBladeStorm, 0},
    {64, EventType::Vfx, EventId::KnightBladeStorm, 0},
    {64, EventType::Audio, EventId::KnightBladeStorm, 0},
    {74, EventType::HitboxOff, EventId::KnightBladeStorm, 0},
    {75, EventType::Phase, EventId::None, PhaseValue(PhaseId::Recovery)}
};

constexpr FrameEvent kKnightDashEvents[] = {
    {0, EventType::Phase, EventId::None, PhaseValue(PhaseId::Active)},
    {0, EventType::Vfx, EventId::KnightDash, 0},
    {0, EventType::Audio, EventId::KnightDash, 0},
    {23, EventType::HitboxOn, EventId::KnightDash, 0},
    {25, EventType::Vfx, EventId::KnightDash, 1},
    {29, EventType::HitboxOff, EventId::KnightDash, 0},
    {30, EventType::Phase, EventId::None, PhaseValue(PhaseId::Recovery)}
};

constexpr FrameEvent kMageRunEvents[] = {
    {0, EventType::Phase, EventId::None, PhaseValue(PhaseId::Locomotion)},
    {15, EventType::Footstep, EventId::LeftFoot, 0},
    {46, EventType::Footstep, EventId::RightFoot, 0}
};

constexpr FrameEvent kMageBasicEvents[] = {
    {0, EventType::Phase, EventId::None, PhaseValue(PhaseId::Windup)},
    {24, EventType::Phase, EventId::None, PhaseValue(PhaseId::Active)},
    {28, EventType::Projectile, EventId::MageBolt, 0},
    {28, EventType::Vfx, EventId::MageBolt, 0},
    {28, EventType::Audio, EventId::MageBolt, 0},
    {36, EventType::Phase, EventId::None, PhaseValue(PhaseId::Recovery)}
};

constexpr FrameEvent kMageSkillOneEvents[] = {
    {0, EventType::Phase, EventId::None, PhaseValue(PhaseId::Windup)},
    {46, EventType::HitboxOn, EventId::MageFrostNova, 0},
    {50, EventType::Phase, EventId::None, PhaseValue(PhaseId::Active)},
    {50, EventType::Vfx, EventId::MageFrostNova, 0},
    {50, EventType::Audio, EventId::MageFrostNova, 0},
    {58, EventType::HitboxOff, EventId::MageFrostNova, 0},
    {59, EventType::Phase, EventId::None, PhaseValue(PhaseId::Recovery)}
};

constexpr FrameEvent kMageSkillTwoEvents[] = {
    {0, EventType::Phase, EventId::None, PhaseValue(PhaseId::Windup)},
    {36, EventType::HitboxOn, EventId::MageGravityWell, 0},
    {40, EventType::Phase, EventId::None, PhaseValue(PhaseId::Active)},
    {40, EventType::Vfx, EventId::MageGravityWell, 0},
    {40, EventType::Audio, EventId::MageGravityWell, 0},
    {48, EventType::HitboxOff, EventId::MageGravityWell, 0},
    {49, EventType::Phase, EventId::None, PhaseValue(PhaseId::Recovery)}
};

constexpr FrameEvent kMageUltimateEvents[] = {
    {0, EventType::Phase, EventId::None, PhaseValue(PhaseId::Windup)},
    {58, EventType::Phase, EventId::None, PhaseValue(PhaseId::Active)},
    {60, EventType::HitboxOn, EventId::MageAstralTempest, 0},
    {64, EventType::Vfx, EventId::MageAstralTempest, 0},
    {64, EventType::Audio, EventId::MageAstralTempest, 0},
    {76, EventType::HitboxOff, EventId::MageAstralTempest, 0},
    {77, EventType::Phase, EventId::None, PhaseValue(PhaseId::Recovery)}
};

constexpr FrameEvent kMageDashEvents[] = {
    {0, EventType::Phase, EventId::None, PhaseValue(PhaseId::Active)},
    {0, EventType::Vfx, EventId::MageDash, 0},
    {0, EventType::Audio, EventId::MageDash, 0},
    {30, EventType::Phase, EventId::None, PhaseValue(PhaseId::Recovery)}
};

// Generic non-hero contacts match the shared 655-frame authoring sheet:
// Basic 190 => local 30, SkillOne 274 => local 50, SkillTwo 348 => local 44.
constexpr FrameEvent kNonHeroRunEvents[] = {
    {0, EventType::Phase, EventId::None, PhaseValue(PhaseId::Locomotion)},
    {14, EventType::Footstep, EventId::LeftFoot, 0},
    {45, EventType::Footstep, EventId::RightFoot, 0}
};

constexpr FrameEvent kNonHeroBasicEvents[] = {
    {0, EventType::Phase, EventId::None, PhaseValue(PhaseId::Windup)},
    {26, EventType::Phase, EventId::None, PhaseValue(PhaseId::Active)},
    {27, EventType::HitboxOn, EventId::NonHeroMelee, 0},
    {30, EventType::Vfx, EventId::NonHeroMelee, 0},
    {30, EventType::Audio, EventId::NonHeroMelee, 0},
    {36, EventType::HitboxOff, EventId::NonHeroMelee, 0},
    {37, EventType::Phase, EventId::None, PhaseValue(PhaseId::Recovery)}
};

constexpr FrameEvent kNonHeroSkillOneEvents[] = {
    {0, EventType::Phase, EventId::None, PhaseValue(PhaseId::Windup)},
    {46, EventType::Phase, EventId::None, PhaseValue(PhaseId::Active)},
    {50, EventType::Projectile, EventId::NonHeroProjectile, 0},
    {50, EventType::Vfx, EventId::NonHeroProjectile, 0},
    {50, EventType::Audio, EventId::NonHeroProjectile, 0},
    {60, EventType::Phase, EventId::None, PhaseValue(PhaseId::Recovery)}
};

constexpr FrameEvent kNonHeroSkillTwoEvents[] = {
    {0, EventType::Phase, EventId::None, PhaseValue(PhaseId::Windup)},
    {40, EventType::Phase, EventId::None, PhaseValue(PhaseId::Active)},
    {41, EventType::HitboxOn, EventId::NonHeroSkill, 0},
    {44, EventType::Vfx, EventId::NonHeroSkill, 0},
    {44, EventType::Audio, EventId::NonHeroSkill, 0},
    {54, EventType::HitboxOff, EventId::NonHeroSkill, 0},
    {55, EventType::Phase, EventId::None, PhaseValue(PhaseId::Recovery)}
};

constexpr FrameEvent kNonHeroUltimatePhaseEvents[] = {
    {0, EventType::Phase, EventId::None, PhaseValue(PhaseId::Windup)},
    {48, EventType::Phase, EventId::NonHeroUltimate, PhaseValue(PhaseId::Transition)},
    {48, EventType::Vfx, EventId::NonHeroUltimate, 0},
    {48, EventType::Audio, EventId::NonHeroUltimate, 0},
    {72, EventType::Phase, EventId::None, PhaseValue(PhaseId::Recovery)}
};

constexpr FrameEvent kNonHeroSpecialEvents[] = {
    {0, EventType::Phase, EventId::None, PhaseValue(PhaseId::Windup)},
    {20, EventType::Phase, EventId::None, PhaseValue(PhaseId::Active)},
    {22, EventType::HitboxOn, EventId::NonHeroSpecial, 0},
    {25, EventType::Vfx, EventId::NonHeroSpecial, 0},
    {25, EventType::Audio, EventId::NonHeroSpecial, 0},
    {31, EventType::HitboxOff, EventId::NonHeroSpecial, 0},
    {32, EventType::Phase, EventId::None, PhaseValue(PhaseId::Recovery)}
};

constexpr FrameEvent kNonHeroHurtEvents[] = {
    {0, EventType::Phase, EventId::NonHeroHurt, PhaseValue(PhaseId::Interrupted)},
    {0, EventType::Vfx, EventId::NonHeroHurt, 0},
    {0, EventType::Audio, EventId::NonHeroHurt, 0},
    {20, EventType::Phase, EventId::None, PhaseValue(PhaseId::Recovery)}
};

constexpr FrameEvent kNonHeroDeathEvents[] = {
    {0, EventType::Phase, EventId::NonHeroDeath, PhaseValue(PhaseId::Interrupted)},
    {8, EventType::Vfx, EventId::NonHeroDeath, 0},
    {8, EventType::Audio, EventId::NonHeroDeath, 0},
    {48, EventType::Phase, EventId::NonHeroDeath, PhaseValue(PhaseId::Dead)}
};

template <std::size_t N>
constexpr EventTrack MakeTrack(const FrameEvent (&events)[N],
                               std::uint16_t frameCount,
                               bool looping = false) noexcept {
    return {events, static_cast<std::uint16_t>(N), frameCount, looping};
}

constexpr EventTrack kHeroTracks[] = {
    MakeTrack(kKnightRunEvents, 63, true),
    MakeTrack(kKnightBasicEvents, 63),
    MakeTrack(kKnightSkillOneEvents, 79),
    MakeTrack(kKnightSkillTwoEvents, 79),
    MakeTrack(kKnightUltimateEvents, 95),
    MakeTrack(kKnightDashEvents, 47),
    MakeTrack(kMageRunEvents, 63, true),
    MakeTrack(kMageBasicEvents, 63),
    MakeTrack(kMageSkillOneEvents, 79),
    MakeTrack(kMageSkillTwoEvents, 79),
    MakeTrack(kMageUltimateEvents, 95),
    MakeTrack(kMageDashEvents, 47)
};

constexpr EventTrack kNonHeroTracks[] = {
    MakeTrack(kNonHeroRunEvents, 63, true),
    MakeTrack(kNonHeroBasicEvents, 63),
    MakeTrack(kNonHeroSkillOneEvents, 79),
    MakeTrack(kNonHeroSkillTwoEvents, 79),
    MakeTrack(kNonHeroUltimatePhaseEvents, 95),
    MakeTrack(kNonHeroSpecialEvents, 47),
    MakeTrack(kNonHeroHurtEvents, 31),
    MakeTrack(kNonHeroDeathEvents, 95)
};

constexpr EventTrack kInvalidTrack{};

constexpr ComboStep kKnightComboSteps[] = {
    {ComboMove::KnightLightOne, 63, 34, 54, 46, 8},
    {ComboMove::KnightLightTwo, 67, 35, 57, 48, 8},
    {ComboMove::KnightLightThree, 75, 0, 0, 0, 0}
};

constexpr ComboStep kMageComboSteps[] = {
    {ComboMove::MageArcaneBolt, 63, 35, 55, 48, 10},
    {ComboMove::MageFrostLink, 79, 48, 68, 60, 10},
    {ComboMove::MageGravityLink, 79, 0, 0, 0, 0}
};

constexpr ComboDefinition kKnightCombo{
    kKnightComboSteps, static_cast<std::uint8_t>(sizeof(kKnightComboSteps)
                                                  / sizeof(kKnightComboSteps[0]))
};
constexpr ComboDefinition kMageCombo{
    kMageComboSteps, static_cast<std::uint8_t>(sizeof(kMageComboSteps)
                                                / sizeof(kMageComboSteps[0]))
};

void DispatchRange(const EventTrack& track,
                   std::uint64_t firstInclusive,
                   std::uint64_t lastInclusive,
                   EventSink sink,
                   void* user) noexcept {
    if (sink == nullptr || firstInclusive > lastInclusive) return;

    if (!track.looping) {
        for (std::uint16_t index = 0; index < track.eventCount; ++index) {
            const std::uint64_t occurrence = track.events[index].frame;
            if (occurrence < firstInclusive) continue;
            if (occurrence > lastInclusive) break;
            const EventOccurrence dispatched{&track.events[index], occurrence, 0};
            sink(dispatched, user);
        }
        return;
    }

    const std::uint64_t period = track.frameCount;
    const std::uint64_t firstLoop = firstInclusive / period;
    const std::uint64_t lastLoop = lastInclusive / period;
    for (std::uint64_t loop = firstLoop;; ++loop) {
        const std::uint64_t base = loop * period;
        for (std::uint16_t index = 0; index < track.eventCount; ++index) {
            const std::uint64_t local = track.events[index].frame;
            if (local > std::numeric_limits<std::uint64_t>::max() - base) break;
            const std::uint64_t occurrence = base + local;
            if (occurrence < firstInclusive) continue;
            if (occurrence > lastInclusive) break;
            const EventOccurrence dispatched{&track.events[index], occurrence, loop};
            sink(dispatched, user);
        }
        if (loop == lastLoop) break;
    }
}

} // namespace

bool EventTrack::IsValid() const noexcept {
    if (frameCount == 0) return false;
    if (eventCount > 0 && events == nullptr) return false;
    std::uint16_t previousFrame = 0;
    for (std::uint16_t index = 0; index < eventCount; ++index) {
        const FrameEvent& event = events[index];
        if (event.frame >= frameCount) return false;
        if (index > 0 && event.frame < previousFrame) return false;
        previousFrame = event.frame;
    }
    return true;
}

void EventCursor::Start(std::uint64_t absoluteFrame,
                        bool emitStartingFrame) noexcept {
    m_lastAbsoluteFrame = absoluteFrame;
    m_started = true;
    m_emitStartingFrame = emitStartingFrame;
}

void EventCursor::Seek(std::uint64_t absoluteFrame) noexcept {
    m_lastAbsoluteFrame = absoluteFrame;
    m_started = true;
    m_emitStartingFrame = false;
}

void EventCursor::Stop() noexcept {
    m_lastAbsoluteFrame = 0;
    m_started = false;
    m_emitStartingFrame = false;
}

CursorAdvance EventCursor::Advance(const EventTrack& track,
                                   std::uint64_t absoluteFrame,
                                   EventSink sink,
                                   void* user) noexcept {
    if (!track.IsValid()) return CursorAdvance::InvalidTrack;
    if (!m_started) {
        Start(absoluteFrame, true);
    }
    if (absoluteFrame < m_lastAbsoluteFrame) {
        Seek(absoluteFrame);
        return CursorAdvance::SeekedBackward;
    }

    if (absoluteFrame == m_lastAbsoluteFrame && !m_emitStartingFrame)
        return CursorAdvance::NoAdvance;

    std::uint64_t firstInclusive = m_lastAbsoluteFrame;
    if (!m_emitStartingFrame) {
        if (firstInclusive == std::numeric_limits<std::uint64_t>::max())
            return CursorAdvance::NoAdvance;
        ++firstInclusive;
    }
    DispatchRange(track, firstInclusive, absoluteFrame, sink, user);
    m_lastAbsoluteFrame = absoluteFrame;
    m_emitStartingFrame = false;
    return CursorAdvance::Advanced;
}

const EventTrack& GetHeroTrack(HeroTrackId id) noexcept {
    const std::size_t index = static_cast<std::size_t>(id);
    if (index >= static_cast<std::size_t>(HeroTrackId::Count))
        return kInvalidTrack;
    return kHeroTracks[index];
}

const EventTrack& GetNonHeroTrack(NonHeroTrackId id) noexcept {
    const std::size_t index = static_cast<std::size_t>(id);
    if (index >= static_cast<std::size_t>(NonHeroTrackId::Count))
        return kInvalidTrack;
    return kNonHeroTracks[index];
}

bool ComboDefinition::IsValid() const noexcept {
    if (steps == nullptr || stepCount == 0) return false;
    for (std::uint8_t index = 0; index < stepCount; ++index) {
        const ComboStep& step = steps[index];
        if (step.move == ComboMove::None || step.durationFrames == 0) return false;
        if (index + 1u < stepCount) {
            if (step.windowOpenFrame > step.transitionFrame
                || step.transitionFrame > step.windowCloseFrame
                || step.windowCloseFrame >= step.durationFrames) {
                return false;
            }
        }
    }
    return true;
}

const ComboDefinition& GetComboDefinition(ComboChainId id) noexcept {
    return id == ComboChainId::MageCastChain ? kMageCombo : kKnightCombo;
}

bool ComboBuffer::Begin(const ComboDefinition& definition,
                        std::uint64_t absoluteFrame) noexcept {
    Cancel();
    if (!definition.IsValid()) return false;
    m_definition = &definition;
    m_stepStartFrame = absoluteFrame;
    m_lastUpdateFrame = absoluteFrame;
    m_active = true;
    return true;
}

void ComboBuffer::Cancel() noexcept {
    m_definition = nullptr;
    m_stepStartFrame = 0;
    m_lastUpdateFrame = 0;
    m_bufferedAtFrame = 0;
    m_stepIndex = 0;
    m_active = false;
    m_buffered = false;
}

bool ComboBuffer::QueueInput(std::uint64_t absoluteFrame) noexcept {
    if (!m_active || m_definition == nullptr
        || m_stepIndex + 1u >= m_definition->stepCount
        || absoluteFrame < m_stepStartFrame
        || absoluteFrame < m_lastUpdateFrame) {
        return false;
    }

    const ComboStep& step = m_definition->steps[m_stepIndex];
    const std::uint64_t local = absoluteFrame - m_stepStartFrame;
    const std::uint64_t earliest = step.windowOpenFrame > step.earlyBufferFrames
        ? step.windowOpenFrame - step.earlyBufferFrames : 0;
    if (local < earliest || local > step.windowCloseFrame) return false;

    m_buffered = true;
    m_bufferedAtFrame = absoluteFrame;
    return true;
}

ComboUpdate ComboBuffer::Update(std::uint64_t absoluteFrame) noexcept {
    ComboUpdate result{};
    if (!m_active || m_definition == nullptr) return result;
    if (absoluteFrame < m_lastUpdateFrame) return result;
    m_lastUpdateFrame = absoluteFrame;

    const ComboStep& step = m_definition->steps[m_stepIndex];
    const std::uint64_t local = absoluteFrame - m_stepStartFrame;
    if (m_buffered && m_stepIndex + 1u < m_definition->stepCount
        && local >= step.transitionFrame) {
        m_stepStartFrame += step.transitionFrame;
        ++m_stepIndex;
        m_buffered = false;
        m_bufferedAtFrame = 0;
        result.type = ComboUpdateType::StepChanged;
        result.stepIndex = m_stepIndex;
        result.move = m_definition->steps[m_stepIndex].move;
        return result;
    }

    if (local >= step.durationFrames) {
        result.type = ComboUpdateType::Finished;
        result.stepIndex = m_stepIndex;
        result.move = step.move;
        Cancel();
    }
    return result;
}

ComboMove ComboBuffer::CurrentMove() const noexcept {
    if (!m_active || m_definition == nullptr
        || m_stepIndex >= m_definition->stepCount) {
        return ComboMove::None;
    }
    return m_definition->steps[m_stepIndex].move;
}

} // namespace Survival3D::Animation
