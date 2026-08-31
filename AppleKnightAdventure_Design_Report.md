# APPLE KNIGHT ADVENTURE

## Software Architecture and Design Report

**Course:** Software Design Patterns - CS202  
**Class group:** Group 55  
**University:** University of Science - VNUHCM  
**Faculty:** Faculty of Information Technology  
**Prepared:** Ho Chi Minh City, August 2026

**Analysis basis:** The complete current project source tree was reviewed, including the 2D Adventure client, application shell, Map Builder, rendering and UI stack, Survival3D mode, persistence layer, and separately built Survival3D HTTP backend. The report is organized around the implemented design rather than repository history.  
**Verification basis:** Static dependency and call-flow tracing across 185 project C++ header/source files, CMake reconfiguration, and successful builds of both `AppleKnightAdventure.exe` and `AegisRiftServer.exe`.

## Group Information

| Student ID | Full name |
|---|---|
| 25125074 | Nguyễn Anh Kiệt |
| 25125037 | Nguyễn Trọng Tiến |

## Table of Contents

1. Executive Summary
2. Project Scope and Source Coverage
3. Architectural Drivers and Design Principles
4. System Architecture and Runtime Composition
5. Adventure Domain Design
6. Gameplay Systems and Collaboration
7. Map Builder and Object Creation Design
8. Rendering, Animation, and UI Architecture
9. Survival3D Architecture
10. Persistence and Online Service Design
11. Applied Design Patterns
12. Design Reasoning and Consequences
13. End-to-End Runtime Flows
14. Build and Reproduction
15. Conclusion
16. Source Evidence Index

<!-- pdf-body -->

# 1. Executive Summary

Apple Knight Adventure is a C++17 game built with raylib. It is a collection of cooperating runtime surfaces: a six-level 2D Adventure campaign, a preparation and progression shell, a custom Map Builder with reversible editing and playtesting, a separate fixed-step 3D survival game, and an optional local HTTP leaderboard service. These surfaces share assets, audio, display management, achievements, and JSON persistence, but each owns a different part of the gameplay lifecycle.

The dominant architectural style is **MVC-inspired orchestration**. Model classes own gameplay state and rules. Controller classes interpret input and establish update order. View classes translate model snapshots and non-owning model references into raylib draw commands. The design deliberately remains pragmatic: the boundary is strong enough to identify ownership and responsibility, but it does not enforce a pure academic MVC rule in which every view receives an immutable view model. This description follows the actual call graph and object lifetimes across the complete project.

The 2D Adventure world is represented by a `GameState` aggregate. It exclusively owns one or two players, the entity collection, newly spawned entities waiting to be merged, three tile layers, level metadata, and completion/timing state. `GameController` owns that aggregate together with collision, particle, scoring, and elemental systems. It sequences input, world simulation, broad-phase rebuilding, combat, item collection, pet/projectile logic, level progression, camera updates, and presentation updates. Views and renderers hold non-owning references; the controller detaches those references before replacing or destroying the world.

The Map Builder reuses the same `GameState` and `LevelFactory` used by play mode. Its editing history is the clearest pattern-centered subsystem. The **Command pattern** represents one reversible mutation through `ICommand` and concrete tile/entity commands. The **Composite pattern** is a separate design layered on top: `CompositeCommand` groups multiple `ICommand` children so a pasted region executes and undoes as one logical transaction. The editor can also import either its legacy `.lvl` format or an LDtk project through a common `ILevelSourceAdapter`; this is a separate **Adapter pattern** that converts incompatible source schemas into the same runtime `GameState`. This report treats Command, Composite, and Adapter as distinct patterns with separate participant mappings and design reasoning.

The strongest implemented patterns are Singleton, Simple Factory, Adapter, Command, Composite, Object Pool, and Template Method. Strategy is meaningfully present in class-specific player skills, although a few consumers still inspect concrete skill types. Enum-driven finite state machines coordinate Adventure behavior and Survival3D animation/phase transitions; they are FSM implementations, not the GoF State-object pattern. `GameView` acts as a partial Facade over several rendering helpers. Shared texture atlases are a cache with flyweight-like reuse, but the code does not formalize Flyweight participants.

Several design decisions are especially important to runtime correctness:

- `std::unique_ptr` expresses exclusive world and entity ownership.
- New entities are buffered until a safe merge point, avoiding iterator invalidation during updates.
- A quadtree is rebuilt after movement and queried during combat to reduce broad-phase work.
- Short-lived particles are acquired from an object pool to reduce allocation churn.
- Asset files are decoded by a worker thread, while GPU texture upload remains on the render thread.
- Survival3D advances gameplay using a 1/60-second fixed time step with bounded catch-up.
- Save writes use versioned JSON, a temporary file, and a backup file for recoverability.
- Survival results are committed locally before optional HTTP synchronization and use idempotency keys for retry safety.

The design is best understood as a set of explicit lifecycles rather than a collection of pattern names. Ownership, update ordering, render-thread constraints, persistence durability, and reversible editor transactions are the reasons the patterns exist. The following sections explain those lifecycles and show how the classes collaborate across the whole project.

# 2. Project Scope and Source Coverage

## 2.1 Product surfaces

| Surface | User-visible responsibility | Primary coordinator | Principal state/view collaborators |
|---|---|---|---|
| Application shell | Startup loading, mode dispatch, window resize, audio/achievement ticks, and ordered shutdown | `src/main.cpp` | `WindowManager`, `AssetManager`, `Renderer`, controllers and views |
| Adventure campaign | Six levels, single-player/local co-op, enemies, bosses, checkpoints, items, pets, skills, elemental reactions, cores, scoring, achievements, and results | `GameController` | `GameState`, `Player`, entity hierarchy, gameplay systems, `GameView` |
| Preparation and progression | Character/pet selection, local co-op configuration, shop purchases, options, leaderboards, achievements, and saved profile | `MenuController`, `PrepareController`, `ShopController` | `MenuView`, `PrepareView`, `ShopView`, `SaveManager` |
| Map Builder | Open/create maps, import `.lvl` or `.ldtk`, paint or erase tiles, place/remove entities, copy/paste, bucket fill, undo/redo for command-backed edits, save as `.lvl`, and playtest | `MapBuilderController` | `GameState`, `CommandManager`, `MapBuilderView`, `LevelFactory`, level-source adapters |
| Rift Survival | Character selection, wave combat, bosses, upgrades, results, records, accessibility options, animation/VFX/IK, and optional ranking | `SurvivalController` | `SurvivalTypes`, animation graph/events, `SurvivalView`, `SurvivalRunService` |
| Survival backend | Guest identity, profile lookup, idempotent run completion, score leaderboard, and durable server data | `AegisRiftServer` entry | `SurvivalServerCore` and JSON data file |

The client entry point is `src/main.cpp:44`. The server has its own entry point at `backend/survival3d/main.cpp:254`. CMake builds them as separate executables, so the server is an optional integration boundary rather than a process embedded inside the game.

## 2.2 Source organization

| Source area | Contents | Architectural role |
|---|---|---|
| `include/Model` and `src/Model` | World aggregate, entities, characters, skills, inventory, commands, level scoring, dual-world data types, and save state | Domain state and rules |
| `include/Controller` and `src/Controller` | Adventure, input, menu, prepare, shop, and editor coordination | Application/use-case layer |
| `include/View` and `src/View` | 2D renderers, atlas animation, HUD, menus, overlays, minimap, editor UI, and result presentation | Presentation and GPU-facing layer |
| `include/Systems` and `src/Systems` | Collision/quadtree, particles/pooling, elements, buffs, cores, sound, achievements, tweens, and display scaling | Reusable runtime services |
| `include/Factories` and `src/Factories` | Level-source adapters, level translation, and enemy/item construction | Format adaptation and creation boundary |
| `include/Survival3D` and `src/Survival3D` | 3D controller, data model, animation graph/events, runtime IK, VFX runtime, view, and run service | Self-contained game-mode subsystem |
| `backend/survival3d` | HTTP routing and server-side validation/storage | Optional process/service boundary |
| `assets` | LDtk/legacy maps, balance JSON, texture atlases, audio, fonts, models, shaders, and UI art | Data-driven content |

The source review covers 185 project `.h` and `.cpp` files under those areas, approximately 37,700 physical source lines. Vendored raylib headers and generated build output are not counted as project design sources.

## 2.3 Active runtime and supporting model code

The report distinguishes an **active runtime path** from a **compiled extension point**. `main.cpp` actively dispatches menu, preparation, shop, options, Adventure, Map Builder, and Survival3D. `GameController` and `MapBuilderController` both operate on `GameState`. The `DualWorld`, `DualWorldPlayer`, and `CrossWorldManager` classes define a light/shadow-world model and `LevelFactory` exposes dual-world serialization methods, but the top-level loop does not dispatch a dual-world game mode.

The same distinction applies to smaller source components. `TriggerZone` can be loaded, saved, and placed in the editor, but `GameController` does not currently consume it as a gameplay interaction. `InventoryView` is compiled and listed by `UIStateManager`, but the active shell has no initialization/open/snapshot call for it. These classes are documented as supporting extensions, not shown as live frame-flow participants.

This distinction is also used for pattern identification. A class name or an unused interface is not enough to claim a design pattern. A pattern is listed as applied only when its participants collaborate in the current source flow.

## 2.4 Technology and platform boundary

The root CMake file selects C++17, adds the bundled raylib library, fetches `nlohmann_json` 3.11.3, synchronizes `assets` into the build directory, and builds the Windows client. A second target builds the Survival backend with Winsock and JSON support. Runtime code relies on raylib for window/input/audio/2D/3D APIs, the C++ standard library for ownership, containers, filesystem, and threads, and WinHTTP for the Survival client transport.

The application assumes relative asset paths such as `assets/textures/...`. Running from the build directory matches the layout created by `SyncAssets`. Save files also use a relative default path; persistence is therefore designed independently from any fixed repository location.

# 3. Architectural Drivers and Design Principles

## 3.1 Quality drivers

| Driver | Concrete project need | Design response |
|---|---|---|
| Stable real-time timing | Input, physics, combat, animation, and rendering must remain ordered under frame hitches | Explicit controller pipelines; frame-delta clamp in the shell; fixed-step loop in Survival3D |
| Clear ownership | Levels are replaced, entities spawn/despawn, and editor commands temporarily transfer entities | `unique_ptr` aggregate ownership, move semantics, non-owning presentation references, explicit detach-before-destroy |
| Data-driven content | Levels, atlases, balance, VFX, and UI content change more often than engine structure | A common level-source adapter target over LDtk/legacy readers, JSON atlas/config loaders, factory translation |
| Reversible authoring | Map edits need reliable undo and redo | Command history plus Composite transactions for grouped paste operations |
| Frame-time predictability | Dense combat and particle effects should not repeatedly scan or allocate | Quadtree broad phase, solid-tile grid, object pool, preallocated layered render buffers |
| Render-thread safety | Image decoding can run in parallel, but GPU resources belong to the graphics context | Worker decode queue and main-thread upload queue |
| Failure-tolerant persistence | A crash or unavailable service must not erase local progress or rewards | Temporary/backup save protocol and local-first Survival submission |
| Mode autonomy | Adventure, editor, and Survival3D have different simulation and presentation requirements | Separate controllers and views behind one application shell |
| Extensible gameplay | New characters, bosses, enemies, items, and map content should reuse common lifecycles | Entity inheritance, skill strategies, boss template method, centralized factories |

## 3.2 Ownership as the primary design rule

The most important rule is that state ownership and state observation are different. `GameController` exclusively owns the active `GameState`. `GameState` exclusively owns players and entities. `Player` exclusively owns its current `CharacterSkillSet`. Editor history exclusively owns command objects. The particle pool owns every allocated particle even when a raw pointer appears in the active list.

Views intentionally do not own gameplay entities. `GameView` stores pointers to tile vectors and the entity vector; character/entity renderers register raw `Entity*` references keyed by entity ID. This avoids copying a complete world every frame, but it creates a lifetime contract: all presentation references must be cleared before the model owner releases its objects. `GameController::StartLevel` and `Shutdown` implement this contract explicitly.

Shared ownership is reserved for reusable presentation resources. `AssetManager` caches `shared_ptr<TextureAtlas>` objects, atlases share animation clips, and animators hold shared clips. This is a good fit because the same immutable atlas/clip can be referenced by many render instances, and destroying one entity must not unload a resource still in use elsewhere.

## 3.3 Separation by lifecycle

The modules are separated less by abstract layer purity than by lifecycle:

- The application shell owns the window and chooses exactly one active surface.
- A controller owns or coordinates the state required for one surface.
- A model survives as long as its gameplay session or saved profile.
- A view owns GPU-facing resources and survives while the OpenGL context exists.
- A worker owns only CPU/network work and hands results to the main thread.
- The backend process owns server validation and its own durable JSON store.

This lifecycle view explains the shutdown order in `main.cpp:447-479`: detach gameplay/editor references, release view textures, stop the run service, clear shared atlases and audio, then destroy the renderer and window.

## 3.4 Determinism and bounded work

Adventure uses the frame delta but clamps large values before physics. It also establishes a safe moment for spatial indexing: entities move first, the quadtree is rebuilt, combat queries run, and removals occur later. Survival3D goes further by accumulating real frame time and advancing gameplay in 1/60-second ticks, with at most six catch-up ticks. Excess accumulated time is discarded rather than allowing an unbounded spiral of death.

Bounded work appears in other components as well. Render submission uses preallocated per-layer buffers and tracks dropped submissions. Leaderboard results have limits. Server input is validated. Animation transition requests use explicit priorities and stale-request rejection. These are examples of designing for worst-case behavior rather than only the expected path.

## 3.5 Pattern selection principle

Patterns are used where they encode a concrete invariant:

- Command encodes “an editor action can be replayed and reversed.”
- Composite encodes “many commands can behave as one command.”
- Adapter encodes “incompatible level sources produce one internal `GameState` contract.”
- Factory encodes “construction defaults belong in one creation boundary.”
- Object Pool encodes “high-frequency transient objects reuse storage.”
- Template Method encodes “all bosses share an update skeleton but specialize phase logic.”
- Strategy encodes “a player owns one interchangeable class-specific skill behavior.”
- Singleton encodes “one process-level service instance coordinates a global device or repository.”

The remainder of the report first explains the architecture and class diagrams, then returns to these patterns with participant-level reasoning.

# 4. System Architecture and Runtime Composition

## 4.1 MVC-inspired ownership diagram

```mermaid
%% id: runtime_mvc
classDiagram
direction LR

class MainLoop {
  +initialize()
  +dispatchActiveSurface()
  +shutdown()
}
class MenuController
class PrepareController
class ShopController
class MapBuilderController
class SurvivalController
class GameController {
  -unique_ptr~GameState~ m_gameState
  -CollisionSystem m_collision
  -ParticleSystem m_particles
  -ElementalSystem m_elemental
  +StartLevel(level)
  +Update(dt)
  +Render()
}
class GameState {
  -unique_ptr~Player~ m_localPlayer
  -vector~EntityPtr~ m_entities
  -vector~Tile~ m_tiles
  +Update(dt)
}
class GameView {
  -TileVector* m_tiles
  -EntityVector* m_entities
  +Render(camera, particles, dt)
}
class SaveManager
class Renderer
class AssetManager

MainLoop ..> MenuController : dispatch
MainLoop ..> PrepareController : dispatch
MainLoop ..> ShopController : dispatch
MainLoop ..> MapBuilderController : dispatch
MainLoop ..> SurvivalController : dispatch
MainLoop ..> GameController : dispatch
GameController *-- GameState : owns
GameController ..> GameView : binds and renders
GameController ..> SaveManager : progression
GameView --> GameState : non-owning projection
GameView ..> Renderer : submits
MainLoop ..> AssetManager : preload and shutdown
```

Figure 1 emphasizes the central ownership relationship. `main.cpp` retrieves process-wide controller instances and dispatches one based on application-mode flags. Only `GameController` owns the Adventure `GameState`; `GameView` observes it. `MapBuilderController` separately owns an editor `GameState` and hands a temporary saved map to `GameController` during playtest.

**Primary evidence:** `src/main.cpp:268-444`; `include/Controller/GameController.h:50-69,236-240`; `include/Model/GameState.h:35-42`; `include/View/GameView.h:42-51,97-101`.

