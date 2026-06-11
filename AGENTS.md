# Apple Knight Adventure — Agent Guide

## Workflow rules

1. **No automatic git operations or binary execution.** After every successful feature
   update, tell the user to stage and commit with `git add` and `git commit`. Never run
   these commands or the compiled game binary yourself.
2. **Write `UPDATE_VERSION` on success.** After every successful update, create
   `AppleKnightAdventure/UPDATE_VERSION` (Markdown) summarizing:
   - Files created/modified
   - What was changed and why
   - Current status
3. **Backend scope only.** This agent is responsible for files under `include/Model/`,
   `include/Systems/`, `include/Network/`, `include/Factories/` and their `.cpp`
   counterparts. **Do not modify** `include/View/` or `include/Controller/`.

## Project state

**All `.h` and `.cpp` files are stubs** (`// write here`). The project is a blank scaffold ready for implementation. Entry point: `src/main.cpp`.

## Stack

- C++17 + raylib (graphics, audio, input)
- No other dependencies. No package manager. Link raylib manually.
- No CI, no formatter/linter config, no test framework.

## Architecture: MVC

```
include/Model/     — data + business logic (no render deps)
include/View/      — passive rendering (reads Model, draws)
include/Controller/ — input handling + game loop orchestration
include/Network/   — TCP multiplayer (Server/Client/Packet serialization)
include/Systems/   — cross-cutting: collision (Quadtree), particles, sound (singleton),
                     elemental reactions (Fire/Water/Thunder), object pool, observable list
include/Factories/ — EnemyFactory, ItemFactory, LevelFactory
```

- Controller mutates Model → Controller calls View → View reads Model → Render.
- `GameController.h/.cpp` = main loop (init → run → shutdown).
- `MenuController.h/.cpp` = menu loop.

## Build

```bash
# Requires: raylib, C++17 compiler. Build command TBD.
# CMakeLists.txt is empty — set up build before implementing.
```

## Notable design features (from README)

- **DualWorld** — co-op levels with Light/Shadow layer switching; `DualWorldPlayer` extends Player with `WorldLayer`.
- **ElementalSystem** — DamagePacket, StatusEffect (Burn/Wet/Shocked), reactions (Vaporize/Conduct/Overload).
- **ObservableList** — generic reactive container adapted from C#; used by Inventory/SkillBar for auto UI updates.
- **ObjectPool** — reuses Projectile/Particle/Effect to avoid runtime allocation.
- **Quadtree** — spatial partitioning for O(n log n) collision.
- **Network** — TCP; `Packet` serializes int/float/bool/string; server-authoritative combat reactions.
- **Checkpoint** — respawn system.
- **Chest** — random loot (coin/apple/key/equip).
- **Pet** — auto-AI follower (Skull/Ghost/BabyDragon/Fairy).
- **LevelScoring** — 1–3 star rating + high score.

## Test & lint

- `tests/` is empty (only `.gitkeep`). No test framework chosen.
- No lint/format config exists.
