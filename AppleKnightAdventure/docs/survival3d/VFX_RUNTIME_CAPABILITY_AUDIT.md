# Aegis Rift — Survival3D VFX runtime capability audit

Audit date: 2026-08-22  
Scope: C++ Survival3D client currently on the `codex/survival3d` branch. This
audit does not treat an ImageGen concept, a static PNG or a GLB prop by itself
as a completed skill. A production skill still needs its authored timeline:
main shape, secondary particles, trail, glow, distortion, impact, light, sound
and camera shake.

## Executive result

The current renderer now provides the complete assignment-grade presentation
stack, with explicit low-cost fallbacks for features that would require a
post-processing renderer.

- Ready now: world-space GLB geometry, raylib primitives, transparent textured
  quads, additive ribbon, a fixed 512-slot 3D particle pool, distance/pan
  spatial audio, contact-driven hit stop and camera translation shake.
- Intentional fallback: scene-color distortion, HDR bloom and real dynamic
  lights resolve to layered emissive glow, keeping the minigame lightweight.

`Survival3D::Vfx::Runtime` owns the fixed timeline/capability contract, while
`SurvivalView` consumes its immutable frame samples and the controller emits a
single presentation cue at each authored contact frame.

## Renderer and backend

| Area | Current implementation | Status for production VFX |
|---|---|---|
| Backend | raylib 6.0 over OpenGL (`raylib.h:89-92`) | Suitable for the planned stylized renderer |
| 3D scene | `BeginMode3D`, raylib primitives and `DrawModelEx`; GLB skill models load with an `Activate` animation (`SurvivalView.cpp:1508`) | Main shape supported |
| Transparent world cards | `DrawVfxQuad` emits an `RL_QUADS` surface in world space and disables depth writes (`SurvivalView.cpp:537`) | Supported; needs sorting/atlas batching |
| Additive blend | Explicit additive ribbon path at `SurvivalView.cpp:708` and `:738` | Supported |
| Skill geometry | Ten GLBs are loaded in `Init` and sampled in `RenderSkillGeometry` (`SurvivalView.cpp:1314`, `:1558`) | Supported structurally; art quality is a separate gate |
| Render order | Arena, actors, projectiles and skill geometry are rendered in one `BeginMode3D` pass (`SurvivalView.cpp:2402-2594`) | No scene-color input for refraction/distortion |

The renderer currently mixes resource ownership, animation sampling and VFX
drawing inside `SurvivalView`. A later adapter should consume VFX frame samples
while an `AssetRegistry3D` remains the sole owner of models, textures, shaders
and audio aliases. Pooled instances must store only integer asset IDs.

## Shader, distortion and bloom audit

Survival3D contains no `LoadShader`, `BeginShaderMode`, `RenderTexture2D` or
post-process pass. The only custom shader pipeline in the project is the menu
blur (`MenuView.cpp:592-647`), so it cannot be counted as an in-game VFX
capability.

| Feature | Current status | Required implementation |
|---|---|---|
| Glow | Multiple translucent cylinders/spheres plus additive quads | Keep as low-quality fallback |
| Emissive material | GLB material data may load, but Survival3D has no explicit emissive/HDR shader contract | Add a shared lit/emissive material shader |
| Distortion/refraction | Missing | Render the 3D scene to a color target; distortion meshes sample scene color with a noise/normal map |
| Bloom | Missing | Extract bright emissive pixels, separable downsampled blur, composite before HUD |

Distortion and bloom are represented as optional backend capabilities in the
new package runtime. Until their render passes exist, distortion degrades to an
additive glow layer and missing bloom marks a sample as degraded instead of
silently claiming full quality.

## Particles, pooling and memory

There is a generic 2D `ParticleSystem`, but it is not appropriate for the 3D
Survival minigame:

- particles use `Vector2` and the 2D `ParticleRenderer`;
- `ObjectPool::Acquire` grows the heap when empty
  (`include/Systems/ObjectPool.h:24`);
- `ObjectPool::Release` linearly scans all slots (`ObjectPool.h:36`);
- the active vector erases elements during update
  (`ParticleSystem.cpp:61`).

Survival3D enemy and projectile pools are better precedents: they resize once
at startup (`SurvivalController.cpp:422`, `:432`) and use free lists in
`AcquireEnemy`/`AcquireProjectile` (`:993`, `:1013`). There was no equivalent
VFX emitter or 3D particle pool before this audit.

The new runtime uses:

- a 384-emitter compile-time hard cap;
- a configurable 256-emitter normal capacity matching the TDD;
- O(1) acquire/release using a fixed free-slot stack;
- generational handles, so a recycled slot rejects stale references;
- fixed arrays for per-frame samples and one-shot cues;
- zero `new`, `delete`, vector growth or string construction in `Spawn`,
  `Stop` and `Update`.

`SurvivalView` now owns a separate fixed 512-slot `SkillParticle3D` pool.
Bursts are spawned only from package layer begin/impact cues, reuse slots in a
ring, update velocity/lifetime without allocation, and enforce a smaller
Reduced Motion render budget. The larger TDD target remains unnecessary for
this assignment-scale vertical slice.

## Trail support

`DrawBladeRibbon` (`SurvivalView.cpp:683`) is a good visual prototype:

- it samples the real weapon root and tip into a fixed 32-entry array;
- it interpolates at a nominal 120 Hz;
- it uses a 0.22 second lifetime;
- it freezes on the authored contact pose during hit stop;
- it renders a textured additive quad strip, with a geometry fallback.

Its limitation is ownership: the trail state is one Knight-specific member and
only runs for `BasicAttack`. Projectiles and other abilities use immediate
lines/quads rather than reusable emitters. The production adapter should lift
the same fixed-history algorithm into a pool keyed by emitter handle and
support weapon sockets, projectile positions and authored world points.

## Dynamic light audit

There is no dynamic point/directional-light manager or lighting shader in
Survival3D. Colored spheres and layered translucent geometry visually imitate
light but do not illuminate actors or the arena.

This is consistent with the original TDD recommendation to fake most skill
lighting. The practical production policy should be:

1. Every skill has emissive/glow fallback geometry.
2. Reserve at most four real point lights for player/boss hero moments.
3. Select lights by priority and camera distance; never create one GPU light
   per projectile.
4. A package `Light` layer automatically resolves to `Glow` when the dynamic
   light capability is unavailable.

## Camera shake and hit stop audit

`EmitCombatFeedback` (`SurvivalController.cpp:2448`) records an immutable cue,
origin, direction, radius and intensity, then updates hit-stop and camera-shake
timers at authored contact. `CombatCamera` (`SurvivalView.cpp:1258`) applies
high-frequency sinusoidal translation and respects Reduced Motion.

This is functional but not yet the TDD impulse system:

- multiple simultaneous impulses are collapsed to maximum timer/intensity;
- there is no distance falloff from the event origin;
- there is no rotational impulse;
- decay is timer based rather than an impulse envelope;
- the shake is coupled to controller getters instead of consuming presentation
  events.

The new runtime emits a one-shot `CameraShake` cue. A future camera adapter can
sum and clamp impulses without letting VFX code alter damage or simulation.

## Sound and spatialization audit

`SoundManager` supports manifest-defined sample variants, event volume, pitch
randomization and cooldown. `PlaySoundAt` now adds XZ distance attenuation and
stereo pan, and Survival3D skill events provide their authored world origin.

The package runtime therefore distinguishes:

- required `AudioPlayback`: current backend can play the cue;
- optional `SpatialAudio`: the backend reports this available and uses the
  spatial playback path; normal UI events remain centered 2D sounds.

Production spatial audio can remain in raylib by computing distance attenuation
and stereo pan against the combat camera, then using a small alias/voice pool so
overlapping impacts do not mutate one shared `Sound` voice.

## New package/runtime contract

Files:

- `include/Survival3D/Vfx/VfxPackage.h`
- `include/Survival3D/Vfx/VfxRuntime.h`
- `src/Survival3D/Vfx/VfxRuntime.cpp`
- `tests/VfxRuntimeTests.cpp`

Each authored package has sorted layers. A layer describes component,
continuous/one-shot playback, start time, duration, envelope, intensity,
radius, asset ID, required/optional capabilities and an optional fallback.
Together these components model the complete quality target:

```text
MainShape + SecondaryParticles + Trail + Glow + Distortion
          + Impact + Light + SpatialSound + CameraShake
```

`Runtime::Update` produces two immutable fixed-capacity views:

- `LayerSample`: active continuous geometry/emitter/light layers, with local
  time and normalized time;
- `TriggerCue`: one-shot particle burst, impact, sound or camera impulse.

The runtime never renders, plays audio or mutates gameplay. This keeps the
future integration event-driven and makes Reduced Motion, quality settings and
backend fallbacks presentation-only.

## Validation performed

`VfxRuntimeTests` is compiled as C++17 with the project's strict warnings
(`-Wall -Wextra -Wpedantic -Werror` on MinGW). It verifies:

- all nine presentation components in one sorted skill timeline;
- exact one-shot dispatch when a frame crosses an authored time;
- available particle/spatial-audio capabilities and missing bloom flags;
- distortion and dynamic-light fallbacks to additive glow;
- fixed-pool particle layers are accepted without degradation;
- fixed output overflow is counted rather than allocating;
- pool hard cap, O(1) recycling and stale-handle rejection;
- 20,000 hot-path ticks with zero heap allocations.

Command and result:

```text
cmake --build build --target VfxRuntimeTests -j 4
ctest --test-dir build -R VfxRuntimeTests --output-on-failure

1/1 VfxRuntimeTests passed
```

## Explicit remaining non-goals

The following are deliberately not claimed complete:

- no scene-color distortion or bloom render targets;
- no real point-light manager;
- no per-voice mixing API beyond raylib's shared `Sound` handles.

These boundaries allow art creation and renderer work to continue without
coupling cosmetic lifetime to gameplay state or creating merge conflicts in
the active controller/view refactor.
