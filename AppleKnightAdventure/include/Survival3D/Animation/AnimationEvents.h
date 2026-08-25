#pragma once

#include <cstddef>
#include <cstdint>

namespace Survival3D::Animation {

// Animation events are authored on integer frames. Multiple events may share
// a frame; the order in the source track is their deterministic dispatch order.
enum class EventType : std::uint8_t {
    HitboxOn,
    HitboxOff,
    Projectile,
    Vfx,
    Audio,
    Phase,
    Footstep
};

enum class PhaseId : std::uint16_t {
    Windup,
    Active,
    Recovery,
    Locomotion,
    Transition,
    Interrupted,
    Dead
};

enum class EventId : std::uint16_t {
    None,
    LeftFoot,
    RightFoot,
    KnightSword,
    KnightGuard,
    KnightRush,
    KnightBladeStorm,
    KnightDash,
    MageBolt,
    MageFrostNova,
    MageGravityWell,
    MageAstralTempest,
    MageDash,
    NonHeroMelee,
    NonHeroProjectile,
    NonHeroSkill,
    NonHeroUltimate,
    NonHeroSpecial,
    NonHeroHurt,
    NonHeroDeath
};

struct FrameEvent {
    std::uint16_t frame = 0;
    EventType type = EventType::Phase;
    EventId id = EventId::None;
    // Phase events store PhaseId here. Other event types may use it as a
    // variant/channel index without changing the hot-path representation.
    std::uint16_t value = 0;
};

struct EventTrack {
    const FrameEvent* events = nullptr;
    std::uint16_t eventCount = 0;
    // Number of discrete frames in the local clip. Valid local frames are
    // [0, frameCount). Loop period is exactly frameCount.
    std::uint16_t frameCount = 0;
    bool looping = false;

    bool IsValid() const noexcept;
};

struct EventOccurrence {
    const FrameEvent* event = nullptr;
    std::uint64_t absoluteFrame = 0;
    std::uint64_t loopIndex = 0;
};

using EventSink = void (*)(const EventOccurrence&, void* user) noexcept;

enum class CursorAdvance : std::uint8_t {
    InvalidTrack,
    NoAdvance,
    Advanced,
    SeekedBackward
};

// Uses a monotonically increasing absolute frame rather than a wrapped local
// frame. This makes dropped frames and even multiple skipped loops unambiguous.
// Start(..., true) emits events authored at the starting frame on the first
// Advance call. Seek never replays events.
class EventCursor {
public:
    void Start(std::uint64_t absoluteFrame = 0,
               bool emitStartingFrame = true) noexcept;
    void Seek(std::uint64_t absoluteFrame) noexcept;
    void Stop() noexcept;

    CursorAdvance Advance(const EventTrack& track,
                          std::uint64_t absoluteFrame,
                          EventSink sink,
                          void* user = nullptr) noexcept;

    bool IsStarted() const noexcept { return m_started; }
    std::uint64_t LastAbsoluteFrame() const noexcept { return m_lastAbsoluteFrame; }

private:
    std::uint64_t m_lastAbsoluteFrame = 0;
    bool m_started = false;
    bool m_emitStartingFrame = false;
};

enum class HeroTrackId : std::uint8_t {
    KnightRun,
    KnightBasic,
    KnightSkillOne,
    KnightSkillTwo,
    KnightUltimate,
    KnightDash,
    MageRun,
    MageBasic,
    MageSkillOne,
    MageSkillTwo,
    MageUltimate,
    MageDash,
    Count
};

// Static production tracks matching the 60 FPS modular hero action sheet.
// Returned references remain valid for the process lifetime.
const EventTrack& GetHeroTrack(HeroTrackId id) noexcept;

// Shared enemy/boss tracks. An archetype adapter maps generic EventId values
// to its concrete damage, projectile, audio and VFX definitions; all three
// enemies and all five bosses can therefore share the cursor implementation.
enum class NonHeroTrackId : std::uint8_t {
    Run,
    Basic,
    SkillOne,
    SkillTwo,
    UltimatePhase,
    Special,
    Hurt,
    Death,
    Count
};

const EventTrack& GetNonHeroTrack(NonHeroTrackId id) noexcept;

enum class ComboMove : std::uint8_t {
    None,
    KnightLightOne,
    KnightLightTwo,
    KnightLightThree,
    MageArcaneBolt,
    MageFrostLink,
    MageGravityLink
};

struct ComboStep {
    ComboMove move = ComboMove::None;
    std::uint16_t durationFrames = 0;
    // Inputs inside [windowOpenFrame, windowCloseFrame] are accepted. Inputs
    // up to earlyBufferFrames before opening are retained as an input buffer.
    std::uint16_t windowOpenFrame = 0;
    std::uint16_t windowCloseFrame = 0;
    std::uint16_t transitionFrame = 0;
    std::uint8_t earlyBufferFrames = 0;
};

struct ComboDefinition {
    const ComboStep* steps = nullptr;
    std::uint8_t stepCount = 0;

    bool IsValid() const noexcept;
};

enum class ComboChainId : std::uint8_t {
    KnightThreeHit,
    MageCastChain
};

const ComboDefinition& GetComboDefinition(ComboChainId id) noexcept;

enum class ComboUpdateType : std::uint8_t {
    None,
    StepChanged,
    Finished
};

struct ComboUpdate {
    ComboUpdateType type = ComboUpdateType::None;
    ComboMove move = ComboMove::None;
    std::uint8_t stepIndex = 0;
};

// A small deterministic input-buffer/branch helper. Frames are absolute and
// monotonic; each transition clears the pending input, preventing one press
// from skipping more than one combo step.
class ComboBuffer {
public:
    bool Begin(const ComboDefinition& definition,
               std::uint64_t absoluteFrame) noexcept;
    void Cancel() noexcept;

    bool QueueInput(std::uint64_t absoluteFrame) noexcept;
    ComboUpdate Update(std::uint64_t absoluteFrame) noexcept;

    bool IsActive() const noexcept { return m_active; }
    bool HasBufferedInput() const noexcept { return m_buffered; }
    std::uint8_t StepIndex() const noexcept { return m_stepIndex; }
    ComboMove CurrentMove() const noexcept;
    std::uint64_t StepStartFrame() const noexcept { return m_stepStartFrame; }

private:
    const ComboDefinition* m_definition = nullptr;
    std::uint64_t m_stepStartFrame = 0;
    std::uint64_t m_lastUpdateFrame = 0;
    std::uint64_t m_bufferedAtFrame = 0;
    std::uint8_t m_stepIndex = 0;
    bool m_active = false;
    bool m_buffered = false;
};

} // namespace Survival3D::Animation
