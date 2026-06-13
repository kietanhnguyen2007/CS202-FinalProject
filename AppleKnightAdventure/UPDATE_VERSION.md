# UPDATE_VERSION — View Code Fix (2026-06-13)

## Files modified
- `assets/textures/enemies/ranged_run.json` — clip name `idle` → `run`
- `assets/textures/enemies/status_atlas.json` — new atlas for 3 status icons (burn/wet/shocked)
- `assets/textures/enemies/status_atlas.png` — 48×16 PNG generated via .NET
- `include/View/CharacterRenderer.h` — added `MergeAtlas()` declaration
- `src/View/CharacterRenderer.cpp` — added `MergeAtlas()` impl; clip fallback chains (walk→run, jump→jump_fall, fall→jump_fall, hurt→hit→idle, dead→death, skill→attack); PlayAction fallback
- `include/View/GameView.h` — added background parallax struct/members/methods; kept public getters
- `src/View/GameView.cpp` — fixed paths (player.json, boss/*.json, pets/baby_dragon.json, boss/boss_attack.png, projectiles/arrow.png); added shadow tileset (tileType 2); added LoadBackgrounds()/RenderBackground()/SetActiveBackground(); added EnemyStatusRenderer atlas load; background unload in Shutdown

## What changed & why
- All asset paths now point to real files (no more `boss/boss.json`, `babydragon.json`, `melee.json`)
- Boss clips are separate atlases (`idle.json`, `walk.json`, `attack.json`, `death.json`, `hit.json`) loaded via PreloadAtlas — MergeAtlas() combines them into one entity's animator at runtime
- Clip fallback chains handle naming mismatches between code expectations and asset clip names
- Background parallax renders 4 backgrounds (bg_1–4) each with multiple layers at different scroll speeds
- Shadow tileset is now loaded (tileType=2) for DualWorld shadow layer rendering

## Current status
- All View layer asset references now match actual files
- Background rendering added (before BeginMode2D)
- Status atlas icons optional — text fallback works if PNG missing
- Flying enemy still needs Controller-side clip mapping (Idle→"fly")
- GameController.cpp/MenuController.cpp remain stubs — game not runnable
