#pragma once

#include "Survival3D/Vfx/VfxPackage.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace Survival3D::Vfx {

constexpr std::size_t kPackageHardCap = 64u;
constexpr std::size_t kEmitterHardCap = 384u;
constexpr std::size_t kFrameSampleHardCap = 2048u;
constexpr std::size_t kFrameCueHardCap = 512u;

struct Handle {
    static constexpr std::uint16_t InvalidSlot = 0xffffu;

    std::uint16_t slot = InvalidSlot;
    std::uint16_t generation = 0u;

    constexpr bool IsValid() const noexcept {
        return slot != InvalidSlot && generation != 0u;
    }
};

constexpr bool operator==(Handle lhs, Handle rhs) noexcept {
    return lhs.slot == rhs.slot && lhs.generation == rhs.generation;
}

constexpr bool operator!=(Handle lhs, Handle rhs) noexcept {
    return !(lhs == rhs);
}

struct SpawnParameters {
    Vec3 position{};
    Vec3 forward{0.0f, 0.0f, 1.0f};
    Vec3 surfaceNormal{0.0f, 1.0f, 0.0f};
    float scale = 1.0f;
    float intensityScale = 1.0f;
};

struct RuntimeConfig {
    // Matches the TDD prewarm/hard-cap contract: normal content uses at most
    // 256 emitters and never grows beyond the fixed 384-slot backing store.
    std::uint16_t emitterCapacity = 256u;
    std::uint16_t frameSampleCapacity = 1536u;
    std::uint16_t frameCueCapacity = 384u;

    bool IsValid() const noexcept;
};

struct LayerSample {
    Handle emitter{};
    PackageId package = 0u;
    AssetId asset = 0u;
    Component requestedComponent = Component::None;
    Component resolvedComponent = Component::None;
    Vec3 position{};
    Vec3 forward{0.0f, 0.0f, 1.0f};
    Vec3 surfaceNormal{0.0f, 1.0f, 0.0f};
    float scale = 1.0f;
    float radius = 0.0f;
    float intensity = 0.0f;
    float localSeconds = 0.0f;
    float normalizedTime = 0.0f;
    CapabilityMask missingOptionalCapabilities = 0u;
    bool beganThisFrame = false;
    bool degraded = false;
};

struct TriggerCue {
    Handle emitter{};
    PackageId package = 0u;
    AssetId asset = 0u;
    Component requestedComponent = Component::None;
    Component resolvedComponent = Component::None;
    Vec3 position{};
    Vec3 forward{0.0f, 0.0f, 1.0f};
    Vec3 surfaceNormal{0.0f, 1.0f, 0.0f};
    float scale = 1.0f;
    float radius = 0.0f;
    float intensity = 0.0f;
    CapabilityMask missingOptionalCapabilities = 0u;
    bool degraded = false;
};

struct FrameOutput {
    std::array<LayerSample, kFrameSampleHardCap> samples{};
    std::array<TriggerCue, kFrameCueHardCap> cues{};
    std::uint16_t sampleCount = 0u;
    std::uint16_t cueCount = 0u;
    std::uint32_t droppedSampleCount = 0u;
    std::uint32_t droppedCueCount = 0u;
    std::uint32_t unsupportedLayerCount = 0u;

    void Clear() noexcept;
};

class Runtime {
public:
    explicit Runtime(RuntimeConfig config = {}) noexcept;

    bool Reset(RuntimeConfig config = {}) noexcept;
    bool RegisterPackage(const Package& package) noexcept;
    const Package* FindPackage(PackageId id) const noexcept;

    void SetCapabilities(CapabilityMask capabilities) noexcept;
    CapabilityMask GetCapabilities() const noexcept { return m_capabilities; }

    Handle Spawn(PackageId package, const SpawnParameters& parameters) noexcept;
    bool Stop(Handle handle) noexcept;
    bool IsAlive(Handle handle) const noexcept;
    void StopAll() noexcept;

    // Produces only fixed-capacity samples/cues. It never allocates, uploads a
    // GPU resource, plays audio or mutates gameplay state.
    const FrameOutput& Update(float deltaSeconds) noexcept;
    const FrameOutput& GetFrameOutput() const noexcept { return m_output; }

    std::uint16_t ActiveEmitterCount() const noexcept { return m_activeCount; }
    std::uint32_t DeniedSpawnCount() const noexcept { return m_deniedSpawnCount; }
    const RuntimeConfig& GetConfig() const noexcept { return m_config; }

private:
    struct Instance {
        const Package* package = nullptr;
        SpawnParameters transform{};
        float elapsedSeconds = 0.0f;
        std::uint32_t startedLayerMask = 0u;
        std::uint16_t generation = 1u;
        bool active = false;
    };

    struct ResolvedLayer {
        Component component = Component::None;
        CapabilityMask missingOptional = 0u;
        bool supported = false;
        bool degraded = false;
    };

    static ResolvedLayer ResolveLayer(const LayerDefinition& layer,
                                      CapabilityMask capabilities) noexcept;
    static float EvaluateEnvelope(Envelope envelope,
                                  float normalizedTime) noexcept;
    static bool IsFinite(Vec3 value) noexcept;
    static Vec3 NormalizeOr(Vec3 value, Vec3 fallback) noexcept;

    Handle MakeHandle(std::uint16_t slot) const noexcept;
    void Release(std::uint16_t slot) noexcept;
    void EmitContinuous(const Instance& instance, std::uint16_t slot,
                        const LayerDefinition& layer,
                        const ResolvedLayer& resolved,
                        float localSeconds, bool began) noexcept;
    void EmitTrigger(const Instance& instance, std::uint16_t slot,
                     const LayerDefinition& layer,
                     const ResolvedLayer& resolved) noexcept;

    RuntimeConfig m_config{};
    CapabilityMask m_capabilities = ExistingSurvival3DBackendCapabilities();
    std::array<const Package*, kPackageHardCap> m_packages{};
    std::uint8_t m_packageCount = 0u;
    std::array<Instance, kEmitterHardCap> m_instances{};
    std::array<std::uint16_t, kEmitterHardCap> m_freeSlots{};
    std::uint16_t m_freeCount = 0u;
    std::uint16_t m_activeCount = 0u;
    std::uint32_t m_deniedSpawnCount = 0u;
    FrameOutput m_output{};
};

} // namespace Survival3D::Vfx
