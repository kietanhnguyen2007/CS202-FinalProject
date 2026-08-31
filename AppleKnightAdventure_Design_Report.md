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

