# UPDATE_VERSION — Sound Asset Overhaul

## Files Modified
- `src/Controller/GameController.cpp`
- `src/Controller/MenuController.cpp`

## Files Created
- `AppleKnightAdventure/SOUND_ASSET_GUIDE.md` — hướng dẫn tải sound asset

## Files Deleted
- `assets/sounds/sfx/*.wav` (23 dummy files — tất cả cùng kích thước 121 946 bytes)
- `assets/sounds/music/*.wav` (3 dummy files)

## What Changed and Why

### 1. Xóa dummy sound files
Toàn bộ 26 file `.wav` trong `assets/sounds/` là placeholder rỗng (cùng kích thước 121 946 bytes).
Đã xóa để tránh nhầm lẫn và chuẩn bị chỗ cho sound asset thật.

### 2. Rewrite toàn bộ sound wiring trong `GameController::Init()`
- **Cũ:** 6 LoadSound + 1 LoadMusic (WAV)
- **Mới:** 26 LoadSound + 3 LoadMusic (BGM dùng .ogg, centralized)
- Music load tập trung tại đây thay vì phân tán (MenuController không còn LoadMusic)
- Thêm đủ 6 nhóm: Player, Enemy, Item/World, GameState, Elemental, Pet, UI

### 3. Thêm PlaySound calls còn thiếu trong `GameController`
| Hàm | Call thêm |
|-----|-----------|
| `HandlePlayerInput()` | `player_jump` khi jump, `player_dash` khi dash |
| `UpdateCombat()` | `player_die` khi player hết máu |
| `UpdateItems()` | Tách `item_pickup` (Apple/Key/...) khỏi `coin_pickup` (Coin) |
| `UpdateInteractions()` | `checkpoint_activate` (chỉ khi chưa active, tránh spam) |
| `CheckLevelComplete()` | `level_complete` |
| `SpawnPet()` | `pet_summon` |
| `FireDragonProjectile()` | `pet_dragon_fire` |

### 4. Rewrite `MenuController`
- Xóa `LoadMusic` duplicate (nay tập trung ở GameController)
- Thêm `ui_hover` khi navigate menu
- Thêm `ui_confirm` khi chọn menu item

### 5. Tạo SOUND_ASSET_GUIDE.md
Hướng dẫn chi tiết từng file: link tải, tên file cần chọn, tên đặt lại.
Sources: Kenney.nl (CC0), Incompetech (CC-BY), Freesound, OpenGameArt, Pixabay.

## Current Status
- ✅ Code wiring hoàn chỉnh — 29 sound keys được định nghĩa
- ✅ Không còn wire cũ/dummy
- ⏳ Cần tải sound asset thật theo `SOUND_ASSET_GUIDE.md`
- ⏳ `game_over` chưa có trigger trong code (cần thêm khi ResultView::ShowGameOver được implement)
- ⏳ `pet_ghost_heal` chưa có trigger (cần thêm trong Pet heal tick logic)
- ⏳ BGM Boss chưa có trigger (cần thêm khi Boss spawn/detect)
