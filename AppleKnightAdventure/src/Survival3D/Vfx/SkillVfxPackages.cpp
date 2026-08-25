#include "Survival3D/Vfx/SkillVfxPackages.h"

namespace Survival3D::Vfx {
namespace {

constexpr CapabilityMask kWorld = ToMask(Capability::WorldGeometry);
constexpr CapabilityMask kAdditive = ToMask(Capability::AdditiveBlend);
constexpr CapabilityMask kRibbon = ToMask(Capability::RibbonTrail);
constexpr CapabilityMask kParticles = ToMask(Capability::ParticleSimulation3D);
constexpr CapabilityMask kDistortion = ToMask(Capability::PostProcessDistortion);
constexpr CapabilityMask kLights = ToMask(Capability::DynamicLights);
constexpr CapabilityMask kAudio = ToMask(Capability::AudioPlayback);
constexpr CapabilityMask kSpatialAudio = ToMask(Capability::SpatialAudio);
constexpr CapabilityMask kShake = ToMask(Capability::CameraShake);
constexpr CapabilityMask kBloom = ToMask(Capability::Bloom);

constexpr LayerDefinition Continuous(Component component, float start,
                                     float duration, float intensity,
                                     float radius, const char* asset,
                                     CapabilityMask required,
                                     CapabilityMask optional = 0u,
                                     Envelope envelope = Envelope::Bell,
                                     Component fallback = Component::None,
                                     CapabilityMask fallbackCapabilities = 0u) {
    return {component, Playback::Continuous, envelope, start, duration,
            intensity, radius, HashId(asset), required, optional,
            fallback, fallbackCapabilities};
}

constexpr LayerDefinition Trigger(Component component, float start,
                                  float intensity, float radius,
                                  const char* asset,
                                  CapabilityMask required,
                                  CapabilityMask optional = 0u,
                                  Component fallback = Component::None,
                                  CapabilityMask fallbackCapabilities = 0u) {
    return {component, Playback::TriggerOnce, Envelope::Constant, start, 0.0f,
            intensity, radius, HashId(asset), required, optional,
            fallback, fallbackCapabilities};
}

// Packages begin at the authoritative gameplay/presentation contact event.
// Anticipation remains authored in the Blender action; projectile travel and
// all damage/collision remain owned by SurvivalController.
constexpr LayerDefinition kVioletEdge[] = {
    Continuous(Component::MainShape, 0.00f, 0.24f, 1.00f, 1.70f,
               "violet_edge.arc", kWorld),
    Continuous(Component::Trail, 0.00f, 0.22f, 0.95f, 1.55f,
               "violet_edge.ribbon", kRibbon, kBloom),
    Continuous(Component::Glow, 0.00f, 0.30f, 0.72f, 1.05f,
               "violet_edge.glow", kAdditive, kBloom),
    Continuous(Component::SecondaryParticles, 0.00f, 0.28f, 0.90f, 1.20f,
               "violet_edge.fragments", kWorld, kParticles,
               Envelope::EaseOut),
    Trigger(Component::Impact, 0.00f, 1.00f, 1.00f,
            "violet_edge.contact", kWorld),
    Continuous(Component::Distortion, 0.00f, 0.10f, 0.55f, 0.90f,
               "violet_edge.distortion", kDistortion, 0u,
               Envelope::EaseOut, Component::Glow, kAdditive),
    Continuous(Component::Light, 0.00f, 0.12f, 0.75f, 2.20f,
               "violet_edge.light", kLights, 0u, Envelope::EaseOut,
               Component::Glow, kAdditive),
    Trigger(Component::SpatialSound, 0.00f, 1.00f, 6.0f,
            "sfx.skill.knight.violet_edge", kAudio, kSpatialAudio),
    Trigger(Component::CameraShake, 0.00f, 0.52f, 0.11f,
            "camera.violet_edge", kShake),
};

constexpr LayerDefinition kAegisCounter[] = {
    Continuous(Component::MainShape, 0.00f, 0.48f, 1.00f, 1.30f,
               "aegis_counter.shield", kWorld),
    Continuous(Component::Glow, 0.00f, 0.48f, 0.80f, 1.35f,
               "aegis_counter.runes", kAdditive, kBloom,
               Envelope::EaseOut),
    Continuous(Component::SecondaryParticles, 0.00f, 0.42f, 0.62f, 1.15f,
               "aegis_counter.orbit", kWorld, kParticles),
    Trigger(Component::Impact, 0.00f, 0.72f, 1.10f,
            "aegis_counter.lock", kWorld),
    Continuous(Component::Distortion, 0.00f, 0.16f, 0.42f, 1.25f,
               "aegis_counter.refraction", kDistortion, 0u,
               Envelope::EaseOut, Component::Glow, kAdditive),
    Continuous(Component::Light, 0.00f, 0.28f, 0.66f, 2.40f,
               "aegis_counter.light", kLights, 0u, Envelope::EaseOut,
               Component::Glow, kAdditive),
    Trigger(Component::SpatialSound, 0.00f, 0.90f, 6.0f,
            "sfx.skill.knight.aegis_counter", kAudio, kSpatialAudio),
    Trigger(Component::CameraShake, 0.00f, 0.34f, 0.08f,
            "camera.aegis_counter", kShake),
};

constexpr LayerDefinition kShieldRush[] = {
    Continuous(Component::MainShape, 0.00f, 0.44f, 1.00f, 1.45f,
               "shield_rush.ram", kWorld),
    Continuous(Component::Trail, 0.00f, 0.40f, 0.90f, 2.40f,
               "shield_rush.wake", kRibbon, kBloom),
    Continuous(Component::Glow, 0.00f, 0.44f, 0.72f, 1.25f,
               "shield_rush.glow", kAdditive, kBloom),
    Continuous(Component::SecondaryParticles, 0.00f, 0.35f, 0.95f, 1.60f,
               "shield_rush.debris", kWorld, kParticles,
               Envelope::EaseOut),
    Trigger(Component::Impact, 0.00f, 1.00f, 1.55f,
            "shield_rush.impact", kWorld),
    Continuous(Component::Distortion, 0.00f, 0.18f, 0.65f, 1.35f,
               "shield_rush.distortion", kDistortion, 0u,
               Envelope::EaseOut, Component::Glow, kAdditive),
    Continuous(Component::Light, 0.00f, 0.16f, 0.75f, 2.50f,
               "shield_rush.light", kLights, 0u, Envelope::EaseOut,
               Component::Glow, kAdditive),
    Trigger(Component::SpatialSound, 0.00f, 1.00f, 8.0f,
            "sfx.skill.knight.shield_rush", kAudio, kSpatialAudio),
    Trigger(Component::CameraShake, 0.00f, 0.78f, 0.18f,
            "camera.shield_rush", kShake),
};

constexpr LayerDefinition kBastionBreaker[] = {
    Continuous(Component::MainShape, 0.00f, 0.76f, 1.00f, 5.50f,
               "bastion_breaker.matrix", kWorld),
    Continuous(Component::Glow, 0.00f, 0.76f, 0.88f, 5.20f,
               "bastion_breaker.runes", kAdditive, kBloom),
    Continuous(Component::SecondaryParticles, 0.00f, 0.65f, 1.00f, 4.20f,
               "bastion_breaker.crystals", kWorld, kParticles,
               Envelope::EaseOut),
    Trigger(Component::Impact, 0.00f, 1.00f, 5.50f,
            "bastion_breaker.shockwave", kWorld),
    Continuous(Component::Distortion, 0.00f, 0.34f, 0.92f, 4.60f,
               "bastion_breaker.distortion", kDistortion, 0u,
               Envelope::EaseOut, Component::Glow, kAdditive),
    Continuous(Component::Light, 0.00f, 0.36f, 1.00f, 7.50f,
               "bastion_breaker.light", kLights, 0u, Envelope::EaseOut,
               Component::Glow, kAdditive),
    Trigger(Component::SpatialSound, 0.00f, 1.00f, 12.0f,
            "sfx.skill.knight.bastion_breaker", kAudio, kSpatialAudio),
    Trigger(Component::CameraShake, 0.00f, 1.00f, 0.34f,
            "camera.bastion_breaker", kShake),
};

constexpr LayerDefinition kSteelStep[] = {
    Continuous(Component::MainShape, 0.00f, 0.20f, 0.78f, 1.00f,
               "steel_step.afterimage", kWorld),
    Continuous(Component::Trail, 0.00f, 0.22f, 0.88f, 2.20f,
               "steel_step.ribbon", kRibbon, kBloom),
    Continuous(Component::Glow, 0.00f, 0.22f, 0.58f, 0.90f,
               "steel_step.glow", kAdditive, kBloom),
    Continuous(Component::SecondaryParticles, 0.00f, 0.20f, 0.62f, 0.82f,
               "steel_step.dust", kWorld, kParticles,
               Envelope::EaseOut),
    Continuous(Component::Distortion, 0.00f, 0.10f, 0.35f, 0.75f,
               "steel_step.distortion", kDistortion, 0u,
               Envelope::EaseOut, Component::Glow, kAdditive),
    Trigger(Component::SpatialSound, 0.00f, 0.75f, 5.0f,
            "sfx.skill.knight.steel_step", kAudio, kSpatialAudio),
    Trigger(Component::CameraShake, 0.00f, 0.25f, 0.06f,
            "camera.steel_step", kShake),
};

constexpr LayerDefinition kArcBolt[] = {
    Continuous(Component::MainShape, 0.00f, 0.30f, 1.00f, 0.62f,
               "arc_bolt.crystal_lance", kWorld),
    Continuous(Component::Trail, 0.00f, 0.30f, 0.88f, 1.20f,
               "arc_bolt.trail", kRibbon, kBloom),
    Continuous(Component::Glow, 0.00f, 0.30f, 0.78f, 0.55f,
               "arc_bolt.glow", kAdditive, kBloom),
    Continuous(Component::SecondaryParticles, 0.00f, 0.24f, 0.55f, 0.55f,
               "arc_bolt.motes", kWorld, kParticles,
               Envelope::EaseOut),
    Continuous(Component::Light, 0.00f, 0.26f, 0.56f, 1.40f,
               "arc_bolt.light", kLights, 0u, Envelope::Bell,
               Component::Glow, kAdditive),
    Trigger(Component::SpatialSound, 0.00f, 0.84f, 6.0f,
            "sfx.skill.mage.arc_bolt", kAudio, kSpatialAudio),
    Trigger(Component::CameraShake, 0.00f, 0.20f, 0.045f,
            "camera.arc_bolt", kShake),
};

constexpr LayerDefinition kFrostRing[] = {
    Continuous(Component::MainShape, 0.00f, 0.72f, 1.00f, 4.00f,
               "frost_ring.ring", kWorld),
    Continuous(Component::Glow, 0.00f, 0.72f, 0.82f, 3.80f,
               "frost_ring.runes", kAdditive, kBloom),
    Continuous(Component::SecondaryParticles, 0.00f, 0.60f, 0.94f, 3.60f,
               "frost_ring.shards", kWorld, kParticles,
               Envelope::EaseOut),
    Trigger(Component::Impact, 0.00f, 0.86f, 4.00f,
            "frost_ring.wave", kWorld),
    Continuous(Component::Distortion, 0.00f, 0.18f, 0.42f, 3.20f,
               "frost_ring.refraction", kDistortion, 0u,
               Envelope::EaseOut, Component::Glow, kAdditive),
    Continuous(Component::Light, 0.00f, 0.30f, 0.70f, 4.60f,
               "frost_ring.light", kLights, 0u, Envelope::EaseOut,
               Component::Glow, kAdditive),
    Trigger(Component::SpatialSound, 0.00f, 0.92f, 9.0f,
            "sfx.skill.mage.frost_ring", kAudio, kSpatialAudio),
    Trigger(Component::CameraShake, 0.00f, 0.48f, 0.09f,
            "camera.frost_ring", kShake),
};

constexpr LayerDefinition kGravityWell[] = {
    Continuous(Component::MainShape, 0.00f, 3.95f, 1.00f, 5.00f,
               "gravity_well.core", kWorld),
    Continuous(Component::Trail, 0.00f, 3.95f, 0.72f, 4.60f,
               "gravity_well.spiral", kRibbon, kBloom),
    Continuous(Component::Glow, 0.00f, 3.95f, 0.76f, 4.80f,
               "gravity_well.runes", kAdditive, kBloom),
    Continuous(Component::SecondaryParticles, 0.00f, 3.95f, 0.86f, 4.30f,
               "gravity_well.debris", kWorld, kParticles),
    Continuous(Component::Distortion, 0.00f, 3.95f, 0.78f, 4.00f,
               "gravity_well.distortion", kDistortion, 0u,
               Envelope::Bell, Component::Glow, kAdditive),
    Continuous(Component::Light, 0.00f, 3.95f, 0.68f, 5.40f,
               "gravity_well.light", kLights, 0u, Envelope::Bell,
               Component::Glow, kAdditive),
    Trigger(Component::SpatialSound, 0.00f, 0.95f, 11.0f,
            "sfx.skill.mage.gravity_well", kAudio, kSpatialAudio),
    Trigger(Component::CameraShake, 0.00f, 0.58f, 0.14f,
            "camera.gravity_well", kShake),
};

constexpr LayerDefinition kAstralTempest[] = {
    Continuous(Component::MainShape, 0.00f, 1.24f, 1.00f, 8.00f,
               "astral_tempest.focus", kWorld),
    Continuous(Component::Trail, 0.00f, 1.24f, 0.92f, 7.40f,
               "astral_tempest.constellations", kRibbon, kBloom),
    Continuous(Component::Glow, 0.00f, 1.24f, 0.90f, 7.80f,
               "astral_tempest.sigil", kAdditive, kBloom),
    Continuous(Component::SecondaryParticles, 0.00f, 1.24f, 1.00f, 7.20f,
               "astral_tempest.meteors", kWorld, kParticles),
    Trigger(Component::Impact, 0.00f, 1.00f, 8.00f,
            "astral_tempest.impact", kWorld),
    Continuous(Component::Distortion, 0.00f, 0.46f, 0.94f, 7.00f,
               "astral_tempest.distortion", kDistortion, 0u,
               Envelope::EaseOut, Component::Glow, kAdditive),
    Continuous(Component::Light, 0.00f, 0.66f, 1.00f, 10.0f,
               "astral_tempest.light", kLights, 0u, Envelope::EaseOut,
               Component::Glow, kAdditive),
    Trigger(Component::SpatialSound, 0.00f, 1.00f, 14.0f,
            "sfx.skill.mage.astral_tempest", kAudio, kSpatialAudio),
    Trigger(Component::CameraShake, 0.00f, 1.00f, 0.30f,
            "camera.astral_tempest", kShake),
};

constexpr LayerDefinition kPhaseBlink[] = {
    Continuous(Component::MainShape, 0.00f, 0.24f, 0.82f, 1.10f,
               "phase_blink.afterimage", kWorld),
    Continuous(Component::Trail, 0.00f, 0.26f, 0.92f, 2.30f,
               "phase_blink.ribbon", kRibbon, kBloom),
    Continuous(Component::Glow, 0.00f, 0.26f, 0.72f, 1.00f,
               "phase_blink.glow", kAdditive, kBloom),
    Continuous(Component::SecondaryParticles, 0.00f, 0.24f, 0.76f, 0.95f,
               "phase_blink.motes", kWorld, kParticles,
               Envelope::EaseOut),
    Continuous(Component::Distortion, 0.00f, 0.14f, 0.62f, 0.90f,
               "phase_blink.distortion", kDistortion, 0u,
               Envelope::EaseOut, Component::Glow, kAdditive),
    Continuous(Component::Light, 0.00f, 0.16f, 0.52f, 1.80f,
               "phase_blink.light", kLights, 0u, Envelope::EaseOut,
               Component::Glow, kAdditive),
    Trigger(Component::SpatialSound, 0.00f, 0.82f, 6.0f,
            "sfx.skill.mage.phase_blink", kAudio, kSpatialAudio),
    Trigger(Component::CameraShake, 0.00f, 0.24f, 0.055f,
            "camera.phase_blink", kShake),
};

template <std::size_t N>
constexpr Package MakePackage(const char* id, float duration,
                              const LayerDefinition (&layers)[N]) {
    return {HashId(id), id, duration, layers, static_cast<std::uint8_t>(N)};
}

constexpr std::array<Package, kSkillPackageCount> kPackages{{
    MakePackage("skill.knight.violet_edge", 0.32f, kVioletEdge),
    MakePackage("skill.knight.aegis_counter", 0.50f, kAegisCounter),
    MakePackage("skill.knight.shield_rush", 0.55f, kShieldRush),
    MakePackage("skill.knight.bastion_breaker", 0.80f, kBastionBreaker),
    MakePackage("skill.knight.steel_step", 0.28f, kSteelStep),
    MakePackage("skill.mage.arc_bolt", 0.34f, kArcBolt),
    MakePackage("skill.mage.frost_ring", 0.80f, kFrostRing),
    MakePackage("skill.mage.gravity_well", 4.00f, kGravityWell),
    MakePackage("skill.mage.astral_tempest", 1.30f, kAstralTempest),
    MakePackage("skill.mage.phase_blink", 0.35f, kPhaseBlink),
}};

} // namespace

const std::array<Package, kSkillPackageCount>& SkillPackages() noexcept {
    return kPackages;
}

const Package& GetSkillPackage(SkillPackage package) noexcept {
    const std::size_t index = static_cast<std::size_t>(package);
    return kPackages[index < kPackages.size() ? index : 0u];
}

} // namespace Survival3D::Vfx
