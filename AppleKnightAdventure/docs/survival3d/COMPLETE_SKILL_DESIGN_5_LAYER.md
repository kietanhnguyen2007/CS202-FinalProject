# Complete Skill Design — Five-Layer Contract

Status: implementation contract  
Runtime: C++17 + raylib  
Authoring: Blender 4.5 LTS, ImageGen, optional TripoSR  
Animation sample rate: 60 FPS  
Coordinate convention: `+Z` up, `-Y` forward in Blender

## Definition of done

A skill is complete only when all five layers below are present and synchronized by animation frames:

1. Character animation: full-body poses, readable anticipation, fast strike/release, follow-through, recovery.
2. Weapon/projectile/main shape: a real runtime asset where the design needs solid geometry.
3. VFX: main shape, secondary particles, trail, glow, distortion, impact flash/shockwave and temporary light.
4. Gameplay timing: hitbox/projectile/collider and damage events remain independent from visual effects.
5. Feedback: layered sound, hit-stop and camera shake with reduced-motion scaling.

The animation frame is authoritative. Runtime code must not use a duplicated `wait(seconds)` value for release or damage. Seconds are derived from `frame / 60`.

## Application ownership

| Work | Owner |
|---|---|
| Character poses, actions and animation curves | Blender 4.5 |
| Sword/staff attachment and slash/ring mesh | Blender 4.5 |
| Solid shield, crystal, rock or pylon donor | TripoSR when useful; clean/finalize in Blender |
| Texture, rune, mask and energy-noise source | ImageGen + Blender material setup |
| Projectile movement and collision | C++ runtime |
| Particle, trail, shader, distortion and temporary light | C++/raylib runtime |
| Damage, hitbox and crowd-control logic | C++ runtime |
| Sound, hit-stop and camera shake | C++ runtime |

TripoSR is never used for lightning, slash ribbons, smoke, flame, glow, shockwaves or spatial distortion. These effects need animated meshes/shaders/particles, not a static generated model.

## Animation quality rules

- Animate `hips`, `spine`, `chest`, `head`, both shoulders/arms/forearms/hands and both legs for every combat action.
- The head keeps the target readable while the hips and chest lead the action.
- Wind-up and recovery use eased curves. Strike/release uses a short, steep acceleration section.
- The weapon socket follows the hand. It must not replace hand, forearm or chest animation.
- Locomotion is in-place. Rush, dash and blink displacement remains authoritative in C++.
- Every skill action has a neutral first pose and a blend-safe final pose.

Implementation audit (completed): both hero files still key all 16 runtime bones. The five combat actions now have non-zero `spine` rotation and substantially larger wrist/hand arcs. Knight combat spine rotation spans `0.20944–0.69813` radians and Mage combat spine spans `0.20944–0.66323` radians; authored hand motion is present in every combat action. `run_forward` also has stronger opposing arm swings while locomotion remains in-place.

---

# Knight

## 1. Violet Edge

Fantasy: a committed horizontal sword cut projects a faceted violet crescent and detonates crystal fragments on contact.

### Layer 1 — Character animation

Action: `basic_01`, frames `0–62`.

| Frame | Pose | Full-body direction |
|---:|---|---|
| 0 | Guard | Weight centered, sword low-right, shoulders relaxed, eyes on target. |
| 6 | Charge begins | Front knee bends, hips rotate opposite the cut, left shoulder points forward, motes gather at blade. |
| 18 | Wind-up | Sword and right hand travel behind the body; spine and chest twist; left arm counterbalances; trail begins. |
| 23 | Anticipation | Lowest stance and maximum torso coil; right wrist cocks the blade; head remains target-locked. |
| 25 | Strike | Hips initiate, then spine/chest, shoulder, elbow and wrist; sword hitbox and crescent become active. |
| 28 | Contact accent | Blade passes the center line; chest rotation peaks; impact/audio/camera event fires. |
| 35 | Follow-through | Sword finishes across the left side; rear heel releases; trail and hitbox stop. |
| 36–62 | Recovery | Hips settle first, then chest, arms and sword blend back to guard. |

Graph rule: frames `18–23` ease into the coil; `23–28` are the steep acceleration; `28–35` decelerate; recovery uses smooth Bezier handles.

### Layer 2 — Weapon/projectile

- Sword: existing Knight greatsword attached to `weapon_socket.R`.
- Main shape: `violet_edge_arc`, a thin crescent mesh with a bright cutting core and soft alpha edges.
- Build the crescent in Blender; use an ImageGen mask/noise texture. Do not use TripoSR.
- The visual is socket-bound during the cut; damage remains the `KnightSword` hitbox, not the VFX mesh.

### Layer 3 — VFX

- Frame 6: violet charge motes along the blade.
- Frame 18–35: 14-segment weapon ribbon.
- Frame 25: faceted crescent appears.
- Frame 28/contact: white-violet flash, 16 crystal fragments, seven smoke puffs, 1.0 m shockwave, short radial distortion and a temporary violet point light.
- Frame 36: crescent dissolves over 0.18 s.

### Layer 4 — Gameplay timing

- Active phase `24–35`; sword hitbox on `25`, off `35`.
- Gameplay event ID: `KnightSword`.
- A visible arc may exceed the collider slightly, but must never be used as the damage source.
- Existing hit-stop: `0.045 s`.

### Layer 5 — Feedback

- Frame 18: quiet charge whoosh.
- Frame 28: sword whoosh plus crystal impact crack.
- Camera: `0.11 m`, `28 Hz`, `0.10 s`, quadratic falloff.

## 2. Aegis Counter

Fantasy: the Knight plants into a defensive stance, raises a layered violet shield and retaliates only when a real parry succeeds.

### Layer 1 — Character animation

Action: `skill_one`, frames `0–78`.

| Frame | Pose | Full-body direction |
|---:|---|---|
| 0 | Guard | Feet shoulder-width, sword hand close, left side prepared to receive the shield. |
| 20 | Raise | Hips drop, spine leans slightly forward, both arms lift and brace the shield; shape spawns. |
| 32 | Lock | Front foot plants, elbows compress behind the shield, chest squares to the threat; rune orbit starts. |
| 46 | Parry window opens | Maximum structural tension from rear foot through hips, spine, shoulders and hands. |
| 50 | Active brace | Shield pulses; head stays above the rim and tracks the attacker. |
| external | Parry recoil | On `parry_success`, shoulders and wrists absorb the hit, hips counter-rotate, then rebound forward. |
| 64 | Window closes | Elbows release and shield lowers slightly. |
| 65–78 | Recovery | Return weight to center and dissolve the shield without snapping the hands. |

### Layer 2 — Weapon/projectile

- Solid object: layered shield at `guard_anchor`, attachment `shield_hand.L`.
- TripoSR may provide silhouette/thickness reference; final topology, back handle and materials are Blender-authored.
- Current clean candidate: `models/skills/knight_aegis_counter_v2.glb`; canonical replacement happens only after QA.

### Layer 3 — VFX

- Rune orbit, edge sparks and controlled purple emission while held.
- On parry: white-gold impact flash, 28 sparks, ten energy flakes, 2.1 m shockwave, shield refraction and temporary point light.
- The shield is a 3D object; ripple, glow and distortion remain engine effects.

### Layer 4 — Gameplay timing

- Guard hitbox on `46`, active phase `50–64`, off `64`.
- Impact is triggered by `parry_success`, never automatically by frame 50.
- Existing hit-stop: `0.075 s` on successful parry.

### Layer 5 — Feedback

- Frame 32: shield-raise layer; frame 50: low magical hum; parry success: metal/crystal crack.
- Camera on success: `0.16 m`, `31 Hz`, `0.14 s`.

## 3. Shield Rush

Fantasy: the Knight braces behind a violet armored ram, deploys exactly three crystal fins and drives through a line of enemies.

### Layer 1 — Character animation

Action: `skill_two`, frames `0–78`.

| Frame | Pose | Full-body direction |
|---:|---|---|
| 0 | Ready | Wide stance, shield side forward, sword hand pulled close. |
| 18 | Brace | Ram appears; both hands and shoulders lock behind it, hips sink, rear leg loads. |
| 30 | Anticipation | Chest nearly parallel to charge direction, rear heel high, arms compressed; wake begins. |
| 35 | Launch | Rear leg extends, hips drive forward, spine stays braced and head remains protected. |
| 36 | Damage begins | Character displacement is controlled by C++; collider turns on. |
| 40 | Contact pose | Front shoulder and hip align behind the ram; impact feedback fires. |
| 46 | Follow-through | Collider closes, torso rises and arms absorb deceleration. |
| 47–78 | Recovery | Three fins fold/dissolve; feet regain neutral placement. |

### Layer 2 — Weapon/projectile

- Solid object: armored forebody plus exactly three rear fin modules.
- TripoSR may supply the forebody texture/form donor; Blender owns clean topology, rails, attachment socket and three fin pivots.
- In-game translation is C++ root movement, not baked Blender root motion.

### Layer 3 — VFX

- Deploy glow on three fins, dual afterimage wake, ground dust, armor sparks, debris, forward pressure distortion, 2.3 m shockwave and temporary purple light.
- Trail starts at frame 30 and stops at 46.

### Layer 4 — Gameplay timing

- Active phase `35–46`; `KnightRush` hitbox on `36`, off `46`.
- Frame 40 is presentation/contact accent; collision still decides which enemies are damaged.
- Existing hit-stop: `0.065 s`.

### Layer 5 — Feedback

- Frame 30 launch layer; frame 40 metal impact plus low boom.
- Camera: `0.18 m`, `24 Hz`, `0.16 s`.

## 4. Bastion Breaker

Fantasy: the Knight raises the sword overhead, slams the ground and erupts a six-pylon fortress matrix around the impact.

### Layer 1 — Character animation

Action: `ultimate`, frames `0–94`.

| Frame | Pose | Full-body direction |
|---:|---|---|
| 0 | Guard | Stable stance and low center of gravity. |
| 18 | Charge | Knees bend, hips sit back, hands begin lifting the sword, runes gather. |
| 42 | Overhead wind-up | Both shoulders externally rotate; chest opens; spine arches; weapon trail starts. |
| 52 | Anticipation | Sword reaches its highest point, hips and chest coil, six-pylon matrix previews on the ground. |
| 58 | Downstroke | Hips drop and rotate first, then spine/chest, shoulders, elbows and wrists. |
| 60 | Damage opens | Ground collider activates just before the visual peak. |
| 64 | Impact | Sword reaches ground; hands, shoulders and torso compress; primary damage/VFX/audio fires. |
| 74 | Follow-through | Weight remains low as debris falls; hitbox and trail stop. |
| 75–94 | Recovery | Sword is pulled free and the Knight stands without an abrupt torso reset. |

### Layer 2 — Weapon/projectile

- Existing greatsword plus ground matrix with exactly six pylon instances.
- TripoSR may generate one reusable pylon master; Blender owns the dais, sockets and radial instancing.
- Runtime must not use the old ten-blade concept count.

### Layer 3 — VFX

- Charge runes, overhead sword ribbon, six sequential crystal eruptions, rock debris, crystal shards, smoke ring, 5.0 m shockwave, ground distortion and high-energy violet light.

### Layer 4 — Gameplay timing

- Active `58–74`; `KnightBladeStorm` hitbox on `60`, primary damage event frame `64`, off `74`.
- Extra pylon eruptions after frame 64 are cosmetic unless a separate gameplay event is explicitly added.
- Hit-stop: `0.11 s`.

### Layer 5 — Feedback

- Charge, weapon drop, bass impact and crystal-shatter layers.
- Camera: `0.34 m`, `21 Hz`, `0.34 s`; reduced-motion mode scales this heavily.

## 5. Steel Step

Fantasy: a short armored dash leaves three staggered Knight echo shells and a sharp arrival burst.

### Layer 1 — Character animation

Action: `dash`, frames `0–46`.

| Frame | Pose | Full-body direction |
|---:|---|---|
| 0 | Launch | Instant low lean, arms tight to silhouette, front foot reaches while C++ starts displacement. |
| 8 | Extension | Hips and chest remain aligned to travel; rear leg fully extends; head stays level. |
| 23 | Arrival guard | Feet prepare to receive weight; optional dash contact hitbox opens. |
| 25 | Arrival impact | Knee and hip compress, chest counters forward momentum, arrival burst fires. |
| 29 | Settle | Hitbox and speed trail stop. |
| 30–46 | Recovery | Rise and blend to locomotion without foot sliding. |

### Layer 2 — Weapon/projectile

- No TripoSR asset.
- Main shape is three translucent instances of the actual Knight mesh, not a boot or static character cutout.

### Layer 3 — VFX

- Root-motion ribbon, speed shards, foot dust, three time-offset echo shells and a compact arrival spark/shockwave.

### Layer 4 — Gameplay timing

- C++ owns dash displacement and invulnerability rules.
- Optional `KnightDash` contact hitbox on `23–29`; presentation starts at frame 0.
- Hit-stop at arrival: `0.025 s`.

### Layer 5 — Feedback

- Frame 0 metallic launch whoosh; frame 25 arrival scrape.
- Camera: `0.06 m`, `34 Hz`, `0.07 s`.

---

# Magic Caster

## 6. Arc Bolt

Fantasy: the caster condenses a cyan crystal lance, releases it toward the nearest visible target and shatters it on contact.

### Layer 1 — Character animation

Action: `basic_01`, frames `0–62`.

| Frame | Pose | Full-body direction |
|---:|---|---|
| 0 | Casting stance | Feet offset, staff hand low, free hand open, eyes on target. |
| 8 | Gather | Hips rotate slightly away; spine lengthens; free hand draws motes toward the staff. |
| 14 | Form core | Both shoulders frame the crystal core; staff wrist aims downrange. |
| 23 | Anticipation | Weight shifts to rear leg, chest coils, staff hand retracts and free hand points. |
| 28 | Release | Rear leg and hips drive the chest forward; shoulder, elbow and wrist extend; projectile event fires. |
| 35 | Follow-through | Staff passes aim line, free hand stabilizes recoil. |
| 36–62 | Recovery | Arms lower and weight returns to casting stance. |

### Layer 2 — Weapon/projectile

- Solid lance core may use a TripoSR crystal donor; Blender owns the true circular rings, braces, pivot and emissive material.
- C++ owns targeting, line-of-sight validation, fallback fixed range, movement, lifetime and collider.

### Layer 3 — VFX

- Charge motes, orbiting electric filaments, 18-segment projectile ribbon, bright contact flash, 18 crystal shards, small shockwave and cyan point light.

### Layer 4 — Gameplay timing

- Projectile spawns at frame `28` with event ID `MageBolt`.
- Targeting selects the nearest enemy visible in camera with an unblocked line; otherwise fire at fixed range.
- `projectile_contact` controls damage and impact; `projectile_expire` controls cleanup.
- Hit-stop: `0.025 s`.

### Layer 5 — Feedback

- Frame 14 charge chime, frame 28 release snap, contact crystal/electric crack.
- Camera on contact: `0.045 m`, `32 Hz`, `0.06 s`.

## 7. Frost Ring

Fantasy: the caster gathers cold around the staff and erupts a circular crown of ice teeth to damage and control nearby enemies.

### Layer 1 — Character animation

Action: `skill_one`, frames `0–78`.

| Frame | Pose | Full-body direction |
|---:|---|---|
| 0 | Ready | Upright stance with staff centered. |
| 20 | Gather | Knees bend, hips lower, shoulders round inward and both hands pull cold toward the chest. |
| 38 | Crown preview | Arms widen, chest opens and staff points at the ground target; ice crown appears. |
| 46 | Anticipation | Maximum low stance, elbows and wrists drawn inward; damage ring arms. |
| 50 | Eruption | Hips and spine rise sharply as both arms throw outward; ring expands. |
| 58 | Follow-through | Hands remain open while mist and shards travel; hitbox closes. |
| 59–78 | Recovery | Arms descend asymmetrically and knees straighten. |

### Layer 2 — Weapon/projectile

- TripoSR may create one ice-spike master; Blender owns the ring base, socket and instancing.
- Main shape uses sequential spike timing rather than a single frozen static cluster.

### Layer 3 — VFX

- Snow motes, expanding 32-segment frost ribbon, ice shards, mist, refraction, 4.4 m shockwave and cyan light.

### Layer 4 — Gameplay timing

- `MageFrostNova` hitbox on `46`, eruption/damage accent at `50`, off `58`.
- Crowd control duration is gameplay state, independent of how long mist remains visible.
- Hit-stop: `0.04 s`.

### Layer 5 — Feedback

- Gather wind, ice eruption and crystal crack layers.
- Camera: `0.09 m`, `26 Hz`, `0.12 s`.

## 8. Gravity Well

Fantasy: the caster forms a compact singularity inside engraved orbital rings, pulls enemies inward and collapses the field.

### Layer 1 — Character animation

Action: `skill_two`, frames `0–78`.

| Frame | Pose | Full-body direction |
|---:|---|---|
| 0 | Ready | Open casting stance with both hands separated. |
| 18 | Form core | Hands cup an invisible sphere; shoulders and chest curl around it; core appears. |
| 24 | Orbit | One hand rotates above the other; hips counter-rotate; orbit ribbons start. |
| 36 | Field arms | Feet plant wide, spine leans toward target and both palms compress the sphere. |
| 40 | Active pulse | Arms separate sharply to place the well; gameplay field becomes active. |
| 48 | Collapse | Hands snap together, chest contracts and head dips; collapse impact fires. |
| 49–78 | Recovery | Fingers, wrists, shoulders and torso release gradually while field visuals dissolve. |

### Layer 2 — Weapon/projectile

- TripoSR may supply one small rock donor only.
- Blender owns the singularity core, pedestal and true circular/asymmetrical rings.
- C++ owns pull force, field lifetime and hostile-projectile interaction.

### Layer 3 — VFX

- Inward motes, orbiting debris, triple ring ribbon, core glow, lens distortion, collapse fragments, 3.5 m shockwave and violet light.

### Layer 4 — Gameplay timing

- `MageGravityWell` field hitbox on `36`, active pulse at `40`, collapse/off at `48`.
- Pull and damage tick cadence must be deterministic C++ state, not particle contact.
- Hit-stop: `0.055 s` at collapse.

### Layer 5 — Feedback

- Core form, low loop and sub-bass collapse layers.
- Camera: `0.14 m`, `18 Hz`, `0.20 s`.

## 9. Astral Tempest

Fantasy: the caster opens a celestial orrery surrounded by exactly seven pylons, then calls down a primary astral strike and two cosmetic pulses.

### Layer 1 — Character animation

Action: `ultimate`, frames `0–94`.

| Frame | Pose | Full-body direction |
|---:|---|---|
| 0 | Ready | Tall silhouette, staff grounded, free hand low. |
| 12 | Invocation | Knees soften, hips rotate, free arm traces upward and star motes begin. |
| 34 | Orrery forms | Both arms spread, chest opens, head tilts toward the sky; orrery appears. |
| 46 | Anticipation | Staff lifts overhead, spine arches and shoulders separate; constellation ribbons begin. |
| 58 | Sky opens | Feet brace and torso stops rising; the caster fixes the ground target. |
| 60 | Damage field arms | Both hands aim through the orrery; gameplay hitbox starts. |
| 64 | Primary strike | Arms drive downward and chest compresses; authoritative damage/impact fires. |
| 69/74 | Echo pulses | Smaller wrist/chest recoil accents; presentation-only pulses. |
| 76 | Follow-through | Hitbox and constellation ribbons stop. |
| 77–94 | Recovery | Arms descend, spine unarches and staff returns to stance. |

### Layer 2 — Weapon/projectile

- TripoSR may create one pylon master; Blender owns the orrery, altar and exactly seven instances.
- Meteor beams and constellation arcs are engine VFX, not static 3D models.

### Layer 3 — VFX

- Star motes, constellation ribbons, seven-pylon activation, meteor shards, ground sparks, cloud wisps, large shockwave, sky distortion and strong temporary light.

### Layer 4 — Gameplay timing

- `MageAstralTempest` hitbox on `60`, primary authoritative hit at `64`, off `76`.
- Frames `69` and `74` remain cosmetic until explicit damage events are designed.
- Hit-stop: `0.09 s`.

### Layer 5 — Feedback

- Choir charge, sky-open swell, primary impact and delayed pulse layers.
- Camera: `0.30 m`, `19 Hz`, `0.38 s`, strongly scaled by reduced-motion mode.

## 10. Phase Blink

Fantasy: paired crescent gates fold space; the caster disappears at the entry gate and reappears at the exit with a short crystal burst.

### Layer 1 — Character animation

Action: `dash`, frames `0–46`.

| Frame | Pose | Full-body direction |
|---:|---|---|
| 0 | Entry snap | Arms and shoulders fold inward, knees compress and entry gate/trail start immediately. |
| 6 | Transit silhouette | Torso narrows and limbs remain close so the dissolve reads cleanly. |
| 13 | Exit snap | Exit gate appears; hips lead the reappearance and arms open to regain balance. |
| 25 | Arrival settle | Spatial thread stops after the feet find the ground. |
| 30–46 | Recovery | Spine, shoulders and hands blend back to casting stance. |

### Layer 2 — Weapon/projectile

- No TripoSR. Blender builds modular crescent frames with a real open aperture and cyan keystones.
- C++ owns teleport destination validation, collision safety and character relocation.

### Layer 3 — VFX

- Paired portal frames, inward entry shards, transit motes, spatial thread, exit fragments, membrane shader, refraction and compact exit shockwave.

### Layer 4 — Gameplay timing

- Relocation is authoritative C++ gameplay at the validated blink event; visual gates do not decide position.
- Entry presentation starts at `0`, exit at `13`, trail stops at `25`, recovery begins at `30`.
- No hit-stop is used.

### Layer 5 — Feedback

- Frame 0 folded entry sound and frame 13 sharp exit crack.
- Camera: `0.055 m`, `30 Hz`, `0.07 s`.

---

# Shared runtime hierarchy

```text
SkillInstance
├── CharacterAction
├── WeaponOrMainShape
│   ├── Model or procedural mesh
│   ├── Material parameters
│   └── Attachment socket / world anchor
├── Presentation
│   ├── Particles
│   ├── Trail
│   ├── Glow
│   ├── Distortion
│   ├── Impact flash + shockwave
│   └── Temporary light
├── Gameplay
│   ├── Animation event cursor
│   ├── Hitbox / projectile / field collider
│   ├── Damage and crowd control
│   └── Lifetime / cooldown
└── Feedback
    ├── Layered sound cues
    ├── Hit-stop
    └── Camera shake
```

# Required implementation order

1. Polish the five primary actions for each hero in Blender with the pose tables above.
2. Export named actions and verify animation names/frame ranges in GLB.
3. Finish only the required solid objects and procedural meshes.
4. Dispatch gameplay and presentation from the same animation event cursor.
5. Add trail/particles/impact/light/distortion in pooled C++ systems.
6. Add sound, hit-stop, camera shake and reduced-motion scaling.
7. Run collision, frame-sync, visual QA and performance-budget tests for all ten skills.

# Acceptance checklist per skill

- [x] Full-body silhouette is readable at wind-up, anticipation, strike/release and recovery.
- [x] Spine and hands have meaningful motion, not merely constant baked curves.
- [x] Weapon/main shape follows the correct socket or world anchor.
- [x] Damage timing is driven by gameplay events, never VFX lifetime.
- [x] Trail follows the actual sword/projectile path.
- [x] Impact combines flash, particles, shockwave and light where specified. Unsupported real light/distortion capabilities degrade to authored additive glow.
- [x] Layered sound, hit-stop and camera shake fire at the authoritative event.
- [x] Reduced-motion mode scales camera, particle count and trail complexity.
- [x] Fixed-capacity VFX emitters release after cosmetic tails expire.
- [x] Runtime stays inside the per-skill triangle budget in `skill_presentation_manifest_v2.json`.

# Implementation result

- Runtime: C++17 only. Blender helper scripts remain ignored under `.codex-temp` and are not shipped.
- Main-shape assets: ten of ten skills load a real GLB with an `Activate` action.
- V2 production replacements: Aegis Counter `2508` tris, Shield Rush `2701` tris, Bastion Breaker `5184` tris and Arc Bolt `628` tris.
- Presentation: ten registered fixed-capacity VFX packages cover main shape, secondary particles, trail, glow, distortion fallback, impact, light fallback, sound and camera shake.
- Feedback: ten dedicated Survival skill sound events; multi-layer events are supported directly by `SoundManager`.
- Verification: `Survival3DConfigTests`, `AnimationEventsIKTests`, `Step10ProductionTests` and `VfxRuntimeTests` all pass; the built executable completed a five-second startup smoke test without exiting or crashing.
