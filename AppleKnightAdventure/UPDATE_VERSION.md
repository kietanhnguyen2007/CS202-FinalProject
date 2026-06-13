# UPDATE_VERSION — Convert View layer to use sprite assets

## Files modified

| File | Change |
|---|---|
| `include/View/HUDView.h` | Added `TextureAtlas`, `Animator` includes; atlas members `m_uiAtlas`, `m_coinAtlas`, `m_coinAnim` |
| `src/View/HUDView.cpp` | `LoadResources` loads `ui_atlas.json` + `items/coin.json`; `Render` draws HP bar via `health_bar` sprite + coin animated icon |
| `include/View/MenuView.h` | Added `TextureAtlas` include; `m_menuBtnAtlas`, `m_pauseBtnAtlas` members |
| `src/View/MenuView.cpp` | `LoadResources` loads `menu_button.json` + `pause_button.json`; `RenderMain`/`RenderPause` draw button sprites with text labels |
| `include/View/ResultView.h` | Added `TextureAtlas` include; `m_uiAtlas` member |
| `src/View/ResultView.cpp` | `LoadResources` loads `ui_atlas.json`; panel background uses `health_bar` sprite instead of raw rectangle |
| `include/View/SkillBarView.h` | Added `TextureAtlas`, `Animator` includes; `SkillIcon` struct; `m_skillIcons` vector |
| `src/View/SkillBarView.cpp` | `LoadResources` loads `fire_bullet.json` (Fireball), `potion.json` (Heal), `slash.json` (Dash), `hit.json` (Shield); renders animated icons in skill slots |
| `include/View/InventoryView.h` | Added `TextureAtlas`, `Animator` includes; `ItemIconInfo` struct; `m_itemIcons` map |
| `src/View/InventoryView.cpp` | `LoadResources` loads 8 item atlases; `Render` draws animated item icons in grid; coin uses `spin` clip |
| `src/View/GameView.cpp` | Added preload calls for all 8 item atlases + 5 projectile atlases in `Init()` |
| `include/View/ParticleRenderer.h` | Added `TextureAtlas`, `Animator` includes; `ProjectileAnim` struct; `m_projectileAnims` map |
| `src/View/ParticleRenderer.cpp` | `InitProjectileAtlases` loads `explosion.json` (BossAttack) + `arrow.json` (Magic); `RenderAll` uses animated atlas sprites, falls back to static textures |

## Summary

- **HUDView**: HP bar now uses `health_bar` sprite (80×16) from `ui_atlas.json`. Coin icon animates with `spin` clip.
- **MenuView**: Menu items rendered as `menu_button` sprites (96×32). Pause overlay uses `pause_button` sprites.
- **ResultView**: Result panel uses `health_bar` sprite stretched as background texture.
- **SkillBarView**: Skill icons loaded from projectile/item atlases. Fireball→fire_bullet (static), Heal→potion (static), Dash→slash (animated), Shield→hit (animated).
- **InventoryView**: 8 item atlases loaded. Each inventory slot shows item icon. Coin uses animated `spin` clip.
- **GameView**: Preloads item and projectile atlases into `CharacterRenderer`'s atlas cache.
- **ParticleRenderer**: BossAttack projectile now uses `explosion.json` animation (7-frame `explode` clip). Magic uses `arrow.json` atlas. Falls back to original static textures if atlas not found.

## Status

All View UI components now load and render from sprite assets instead of procedural shapes. Animated items (coin spin, slash, hit, explosion) update per frame via `Animator`.
