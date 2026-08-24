#include "Survival3D/Vfx/VfxRuntime.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace Survival3D::Vfx {
namespace {

constexpr float kEpsilon = 1.0e-5f;

bool IsFinite(float value) noexcept {
    return std::isfinite(value);
}

float Clamp01(float value) noexcept {
    return std::clamp(value, 0.0f, 1.0f);
}

std::uint16_t NextGeneration(std::uint16_t generation) noexcept {
    ++generation;
    return generation == 0u ? 1u : generation;
}

} // namespace

bool Package::IsValid() const noexcept {
    if (id == 0u || debugName == nullptr || debugName[0] == '\0'
        || !IsFinite(durationSeconds) || durationSeconds <= 0.0f
        || layers == nullptr || layerCount == 0u
        || layerCount > kMaxLayersPerPackage) {
        return false;
    }

    float previousStart = -1.0f;
    for (std::uint8_t index = 0u; index < layerCount; ++index) {
        const LayerDefinition& layer = layers[index];
        if (layer.component == Component::None
            || !IsFinite(layer.startSeconds) || layer.startSeconds < 0.0f
            || layer.startSeconds + kEpsilon < previousStart
            || layer.startSeconds > durationSeconds + kEpsilon
            || !IsFinite(layer.durationSeconds) || layer.durationSeconds < 0.0f
            || !IsFinite(layer.intensity) || layer.intensity < 0.0f
            || !IsFinite(layer.radius) || layer.radius < 0.0f) {
            return false;
        }
        if (layer.playback == Playback::Continuous
            && (layer.durationSeconds <= 0.0f
                || layer.startSeconds + layer.durationSeconds
                    > durationSeconds + kEpsilon)) {
            return false;
        }
        if (layer.fallbackComponent == Component::None
            && layer.fallbackCapabilities != 0u) {
            return false;
        }
        previousStart = layer.startSeconds;
    }
    return true;
}

bool RuntimeConfig::IsValid() const noexcept {
    return emitterCapacity > 0u && emitterCapacity <= kEmitterHardCap
        && frameSampleCapacity > 0u
        && frameSampleCapacity <= kFrameSampleHardCap
        && frameCueCapacity > 0u
        && frameCueCapacity <= kFrameCueHardCap;
}

void FrameOutput::Clear() noexcept {
    sampleCount = 0u;
    cueCount = 0u;
    droppedSampleCount = 0u;
    droppedCueCount = 0u;
    unsupportedLayerCount = 0u;
}

Runtime::Runtime(RuntimeConfig config) noexcept {
    if (!Reset(config)) Reset(RuntimeConfig{});
}

bool Runtime::Reset(RuntimeConfig config) noexcept {
    if (!config.IsValid()) return false;

    m_config = config;
    m_packageCount = 0u;
    m_packages.fill(nullptr);
    m_freeCount = config.emitterCapacity;
    m_activeCount = 0u;
    m_deniedSpawnCount = 0u;
    m_output.Clear();

    for (std::uint16_t slot = 0u; slot < kEmitterHardCap; ++slot) {
        const std::uint16_t generation = NextGeneration(m_instances[slot].generation);
        m_instances[slot] = {};
        m_instances[slot].generation = generation;
    }
    for (std::uint16_t index = 0u; index < config.emitterCapacity; ++index)
        m_freeSlots[index] = static_cast<std::uint16_t>(config.emitterCapacity - 1u - index);
    return true;
}

bool Runtime::RegisterPackage(const Package& package) noexcept {
    if (!package.IsValid() || m_packageCount >= kPackageHardCap) return false;
    if (FindPackage(package.id) != nullptr) return false;
    m_packages[m_packageCount++] = &package;
    return true;
}

const Package* Runtime::FindPackage(PackageId id) const noexcept {
    if (id == 0u) return nullptr;
    for (std::uint8_t index = 0u; index < m_packageCount; ++index) {
        if (m_packages[index] != nullptr && m_packages[index]->id == id)
            return m_packages[index];
    }
    return nullptr;
}

void Runtime::SetCapabilities(CapabilityMask capabilities) noexcept {
    m_capabilities = capabilities;
}

Handle Runtime::Spawn(PackageId packageId,
                      const SpawnParameters& parameters) noexcept {
    const Package* package = FindPackage(packageId);
    if (package == nullptr || m_freeCount == 0u
        || !IsFinite(parameters.position) || !IsFinite(parameters.forward)
        || !IsFinite(parameters.surfaceNormal)
        || !std::isfinite(parameters.scale) || parameters.scale <= 0.0f
        || !std::isfinite(parameters.intensityScale)
        || parameters.intensityScale < 0.0f) {
        ++m_deniedSpawnCount;
        return {};
    }

    const std::uint16_t slot = m_freeSlots[--m_freeCount];
    Instance& instance = m_instances[slot];
    instance.package = package;
    instance.transform = parameters;
    instance.transform.forward = NormalizeOr(parameters.forward,
                                             {0.0f, 0.0f, 1.0f});
    instance.transform.surfaceNormal = NormalizeOr(parameters.surfaceNormal,
                                                   {0.0f, 1.0f, 0.0f});
    instance.elapsedSeconds = 0.0f;
    instance.startedLayerMask = 0u;
    instance.active = true;
    ++m_activeCount;
    return MakeHandle(slot);
}

bool Runtime::Stop(Handle handle) noexcept {
    if (!IsAlive(handle)) return false;
    Release(handle.slot);
    return true;
}

bool Runtime::IsAlive(Handle handle) const noexcept {
    return handle.IsValid() && handle.slot < m_config.emitterCapacity
        && m_instances[handle.slot].active
        && m_instances[handle.slot].generation == handle.generation;
}

void Runtime::StopAll() noexcept {
    for (std::uint16_t slot = 0u; slot < m_config.emitterCapacity; ++slot) {
        if (m_instances[slot].active) Release(slot);
    }
    m_output.Clear();
}

const FrameOutput& Runtime::Update(float deltaSeconds) noexcept {
    m_output.Clear();
    if (!std::isfinite(deltaSeconds) || deltaSeconds <= 0.0f) return m_output;

    for (std::uint16_t slot = 0u; slot < m_config.emitterCapacity; ++slot) {
        Instance& instance = m_instances[slot];
        if (!instance.active || instance.package == nullptr) continue;

        const Package& package = *instance.package;
        const float nextSeconds = std::min(package.durationSeconds,
                                           instance.elapsedSeconds + deltaSeconds);
        for (std::uint8_t layerIndex = 0u;
             layerIndex < package.layerCount; ++layerIndex) {
            const LayerDefinition& layer = package.layers[layerIndex];
            const std::uint32_t bit = 1u << layerIndex;
            const bool began = (instance.startedLayerMask & bit) == 0u
                && nextSeconds + kEpsilon >= layer.startSeconds;
            if (began) instance.startedLayerMask |= bit;

            const ResolvedLayer resolved = ResolveLayer(layer, m_capabilities);
            if (!resolved.supported) {
                if (began) ++m_output.unsupportedLayerCount;
                continue;
            }

            if (layer.playback == Playback::TriggerOnce) {
                if (began) EmitTrigger(instance, slot, layer, resolved);
                continue;
            }

            const float layerEnd = layer.startSeconds + layer.durationSeconds;
            if (nextSeconds + kEpsilon >= layer.startSeconds
                && nextSeconds < layerEnd - kEpsilon) {
                EmitContinuous(instance, slot, layer, resolved,
                               std::max(0.0f, nextSeconds - layer.startSeconds),
                               began);
            }
        }

        instance.elapsedSeconds = nextSeconds;
        if (nextSeconds + kEpsilon >= package.durationSeconds) Release(slot);
    }
    return m_output;
}

Runtime::ResolvedLayer Runtime::ResolveLayer(
    const LayerDefinition& layer, CapabilityMask capabilities) noexcept {
    ResolvedLayer result{};
    result.component = layer.component;
    result.missingOptional = layer.optionalCapabilities & ~capabilities;
    result.supported = HasAllCapabilities(capabilities,
                                          layer.requiredCapabilities);
    result.degraded = result.missingOptional != 0u;
    if (result.supported) return result;

    if (layer.fallbackComponent != Component::None
        && HasAllCapabilities(capabilities, layer.fallbackCapabilities)) {
        result.component = layer.fallbackComponent;
        result.supported = true;
        result.degraded = true;
        return result;
    }
    result.component = Component::None;
    return result;
}

float Runtime::EvaluateEnvelope(Envelope envelope,
                                float normalizedTime) noexcept {
    const float t = Clamp01(normalizedTime);
    switch (envelope) {
        case Envelope::EaseIn: return t * t;
        case Envelope::EaseOut: {
            const float inverse = 1.0f - t;
            return 1.0f - inverse * inverse;
        }
        case Envelope::Bell: return std::sin(t * 3.14159265358979323846f);
        case Envelope::Constant: default: return 1.0f;
    }
}

bool Runtime::IsFinite(Vec3 value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y)
        && std::isfinite(value.z);
}

Vec3 Runtime::NormalizeOr(Vec3 value, Vec3 fallback) noexcept {
    const float lengthSquared = value.x * value.x + value.y * value.y
                              + value.z * value.z;
    if (!std::isfinite(lengthSquared)
        || lengthSquared <= std::numeric_limits<float>::epsilon()) {
        return fallback;
    }
    const float inverseLength = 1.0f / std::sqrt(lengthSquared);
    return {value.x * inverseLength, value.y * inverseLength,
            value.z * inverseLength};
}

Handle Runtime::MakeHandle(std::uint16_t slot) const noexcept {
    return {slot, m_instances[slot].generation};
}

void Runtime::Release(std::uint16_t slot) noexcept {
    Instance& instance = m_instances[slot];
    if (!instance.active) return;
    instance.active = false;
    instance.package = nullptr;
    instance.elapsedSeconds = 0.0f;
    instance.startedLayerMask = 0u;
    instance.generation = NextGeneration(instance.generation);
    m_freeSlots[m_freeCount++] = slot;
    if (m_activeCount > 0u) --m_activeCount;
}

void Runtime::EmitContinuous(const Instance& instance, std::uint16_t slot,
                             const LayerDefinition& layer,
                             const ResolvedLayer& resolved,
                             float localSeconds, bool began) noexcept {
    if (m_output.sampleCount >= m_config.frameSampleCapacity) {
        ++m_output.droppedSampleCount;
        return;
    }
    const float normalized = layer.durationSeconds > 0.0f
        ? Clamp01(localSeconds / layer.durationSeconds) : 1.0f;
    LayerSample& sample = m_output.samples[m_output.sampleCount++];
    sample.emitter = MakeHandle(slot);
    sample.package = instance.package->id;
    sample.asset = layer.asset;
    sample.requestedComponent = layer.component;
    sample.resolvedComponent = resolved.component;
    sample.position = instance.transform.position;
    sample.forward = instance.transform.forward;
    sample.surfaceNormal = instance.transform.surfaceNormal;
    sample.scale = instance.transform.scale;
    sample.radius = layer.radius * instance.transform.scale;
    sample.intensity = layer.intensity * instance.transform.intensityScale
                     * EvaluateEnvelope(layer.envelope, normalized);
    sample.localSeconds = localSeconds;
    sample.normalizedTime = normalized;
    sample.missingOptionalCapabilities = resolved.missingOptional;
    sample.beganThisFrame = began;
    sample.degraded = resolved.degraded;
}

void Runtime::EmitTrigger(const Instance& instance, std::uint16_t slot,
                          const LayerDefinition& layer,
                          const ResolvedLayer& resolved) noexcept {
    if (m_output.cueCount >= m_config.frameCueCapacity) {
        ++m_output.droppedCueCount;
        return;
    }
    TriggerCue& cue = m_output.cues[m_output.cueCount++];
    cue.emitter = MakeHandle(slot);
    cue.package = instance.package->id;
    cue.asset = layer.asset;
    cue.requestedComponent = layer.component;
    cue.resolvedComponent = resolved.component;
    cue.position = instance.transform.position;
    cue.forward = instance.transform.forward;
    cue.surfaceNormal = instance.transform.surfaceNormal;
    cue.scale = instance.transform.scale;
    cue.radius = layer.radius * instance.transform.scale;
    cue.intensity = layer.intensity * instance.transform.intensityScale;
    cue.missingOptionalCapabilities = resolved.missingOptional;
    cue.degraded = resolved.degraded;
}

} // namespace Survival3D::Vfx
