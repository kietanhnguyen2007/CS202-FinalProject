# UPDATE_VERSION — LDtk Crash Fix

## Files Modified

| File | Change |
|------|--------|
| `src/Factories/LevelFactory.cpp` | Fixed 5 LDtk JSON parsing bugs |

## What Was Changed & Why

### Bug 1 — `__tilesetDefUid` null crash (primary crash cause)
`layer.value("__tilesetDefUid", -1)` throws `nlohmann::json type_error.302` when the key
exists but its value is JSON `null` (present on the **Entities** and **Collision** layers
in `world.ldtk`). `value()` only falls back for **absent** keys, not null ones.

**Fix:** Replaced with explicit `contains()` + `is_null()` guard before calling `.get<int>()`.

### Bug 2 — `intGridCsv` null element crash
`csv[i].get<int>()` throws when LDtk emits a null token for an empty cell.

**Fix:** Added `if (csv[i].is_null()) continue;` inside the loop, plus a pre-check for the
`intGridCsv` key's existence.

### Bug 3 — `uid` in `defs.tilesets` accessed without null-check
`ts["uid"].get<int>()` would throw if a partially-constructed tileset entry had a null uid.

**Fix:** Added `!ts.contains("uid") || ts["uid"].is_null()` guard before reading. Applied
in both `BuildTileTypeMap()` and `LoadLDtkLevel()`.

### Bug 4 — Wrong positional `uid → tileType` mapping
The old code mapped tilesets by **position index** (idx = 1, 2, 3…) over all 21 entries
in `defs.tilesets` (including thumbnail tilesets uid 7–21). This could map thumbnail
tilesets to gameplay tileType slots, breaking tile rendering.

**Fix:** Map UID directly as tileType — only for UIDs 1–6, which match the 6 tilesheets
pre-loaded in `GameView::LoadTileset()` (Tiles.png, Buildings.png, Hive.png, Interior-01.png,
Props-Rocks.png, Tree-Assets.png). UIDs 7+ are ignored.

### Bug 5 — Silent discard when no `SpawnSolo` entity in LDtk
When `hasLocalSpawn == false`, all parsed tile data was silently thrown away and
`CreateDefaultLevel()` returned, with no indication of what went wrong.

**Fix:** Added `TraceLog(LOG_WARNING, ...)` with a clear message directing the developer
to add a `SpawnSolo` entity in the LDtk editor before the fallback return.

## Current Status

- Code compiles (no new APIs introduced; only null-guard conditionals and `TraceLog()`).
- The primary `type_error.302` crash on play mode entry is fixed.
- **Action required:** Open `world.ldtk` in the LDtk editor and place a **`SpawnSolo`**
  entity on the `Entities` layer so the level loads correctly (otherwise the fallback
  default level is used and a warning is printed to the console).
