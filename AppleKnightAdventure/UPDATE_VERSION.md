# UPDATE_VERSION — Asset Replacement Complete

## Files Created
**Textures (39 PNG files):**
- `player.png` — merged from 7 noBKG Knight strips (3168×448, vertical stack)
- `tiles/light_tileset.png`, `tiles/shadow_tileset.png` — brackeys world tileset
- `enemies/melee.png`, `enemies/ranged.png`, `enemies/flying.png` — Monster Creatures Fantasy skeleton/goblin/flying eye
- `enemies/melee_idle.png`, `enemies/ranged_idle.png`, `enemies/ranged_run.png` — dungeon crawler slime/goblin side sprites
- `pets/baby_dragon.png`, `fairy.png`, `ghost.png`, `skull.png`, `wisp.png`
- `items/coin.png`, `apple.png`, `potion.png`, `key.png`, `equipment.png`, `bag_coins.png`, etc.
- `objects/chest.png`, `chest_open.png`, `checkpoint_captured.png`, `checkpoint_uncaptured.png`
- `projectiles/fire_bullet.png`, `explosion.png`, `slash.png`, `hit.png`, `arrow.png`
- `ui/health_ui.png`, `menu_button.png`, `pause_button.png`

**JSON atlases (39 files):**
- `player.json` — 96 frames across 7 clips (idle, run, attack, death, jump_fall, roll, shield)
- `enemies/*.json` — per-enemy atlas with frame definitions
- `pets/*.json` — single-frame + wisp multi-frame atlases
- `items/*.json` — single-frame + coin spin multi-frame atlas
- `objects/*json` — checkpoint animated + chest single-frame atlases
- `projectiles/*.json` — explosion/slash/hit multi-frame + single-frame atlases
- `tiles/*.json` — tileset atlases

## What Was Changed & Why
- **Deleted old textures** — replaced all placeholder/old assets with sourced counterparts
- **Merged player strips** — `noBKG_Knight*.png` combined into one `player.png` vertically, each row = one animation
- **Created JSON atlases** — every PNG has a matching `.json` with `image`, `frames`, and `clips` in the format expected by `TextureAtlas::LoadFromJSON()`
- **UI atlas** — `ui_atlas.json` references `health_ui.png`; separate `menu_button.json` and `pause_button.json`

## Current Status
- **Done:** All asset files populated, all JSON atlases valid
- **Blocked:** Boss sprite (Dark Fantasy Big Boss on itch.io), brick wall tileset (LPC OpenGameArt), parallax sky background (CraftPix) — require user to download via browser
