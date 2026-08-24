#include "Survival3D/Animation/AnimationEvents.h"
#include "Survival3D/Systems/RuntimeIK.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <new>
#include <stdexcept>

namespace {

std::size_t g_allocationCount = 0;

struct EventLog {
    Survival3D::Animation::EventOccurrence entries[128]{};
    int count = 0;
};

void RecordEvent(const Survival3D::Animation::EventOccurrence& occurrence,
                 void* user) noexcept {
    EventLog& log = *static_cast<EventLog*>(user);
    if (log.count < static_cast<int>(sizeof(log.entries) / sizeof(log.entries[0])))
        log.entries[log.count++] = occurrence;
}

void CountEvent(const Survival3D::Animation::EventOccurrence&, void* user) noexcept {
    ++*static_cast<std::uint64_t*>(user);
}

void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

bool Nearly(float a, float b, float epsilon = 1.0e-4f) {
    return std::abs(a - b) <= epsilon;
}

struct PlaneProbeContext {
    float height = 0.0f;
    Survival3D::RuntimeIK::Vec3 normal{0.0f, 1.0f, 0.0f};
    int calls = 0;
    bool enabled = true;
};

bool ProbePlane(const Survival3D::RuntimeIK::GroundRay& ray,
                Survival3D::RuntimeIK::GroundHit& hit,
                void* user) noexcept {
    using namespace Survival3D::RuntimeIK;
    PlaneProbeContext& context = *static_cast<PlaneProbeContext*>(user);
    ++context.calls;
    if (!context.enabled || std::abs(ray.direction.y) < 1.0e-6f) return false;
    const float distance = (context.height - ray.origin.y) / ray.direction.y;
    if (distance < 0.0f || distance > ray.maxDistance) return false;
    hit.point = ray.origin + ray.direction * distance;
    hit.normal = context.normal;
    hit.distance = distance;
    return true;
}

void TestEventCursor() {
    using namespace Survival3D::Animation;
    static constexpr FrameEvent events[] = {
        {0, EventType::Footstep, EventId::LeftFoot, 0},
        {3, EventType::Vfx, EventId::KnightSword, 0},
        {8, EventType::Audio, EventId::KnightSword, 0}
    };
    constexpr EventTrack looping{events, 3, 10, true};
    Require(looping.IsValid(), "valid looping track rejected");

    EventLog log{};
    EventCursor cursor;
    cursor.Start(0, true);
    Require(cursor.Advance(looping, 8, RecordEvent, &log) == CursorAdvance::Advanced,
            "initial event advance failed");
    Require(log.count == 3, "dropped-frame event range was not fully dispatched");
    Require(log.entries[0].absoluteFrame == 0
            && log.entries[1].absoluteFrame == 3
            && log.entries[2].absoluteFrame == 8,
            "events were not dispatched in authored frame order");

    cursor.Advance(looping, 32, RecordEvent, &log);
    constexpr std::uint64_t expected[] = {10, 13, 18, 20, 23, 28, 30};
    Require(log.count == 10, "multi-wrap event count is wrong");
    for (int i = 0; i < 7; ++i)
        Require(log.entries[i + 3].absoluteFrame == expected[i],
                "multi-wrap event occurrence is wrong");
    Require(log.entries[9].loopIndex == 3, "loop index is wrong after wraps");

    const int beforeSeek = log.count;
    Require(cursor.Advance(looping, 4, RecordEvent, &log)
                == CursorAdvance::SeekedBackward,
            "backward seek was not detected");
    Require(log.count == beforeSeek, "backward seek replayed stale events");
    Require(cursor.Advance(looping, 4, RecordEvent, &log)
                == CursorAdvance::NoAdvance,
            "stationary cursor should not dispatch");

    static constexpr FrameEvent unsortedEvents[] = {
        {5, EventType::Audio, EventId::None, 0},
        {2, EventType::Vfx, EventId::None, 0}
    };
    constexpr EventTrack unsorted{unsortedEvents, 2, 8, false};
    Require(!unsorted.IsValid(), "unsorted track accepted");
}

void TestProductionTracks() {
    using namespace Survival3D::Animation;
    bool seenTypes[7]{};
    for (std::uint8_t raw = 0; raw < static_cast<std::uint8_t>(HeroTrackId::Count); ++raw) {
        const EventTrack& track = GetHeroTrack(static_cast<HeroTrackId>(raw));
        Require(track.IsValid(), "production hero event track is invalid");
        for (std::uint16_t index = 0; index < track.eventCount; ++index)
            seenTypes[static_cast<std::uint8_t>(track.events[index].type)] = true;
    }
    for (bool seen : seenTypes)
        Require(seen, "one required animation event type has no production track");

    const EventTrack& knightBasic = GetHeroTrack(HeroTrackId::KnightBasic);
    EventLog log{};
    EventCursor cursor;
    cursor.Start(0, true);
    cursor.Advance(knightBasic, 40, RecordEvent, &log);
    Require(log.count == knightBasic.eventCount,
            "Knight Basic dropped-frame dispatch missed events");
    Require(log.entries[2].event->type == EventType::HitboxOn
            && log.entries[2].absoluteFrame == 25,
            "Knight Basic hitbox-on frame changed");
    Require(log.entries[3].event->type == EventType::Vfx
            && log.entries[4].event->type == EventType::Audio
            && log.entries[3].absoluteFrame == 28
            && log.entries[4].absoluteFrame == 28,
            "Knight Basic contact cues are not frame-exact");
    Require(log.entries[5].event->type == EventType::HitboxOff
            && log.entries[5].absoluteFrame == 35,
            "Knight Basic hitbox-off frame changed");

    const EventTrack& mageBasic = GetHeroTrack(HeroTrackId::MageBasic);
    log = {};
    cursor.Start(0, true);
    cursor.Advance(mageBasic, 28, RecordEvent, &log);
    bool projectileAtContact = false;
    for (int i = 0; i < log.count; ++i) {
        if (log.entries[i].event->type == EventType::Projectile
            && log.entries[i].absoluteFrame == 28) {
            projectileAtContact = true;
        }
    }
    Require(projectileAtContact, "Mage projectile event is not on contact frame");
}

void TestNonHeroTracks() {
    using namespace Survival3D::Animation;
    for (std::uint8_t raw = 0;
         raw < static_cast<std::uint8_t>(NonHeroTrackId::Count); ++raw) {
        Require(GetNonHeroTrack(static_cast<NonHeroTrackId>(raw)).IsValid(),
                "generic enemy/boss event track is invalid");
    }

    EventLog log{};
    EventCursor cursor;
    const EventTrack& basic = GetNonHeroTrack(NonHeroTrackId::Basic);
    cursor.Start(0, true);
    cursor.Advance(basic, 50, RecordEvent, &log); // skip entire contact window
    Require(log.count == basic.eventCount,
            "dropped frame skipped generic Basic contact events");
    bool hitboxOn = false;
    bool contactVfx = false;
    bool hitboxOff = false;
    for (int i = 0; i < log.count; ++i) {
        const FrameEvent& event = *log.entries[i].event;
        hitboxOn |= event.type == EventType::HitboxOn && event.frame == 27;
        contactVfx |= event.type == EventType::Vfx && event.frame == 30;
        hitboxOff |= event.type == EventType::HitboxOff && event.frame == 36;
    }
    Require(hitboxOn && contactVfx && hitboxOff,
            "generic Basic contact window is incomplete");

    log = {};
    const EventTrack& ranged = GetNonHeroTrack(NonHeroTrackId::SkillOne);
    cursor.Start(0, true);
    cursor.Advance(ranged, 68, RecordEvent, &log);
    bool projectile = false;
    for (int i = 0; i < log.count; ++i) {
        const FrameEvent& event = *log.entries[i].event;
        projectile |= event.type == EventType::Projectile
                   && event.id == EventId::NonHeroProjectile
                   && event.frame == 50;
    }
    Require(projectile, "generic ranged projectile event was lost after a frame drop");

    log = {};
    const EventTrack& run = GetNonHeroTrack(NonHeroTrackId::Run);
    cursor.Start(60, false);
    cursor.Advance(run, 110, RecordEvent, &log);
    Require(log.count == 3
            && log.entries[0].event->type == EventType::Phase
            && log.entries[1].event->type == EventType::Footstep
            && log.entries[1].absoluteFrame == 77
            && log.entries[2].absoluteFrame == 108,
            "generic locomotion events did not survive loop wrap/drop");
}

void TestComboBuffers() {
    using namespace Survival3D::Animation;
    const ComboDefinition& knight = GetComboDefinition(ComboChainId::KnightThreeHit);
    const ComboDefinition& mage = GetComboDefinition(ComboChainId::MageCastChain);
    Require(knight.IsValid() && mage.IsValid(), "production combo definition invalid");

    ComboBuffer combo;
    Require(combo.Begin(knight, 100), "Knight combo did not begin");
    Require(combo.CurrentMove() == ComboMove::KnightLightOne,
            "Knight combo began on the wrong move");
    Require(!combo.QueueInput(125), "input before early buffer was accepted");
    Require(combo.QueueInput(126), "early-buffered Knight input was rejected");
    Require(combo.Update(145).type == ComboUpdateType::None,
            "Knight combo transitioned before branch frame");
    ComboUpdate update = combo.Update(160); // dropped across frame 46
    Require(update.type == ComboUpdateType::StepChanged
            && update.move == ComboMove::KnightLightTwo
            && combo.StepStartFrame() == 146,
            "Knight combo did not preserve frame-exact transition after a drop");
    Require(combo.QueueInput(173), "Knight second-link input was rejected");
    update = combo.Update(200);
    Require(update.type == ComboUpdateType::StepChanged
            && update.move == ComboMove::KnightLightThree
            && combo.StepStartFrame() == 194,
            "Knight third hit did not start");
    Require(combo.Update(269).type == ComboUpdateType::Finished
            && !combo.IsActive(),
            "Knight three-hit chain did not finish");

    Require(combo.Begin(mage, 0), "Mage cast chain did not begin");
    Require(combo.QueueInput(25), "Mage early cast buffer was rejected");
    update = combo.Update(48);
    Require(update.type == ComboUpdateType::StepChanged
            && update.move == ComboMove::MageFrostLink,
            "Mage Frost link did not start");
    Require(combo.QueueInput(86), "Mage Gravity link input was rejected");
    update = combo.Update(108);
    Require(update.type == ComboUpdateType::StepChanged
            && update.move == ComboMove::MageGravityLink,
            "Mage Gravity link did not start");
}

void TestFootIk() {
    using namespace Survival3D::RuntimeIK;
    FootIkSettings settings;
    FootIkState state{};
    FootIkInput input;
    input.pelvisPosition = {0.0f, 1.0f, 0.0f};
    input.leftAnimatedFoot = {-0.2f, 0.20f, 0.0f};
    input.rightAnimatedFoot = {0.2f, -0.10f, 0.0f};
    input.deltaSeconds = 1.0f / 60.0f;
    input.snap = true;
    PlaneProbeContext plane{};
    const FootIkOutput result = SolveFeetAndPelvis(settings, input,
                                                   ProbePlane, &plane, state);
    Require(plane.calls == 2, "foot solver did not perform exactly two probes");
    Require(result.left.grounded && result.right.grounded,
            "flat ground contacts were missed");
    Require(Nearly(result.left.target.y, settings.soleOffset)
            && Nearly(result.right.target.y, settings.soleOffset),
            "foot soles were not placed on the plane");
    Require(Nearly(result.pelvisOffset, settings.soleOffset - 0.20f),
            "pelvis did not follow the lower foot");
    Require(Nearly(result.pelvisPosition.y, 1.0f + result.pelvisOffset),
            "pelvis output position is inconsistent");

    plane.normal = NormalizeOr({0.8f, 0.1f, 0.0f}, {1.0f, 0.0f, 0.0f});
    input.snap = true;
    const FootIkOutput steep = SolveFeetAndPelvis(settings, input,
                                                  ProbePlane, &plane, state);
    Require(!steep.left.grounded && !steep.right.grounded
            && Nearly(steep.left.weight, 0.0f),
            "too-steep ground was accepted");

    input.weight = 0.0f;
    plane.normal = {0.0f, 1.0f, 0.0f};
    const FootIkOutput disabled = SolveFeetAndPelvis(settings, input,
                                                     ProbePlane, &plane, state);
    Require(Nearly(disabled.left.target.y, input.leftAnimatedFoot.y)
            && Nearly(disabled.pelvisOffset, 0.0f),
            "global IK weight did not restore authored pose");
}

void TestAimAndHands() {
    using namespace Survival3D::RuntimeIK;
    AimSettings aimSettings;
    aimSettings.maxYawRadians = 0.50f;
    AimInput input;
    input.origin = {0.0f, 1.5f, 0.0f};
    input.characterForward = {0.0f, 0.0f, 1.0f};
    input.desiredWorldPoint = {10.0f, 1.5f, 0.0f};
    input.snap = true;
    AimState aimState{};
    const AimOutput aim = SolveAimTarget(aimSettings, input, aimState);
    Require(aim.clamped && Nearly(aim.yawRadians, 0.50f),
            "aim yaw was not clamped in character space");
    Require(Nearly(Length(aim.direction), 1.0f)
            && Nearly(Length(aim.target - input.origin), aimSettings.targetDistance),
            "aim direction/target length is invalid");

    HandTargetSettings handSettings;
    HandTargetInput handsInput;
    handsInput.chestPosition = {0.0f, 1.3f, 0.0f};
    handsInput.aimDirection = {0.0f, 0.0f, 1.0f};
    const HandTargets rightHanded = BuildHandTargets(handSettings, handsInput);
    Require(rightHanded.mainHand.x > 0.0f && rightHanded.supportHand.x < 0.0f,
            "right-handed goals are on the wrong sides");
    handsInput.rightHanded = false;
    const HandTargets leftHanded = BuildHandTargets(handSettings, handsInput);
    Require(Nearly(leftHanded.mainHand.x, -rightHanded.mainHand.x)
            && Nearly(leftHanded.supportHand.x, -rightHanded.supportHand.x),
            "left-handed goals did not mirror");
    Require(rightHanded.mainElbowPole.x > 0.0f
            && rightHanded.supportElbowPole.x < 0.0f,
            "elbow poles do not point outward");
}

void TestAllocationFreeHotPaths() {
    using namespace Survival3D::Animation;
    using namespace Survival3D::RuntimeIK;
    const std::size_t before = g_allocationCount;

    std::uint64_t dispatched = 0;
    EventCursor cursor;
    cursor.Start(0, true);
    const EventTrack& run = GetHeroTrack(HeroTrackId::KnightRun);
    for (std::uint64_t frame = 0; frame < 1000; ++frame)
        cursor.Advance(run, frame, CountEvent, &dispatched);

    FootIkSettings settings;
    FootIkState state{};
    FootIkInput feet;
    feet.pelvisPosition = {0.0f, 1.0f, 0.0f};
    feet.leftAnimatedFoot = {-0.2f, 0.1f, 0.0f};
    feet.rightAnimatedFoot = {0.2f, 0.1f, 0.0f};
    feet.deltaSeconds = 1.0f / 60.0f;
    PlaneProbeContext plane{};
    AimSettings aimSettings;
    AimInput aimInput;
    aimInput.desiredWorldPoint = {2.0f, 1.0f, 5.0f};
    AimState aimState{};
    for (int tick = 0; tick < 1000; ++tick) {
        SolveFeetAndPelvis(settings, feet, ProbePlane, &plane, state);
        SolveAimTarget(aimSettings, aimInput, aimState);
    }

    Require(g_allocationCount == before,
            "animation/IK hot update allocated heap memory");
    Require(dispatched > 0, "allocation test did not exercise event dispatch");
}

} // namespace

void* operator new(std::size_t size) {
    ++g_allocationCount;
    if (void* memory = std::malloc(size)) return memory;
    throw std::bad_alloc();
}

void operator delete(void* memory) noexcept {
    std::free(memory);
}

void operator delete(void* memory, std::size_t) noexcept {
    std::free(memory);
}

void* operator new[](std::size_t size) {
    ++g_allocationCount;
    if (void* memory = std::malloc(size)) return memory;
    throw std::bad_alloc();
}

void operator delete[](void* memory) noexcept {
    std::free(memory);
}

void operator delete[](void* memory, std::size_t) noexcept {
    std::free(memory);
}

int main() {
    try {
        TestEventCursor();
        std::cout << "[PASS] frame-exact dropped/wrapped animation events\n";
        TestProductionTracks();
        std::cout << "[PASS] Knight/Mage production event tracks\n";
        TestNonHeroTracks();
        std::cout << "[PASS] shared enemy/boss event tracks\n";
        TestComboBuffers();
        std::cout << "[PASS] Knight three-hit and Mage cast-chain buffering\n";
        TestFootIk();
        std::cout << "[PASS] runtime feet and pelvis IK\n";
        TestAimAndHands();
        std::cout << "[PASS] runtime aim and hand target helpers\n";
        TestAllocationFreeHotPaths();
        std::cout << "[PASS] allocation-free hot updates\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
