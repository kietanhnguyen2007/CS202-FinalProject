#include "Survival3D/Vfx/VfxRuntime.h"
#include "Survival3D/Vfx/SkillVfxPackages.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <new>
#include <stdexcept>

namespace {

std::size_t g_allocationCount = 0u;

void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

bool Nearly(float lhs, float rhs, float epsilon = 1.0e-4f) {
    return std::abs(lhs - rhs) <= epsilon;
}

using namespace Survival3D::Vfx;

constexpr LayerDefinition kCompleteLayers[] = {
    {Component::MainShape, Playback::Continuous, Envelope::Bell,
     0.00f, 1.00f, 1.00f, 1.50f, HashId("shape.violet_edge"),
     ToMask(Capability::WorldGeometry), 0u,
     Component::None, 0u},
    {Component::Glow, Playback::Continuous, Envelope::EaseOut,
     0.00f, 1.00f, 0.85f, 1.70f, HashId("glow.violet_edge"),
     ToMask(Capability::AdditiveBlend), ToMask(Capability::Bloom),
     Component::None, 0u},
    {Component::SpatialSound, Playback::TriggerOnce, Envelope::Constant,
     0.05f, 0.00f, 0.90f, 18.0f, HashId("audio.knight_cast"),
     ToMask(Capability::AudioPlayback), ToMask(Capability::SpatialAudio),
     Component::None, 0u},
    {Component::Trail, Playback::Continuous, Envelope::EaseOut,
     0.10f, 0.65f, 0.95f, 1.20f, HashId("trail.violet_edge"),
     ToMask(Capability::RibbonTrail), 0u,
     Component::None, 0u},
    {Component::Distortion, Playback::Continuous, Envelope::Bell,
     0.20f, 0.40f, 0.65f, 1.80f, HashId("distortion.violet_edge"),
     ToMask(Capability::PostProcessDistortion), 0u,
     Component::Glow, ToMask(Capability::AdditiveBlend)},
    {Component::SecondaryParticles, Playback::TriggerOnce, Envelope::Constant,
     0.35f, 0.00f, 1.00f, 1.30f, HashId("particles.crystal_fragments"),
     ToMask(Capability::ParticleSimulation3D), 0u,
     Component::None, 0u},
    {Component::Impact, Playback::TriggerOnce, Envelope::Constant,
     0.55f, 0.00f, 1.00f, 1.60f, HashId("impact.violet_edge"),
     ToMask(Capability::WorldGeometry), 0u,
     Component::None, 0u},
    {Component::Light, Playback::Continuous, Envelope::EaseOut,
     0.55f, 0.25f, 1.00f, 4.20f, HashId("light.violet_edge"),
     ToMask(Capability::DynamicLights), 0u,
     Component::Glow, ToMask(Capability::AdditiveBlend)},
    {Component::CameraShake, Playback::TriggerOnce, Envelope::Constant,
     0.55f, 0.00f, 0.70f, 7.00f, HashId("shake.knight_heavy"),
     ToMask(Capability::CameraShake), 0u,
     Component::None, 0u}
};

constexpr Package kCompletePackage{
    HashId("knight.violet_edge"), "Knight Violet Edge", 1.0f,
    kCompleteLayers,
    static_cast<std::uint8_t>(sizeof(kCompleteLayers) / sizeof(kCompleteLayers[0]))
};

void TestPackageValidationAndBackendProfile() {
    Require(kCompletePackage.IsValid(), "valid complete VFX package rejected");
    Require(HashId("knight.violet_edge") != HashId("mage.arc_bolt"),
            "representative package hashes collided");

    const CapabilityMask current = ExistingSurvival3DBackendCapabilities();
    Require(HasAllCapabilities(current, ToMask(Capability::WorldGeometry)),
            "current backend profile lost world geometry");
    Require(HasAllCapabilities(current, ToMask(Capability::RibbonTrail)),
            "current backend profile lost authored ribbon support");
    Require(HasAllCapabilities(current, ToMask(Capability::ParticleSimulation3D)),
            "current backend profile lost fixed 3D particles");
    Require(HasAllCapabilities(current, ToMask(Capability::SpatialAudio)),
            "current backend profile lost spatial audio");
    Require(!HasAllCapabilities(current, ToMask(Capability::DynamicLights)),
            "current backend incorrectly advertises dynamic lights");
    Require(!HasAllCapabilities(current, ToMask(Capability::PostProcessDistortion)),
            "current backend incorrectly advertises distortion");
    Require(!HasAllCapabilities(current, ToMask(Capability::Bloom)),
            "current backend incorrectly advertises bloom");

    static constexpr LayerDefinition unsorted[] = {
        {Component::MainShape, Playback::Continuous, Envelope::Constant,
         0.50f, 0.10f, 1.0f, 1.0f, 1u,
         ToMask(Capability::WorldGeometry), 0u, Component::None, 0u},
        {Component::Glow, Playback::Continuous, Envelope::Constant,
         0.25f, 0.10f, 1.0f, 1.0f, 2u,
         ToMask(Capability::AdditiveBlend), 0u, Component::None, 0u}
    };
    constexpr Package invalid{55u, "Unsorted", 1.0f, unsorted, 2u};
    Require(!invalid.IsValid(), "unsorted package timeline accepted");
}

void TestProductionSkillPackages() {
    const auto& packages = SkillPackages();
    Require(packages.size() == kSkillPackageCount,
            "production skill package count is incomplete");
    Runtime runtime;
    for (std::size_t index = 0; index < packages.size(); ++index) {
        const Package& package = packages[index];
        Require(package.IsValid(), "invalid production skill VFX package");
        Require(runtime.RegisterPackage(package),
                "production skill package could not be registered");
        Require(package.layerCount >= 7u,
                "production skill package is missing a presentation layer");
        for (std::size_t other = index + 1u; other < packages.size(); ++other)
            Require(package.id != packages[other].id,
                    "production skill package id collision");
    }

    for (const Package& package : packages) {
        const Handle handle = runtime.Spawn(package.id, {});
        Require(handle.IsValid(), "production skill package spawn failed");
        const FrameOutput& frame = runtime.Update(1.0f / 60.0f);
        Require(frame.sampleCount > 0u,
                "production skill package produced no first-frame visuals");
        runtime.Stop(handle);
    }
}

void TestTimelineFallbackAndOneShotDispatch() {
    Runtime runtime;
    Require(runtime.RegisterPackage(kCompletePackage),
            "complete package registration failed");
    Require(!runtime.RegisterPackage(kCompletePackage),
            "duplicate package id registration succeeded");

    SpawnParameters spawn{};
    spawn.position = {2.0f, 0.5f, -3.0f};
    spawn.forward = {4.0f, 0.0f, 0.0f};
    spawn.scale = 1.5f;
    spawn.intensityScale = 0.8f;
    const Handle handle = runtime.Spawn(kCompletePackage.id, spawn);
    Require(runtime.IsAlive(handle), "spawned VFX handle is not alive");

    const FrameOutput& startup = runtime.Update(0.06f);
    Require(startup.sampleCount == 2u,
            "startup frame should contain shape and glow samples");
    Require(startup.cueCount == 1u,
            "spatial sound trigger did not fire once at authored time");
    Require(startup.cues[0].resolvedComponent == Component::SpatialSound,
            "audio fallback changed the requested sound component");
    Require(!startup.cues[0].degraded,
            "available spatial sound was incorrectly marked degraded");
    Require(Nearly(startup.samples[0].scale, 1.5f)
            && Nearly(startup.samples[0].radius, 2.25f),
            "spawn scale was not applied to layer sample");
    Require(Nearly(startup.samples[0].forward.x, 1.0f)
            && Nearly(startup.samples[0].forward.z, 0.0f),
            "spawn facing was not normalized");

    const FrameOutput& impact = runtime.Update(0.50f);
    Require(impact.sampleCount == 5u,
            "impact frame did not retain all supported continuous layers");
    Require(impact.cueCount == 3u,
            "particle, impact and camera-shake triggers were not dispatched");
    Require(impact.unsupportedLayerCount == 0u,
            "supported fixed-pool particle layer was reported unsupported");

    bool sawDistortionFallback = false;
    bool sawLightFallback = false;
    for (std::uint16_t index = 0u; index < impact.sampleCount; ++index) {
        const LayerSample& sample = impact.samples[index];
        if (sample.requestedComponent == Component::Distortion) {
            sawDistortionFallback = sample.resolvedComponent == Component::Glow
                                 && sample.degraded;
        }
        if (sample.requestedComponent == Component::Light) {
            sawLightFallback = sample.resolvedComponent == Component::Glow
                            && sample.degraded;
        }
    }
    Require(sawDistortionFallback,
            "distortion did not degrade to additive glow");
    Require(sawLightFallback,
            "dynamic light did not degrade to emissive glow");

    const FrameOutput& next = runtime.Update(0.01f);
    Require(next.cueCount == 0u,
            "trigger-once layers fired again on a later frame");
    Require(next.unsupportedLayerCount == 0u,
            "unsupported layer was repeatedly reported every frame");

    runtime.Update(1.0f);
    Require(!runtime.IsAlive(handle) && runtime.ActiveEmitterCount() == 0u,
            "finished package did not return its emitter slot");
}

void TestGenerationalPoolAndHardCap() {
    RuntimeConfig config{};
    config.emitterCapacity = 2u;
    Runtime runtime(config);
    Require(runtime.RegisterPackage(kCompletePackage), "package registration failed");

    const Handle first = runtime.Spawn(kCompletePackage.id, {});
    const Handle second = runtime.Spawn(kCompletePackage.id, {});
    const Handle denied = runtime.Spawn(kCompletePackage.id, {});
    Require(first.IsValid() && second.IsValid() && !denied.IsValid(),
            "fixed emitter capacity was not enforced");
    Require(runtime.DeniedSpawnCount() == 1u,
            "denied emitter spawn was not counted");

    Require(runtime.Stop(first), "live emitter could not be stopped");
    const Handle recycled = runtime.Spawn(kCompletePackage.id, {});
    Require(recycled.slot == first.slot && recycled.generation != first.generation,
            "recycled slot did not advance its generation");
    Require(!runtime.IsAlive(first) && runtime.IsAlive(recycled),
            "stale generation was accepted by the pool");
    runtime.StopAll();
    Require(runtime.ActiveEmitterCount() == 0u,
            "StopAll left active emitters behind");
}

void TestFixedFrameCapacityAndNoHotPathAllocation() {
    static constexpr LayerDefinition layers[] = {
        {Component::MainShape, Playback::Continuous, Envelope::Constant,
         0.0f, 1.0f, 1.0f, 1.0f, 1u,
         ToMask(Capability::WorldGeometry), 0u, Component::None, 0u},
        {Component::Glow, Playback::Continuous, Envelope::Constant,
         0.0f, 1.0f, 1.0f, 1.0f, 2u,
         ToMask(Capability::AdditiveBlend), 0u, Component::None, 0u},
        {Component::Trail, Playback::Continuous, Envelope::Constant,
         0.0f, 1.0f, 1.0f, 1.0f, 3u,
         ToMask(Capability::RibbonTrail), 0u, Component::None, 0u}
    };
    constexpr Package package{HashId("test.capacity"), "Capacity Test", 1.0f,
                              layers, 3u};
    RuntimeConfig config{};
    config.emitterCapacity = 4u;
    config.frameSampleCapacity = 2u;
    config.frameCueCapacity = 2u;
    Runtime runtime(config);
    Require(runtime.RegisterPackage(package), "capacity package registration failed");
    Require(runtime.Spawn(package.id, {}).IsValid(), "capacity package spawn failed");
    const FrameOutput& frame = runtime.Update(1.0f / 60.0f);
    Require(frame.sampleCount == 2u && frame.droppedSampleCount == 1u,
            "fixed frame sample overflow policy is wrong");

    runtime.StopAll();
    const std::size_t before = g_allocationCount;
    for (int tick = 0; tick < 20000; ++tick) {
        if ((tick % 45) == 0) (void)runtime.Spawn(package.id, {});
        (void)runtime.Update(1.0f / 60.0f);
    }
    Require(g_allocationCount == before,
            "VFX spawn/update hot path performed a heap allocation");
}

} // namespace

void* operator new(std::size_t size) {
    ++g_allocationCount;
    if (void* memory = std::malloc(size)) return memory;
    throw std::bad_alloc();
}

void* operator new[](std::size_t size) {
    ++g_allocationCount;
    if (void* memory = std::malloc(size)) return memory;
    throw std::bad_alloc();
}

void operator delete(void* memory) noexcept { std::free(memory); }
void operator delete[](void* memory) noexcept { std::free(memory); }
void operator delete(void* memory, std::size_t) noexcept { std::free(memory); }
void operator delete[](void* memory, std::size_t) noexcept { std::free(memory); }

int main() {
    try {
        TestPackageValidationAndBackendProfile();
        TestProductionSkillPackages();
        TestTimelineFallbackAndOneShotDispatch();
        TestGenerationalPoolAndHardCap();
        TestFixedFrameCapacityAndNoHotPathAllocation();
        std::cout << "[PASS] VFX package/runtime: nine-layer timeline, capability "
                     "fallbacks, generational pool, fixed output and zero hot-path "
                     "allocations\n";
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
