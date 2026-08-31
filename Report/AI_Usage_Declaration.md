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

