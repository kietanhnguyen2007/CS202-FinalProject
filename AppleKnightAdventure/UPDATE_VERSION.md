# UPDATE_VERSION — LDtk Integration

## Files Modified

| File | Thay đổi |
|---|---|
| `include/Model/GameState.h` | Thêm `flipFlags` vào `struct Tile` |
| `include/Factories/LevelFactory.h` | Thêm `LoadLDtkLevel()`, `LoadLDtkDualWorld()`, `ParseBackgroundTheme()`, `BuildTileTypeMap()`; cập nhật `LoadLevel()` signature |
| `src/Factories/LevelFactory.cpp` | Implement toàn bộ: `ParseBackgroundTheme`, `BuildTileTypeMap`, LDtk auto-detect trong `LoadLevel()`, `LoadLDtkLevel()` (full entity parser), `LoadLDtkDualWorld()` |
| `include/Controller/GameController.h` | Thêm `class Boss` forward decl + `RegisterBossVisuals()` |
| `src/Controller/GameController.cpp` | `GetLevelPath()` ưu tiên `world.ldtk`; `StartLevel()` dùng ldtkLevelIndex; `LoadTilesets()` xóa hardcode background; `RegisterBossVisuals()` mới; `RegisterEntityVisuals()` thêm case Boss/FakeWall |
| `src/View/GameView.cpp` | `RenderTilemap()` dùng `DrawTexturePro` với flip flags thay `SubmitSprite` |

## Files Created

*(Không có file mới — `world.ldtk` cần tạo bằng LDtk editor)*

## What Changed & Why

- **LDtk auto-detect**: `LoadLevel()` kiểm tra extension `.ldtk` → gọi `LoadLDtkLevel()`, ngược lại fallback sang `.lvl` cũ — **backward compatible 100%**
- **Entity parser đầy đủ**: Xử lý 19 entity identifier: SpawnSolo/Guide/Warrior/DualLight/DualShadow, EnemyMelee/Ranged/Flying, Boss1/Boss2/Boss3, Chest, CheckpointMid/End, FakeWall, ItemCoin/Apple/Key/Potion/Equipment
- **Auto-count**: Nếu `TotalItems/TotalEnemies = 0` trong LDtk field → đếm tự động từ entity list
- **Background từ theme**: `StartLevel()` gọi `LoadBackgrounds(theme)` từ `GameState.GetBackgroundTheme()` — xóa hardcode và TODO comment cũ
- **Flip flags**: `RenderTilemap()` dùng `DrawTexturePro` với `src.width/height` âm để flip, hỗ trợ LDtk `"f"` field

## Current Status

✅ **Code hoàn chỉnh, sẵn sàng build**

**Bước tiếp theo (do Designer):**
1. Tải LDtk editor: https://ldtk.io
2. Tạo project `assets/levels/world.ldtk`
3. Import tileset theo đúng thứ tự (Tiles → Buildings → Hive → Interior-01 → Props-Rocks → Tree-Assets)
4. Tạo layer: BG_Tiles, Collision (IntGrid), Tiles, Entities
5. Tạo entity definitions: 19 entities + level fields
6. Vẽ Level 1 (40×18 tiles = 2560×1152 px)
