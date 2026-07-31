# Hướng Dẫn Build Map LDtk — Apple Knight Adventure

> Tài liệu này mô tả **đúng theo source code** (`LevelFactory.cpp`, `GameView.cpp`,
> `GameController.cpp`) để đảm bảo map LDtk tương thích hoàn toàn với game.

---

## Mục lục
1. [Thiết lập Project LDtk](#1-thiết-lập-project-ldtk)
2. [Cấu trúc Layer bắt buộc](#2-cấu-trúc-layer-bắt-buộc)
3. [Tileset UID — ánh xạ quan trọng](#3-tileset-uid--ánh-xạ-quan-trọng)
4. [Layer: Collision (IntGrid)](#4-layer-collision-intgrid)
5. [Layer: Tiles (tile chính)](#5-layer-tiles-tile-chính)
6. [Layer: BG\_Tiles (tile nền)](#6-layer-bg_tiles-tile-nền)
7. [Layer: Entities](#7-layer-entities)
8. [Level Field Instances](#8-level-field-instances)
9. [Ánh xạ Level Index → Level Number](#9-ánh-xạ-level-index--level-number)
10. [DualWorld Layers (tuỳ chọn)](#10-dualworld-layers-tuỳ-chọn)
11. [Checklist trước khi lưu](#11-checklist-trước-khi-lưu)
12. [Lỗi thường gặp](#12-lỗi-thường-gặp)

---

## 1. Thiết lập Project LDtk

| Thuộc tính | Giá trị bắt buộc |
|-----------|-----------------|
| File path | `assets/levels/world.ldtk` |
| Grid size  | **64 px** (= `TILE_SIZE` trong code) |
| World layout | Free / Horizontal — tuỳ ý |

> **Lưu ý:** Game luôn ưu tiên `assets/levels/world.ldtk`. Nếu file tồn tại, mọi
> file `.lvl` cũ đều bị bỏ qua.

---

## 2. Cấu trúc Layer bắt buộc

Mỗi Level trong LDtk **phải có** đúng các layer sau với tên và kiểu khớp chính xác:

| Tên Layer (identifier) | Kiểu LDtk | Bắt buộc? | Mô tả |
|------------------------|-----------|-----------|-------|
| `Collision` | **IntGrid** | ✅ Bắt buộc | Xác định ô nào là solid |
| `Tiles` | **Tiles** | ✅ Bắt buộc | Tile foreground hiển thị |
| `BG_Tiles` | **Tiles** | ⭕ Tuỳ chọn | Tile background (không có collision) |
| `Entities` | **Entities** | ✅ Bắt buộc | Spawn player, kẻ địch, vật phẩm |
| `LightTiles` | **Tiles** | ⭕ DualWorld | Layer Light cho chế độ DualWorld |
| `ShadowTiles` | **Tiles** | ⭕ DualWorld | Layer Shadow cho chế độ DualWorld |

> ⚠️ **Tên identifier phân biệt hoa thường.** Ví dụ `collision` ≠ `Collision`.

---

## 3. Tileset UID — ánh xạ quan trọng

Code ánh xạ **UID của tileset trong LDtk trực tiếp làm `tileType`** (chỉ UID 1–6 được
nhận dạng). Các tileset phải được thêm vào LDtk project **với đúng UID** sau:

| UID LDtk | File Texture | Số cột | Ghi chú |
|----------|-------------|--------|---------|
| **1** | `assets/textures/tiles/Tiles.png` | 25 | Tileset chính (Forest) |
| **2** | `assets/textures/tiles/Buildings.png` | 25 | Công trình |
| **3** | `assets/textures/tiles/Hive.png` | 25 | Tổ ong / hang động |
| **4** | `assets/textures/tiles/Interior-01.png` | 25 | Nội thất |
| **5** | `assets/textures/tiles/Props-Rocks.png` | 18 | Đá / props |
| **6** | `assets/textures/tiles/Tree-Assets.png` | 21 | Cây cối |

> UID phải đúng vì code dùng `uidToTileType[uid] = uid` (chỉ nhận 1–6).
> Thumbnail/preview tileset (uid 7+) bị bỏ qua hoàn toàn.

**Cách kiểm tra UID trong LDtk:**
`Project Settings → Tilesets → (chọn tileset) → UID hiển thị ở góc trên`

---

## 4. Layer: Collision (IntGrid)

- **Kiểu:** IntGrid
- **Grid size:** 64 px
- **Tileset:** Không cần gán tileset (`__tilesetDefUid = null` là bình thường)

### Giá trị ô IntGrid

| Giá trị | Ý nghĩa |
|---------|---------|
| `0` | Không khí (không solid) |
| `1` | Solid (ô chặn nhân vật) |

> Code đọc `intGridCsv` — chỉ các ô có giá trị `!= 0` mới được đánh dấu solid.
> Ô `0` và ô `null` đều được bỏ qua an toàn.

**Lưu ý quan trọng:**
Layer `Tiles` và `BG_Tiles` dùng `Collision` để quyết định tile nào solid:
- Tile ở `Tiles` layer → **solid** nếu ô Collision tương ứng `!= 0`
- Tile ở `BG_Tiles` layer → **không bao giờ solid** (background thuần)

---

## 5. Layer: Tiles (tile chính)

- **Kiểu:** Tiles
- **Identifier:** phải là `Tiles` (chính xác)
- **Grid size:** 64 px
- **Tileset:** Gán một trong 6 tileset (UID 1–6)

Mỗi tile được vẽ sẽ tự động lấy solid state từ layer `Collision`.
Hỗ trợ **flip** (LDtk field `f`):

| Giá trị `f` | Hiệu ứng |
|------------|---------|
| `0` | Bình thường |
| `1` | Lật ngang (flip X) |
| `2` | Lật dọc (flip Y) |
| `3` | Lật cả hai |

---

## 6. Layer: BG\_Tiles (tile nền)

- **Kiểu:** Tiles
- **Identifier:** phải là `BG_Tiles` (chính xác)
- **Grid size:** 64 px
- **Tileset:** Gán một trong 6 tileset (UID 1–6)

Tile ở đây **không bao giờ solid** dù Collision layer có giá trị. Dùng để vẽ
trang trí nền phía sau nhân vật.

---

## 7. Layer: Entities

- **Kiểu:** Entities
- **Identifier:** phải là `Entities` (chính xác)
- **Grid size:** 64 px

### 7.1 Spawn Points (bắt buộc có ít nhất một)

| Entity Identifier | Mô tả | Khi nào dùng |
|------------------|-------|-------------|
| `SpawnSolo` | Vị trí spawn Player (chế độ 1 người) | **Bắt buộc** cho single-player |
| `SpawnGuide` | Vị trí spawn Host (chế độ multiplayer) | Multiplayer Host |
| `SpawnWarrior` | Vị trí spawn Client (chế độ multiplayer) | Multiplayer Client |
| `SpawnDualLight` | Spawn DualWorldPlayer ở lớp Light | DualWorld mode |
| `SpawnDualShadow` | Spawn DualWorldPlayer ở lớp Shadow | DualWorld mode |

> ⚠️ **Nếu không có spawn entity nào phù hợp, toàn bộ tile data bị huỷ và game
> báo warning rồi load level mặc định!**
> Luôn đặt `SpawnSolo` cho single-player map.

**Lời khuyên vị trí:** Đặt spawn trên một ô solid trong Collision layer, tránh
đặt giữa không trung (nhân vật sẽ rơi xuống ngay khi spawn).

---

### 7.2 Kẻ địch

| Entity Identifier | Kiểu kẻ địch | Tốc độ | Tầm tấn công |
|------------------|-------------|--------|-------------|
| `EnemyMelee` | Melee (cận chiến) | 120 px/s | 100 px |
| `EnemyRanged` | Ranged (tầm xa, ném bom) | 80 px/s | 300 px |
| `EnemyFlying` | Flying (bay, có projectile) | 100 px/s | 300 px |

Không có field instance tuỳ chỉnh — spawn tại đúng toạ độ pixel LDtk.

---

### 7.3 Boss

| Entity Identifier | Kích thước hitbox | Field Instance | Mô tả field |
|------------------|-------------------|---------------|-------------|
| `Boss1` | 96 × 96 px | `PatrolRight` (int) | Toạ độ X pixel giới hạn phải |
| `Boss2` | 96 × 96 px | `PatrolRight` (int) | Toạ độ X pixel giới hạn phải |
| `Boss3` | 128 × 128 px | `PatrolRight` (int) | Toạ độ X pixel giới hạn phải |

**Cách thêm field `PatrolRight` trong LDtk:**
1. Chọn entity definition (Boss1/2/3) → **+ Add field**
2. Đặt tên: `PatrolRight`, kiểu: `Int`
3. Khi đặt Boss trong map, nhập giá trị pixel X tối đa mà boss có thể đi tới

> Nếu không đặt `PatrolRight`:
> - Boss1/2 mặc định patrol đến `spawnX + 400` px
> - Boss3 mặc định patrol đến `spawnX + 500` px

---

### 7.4 Interactables (Vật tương tác)

| Entity Identifier | Mô tả | Auto-count items? |
|------------------|-------|------------------|
| `Chest` | Hòm báu (loot ngẫu nhiên: coin/apple/key/equip) | ✅ Có |
| `CheckpointMid` | Checkpoint giữa màn (respawn point, không kết thúc màn) | ❌ Không |
| `CheckpointEnd` | Checkpoint cuối màn (kết thúc level) | ❌ Không |
| `FakeWall` | Tường giả (64×64, HP=3, có thể phá) | ❌ Không |

> **Lưu ý `CheckpointEnd`:** Đây là cách duy nhất để kết thúc level. Nếu không
> có `CheckpointEnd`, người chơi không thể qua màn.

---

### 7.5 Fixed Items (Vật phẩm cố định)

| Entity Identifier | Field Instance | Mô tả field | Auto-count? |
|------------------|---------------|-------------|------------|
| `ItemCoin` | `Amount` (int, mặc định 1) | Số lượng coin khi nhặt | ✅ Có |
| `ItemApple` | _(không có)_ | Apple hồi máu | ✅ Có |
| `ItemKey` | _(không có)_ | Chìa khoá mở Chest | ✅ Có |
| `ItemPotion` | _(không có)_ | Bình máu | ✅ Có |
| `ItemEquipment` | _(không có)_ | Trang bị | ✅ Có |

**Thêm field `Amount` cho `ItemCoin`:**
1. Entity definition `ItemCoin` → **+ Add field**
2. Tên: `Amount`, kiểu: `Int`, giá trị mặc định: `1`

---

## 8. Level Field Instances

Mỗi Level trong LDtk có thể khai báo các **Level Custom Fields** để điều khiển
game. Thêm ở `Level Settings → Custom Fields`:

| Field Identifier | Kiểu | Giá trị hợp lệ | Mô tả |
|-----------------|------|----------------|-------|
| `BackgroundTheme` | String | `Forest`, `ColdCorridor`, `Underwater` | Bộ parallax background |
| `PlayerClass` | String | `Knight`, `Fighter`, `Ninja`, `MagicCaster` | Class nhân vật mặc định |
| `TotalItems` | Int | Bất kỳ >= 0 | Override tổng item (0 = tự đếm) |
| `TotalEnemies` | Int | Bất kỳ >= 0 | Override tổng kẻ địch (0 = tự đếm) |

**Cách thêm Level Fields trong LDtk:**
`Right-click Level → Edit Level Fields → + Add field`

### Giá trị BackgroundTheme

| Giá trị chuỗi | Background được load |
|--------------|---------------------|
| `Forest` | `backgrounds/forest/` (back + middle + front) |
| `ColdCorridor` | `backgrounds/cold_corridor/` (5 layers) |
| `Underwater` | `backgrounds/underwater/` (4 layers) |

> Nếu không khai báo `BackgroundTheme`, mặc định là **Forest**.

### TotalItems / TotalEnemies

- Nếu đặt `> 0`: dùng giá trị này cho hệ thống chấm điểm (LevelScoring)
- Nếu để `0` hoặc không khai báo: **tự động đếm** từ các entity trong map

---

## 9. Ánh xạ Level Index → Level Number

Game load level theo công thức:

```
levelIndex = levelNumber - 1
```

| Level Number (game) | Level Index (LDtk array) | Tên Level LDtk (gợi ý) |
|--------------------|--------------------------|----------------------|
| 1 | 0 (phần tử đầu tiên) | `Level_0` hoặc `Level1` |
| 2 | 1 | `Level_1` hoặc `Level2` |
| 3 | 2 | `Level_2` hoặc `Level3` |

> Thứ tự các Level trong LDtk **World** quyết định index, không phải tên.
> Level đầu tiên trong danh sách = index 0 = level 1 của game.

---

## 10. DualWorld Layers (tuỳ chọn)

Chỉ dùng khi level có chế độ **DualWorld** (chuyển đổi Light/Shadow).

| Layer Identifier | Kiểu LDtk | World Layer |
|-----------------|-----------|-------------|
| `LightTiles` | Tiles | Light |
| `ShadowTiles` | Tiles | Shadow |

- Tileset gán cho cả hai layer: **UID 1** (`Tiles.png`)
- Cả hai layer đều solid = `true` (không đọc từ Collision)
- Spawn entity dùng `SpawnDualLight` hoặc `SpawnDualShadow` thay vì `SpawnSolo`

---

## 11. Checklist trước khi lưu

Trước mỗi lần save `world.ldtk`, kiểm tra:

- [ ] Layer `Collision` (IntGrid, grid 64px) tồn tại và có dữ liệu
- [ ] Layer `Tiles` (Tiles, grid 64px) được gán tileset UID 1–6
- [ ] Layer `Entities` (Entities, grid 64px) tồn tại
- [ ] Đã đặt ít nhất **một** `SpawnSolo` entity trên layer `Entities`
- [ ] Đã đặt ít nhất **một** `CheckpointEnd` để kết thúc màn
- [ ] Mỗi Boss có field `PatrolRight` hợp lệ (hoặc chấp nhận mặc định)
- [ ] `ItemCoin` có field `Amount` nếu muốn đặt số lượng khác 1
- [ ] Level Fields: `BackgroundTheme` và `PlayerClass` được khai báo nếu muốn override
- [ ] File được lưu tại `assets/levels/world.ldtk` (đúng đường dẫn)

---

## 12. Lỗi thường gặp

| Triệu chứng | Nguyên nhân | Cách sửa |
|------------|-------------|---------|
| Game báo `WARNING: LDtk Level X has no SpawnSolo entity` | Thiếu spawn entity | Thêm `SpawnSolo` vào layer `Entities` |
| Tile hiển thị sai (màu trắng/1x1) | UID tileset không khớp (1–6) | Kiểm tra UID trong `Project Settings → Tilesets` |
| Nhân vật xuyên qua tile | Layer `Collision` chưa vẽ hoặc sai identifier | Đặt đúng tên `Collision`, vẽ IntGrid đầy đủ |
| Level load xong nhưng hiển thị màn default | Tên layer sai (chữ hoa/thường) | Kiểm tra tên: `Tiles`, `BG_Tiles`, `Entities`, `Collision` |
| Boss không patrol | Field `PatrolRight` thiếu hoặc giá trị nhỏ hơn spawn X | Tăng `PatrolRight` > vị trí X spawn của Boss |
| `CheckpointEnd` không kết thúc màn | Sai identifier (vd: `CheckPointEnd`) | Dùng đúng `CheckpointEnd` |
| Level sau không load được | Level index vượt quá số level trong world | Thêm level mới trong LDtk tương ứng `levelNumber + 1` |

---

*Tài liệu được sinh từ source code — cập nhật khi `LevelFactory.cpp` thay đổi.*
