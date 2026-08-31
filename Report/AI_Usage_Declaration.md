# AI Usage Declaration

**Course:** CS202 — Programming Techniques
**Project:** Apple Knight Adventure (2D action-platformer, C++17 / raylib / LDtk)
**Repository:** `CS202-FinalProject`
**Period covered:** 2026-06-08 → 2026-08-31

---

## About this document

Our team used an AI coding assistant during development. We did not keep verbatim
chat transcripts for the whole project, so **this log is a reconstruction**: each
entry was rebuilt from the commit that the conversation produced, and records the
substance of what we asked and what we got rather than the exact wording. The
commit hashes and messages referenced in each entry are real and can be checked
against `git log`.

Everything below was reviewed, compiled, and tested by us before being committed.
Where the assistant produced something wrong, that is recorded too — several
entries are follow-up prompts after a first answer failed.

**Division of work.** The project was split by layer. Nguyễn Anh Kiệt owned the
View/UI/rendering layer, the animation system, the boss system, the LDtk and Map
Builder tooling, and the audio system. Nguyễn Trọng Tiến owned the Model layer and
entity hierarchy, level design and map authoring, gameplay systems (checkpoints,
save data, elemental combat, buffs), and gameplay bug-fixing.

| Member | Primary area | Commits |
|---|---|---|
| Nguyễn Anh Kiệt | View / rendering, UI, animation, boss system, tooling, audio | ~158 |
| Nguyễn Trọng Tiến | Model, level design, gameplay systems, save data, bug-fixing | ~42 |

---

# Part 1 — Nguyễn Anh Kiệt

## K-01 · Animation system and texture atlas
**Date:** 2026-06-10
**Related commits:** `Add AnimationSystem with Animator class and functionality for clip management`, `Add TextureAtlas class for managing textures and animation clips from JSON`, `Refactor AnimationSystem and TextureAtlas for improved memory management and JSON parsing`

**Prompt**
> I am writing a 2D game in C++17 with raylib. I need an animation system with two
> classes: `TextureAtlas` that loads a spritesheet plus a JSON descriptor
> (Aseprite-style `frames` map with `x/y/w/h`, and an optional `clips` map with a
> frame-name list and per-frame durations), and `Animator` that plays a named clip
> and reports the current source rectangle. Requirements: several animators must be
> able to share one atlas without reloading the texture, and a clip must be able to
> loop or play once. Show me the headers and the parsing code.

**AI response (summary)**
Produced `TextureAtlas` with a `frames` name→`Rectangle` map and a `clips`
name→`AnimationClip` map, plus `Animator` holding a pointer to the current clip, a
frame index, and an accumulator ticked in `Update(dt)`. Atlas sharing was handled
with `std::shared_ptr<TextureAtlas>` handed out by a cache keyed on the JSON path.

**Outcome**
Accepted as the base of `View/Animator.*` and `View/TextureAtlas.*`. We changed two
things: the assistant unloaded the raylib `Texture2D` in the destructor without
checking `id != 0`, which crashed when a JSON path was wrong and the texture had
never loaded; and clip durations were assumed uniform, which did not fit our
hand-timed boss frames, so we changed `durations` to a per-frame vector.

---

## K-02 · Render command queue and layer sorting
**Date:** 2026-06-11 → 2026-06-12
**Related commits:** `feat: implement Renderer class with initialization, shutdown, and sprite submission functionality`, `feat: add RenderTypes header with Layer enum and RenderCommand struct`, `Systems: renderer core fixes + lock-free MPSC queue + ResizeWindow + dropped counter race fix`

**Prompt**
> Our entities currently call `DrawTexture` directly from `Render()` inside the
> Model classes, so draw order depends on entity iteration order and the Model
> depends on raylib. I want a `Renderer` singleton that entities submit draw
> commands to (`SubmitSprite(texture, src, pos, scale, rotation, origin, tint,
> layer, z, flipX, entityId)`), which then sorts by layer then z and flushes once
> per frame. Explain the trade-offs of sorting per frame vs keeping buckets.

**AI response (summary)**
Recommended fixed per-layer buckets (`Background`, `World`, `UI`) with a
preallocated command array per bucket and an index array sorted with
`std::sort` on flush, so the command structs themselves are never moved. Warned
that submitting from a non-main thread needs a queue and gave an MPSC ring buffer
sketch.

**Outcome**
Accepted. This became `View/Renderer.cpp` and let us delete every `Render()` method
from the Model layer (commit `cleanup(model): remove all Render() traces from Model
layer`). The threading part we kept but simplified — we only submit from the main
thread in practice, and the queue is a guard rail. The assistant's first version of
the dropped-command counter used a plain `int` incremented from both paths; we
changed it to `std::atomic` after a data race showed up under `-fsanitize=thread`.

---

## K-03 · View bug sweep after the render refactor
**Date:** 2026-06-13
**Related commits:** `fix: various View bugs — UIStateManager menu layer, missing includes, EnemyStatusRenderer layer/world->screen, SkillSlotData operator==, GameView missing Update call, audio init guards, entity ID assertion, stale comments`

**Prompt**
> After moving all rendering into the View layer, several things broke at once:
> the menu draws underneath the HUD, enemy status icons are drawn at world
> coordinates so they slide off-screen when the camera moves, and `GameView` never
> animates because nothing ticks the animators. Here are the relevant files.
> Go through them and list what is actually wrong, most severe first, instead of
> rewriting everything.

**AI response (summary)**
Reported seven distinct defects: `UIStateManager` pushed the menu onto the world
layer instead of the UI layer; `EnemyStatusRenderer` passed world coordinates to a
screen-space draw and needed `GetWorldToScreen2D` with the active camera;
`GameView::Render` never called `EntityRenderer::Update(dt)`; `SkillSlotData`
lacked `operator==` so the "slot changed" check never fired; two missing includes
that only compiled by luck through transitive headers; `SoundManager` calls before
`InitAudioDevice`; and an entity-ID assertion that fired on the valid ID `0`.

**Outcome**
All seven were real and all were fixed in that commit. This was the single most
useful AI interaction of the project — asking for a ranked diagnosis rather than a
rewrite kept the diff small and reviewable.

---

## K-04 · Boss phase state machine
**Date:** 2026-08-04 → 2026-08-05
**Related commits:** `Refactor boss implementation and add new boss types`, `Enhance boss mechanics with combo steps, super armor, and damage tracking`, `Enhance boss mechanics and projectile behavior: add 2D scaling, refine phase transitions, and implement new projectile subtypes`

**Prompt**
> I have one `Boss` base class and three subclasses. Each boss has 3–4 phases that
> change at health thresholds; a phase change plays a transition animation during
> which the boss must be invulnerable and must not act; and inside a phase the boss
> runs multi-step melee combos where each step has its own hit window. Design the
> state machine. I want phase data to be declarative per subclass, not a `switch`
> repeated in three files.

**AI response (summary)**
Suggested a `BossState` enum (`Idle`, `Move`, `Attack`, `Hurt`, `Transition`,
`Die`) held in the base class, with each subclass supplying a table of phase
descriptors (health threshold, move set, cooldowns). Combos were modelled as a step
index plus a per-step active window, and "super armor" as a flag that suppresses the
`Hurt` transition while still applying damage.

**Outcome**
Adopted as the shape of `Boss.*`, `Boss1/2/3`. We rejected the assistant's proposal
to drive phase transitions from inside `TakeDamage` — a transition that starts in
the middle of collision resolution left projectiles referencing a boss that had
already swapped its hitbox. We moved the check to the boss's own `Update`, which is
why `BossState::Transition` is tested explicitly in the projectile collision code.

---

## K-05 · Oversized animation frames breaking hitbox alignment
**Date:** 2026-08-04 → 2026-08-05
**Related commits:** `Add scale multiplier to AnimationClip and normalize oversized frames in CharacterRenderer`, `Implement hit tracking for projectiles and adjust boss cooldowns and scaling in CharacterRenderer`

**Prompt**
> Boss hurt and ultimate animations come from sheets with much larger frames than
> the idle sheet (the artist padded them for the effect). When those clips play, the
> boss visually doubles in size and no longer lines up with its collision box. I do
> not want to re-export the art. How do I normalize this in the renderer?

**AI response (summary)**
First answer: scale every frame so its width matches the idle frame width. That was
wrong for us — the padding is not symmetric, so width-matching made the boss sink
into the floor.

Second prompt from us clarified that the sprites are anchored at the feet. The
assistant then proposed a per-clip `scale` multiplier stored in `AnimationClip`
plus a per-clip ground-origin point, so the renderer anchors on the feet and scales
around that anchor.

**Outcome**
The second answer was correct and shipped as `GetCurrentClipScale()` and
`GetCurrentGroundOrigin()` in `Animator`, consumed by `CharacterRenderer::RenderAll`.
Recorded here because the first answer was accepted, tested, and then reverted.

---

## K-06 · LDtk level loader
**Date:** 2026-07-24 → 2026-07-26
**Related commits:** `feat: integrate LDtk level editor (full MVC)`, `fix: compile errors with nlohmann json and stat64i32 on MinGW 14`, `fix: adjust LDtk entity position scaling for correct rendering`

**Prompt**
> We want to author maps in LDtk instead of a custom text format. Given a `.ldtk`
> JSON file, write a loader that reads: level pixel size, the `Collision` IntGrid
> layer into a solid map, the `Tiles`/`BG_Tiles` tile layers, and the `Entities`
> layer where each entity identifier maps to one of our game classes. Use
> nlohmann/json. Assume the game renders at 64px tiles while LDtk authors at 16px.

**AI response (summary)**
Produced the pass structure still in `LevelFactory::LoadLDtkLevel`: parse tileset
defs into a UID→tileType map, pass 1 builds the solid set from `intGridCsv`, pass 2
walks the layers and dispatches entities by `__identifier`.

**Outcome**
Accepted, but it took three rounds to actually work:

1. `layer["__tilesetDefUid"]` is JSON `null` on Entities and IntGrid layers, and
   `nlohmann::json::value()` throws `type_error.302` when a key exists but is null —
   it only falls back for *absent* keys. The assistant's code used `value()` and
   threw on every map. Fixed with an explicit null check.
2. `intGridCsv` can contain `null` for empty cells; `get<int>()` threw. Fixed with a
   null guard.
3. Entity positions were written straight from `px`, so every entity landed at ¼ of
   its correct position. The assistant had applied the 64/16 scale to tiles but not
   to entities. This is the `fix: adjust LDtk entity position scaling` commit.

The MinGW `stat64i32` link error was unrelated to the AI — it came from an older
raylib binary against MinGW 14 and we solved it ourselves in `CMakeLists.txt`.

---

## K-07 · Minimap with fog-of-war exploration
**Date:** 2026-08-21
**Related commits:** `feat: Implement minimap functionality with exploration tracking and rendering`

**Prompt**
> Add a minimap that only reveals areas the player has visited. Our maps are up to
> 258×217 tiles, so I do not want a per-tile visited array drawn every frame.
> Suggest a representation and a draw strategy, and tell me where in the frame it
> should be updated.

**AI response (summary)**
Recommended coarse exploration cells (a block of N×N tiles marked explored when the
player centre enters it) stored in a hash set, with the map itself drawn from the
already-loaded tile list transformed to panel space. Entities of interest
(checkpoints, portals, cup, boss) drawn as shape markers, filtered by whether their
cell is explored.

**Outcome**
Accepted and implemented as `View/MinimapView.cpp`. The marker shapes are ours —
diamond for checkpoint, ring for portal, pentagon for the level-complete cup,
circle for boss.

---

## K-08 · Audio manifest and sound pooling
**Date:** 2026-08-14 → 2026-08-25
**Related commits:** `Add new sound assets and update audio manifest`, `feat: rebuild gameplay sound effects`

**Prompt**
> Sounds are currently loaded ad-hoc with hardcoded paths and the same effect
> retriggering every frame produces a machine-gun sound. Design a manifest-driven
> `SoundManager`: a JSON file maps a logical event name to one or more samples with
> volume, a pitch range, and a minimum cooldown, and `PlaySound("enemy_hurt")`
> picks a sample, randomizes pitch, and refuses to play if the cooldown has not
> elapsed.

**AI response (summary)**
Gave the manifest schema we now use in `assets/sounds/audio_manifest.json`
(`samples`, `volume`, `pitch: [min,max]`, `cooldown`) and a `SoundManager` that
preloads samples once and keeps a last-played timestamp per event.

**Outcome**
Accepted essentially as given. The cooldown field is what fixed the footstep and
`enemy_hurt` spam.

---

## K-09 · Level source adapter refactor
**Date:** 2026-08-31
**Related commits:** `Add level source adapter and remove unused runtime code`

**Prompt**
> `LevelFactory::LoadLevel` decides between the legacy `.lvl` text format and the
> LDtk format with a string suffix comparison in the middle of the function, and the
> two loaders are now ~700 lines in one file. Refactor to an interface so the format
> choice is a lookup, without changing behaviour — the legacy loader must stay the
> fallback for extensionless paths and extension matching must become
> case-insensitive.

**AI response (summary)**
Proposed an `ILevelSourceAdapter` with `CanLoad(path)` / `Load(request)` and a
`LevelLoadRequest` struct bundling the four parameters, with `LegacyLevelAdapter`
and `LDtkLevelAdapter` as the two implementations and the legacy one as the default.

**Outcome**
Accepted and applied — this is the current shape of `Factories/LevelSourceAdapter.h`
and the top of `LevelFactory.cpp`. We deliberately kept it as a pure move with no
behaviour change so the refactor could be verified by playing every level.

---

# Part 2 — Nguyễn Trọng Tiến

## T-01 · Entity and Character hierarchy
**Date:** 2026-06-10
**Related commits:** `Foundation and Model hierarchy`, `Build system + include paths`

**Prompt**
> I am designing the Model layer for a 2D platformer. I need a base `Entity` (id,
> position, size, velocity, rotation, scale, active flag, type tag, virtual
> `Update`) and a `Character : Entity` adding health, damage, direction, and an
> attack timer. Entity types include Player, Enemy, Boss, Item, Chest, Checkpoint,
> Projectile, TeleportPortal, FakeWall. Advise on whether to use an enum tag plus
> `static_cast` or `dynamic_cast` for type dispatch, given this is a course project
> graded on OOP.

