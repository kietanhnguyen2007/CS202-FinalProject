# Apple Knight Adventure - Update Status

## Files Modified
- `include/Utils/Constants.h`
- `src/Factories/LevelFactory.cpp`

## What was changed and why
- Changed `TILE_SIZE` from `64` to `64.0f * (2.0f / 3.0f)` (which is `~42.66`). 
- Updated `LevelFactory.cpp` to use `16` directly for LDtk's `__gridSize` default value instead of relying on `TILE_SIZE` to avoid float-to-int type deduction warnings with nlohmann::json.
- **Why:** The user requested to scale down the tileset size to 2/3 of its current size. Modifying `TILE_SIZE` ensures both rendering logic and physics/collision logic scale harmoniously to prevent visual gaps.

## Current Status
- Tileset scale is successfully changed to 2/3. `TILE_SIZE` is now a floating point constant.
