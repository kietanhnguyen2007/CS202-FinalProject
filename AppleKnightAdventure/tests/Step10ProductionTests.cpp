#include "Survival3D/Animation/AnimationEvents.h"
#include "Survival3D/Animation/AnimationGraph.h"
#include "Survival3D/Systems/RuntimeIK.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <new>
#include <regex>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::size_t g_allocationCount = 0;

void Require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

std::string ReadText(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    Require(static_cast<bool>(input), "cannot open " + path.string());
    return std::string(std::istreambuf_iterator<char>(input),
                       std::istreambuf_iterator<char>());
}

std::size_t ParseSize(const std::string& text, const std::regex& pattern,
                      const std::string& label) {
    std::smatch match;
    Require(std::regex_search(text, match, pattern) && match.size() == 2,
            "cannot parse " + label);
    return static_cast<std::size_t>(std::stoull(match[1].str()));
}

struct RepositoryAudit {
    std::size_t enemyPoolCapacity = 0;
    std::size_t projectilePoolCapacity = 0;
    std::size_t directorActiveCap = 0;
    std::size_t animatedModelCount = 0;
    std::uintmax_t animatedModelBytes = 0;
    std::size_t qaPngCount = 0;
    std::size_t survivalRuntimePythonFiles = 0;
};

RepositoryAudit AuditRepository(const std::filesystem::path& repository) {
    const std::filesystem::path app = repository / "AppleKnightAdventure";
    const std::filesystem::path assets = repository / "assets" / "survival3d";
    const std::string controllerHeader = ReadText(
        app / "include" / "Survival3D" / "Controller" / "SurvivalController.h");
    const std::string controllerSource = ReadText(
        app / "src" / "Survival3D" / "Controller" / "SurvivalController.cpp");
    const std::string viewSource = ReadText(
        app / "src" / "Survival3D" / "View" / "SurvivalView.cpp");
    const std::string balance = ReadText(assets / "config" / "balance.json");
    const std::string animationManifest = ReadText(
        assets / "config" / "animation_manifest.json");
    const std::string step7ActorManifest = ReadText(
        assets / "config" / "animation_clips_step7.json");
    const std::string skillManifest = ReadText(
        assets / "config" / "skill_asset_manifest.json");

    RepositoryAudit audit;
    audit.enemyPoolCapacity = ParseSize(
        controllerHeader,
        std::regex(R"(kEnemyCapacity\s*=\s*([0-9]+))"),
        "kEnemyCapacity");
    audit.projectilePoolCapacity = ParseSize(
        controllerHeader,
        std::regex(R"(kProjectileCapacity\s*=\s*([0-9]+))"),
        "kProjectileCapacity");
    audit.directorActiveCap = ParseSize(
        balance,
        std::regex(R"("activeCapMax"\s*:\s*([0-9]+))"),
        "director.activeCapMax");
    Require(audit.enemyPoolCapacity >= audit.directorActiveCap,
            "director active cap exceeds enemy pool capacity");
    Require(audit.enemyPoolCapacity == 144,
            "production enemy pool contract changed from 144");
    Require(audit.projectilePoolCapacity == 384,
            "production projectile pool contract changed from 384");
    Require(audit.directorActiveCap == 120,
            "production director cap changed from 120");

    for (const char* required : {
             "m_enemies.resize(kEnemyCapacity)",
             "m_enemyAnimationGraphs.reserve(kEnemyCapacity)",
             "m_enemyEventCursors.resize(kEnemyCapacity)",
             "m_freeEnemies.reserve(kEnemyCapacity)",
             "m_projectiles.resize(kProjectileCapacity)",
             "m_freeProjectiles.reserve(kProjectileCapacity)"}) {
        Require(controllerSource.find(required) != std::string::npos,
                std::string("pool preallocation contract missing: ") + required);
    }

    Require(ParseSize(animationManifest,
                      std::regex(R"("sampleRate"\s*:\s*([0-9]+))"),
                      "animation sampleRate") == 60,
            "animation sample rate must remain 60 FPS");
    Require(ParseSize(animationManifest,
                      std::regex(R"("totalFrames"\s*:\s*([0-9]+))"),
                      "animation totalFrames") == 655,
            "animation master contract must remain 655 frames");

    const std::regex modelPattern(R"json("model"\s*:\s*"([^"]+\.glb)")json");
    std::set<std::string> modelPaths;
    for (const std::string* manifest : {
             &animationManifest, &step7ActorManifest, &skillManifest}) {
        for (std::sregex_iterator it(manifest->begin(), manifest->end(),
                                     modelPattern), end;
             it != end; ++it) {
            modelPaths.insert((*it)[1].str());
        }
    }
    Require(modelPaths.size() == 33,
            "production manifests must reference 13 legacy/modular models, "
            "10 named actors, and 10 skill models");
    for (const std::string& relative : modelPaths) {
        const std::filesystem::path model = assets / relative;
        Require(std::filesystem::is_regular_file(model),
                "missing runtime model " + model.string());
        const std::uintmax_t bytes = std::filesystem::file_size(model);
        Require(bytes > 1024, "runtime model is unexpectedly small " + model.string());
        audit.animatedModelBytes += bytes;
    }
    audit.animatedModelCount = modelPaths.size();
    for (const std::string& requiredPath : modelPaths) {
        if (requiredPath.rfind("models/step7/actors/", 0) == 0
            || requiredPath.rfind("models/skills/", 0) == 0) {
            Require(viewSource.find("assets/survival3d/" + requiredPath)
                        != std::string::npos,
                    "Step 7 runtime asset is not loaded by the C++ view: "
                        + requiredPath);
        }
    }

    const std::filesystem::path qaRoot = assets / "production_v3";
    if (std::filesystem::exists(qaRoot)) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(qaRoot)) {
            if (entry.is_regular_file() && entry.path().extension() == ".png")
                ++audit.qaPngCount;
        }
    }
    Require(audit.qaPngCount > 0, "no production-v3 visual QA renders found");

    const std::filesystem::path runtimeRoot = app / "src" / "Survival3D";
    for (const auto& entry : std::filesystem::recursive_directory_iterator(runtimeRoot)) {
        if (entry.is_regular_file() && entry.path().extension() == ".py")
            ++audit.survivalRuntimePythonFiles;
    }
    Require(audit.survivalRuntimePythonFiles == 0,
            "Python file leaked into Survival3D runtime source");
    return audit;
}

struct PlaneContext {
    float height = 0.0f;
    std::uint64_t calls = 0;
};

bool ProbePlane(const Survival3D::RuntimeIK::GroundRay& ray,
                Survival3D::RuntimeIK::GroundHit& hit,
                void* user) noexcept {
    PlaneContext& context = *static_cast<PlaneContext*>(user);
    ++context.calls;
    if (std::abs(ray.direction.y) < 1.0e-6f) return false;
    const float distance = (context.height - ray.origin.y) / ray.direction.y;
    if (distance < 0.0f || distance > ray.maxDistance) return false;
    hit.point = ray.origin + ray.direction * distance;
    hit.normal = {0.0f, 1.0f, 0.0f};
    hit.distance = distance;
    return true;
}

void CountEvent(const Survival3D::Animation::EventOccurrence&,
                void* user) noexcept {
    ++*static_cast<std::uint64_t*>(user);
}

void Mix(std::uint64_t& hash, std::uint64_t value) noexcept {
    hash ^= value;
    hash *= 1099511628211ull;
}

std::uint64_t Quantize(float value) noexcept {
    return static_cast<std::uint64_t>(
        static_cast<std::int64_t>(std::llround(value * 10000.0f)));
}

struct StressActor {
    explicit StressActor(Survival3D::Animation::ActorClass actorClass)
        : graph(actorClass), boss(actorClass == Survival3D::Animation::ActorClass::Boss) {
        graph.Reset(Survival3D::Animation::State::Locomotion);
        cursor.Start(0, true);
    }

    Survival3D::Animation::StateMachine graph;
    Survival3D::Animation::EventCursor cursor;
    Survival3D::RuntimeIK::FootIkState footIk;
    Survival3D::RuntimeIK::AimState aimIk;
    float progress = 0.0f;
    std::uint64_t requestSerial = 0;
    bool boss = false;
};

struct StressProjectile {
    float x = 0.0f;
    float z = 0.0f;
    float velocityX = 0.0f;
    float velocityZ = 0.0f;
    float lifetime = 0.0f;
};

struct StressResult {
    std::uint64_t checksum = 0;
    std::uint64_t dispatchedEvents = 0;
    std::uint64_t ikProbeCalls = 0;
    std::size_t hotPathAllocations = 0;
};

StressResult RunDeterministicStress(std::size_t actorCapacity,
                                    std::size_t projectileCapacity,
                                    int fixedTicks) {
    using namespace Survival3D::Animation;
    using namespace Survival3D::RuntimeIK;

    std::vector<StressActor> actors;
    actors.reserve(actorCapacity);
    for (std::size_t index = 0; index < actorCapacity; ++index) {
        actors.emplace_back(index < 5 ? ActorClass::Boss : ActorClass::Enemy);
    }

    std::vector<StressProjectile> projectiles(projectileCapacity);
    for (std::size_t index = 0; index < projectiles.size(); ++index) {
        StressProjectile& projectile = projectiles[index];
        projectile.x = static_cast<float>(index % 24) * 0.2f;
        projectile.z = static_cast<float>(index / 24) * 0.2f;
        projectile.velocityX = static_cast<float>(static_cast<int>(index % 7) - 3) * 0.03f;
        projectile.velocityZ = static_cast<float>(static_cast<int>(index % 5) - 2) * 0.04f;
        projectile.lifetime = 1.0f + static_cast<float>(index % 180) / 60.0f;
    }

    const EventTrack& runTrack = GetNonHeroTrack(NonHeroTrackId::Run);
    Require(runTrack.IsValid(), "stress event track is invalid");
    FootIkSettings footSettings;
    AimSettings aimSettings;
    PlaneContext plane;
    std::uint64_t events = 0;
    std::uint64_t checksum = 1469598103934665603ull;
    const std::size_t allocationsBefore = g_allocationCount;

    for (int tick = 0; tick < fixedTicks; ++tick) {
        const std::uint64_t absoluteFrame = static_cast<std::uint64_t>(tick);
        for (std::size_t index = 0; index < actors.size(); ++index) {
            StressActor& actor = actors[index];
            actor.cursor.Advance(runTrack, absoluteFrame, CountEvent, &events);

            const float right = static_cast<float>(static_cast<int>((tick + index) % 9) - 4);
            const float forward = static_cast<float>(static_cast<int>((tick * 3 + index) % 11) - 5);
            const LocomotionWeights weights = SolveLocomotionBlend(
                {right, forward}, 5.0f, 0.05f);

            State state = actor.graph.Current();
            const bool base = StateMachine::IsBaseState(state);
            if (base && (tick + static_cast<int>(index) * 13) % 181 == 0) {
                actor.graph.Submit(
                    {State::BasicAttack, TransitionReason::Gameplay,
                     ++actor.requestSerial}, 0.0f);
                actor.progress = 0.0f;
            }
            state = actor.graph.Current();
            if ((tick + static_cast<int>(index) * 17) % 997 == 0) {
                actor.graph.Submit(
                    {State::Hurt, TransitionReason::HitReaction,
                     ++actor.requestSerial}, actor.progress);
                actor.progress = 0.0f;
            }
            if (actor.boss
                && (tick + static_cast<int>(index) * 29) % 2003 == 0) {
                actor.graph.Submit(
                    {State::PhaseTransition, TransitionReason::PhaseChange,
                     ++actor.requestSerial}, actor.progress);
                actor.progress = 0.0f;
            }

            state = actor.graph.Current();
            if (!StateMachine::IsBaseState(state)) {
                const float duration = state == State::Hurt ? 0.20f
                    : (state == State::PhaseTransition ? 1.20f : 0.75f);
                actor.progress += (1.0f / 60.0f) / duration;
                if (actor.progress >= 1.0f) {
                    actor.graph.CompleteCurrent(true);
                    actor.progress = 0.0f;
                }
            }

            FootIkInput feet;
            feet.pelvisPosition = {0.0f, 1.0f, 0.0f};
            feet.leftAnimatedFoot = {-0.2f, 0.08f, 0.0f};
            feet.rightAnimatedFoot = {0.2f, 0.08f, 0.0f};
            feet.deltaSeconds = 1.0f / 60.0f;
            feet.snap = tick == 0;
            plane.height = static_cast<float>(static_cast<int>(index % 5) - 2) * 0.015f;
            const FootIkOutput footOutput = SolveFeetAndPelvis(
                footSettings, feet, ProbePlane, &plane, actor.footIk);

            AimInput aim;
            aim.origin = {0.0f, 1.4f, 0.0f};
            aim.characterForward = {0.0f, 0.0f, 1.0f};
            aim.desiredWorldPoint = {right, 1.2f, 5.0f + forward * 0.1f};
            aim.deltaSeconds = 1.0f / 60.0f;
            aim.snap = tick == 0;
            const AimOutput aimOutput = SolveAimTarget(
                aimSettings, aim, actor.aimIk);

            const float previousMotion = static_cast<float>(tick % 60) / 60.0f;
            const float currentMotion = static_cast<float>((tick % 60) + 1) / 60.0f;
            const RootMotionProfile motion{
                index % 2 == 0 ? RootMotionMode::Dash : RootMotionMode::Rush,
                index % 2 == 0 ? 4.0f : 7.0f,
                0.0f, 0.2f, 0.8f, 0.5f, true
            };
            const RootMotionDelta root = SampleRootMotionDelta(
                motion, previousMotion, currentMotion, 0.0f, 1.0f);

            if (tick % 60 == 0) {
                Mix(checksum, static_cast<std::uint8_t>(actor.graph.Current()));
                Mix(checksum, Quantize(weights.Sum()));
                Mix(checksum, Quantize(footOutput.pelvisOffset));
                Mix(checksum, Quantize(aimOutput.direction.x));
                Mix(checksum, Quantize(root.localForward));
            }
        }

        for (std::size_t index = 0; index < projectiles.size(); ++index) {
            StressProjectile& projectile = projectiles[index];
            projectile.x += projectile.velocityX;
            projectile.z += projectile.velocityZ;
            projectile.lifetime -= 1.0f / 60.0f;
            if (projectile.lifetime <= 0.0f) {
                projectile.x = 0.0f;
                projectile.z = 0.0f;
                projectile.lifetime = 2.0f
                    + static_cast<float>((index + static_cast<std::size_t>(tick)) % 120)
                        / 60.0f;
            }
            if (tick % 60 == 0) {
                Mix(checksum, Quantize(projectile.x));
                Mix(checksum, Quantize(projectile.z));
            }
        }
    }

    const std::size_t hotAllocations = g_allocationCount - allocationsBefore;
    Require(hotAllocations == 0,
            "graph/events/IK/capacity stress allocated in its fixed-tick hot path");
    Require(plane.calls == static_cast<std::uint64_t>(actorCapacity)
                             * static_cast<std::uint64_t>(fixedTicks) * 2ull,
            "foot IK did not perform exactly two probes per actor/tick");
    Require(events > 0, "stress run did not dispatch animation events");
    return {checksum, events, plane.calls, hotAllocations};
}

} // namespace

void* operator new(std::size_t size) {
    ++g_allocationCount;
    if (void* memory = std::malloc(size)) return memory;
    throw std::bad_alloc();
}

void operator delete(void* memory) noexcept { std::free(memory); }
void operator delete(void* memory, std::size_t) noexcept { std::free(memory); }

void* operator new[](std::size_t size) {
    ++g_allocationCount;
    if (void* memory = std::malloc(size)) return memory;
    throw std::bad_alloc();
}

void operator delete[](void* memory) noexcept { std::free(memory); }
void operator delete[](void* memory, std::size_t) noexcept { std::free(memory); }

int main(int argc, char** argv) {
    try {
        const std::filesystem::path repository = argc > 1
            ? std::filesystem::absolute(argv[1])
            : std::filesystem::current_path().parent_path();
        const RepositoryAudit audit = AuditRepository(repository);
        constexpr int kFixedTicks = 12000; // 200 seconds at 60 Hz.
        const StressResult first = RunDeterministicStress(
            audit.enemyPoolCapacity, audit.projectilePoolCapacity, kFixedTicks);
        const StressResult second = RunDeterministicStress(
            audit.enemyPoolCapacity, audit.projectilePoolCapacity, kFixedTicks);
        Require(first.checksum == second.checksum,
                "identical fixed-tick stress runs produced different checksums");
        Require(first.dispatchedEvents == second.dispatchedEvents,
                "identical stress runs dispatched different event counts");
        Require(first.ikProbeCalls == second.ikProbeCalls,
                "identical stress runs performed different IK work");

        std::cout << "Step10ProductionTests: all checks passed\n"
                  << "enemyPoolCapacity=" << audit.enemyPoolCapacity << '\n'
                  << "directorActiveCap=" << audit.directorActiveCap << '\n'
                  << "projectilePoolCapacity=" << audit.projectilePoolCapacity << '\n'
                  << "fixedTicksPerRun=" << kFixedTicks << '\n'
                  << "stressRuns=2\n"
                  << "hotPathAllocationsPerRun=" << first.hotPathAllocations << '\n'
                  << "dispatchedEventsPerRun=" << first.dispatchedEvents << '\n'
                  << "ikProbeCallsPerRun=" << first.ikProbeCalls << '\n'
                  << "deterministicChecksum=" << first.checksum << '\n'
                  << "manifestGlbCount=" << audit.animatedModelCount << '\n'
                  << "manifestGlbBytes=" << audit.animatedModelBytes << '\n'
                  << "productionQaPngCount=" << audit.qaPngCount << '\n'
                  << "survivalRuntimePythonFiles="
                  << audit.survivalRuntimePythonFiles << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Step10ProductionTests: " << error.what() << '\n';
        return 1;
    }
}
