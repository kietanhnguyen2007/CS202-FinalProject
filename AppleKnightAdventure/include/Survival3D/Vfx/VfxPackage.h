#pragma once

#include <cstddef>
#include <cstdint>

namespace Survival3D::Vfx {

using AssetId = std::uint32_t;
using PackageId = std::uint32_t;

// FNV-1a keeps authored package/asset keys readable at call sites while the
// runtime stores only fixed-size integer identifiers.
constexpr std::uint32_t HashId(const char* text) noexcept {
    std::uint32_t hash = 2166136261u;
    if (text == nullptr) return 0u;
    while (*text != '\0') {
        hash ^= static_cast<std::uint8_t>(*text++);
        hash *= 16777619u;
    }
    return hash;
}

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

enum class Component : std::uint8_t {
    None = 0,
    MainShape,
    SecondaryParticles,
    Trail,
    Glow,
    Distortion,
    Impact,
    Light,
    SpatialSound,
    CameraShake
};

enum class Playback : std::uint8_t {
    Continuous,
    TriggerOnce
};

enum class Envelope : std::uint8_t {
    Constant,
    EaseIn,
    EaseOut,
    Bell
};

enum class Capability : std::uint32_t {
    None = 0,
    WorldGeometry = 1u << 0u,
    TransparentQuads = 1u << 1u,
    AdditiveBlend = 1u << 2u,
    ParticleSimulation3D = 1u << 3u,
    RibbonTrail = 1u << 4u,
    PostProcessDistortion = 1u << 5u,
    DynamicLights = 1u << 6u,
    AudioPlayback = 1u << 7u,
    SpatialAudio = 1u << 8u,
    CameraShake = 1u << 9u,
    Bloom = 1u << 10u
};

using CapabilityMask = std::uint32_t;

constexpr CapabilityMask ToMask(Capability capability) noexcept {
    return static_cast<CapabilityMask>(capability);
}

constexpr CapabilityMask operator|(Capability lhs, Capability rhs) noexcept {
    return ToMask(lhs) | ToMask(rhs);
}

constexpr CapabilityMask operator|(CapabilityMask lhs,
                                   Capability rhs) noexcept {
    return lhs | ToMask(rhs);
}

constexpr bool HasAllCapabilities(CapabilityMask available,
                                  CapabilityMask required) noexcept {
    return (available & required) == required;
}

struct LayerDefinition {
    Component component = Component::None;
    Playback playback = Playback::Continuous;
    Envelope envelope = Envelope::Constant;
    float startSeconds = 0.0f;
    float durationSeconds = 0.0f;
    float intensity = 1.0f;
    float radius = 0.0f;
    AssetId asset = 0u;
    CapabilityMask requiredCapabilities = 0u;
    CapabilityMask optionalCapabilities = 0u;

    // A layer may degrade to a cheaper visual when its required backend
    // feature is absent. For example distortion can fall back to additive
    // glow, and a dynamic light can fall back to an emissive halo.
    Component fallbackComponent = Component::None;
    CapabilityMask fallbackCapabilities = 0u;
};

struct Package {
    PackageId id = 0u;
    const char* debugName = nullptr;
    float durationSeconds = 0.0f;
    const LayerDefinition* layers = nullptr;
    std::uint8_t layerCount = 0u;

    bool IsValid() const noexcept;
};

constexpr std::size_t kMaxLayersPerPackage = 32u;

// Documents the presentation features exposed by SurvivalView and
// SoundManager. Particles use a fixed-capacity 3D pool and audio has distance
// attenuation/stereo pan. Post-processing and real point lights remain on the
// explicit emissive/glow fallback path.
constexpr CapabilityMask ExistingSurvival3DBackendCapabilities() noexcept {
    return Capability::WorldGeometry
         | Capability::TransparentQuads
         | Capability::AdditiveBlend
         | Capability::ParticleSimulation3D
         | Capability::RibbonTrail
         | Capability::AudioPlayback
         | Capability::SpatialAudio
         | Capability::CameraShake;
}

} // namespace Survival3D::Vfx
