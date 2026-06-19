# Master Builder & Architecture Guide (Updated)
## Apple Knight Adventure — Assets mới + Gap Analysis

---

# PHẦN 1 — TILESET & BACKGROUND MỚI (đã cập nhật)

## 1.1 Tileset mới — `assets/textures/tiles/`

Bộ tileset cũ (60 tile, 10 cols) đã được **thay bằng 6 sprite sheet mới**, tất cả dùng chung tile size **16×16 px** và cấu trúc JSON frame-based.

| File | Frame name | Số tile | Cols × Rows | Dùng cho |
|---|---|---|---|---|
| `Tiles.png` + `Tiles.json` | `tile_0` … `tile_624` | **625 tile** | **25 × 25** | Nền sàn, tường, nền đất chính |
| `Buildings.png` + `Buildings.json` | `tile_0` … `tile_624` | 625 | 25 × 25 | Tòa nhà, cổng, tường gạch |
| `Hive.png` + `Hive.json` | `tile_0` … `tile_624` | 625 | 25 × 25 | Hang tổ ong, dungeon hữu cơ |
| `Interior-01.png` + `Interior-01.json` | `tile_0` … `tile_624` | 625 | 25 × 25 | Nội thất, sàn gỗ, đồ vật trong nhà |
| `Props-Rocks.png` + `Props-Rocks.json` | `tile_0` … `tile_377` | **378 tile** | **18 × 21** | Đá, cục đất, prop thiên nhiên |
| `Tree-Assets.png` + `Tree-Assets.json` | `tile_0` … `tile_524` | **525 tile** | **21 × 25** | Cây cối, tán lá, rễ cây |

> **TILE_SIZE:** Giữ nguyên `TILE_SIZE = 32` (wold grid). Source tile mới là **16×16 px**,  
> được tính động từ `texture.width / gridCols` và scale = `TILE_SIZE / srcTileSize` (≈ 2) khi render.

### Cách tileset được load

| tileType | File | gridCols |
|---|---|---|
| 1 | `Tiles.png` | 25 |
| 2 | `Buildings.png` | 25 |
| 3 | `Hive.png` | 25 |
| 4 | `Interior-01.png` | 25 |
| 5 | `Props-Rocks.png` | 18 |
| 6 | `Tree-Assets.png` | 21 |

> `tileType = 1-6` (không phải 0-5). `gridCols` khác nhau giữa các sheet.

### Tile struct — `GameState.h`

```cpp
struct Tile {
    int x = 0, y = 0;    // grid position
    int tileType = 1;     // 1=Tiles, 2=Buildings, 3=Hive, 4=Interior, 5=Rocks, 6=Trees
    int tileId   = -1;    // -1 = empty cell
    bool solid   = true;  // collision check
};
```

---

## 1.2 Background mới — `assets/textures/backgrounds/forest/`

Background cũ (bg_1…bg_4) đã được **thay bằng 1 bộ duy nhất: `forest/`** với 7 lớp parallax:

| File | Parallax speed | Nội dung |
|---|---|---|
| `far.png` | `0.05f` | Bầu trời xa, mây mù |
| `main.png` | `0.15f` | Nền chính forest |
| `tree_dark.png` | `0.30f` | Cây tối (silhouette xa) |
| `tree_golden.png` | `0.45f` | Cây vàng mùa thu |
| `tree_green.png` | `0.60f` | Cây xanh mùa hè |
| `tree_red.png` | `0.75f` | Cây đỏ mùa thu |
| `tree_yellow.png` | `0.90f` | Cây vàng nhạt |

> Bộ background này là **forest theme duy nhất**. **Tất cả 7 layer được load cùng lúc** (không chọn variant).

---

# PHẦN 2 — HƯỚNG DẪN TÍCH HỢP & SỬ DỤNG

Tài liệu này hướng dẫn cách thiết lập 5 hệ thống: **Map**, **Entity**, **Menu**, **Inventory**, và **Score Table**.

> Bạn **không cần** đọc code C++ trong thư mục `View/` để sử dụng tài liệu này.  
> Tất cả đều dùng data format (file `.lvl`) + bảng tham khảo + quy tắc thiết kế.

---

## 2.1 Map System

### 2.1.1 File `.lvl` — Level format

Mỗi level là một file text `.lvl` trong `assets/levels/`. Cú pháp từng dòng:

```
maptype <0|1|2>
width <số_cột>
height <số_hàng>
tile <x> <y> <tileType> <tileId> <solid>
spawn_solo <tx> <ty>
spawn_guide <tx> <ty>
spawn_warrior <tx> <ty>
enemy <type> <tx> <ty>
boss <id> <tx> <ty>
chest <tx> <ty>
checkpoint <tx> <ty>
scoring <totalItems> <totalEnemies>
```

#### Ý nghĩa từng token

| Token | Mô tả |
|---|---|
| `maptype` | 0=Flexible, 1=GuideSpecial, 2=WarriorSpecial |
| `width` / `height` | Kích thước grid tính bằng ô tile (ví dụ 40×30) |
| `tile` | Một ô tile tại (x,y): tileType=1..6 (chọn sheet), tileId=index trong sheet, solid=0/1 |
| `spawn_solo` | Điểm spawn cho chế độ 1 người (chỉ dùng khi maptype=0) |
| `spawn_guide` | Điểm spawn cho Guide (chế độ Co-op host) |
| `spawn_warrior` | Điểm spawn cho Warrior (chế độ Co-op client) |
| `enemy` | Loại enemy: melee / ranged / flying |
| `boss` | Boss: id=1 (Knight), 2 (Witch), 3 (4-phase) |
| `chest` | Rương ngẫu nhiên |
| `checkpoint` | Điểm hồi sinh |
| `scoring` | Số item tối đa + số enemy tối đa trong level |

#### Quy tắc

- Toạ độ `x y` tính bằng **ô tile** (0-based). Game tự nhân với `TILE_SIZE=32` để ra pixel.
- `spawn_solo` chỉ được đọc khi maptype=0 (Flexible). Nếu maptype=1 hoặc 2, token này bị bỏ qua.
- `spawn_guide` chỉ được đọc khi GameMode=AsymCoopHost.
- `spawn_warrior` chỉ được đọc khi GameMode=AsymCoopClient.
- **Mỗi level phải có ít nhất 1 spawn token hợp lệ**, nếu không game sẽ dùng level mặc định.

#### File mẫu — `assets/levels/level1.lvl`

```
maptype 0
width 40
height 30
tile 0 0 1 0 1
tile 1 0 1 1 1
tile 2 0 1 2 1
tile 0 1 1 25 1
tile 1 1 1 26 0
tile 2 1 1 27 1
spawn_solo 5 10
spawn_guide 5 10
spawn_warrior 15 10
enemy melee 20 15
enemy ranged 22 15
boss 1 20 15
chest 18 12
checkpoint 10 20
scoring 10 15
```

### 2.1.2 Tile sheets — Ánh xạ tileType

Mỗi `tileType` tương ứng với 1 sprite sheet 16×16 px:

| tileType | File | gridCols | Số tile |
|---|---|---|---|
| 1 | `Tiles.png` | 25 | 625 |
| 2 | `Buildings.png` | 25 | 625 |
| 3 | `Hive.png` | 25 | 625 |
| 4 | `Interior-01.png` | 25 | 625 |
| 5 | `Props-Rocks.png` | 18 | 378 |
| 6 | `Tree-Assets.png` | 21 | 525 |

#### Cách xác định `tileId`

Mỗi sheet đi kèm file JSON chứa frame list. Ví dụ `Tiles.json`:

```json
{
  "frames": [
    { "filename": "tile_0",  "frame": { "x": 0,   "y": 0,   "w": 16, "h": 16 } },
    { "filename": "tile_1",  "frame": { "x": 16,  "y": 0,   "w": 16, "h": 16 } },
    ...
    { "filename": "tile_42", "frame": { "x": 672, "y": 16,  "w": 16, "h": 16 } }
  ]
}
```

- `tileId` chính là số trong `tile_N` (0-based).
- Công thức render: `col = tileId % gridCols`, `row = tileId / gridCols`.

### 2.1.3 Background parallax

7 layer forest, load tự động từ `assets/textures/backgrounds/forest/`:

| Layer | File | Tốc độ parallax |
|---|---|---|
| 1 (xa nhất) | `far.png` | rất chậm |
| 2 | `main.png` | chậm |
| 3 | `tree_dark.png` | trung bình |
| 4 | `tree_golden.png` | trung bình |
| 5 | `tree_green.png` | nhanh |
| 6 | `tree_red.png` | nhanh |
| 7 (gần nhất) | `tree_yellow.png` | rất nhanh |

> Không cần cấu hình gì thêm. Background tự động cuộn theo camera.

### Camera2D setup

```cpp
// Trong GameController::Init() hoặc StartLevel():
Camera2D cam;
cam.target = playerSpawnWorldPos;  // từ LevelFactory (spawn_solo hoặc spawn_guide)
cam.offset = { screenW/2.0f, screenH/2.0f };
cam.rotation = 0.0f;
cam.zoom = 1.0f;

// Trong GameController::Update() — mỗi frame:
cam.target = Vector2Lerp(cam.target, playerPos, 0.1f);  // follow player
```

> Camera pass vào `GameView::Render(camera, particles, dt)` sẽ được GameView dùng làm camera transform cho BeginMode2D.

### Các cách gọi View

**Trong frame loop (GameController::Update — mỗi frame):**
- `GameView::Update(dt)` — scroll background + shake timer + EntityRenderer::Update + CharacterRenderer::UpdateAll + FloatingText::Update
- `GameView::Render(camera, particles, dt)` — gọi tất cả: background → tilemap → CharacterRenderer → EntityRenderer → ParticleRenderer → EnemyStatusRenderer → FloatingText → FPS → UIStateManager

**Theo event:**
| Event | Ai gọi | View method |
|---|---|---|
| Level load | `GameController::StartLevel()` | `GameView::LoadTileset(tileType, path, gridCols)` (×6 sheets), `GameView::LoadBackgrounds()` |
| Level load | `GameController::Init()` | `GameView::Init()` — PreloadAtlas cho tất cả entity |

**View tự đọc Model:**
- `GameView::RenderTilemap()` đọc `m_tiles` (con trỏ do Controller set qua `GameView::SetTiles()`)
- `GameView::RenderBackground()` đọc camera position (từ GameController)

---

## 2.2 Entity System

### 2.2.1 Player Class

Có 4 class, chọn từ Main Menu. Cả Guide và Warrior đều có thể chọn class riêng.

| Class | HP | Damage | Speed | Vũ khí | Ultimate Skill | Pet mặc định |
|---|---|---|---|---|---|---|
| **Fighter** | Trung bình | Trung bình | Trung bình | Kiếm | Chém AoE | Skull |
| **Knight** | Cao | Thấp | Chậm | Kiếm lớn + Khiên | Tạo khiên bảo vệ | Ghost |
| **Ninja** | Thấp | Cao | Nhanh | Kunai | Teleport + chém nhanh | Baby Dragon |
| **Magic Caster** | Rất thấp | Rất cao | Trung bình | Pháp sư | Nuke AoE | Fairy |

> Class ảnh hưởng đến chỉ số và kỹ năng, **không** ảnh hưởng đến role (Guide/Warrior).  
> Guide và Warrior trong cùng một game có thể khác class.

### 2.2.2 Player Spawn Tokens

| Token | Khi nào dùng | Role | Đặc điểm |
|---|---|---|---|
| `spawn_solo` | maptype=0, chế độ SinglePlayer | Solo | Có thể tấn công, thấy toàn bộ map |
| `spawn_guide` | Co-op Host | Guide | Chỉ di chuyển + interact, **không tấn công**, thấy toàn bộ map |
| `spawn_warrior` | Co-op Client | Warrior | Có thể tấn công, **fog of war** (chỉ thấy vùng tròn nhỏ quanh nhân vật) |

> Mỗi level nên đặt cả 3 spawn token. Game tự chọn token phù hợp dựa trên GameMode.  
> Class không gắn trong `.lvl` — chọn từ Main Menu.

### 2.2.3 Enemy

```
enemy <type> <tx> <ty>
```

| Enemy | HP | Speed | Attack | Hành vi |
|---|---|---|---|---|
| **Melee** | Trung bình | Trung bình | Đánh gần, damage cao | Đuổi theo player, tấn công khi trong tầm |
| **Ranged** | Thấp | Chậm | Bắn đạn, damage trung bình | Giữ khoảng cách, bắn từ xa |
| **Flying** | Thấp | Nhanh | Damage thấp | Bay qua tile/enemy, khó bị chặn |

#### Difficulty scaling

Stats enemy tăng theo level number:

- Level 1: x1 (gốc)
- Level 2: x1.2
- Level 3: x1.4
- Level 4: x1.7
- Level 5: x2.0
- Level 6: x2.5

#### Drop khi chết

| Enemy | Drop | Xác suất |
|---|---|---|
| Melee | Coin | 30% (1 coin) |
| Ranged | Coin | 40% (1 coin) |
| Flying | Coin | 20% (1 coin) |

### 2.2.4 Boss

```
boss <id> <tx> <ty>
```

| Boss | Phases | HP | Kỹ năng | Xuất hiện ở level |
|---|---|---|---|---|
| **1 — Knight** | 2 | Cao | Phase 1: chém thường. Phase 2: chém nhanh + AoE xoay | Level 1, 4 |
| **2 — Witch** | 3+ | Trung bình | Bắn đạn (phase 1), hồi máu (phase 2), teleport (phase 3) | Level 2, 5 |
| **3 — 4-phase** | 4 | Rất cao | Energy sphere → blast → beam (phase 1-3), tổng hợp (phase 4) | Level 3, 6 |

> Boss 1 và 2 dùng lại ở level Special (4-5) với difficulty scaling cao hơn.  
> Boss 3 (4-phase) dùng lại ở level 6.

#### Chi tiết từng boss

**Boss 1 — Knight:**
- Phase 1: Di chuyển chậm, chém đơn mục tiêu
- Phase 2 (HP < 50%): Tăng tốc, chém AoE xoay 360°
- Chuyển phase: animation transition

**Boss 2 — Witch:**
- Phase 1: Bắn đạn đơn, đứng xa player
- Phase 2 (HP < 60%): Bắn đạn chùm + hồi máu định kỳ
- Phase 3 (HP < 30%): Teleport né tránh + bắn liên tục
- Chuyển phase: animation transition

**Boss 3 — 4-phase:**
- Phase 1: Energy sphere (bắn hình cầu)
- Phase 2: Energy blast (chùm tia rộng)
- Phase 3: Energy beam (tia laser liên tục)
- Phase 4 (HP < 25%): Kết hợp cả 3 skill trên
- Chuyển phase: animation transition

### 2.2.5 Pet

Pet đi kèm player khi bắt đầu level, chọn từ Main Menu (riêng biệt với class).

| Pet | Tấn công | Kỹ năng đặc biệt |
|---|---|---|
| **Skull** | Bắn đạn đơn mục tiêu | — |
| **Ghost** | Không tấn công | Hồi máu cho player định kỳ |
| **Baby Dragon** | AoE tầm ngắn | — |
| **Fairy** | Không tấn công | Nhặt item tự động (coin, apple, key) |

> Pet không có token trong `.lvl`. Pet được chọn từ Main Menu cùng lúc với class và role.

### 2.2.6 Checkpoint & Chest

```
checkpoint <tx> <ty>
chest <tx> <ty>
```

| Entity | Chức năng |
|---|---|
| Checkpoint | Điểm hồi sinh khi player chết. Player chạm vào → kích hoạt |
| Chest | Rương ngẫu nhiên. Player tương tác → nhận loot |

### 2.2.7 Item

Item xuất hiện qua chest drop hoặc enemy drop.

| Item | Hiệu ứng | Stack? |
|---|---|---|
| Coin | +1 điểm, +score | ✅ (không giới hạn) |
| Apple | Hồi 25 HP | ❌ (mỗi ô 1 item) |
| Key | Mở cửa khoá | ❌ |
| Equip | Tăng stats (dùng trong Inventory) | ❌ |

### 2.2.8 Entity Summary Table

Bảng tổng hợp tất cả entity tokens trong `.lvl`:

| Token | Ví dụ | Entity | Ghi chú |
|---|---|---|---|
| `spawn_solo <tx> <ty>` | `spawn_solo 5 10` | Player (Solo) | Chỉ đọc khi maptype=0 |
| `spawn_guide <tx> <ty>` | `spawn_guide 5 10` | Player (Guide) | Chỉ đọc khi GameMode=AsymCoopHost |
| `spawn_warrior <tx> <ty>` | `spawn_warrior 15 10` | Player (Warrior) | Chỉ đọc khi GameMode=AsymCoopClient |
| `enemy <type> <tx> <ty>` | `enemy melee 20 15` | Enemy | type = melee / ranged / flying |
| `boss <id> <tx> <ty>` | `boss 1 20 15` | Boss | id = 1 (Knight) / 2 (Witch) / 3 (4-phase) |
| `chest <tx> <ty>` | `chest 18 12` | Chest | Random loot theo bảng |
| `checkpoint <tx> <ty>` | `checkpoint 10 20` | Checkpoint | Respawn point |

> Pet không có token — chọn từ Main Menu.  
> Item xuất hiện qua chest/enemy drop, không có token riêng.

### Entity Renderer

`EntityRenderer` dùng cho entity **không animate** (static sprite, xoay vòng). `CharacterRenderer` dùng cho entity **có animation atlas**. Cả 2 đều được `GameView::Update()` / `GameView::Render()` gọi tự động — Controller không cần gọi trực tiếp.

### Các cách gọi View

**Trong frame loop (mỗi frame — GameView::Update/Render tự gọi):**
- `CharacterRenderer::UpdateAll(dt)` — update animation frame
- `CharacterRenderer::RenderAll()` — render entity sprites + boss phase glow overlay + elemental tint
- `EntityRenderer::Update(dt)` + `EntityRenderer::RenderAll()`

**Theo event:**
| Event | Ai gọi | View method |
|---|---|---|
| Spawn enemy | `GameController::StartLevel()` | `CharacterRenderer::Register(enemy, path, "idle")` |
| Spawn boss | `GameController::StartLevel()` | `CharacterRenderer::Register(boss, path, "idle")` + `SetBossAssetRoot(id, root)` |
| Spawn chest | `GameController::StartLevel()` | `CharacterRenderer::Register(chest, "objects/chest_closed.json", "default")` |
| Spawn checkpoint | `GameController::StartLevel()` | `CharacterRenderer::Register(checkpoint, "objects/checkpoint_uncaptured.json", "default")` |
| Entity die | `GameController::OnEntityRemoved()` | `CharacterRenderer::Unregister(id)` |
| Player attack | `GameController::Update()` | `CharacterRenderer::PlayAction(playerId, ACTION_ATTACK)` |
| Player skill | `GameController::Update()` | `CharacterRenderer::PlayAction(playerId, ACTION_SKILL)` |
| Boss phase change | `GameController::OnBossPhase()` | `CharacterRenderer::SwitchPhase(id, phase)` |
| Pet attack | GameController tự động | `CharacterRenderer::PlayAction(petId, ACTION_ATTACK)` |
| Open chest | `GameController::OnInteract()` | `CharacterRenderer::PlayAction(chestId, ACTION_ACTIVATE)` |

**View tự đọc Model (trong UpdateAll):**
- `CharacterRenderer::UpdateAll()` đọc từ mỗi `Entity`: `GetId()`, `GetType()`, `GetState()` (→ infer clip), `GetPosition()`, `GetScale()`, `GetDirection()`, `IsActive()`
- `CharacterRenderer::RenderAll()` đọc `Boss::GetPhase()` (cho glow overlay)
- `EntityRenderer` đọc `Entity::GetPosition()` × `TILE_SIZE` cho world position

### Entity → Atlas path mapping

Khi gọi `CharacterRenderer::Register(entity, path, clip)`, dùng path sau cho từng loại entity:

| Entity type | Atlas path | Default clip | Ghi chú |
|---|---|---|---|
| **Player** — Fighter | `"assets/textures/player/fighter/idle.json"` | `"idle"` | 10+ state files riêng (walk/run/jump/attack1-3/hurt/dead/...) |
| **Player** — Knight | `"assets/textures/player/knight/idle.json"` | `"idle"` | Tương tự Fighter |
| **Player** — Ninja | `"assets/textures/player/ninja/idle.json"` | `"idle"` | Có thêm teleport + projectile |
| **Player** — Magic Caster | `"assets/textures/player/magic_caster/idle.json"` | `"idle"` | Có thêm projectile_attack |
| **Enemy** — Melee | `"assets/textures/enemies/melee_idle.json"` | `"idle"` | 6 state files (idle/walk/attack/hurt/death/...) |
| **Enemy** — Ranged | `"assets/textures/enemies/ranged_idle.json"` | `"idle"` | 6 state files + `ranged_bomb.json` |
| **Enemy** — Flying | `"assets/textures/enemies/flying_spritesheet.json"` | `"idle"` | 5 state files + `flying_projectile.json` |
| **Boss 1 — Knight** (phase1) | `"assets/textures/boss/boss1/phase1/idle.json"` | `"idle"` | Dùng `SetBossAssetRoot(id, "boss/boss1")` + `SwitchPhase()` |
| **Boss 2 — Witch** (phase1) | `"assets/textures/boss/boss2/phase1/idle.json"` | `"idle"` | 3 phase, mỗi phase có projectile |
| **Boss 3 — 4-phase** (phase1) | `"assets/textures/boss/boss3/phase1/idle.json"` | `"idle"` | 4 phase, mỗi phase attack riêng + projectile |
| **Chest** | `"assets/textures/objects/chest_closed.json"` | `"default"` | Đổi clip khi mở |
| **Checkpoint** (uncaptured) | `"assets/textures/objects/checkpoint_uncaptured.json"` | `"default"` | Dùng `RegisterAnimated()` với flag_out + captured |
| **Pet — Skull** | `"assets/textures/pets/skull/idle.json"` | `"idle"` | Thêm move/attack |
| **Pet — Ghost** | `"assets/textures/pets/ghost/idle.json"` | `"idle"` | Thêm move/healing |
| **Pet — Baby Dragon** | `"assets/textures/pets/baby_dragon/idle.json"` | `"idle"` | Thêm move/attack |
| **Pet — Fairy** | `"assets/textures/pets/fairy/idle.json"` | `"idle"` | Thêm move/collect_item |

> Tất cả state files (walk/run/jump/attack/...) đã được PreloadAtlas trong `GameView::Init()`. Controller chỉ cần gọi `Register()` với idle path làm base — `CharacterRenderer` tự động switch clip khi Controller gọi `PlayAction()`.
>
> Boss: dùng `SetBossAssetRoot(id, root)` + từng `Register()` riêng cho mỗi phase. `SwitchPhase(id, phase)` tự động load phase atlas nếu đã preload.

---

## 2.3 Menu System

### 2.3.1 Danh sách level

6 level cố định, hiển thị dạng grid 2×3 trên màn hình chọn level:

| Level | maptype | Chế độ chơi | Boss | Ghi chú |
|---|---|---|---|---|---|
| 1 | Flexible (0) | Single Player + Co-op | Boss 1 — Knight | |
| 2 | Flexible (0) | Single Player + Co-op | Boss 2 — Witch | |
| 3 | Flexible (0) | Single Player + Co-op | Boss 3 (4-phase) | |
| 4 | GuideSpecial (1) | **Chỉ Co-op** | Boss 1 — Knight (scaled) | Host=Guide, Client=Warrior. **Guide là chủ đạo** — tìm đường, giải đố, kích hoạt checkpoint. |
| 5 | GuideSpecial (1) | **Chỉ Co-op** | Boss 2 — Witch (scaled) | Host=Guide, Client=Warrior. **Guide là chủ đạo** — tìm đường, giải đố, kích hoạt checkpoint. |
| 6 | WarriorSpecial (2) | **Chỉ Co-op** | Boss 3 (4-phase, scaled) | Host=Guide, Client=Warrior. **Warrior là chủ đạo** — combat nặng, nhiều enemy. |

#### Quy tắc hiển thị
- Level 1-3: luôn sáng (available).
- Level 4-6: **chỉ sáng khi chọn chế độ Co-op**.
- Nếu chưa hoàn thành level N, level N+1 bị mờ.

### 2.3.2 Chọn role (Co-op)

Khi chơi Co-op ở level Flexible (1-3), hiển thị màn hình chọn role:

- **Guide:** Di chuyển, interact với object. Không thể tấn công. Thấy toàn bộ map.
- **Warrior:** Tấn công enemy, boss. Fog of war (chỉ thấy vùng sáng nhỏ).

Level Special (4-6) tự động gán: Host = Guide, Client = Warrior.

### 2.3.3 Main Menu

| Mục | Hành động |
|---|---|
| **Start** | Mở màn hình chọn level |
| **Options** | (dự phòng) |
| **Quit** | Thoát game |

Điều hướng: phím UP/DOWN, ENTER để chọn.

### 2.3.4 Pause Menu

Phím **Escape** khi đang chơi → pause overlay:

| Mục | Hành động |
|---|---|
| **Resume** | Tiếp tục chơi |
| **Restart** | Chơi lại level hiện tại |
| **Quit to Menu** | Về Main Menu |

### 2.3.5 Màn hình kết thúc (Result)

Hiển thị sau khi hoàn thành level hoặc Game Over:

- **Level Complete:** 3 nút — Retry / Next Level / Menu
- **Game Over:** 2 nút — Retry / Menu

Thông tin hiển thị (từ LevelScoring):
- Số sao (1-3), thời gian clear, số enemy đã tiêu diệt
- % item đã nhặt, điểm số + High Score

### Các cách gọi View

**Trong frame loop (MenuController::Update):**
- `MenuView::Update(dt, selectedIndex)` — highlight mục đang chọn
- `MenuView::Render()` — render menu hiện tại

**Theo event:**
| Event | Ai gọi | View method |
|---|---|---|
| Game start | `MenuController::Init()` | `MenuView::LoadResources(path)`, `MenuView::Init()` |
| Show main menu | `MenuController::Update()` | `MenuView::ShowMainMenu()` |
| Show level select | `MenuController::Update()` | Controller tự render grid 2×3 từ `GameState` completion data |
| Show role select | `MenuController::Update()` | `MenuView::ShowRoleSelect(roles)` |
| Pause | `GameController::OnInputESC()` | `MenuView::ShowPauseOverlay()` (qua UIStateManager) |
| Resume | `GameController::OnInputESC()` (khi paused) | `UIStateManager::Pop()`, `MenuView::HidePauseOverlay()` |
| Error dialog | `NetworkManager` | `MenuView::ShowErrorDialog(message)` |
| Connection status | `NetworkManager` | `MenuView::ShowConnectionStatus(ip, connected)` |

**View tự đọc Model:**
- `MenuView` (level select) đọc `GameState` completion flags để quyết định level sáng/tối
- `MenuView` (role select) đọc `GameMode` để quyết định hiển thị guide/warrior

---

## 2.4 Inventory System

### 2.4.1 Item & Inventory

Player có inventory giới hạn (số slot, item không stack ngoại trừ Coin).

#### Khi nhặt item
1. Item biến mất khỏi map.
2. Coin → +score, coin count tăng.
3. Apple → +25 HP (nếu HP < max, nếu HP đầy → item ở lại map).
4. Key → thêm vào inventory, dùng để mở cửa khoá.
5. Equip → thêm vào inventory, có thể equip từ màn hình Inventory.

### 2.4.2 Chest loot table

Khi player mở chest, loot được random theo tỉ lệ:

| Item | Xác suất | Số lượng |
|---|---|---|
| Coin | 45% | 1-3 |
| Apple | 25% | 1 |
| Key | 15% | 1 |
| Equip | 10% | 1 |
| (Rỗng) | 5% | — |

### 2.4.3 Mở Inventory (phím I)

- Phím **I** khi đang chơi → mở Inventory overlay.
- Các item trong inventory hiển thị dạng grid.
- Chọn item → **Use** (nếu có thể):
  - Apple: hồi HP.
  - Key: mở cửa (nếu đang đứng gần cửa khoá).
  - Equip: thay đổi stats.
- Phím **I** lần nữa (hoặc Escape) → đóng Inventory.

### 2.4.4 Tự động cập nhật

Khi inventory thay đổi (nhặt/dùng/xoá item), UI **tự động** cập nhật.  
Designer không cần làm gì thêm.

### Các cách gọi View

**Theo event:**
| Event | Ai gọi | View method |
|---|---|---|
| Open inventory (phím I) | `GameController::OnInputI()` | `UIStateManager::Push(UILayer::Inventory)`, `InventoryView::Open()` |
| Close inventory (phím I/Esc) | `GameController::OnInputI()` | `UIStateManager::Pop()`, `InventoryView::Close()` |
| Chest auto-open | `GameController::OnInteract()` | `UIStateManager::Push(UILayer::Inventory)` |
| Item use | Controller (callback) | `InventoryView::RegisterOnRequestUseItem(callback)` |

**View tự đọc Model (ObservableList — auto-refresh):**
- `InventoryView` attach `ObservableList<const Item*>` từ `Player::GetInventory()`
  - Khi Model thêm/xoá item → View tự cập nhật grid
- `InventoryView` đọc `Item::GetType()`, `Item::GetIconName()`, `Item::GetStackCount()` cho từng item trong list

**Init:**
| View method | Ai gọi |
|---|---|
| `PreloadAtlas("assets/textures/items/apple.json")` | `GameView::Init()` |
| `PreloadAtlas("assets/textures/objects/chest.json")` | `GameView::Init()` |
| `PreloadAtlas("assets/textures/ui/inventory_icons.json")` | `GameView::Init()` |

---

## 2.5 Score Table

### 2.5.1 Star rating

Mỗi level có rating 1-3 sao dựa trên điểm số:

```
maxScore = totalItems × 100 + totalEnemies × 50 + timeBonus
timeBonus = max(0, 300 - clearTime) × 10

3★:  score >= maxScore × 0.8
2★:  score >= maxScore × 0.5
1★:  đã clear level (bất kỳ điểm nào)
```

Trong đó:
- `totalItems`: từ token `scoring <totalItems> <totalEnemies>` trong `.lvl`.
- `totalEnemies`: từ token `scoring`.
- `clearTime`: thời gian hoàn thành level (giây).

#### Ví dụ

Level1.lvl: `scoring 10 15`. Clear trong 120 giây, thu thập đủ 10 item, giết 15 enemy:

```
maxScore = 10×100 + 15×50 + (300-120)×10 = 1000 + 750 + 1800 = 3550
3★ cần: 3550 × 0.8 = 2840
```

### 2.5.2 Timer

- Timer **tự động bắt đầu** khi level load xong.
- Timer **dừng** khi pause game.
- Timer **dừng** khi level kết thúc (thắng hoặc thua).
- Thời gian hiển thị trên HUD.

### 2.5.3 High Score

- High score được **lưu tự động** khi player đạt điểm cao hơn record trước đó.
- High score được lưu riêng cho **từng level**.
- Hiển thị trên màn hình Result và màn hình chọn level.

### 2.5.4 Token `scoring` trong `.lvl`

```
scoring <totalItems> <totalEnemies>
```

- `totalItems`: tổng số item có thể nhặt trong level (coin + apple + key + equip).
- `totalEnemies`: tổng số enemy trong level (không tính boss riêng).
- Hai giá trị này quyết định maxScore và % hoàn thành.

### Các cách gọi View

**Theo event:**
| Event | Ai gọi | View method |
|---|---|---|
| Level complete | `GameController::OnLevelComplete()` | `UIStateManager::Push(UILayer::Result)`, `ResultView::Show(LevelScoring snapshot)` |
| Game over | `GameController::OnGameOver()` | `UIStateManager::Push(UILayer::Result)`, `ResultView::ShowGameOver(snapshot)` |
| Dismiss result | Controller | `UIStateManager::Pop()`, `ResultView::Dismiss()` |
| New high score | `GameController::OnLevelComplete()` | `ResultView::ShowHighScore(newScore)` (nếu vượt record) |

**Trong frame loop (ResultView visible):**
- `ResultView::Update(dt)` — animation (star fill, number count-up)
- `ResultView::Render()` — render result screen

**View tự đọc Model:**
- `ResultView` nhận `LevelScoring` snapshot khi show — đọc `GetTotalItems()`, `GetTotalEnemies()`, `GetCollectedItems()`, `GetDefeatedEnemies()`, `GetClearTime()`, `GetScore()`
- `HUDView` đọc `GameState::GetClearTime()` mỗi frame (timer display)

**Init:**
| View method | Ai gọi |
|---|---|
| `ResultView::LoadResources(path)` | `GameController::Init()` (hoặc `GameView::Init()`) |
| `PreloadAtlas("assets/ui/result_stars.json")` | `GameView::Init()` |

---

## 2.6 HUD System

### Kiến trúc

```
Player (Model)  ──→  HUDView (View)
  • HP/maxHP        • HP bar (máu)
  • MP/maxMP        • MP bar (năng lượng)
  • SP/maxSP        • SP bar (stamina)
  • coinCount       • Coin count icon + số
  • ultimateCharge  • Ultimate bar
  • buffSlots       • Buff/debuff slots (ObservableList)

ElementalSystem (Model)  ──→  EnemyStatusRenderer (View)
  • status[entityId]       • Icons: Burn/Wet/Shocked

GameState (Model)  ──→  HUDView (View)
  • m_clearTime           • Timer display
```

### Các cách gọi View

**Trong frame loop (GameController::Update — mỗi frame):**
| Thứ tự | View method | Ghi chú |
|---|---|---|
| 1 | `HUDView::Update(dt, player)` | Cập nhật bar width, timer text |
| 2 | `FloatingTextManager::Update(dt)` | Tick lifetime của floating text |
| 3 | Sau `EndMode2D()`: | |
| 4 | `HUDView::Render()` | Render bars, timer, coin count |

**Theo event:**
| Event | Ai gọi | View method |
|---|---|---|
| Player nhận damage | `GameController::OnHit()` | `FloatingTextManager::Emit(pos, "-N", RED, 1.5f)` |
| Player heal | `GameController::OnHeal()` | `FloatingTextManager::Emit(pos, "+N", GREEN, 1.5f)` |
| Nhặt coin | `GameController::OnItemCollect()` | `FloatingTextManager::Emit(pos, "+N", YELLOW, 1.5f)` |
| Nhặt item | `GameController::OnItemCollect()` | `FloatingTextManager::Emit(pos, "Apple", WHITE, 1.5f)` |
| Enemy bị Burn | `ElementalSystem::ApplyDamage()` | `EnemyStatusRenderer::SetStatus(id, pos, true, false, false)` |
| Enemy hết Burn | `ElementalSystem::Update()` | `EnemyStatusRenderer::SetStatus(id, pos, false, false, false)` |
| Interact prompt | `GameController::OnNearChest()` | `InteractPrompt::Show("Press E to open")` |
| Interact done | `GameController::OnInteract()` | `InteractPrompt::Hide()` |

**View tự đọc Model (trong Update):**
| View class | Đọc từ Model | Field/Method |
|---|---|---|
| `HUDView` | `Player` | `GetHP()`, `GetMaxHP()`, `GetMP()`, `GetMaxMP()`, `GetSP()`, `GetMaxSP()`, `GetCoinCount()`, `GetUltimateCharge()` |
| `HUDView` | `GameState` | `GetClearTime()` |
| `FloatingTextManager` | (tự quản lý) | Danh sách text đang active |
| `EnemyStatusRenderer` | `ElementalSystem` | `GetStatus(entityId)` → StatusEffect |
| `InteractPrompt` | (Controller gọi) | Show/Hide theo context |

**ObservableList (Model mutate → View auto-refresh):**
| Model list | View attach | Trigger mutate |
|---|---|---|
| `Player::m_buffs` | `HUDView::AttachObservable()` | Khi `ElementalSystem` thêm/xoá buff |
| `Player::m_skillSlots` | `SkillBarView::AttachObservable()` | Khi cooldown tick / skill dùng |

**Init:**
| View method | Ai gọi |
|---|---|
| `HUDView::LoadResources("assets/ui/darkDwellers/...")` | `GameController::Init()` |
| `PreloadAtlas("assets/ui/hud_bars.json")` | `GameView::Init()` |

---

## 2.7 Elemental System & Status Effects

### Kiến trúc

```
ElementalSystem (Model)
├── ApplyDamage(entityId, DamagePacket) → trừ HP + gán StatusEffect
├── React(entityA, entityB)              → Vaporize / Conduct / Overload
└── GetStatus(entityId) → StatusEffect   → EnemyStatusRenderer đọc
```

### Damage types & phản ứng nguyên tố

| DamageType | Gây ra |
|---|---|
| `Physical` | Damage thường, không gây status |
| `Fire` | Gây `Burn` (DOT 3s, mỗi tick 5% ATK) |
| `Water` | Gây `Wet` (nhận thêm 30% damage từ Thunder) |
| `Thunder` | Gây `Shocked` (stun 0.5s, nhân đôi damage từ Fire) |

### Reactions (ElementalSystem::React)

| Status A | Status B | Reaction | Hiệu ứng |
|---|---|---|---|
| `Burn` | `Wet` | **Vaporize** | Xoá cả 2, nhân 1.5× damage cuối |
| `Wet` | `Shocked` | **Conduct** | Xoá Wet, stun 1.5s, damage lan 3 entity gần nhất |
| `Burn` | `Shocked` | **Overload** | Xoá cả 2, nổ AoE 100px, knockback |

### DamagePacket

```cpp
struct DamagePacket {
    float baseDamage;
    DamageType type;
    float multiplier;     // 1.0f default, tăng khi có reaction
    float knockbackForce; // 0 = không knockback
    Entity* source;       // ai gây damage
};
```

### Các cách gọi View

**Theo event:**
| Event | Ai gọi | View method |
|---|---|---|
| Entity nhận damage | `ElementalSystem::ApplyDamage()` | `ElementalFX::SetElementTint(entityId, packet.type)` — tint sprite theo element |
| Reaction Vaporize | `ElementalSystem::React()` | `ParticleRenderer::EmitReaction(pos, ReactionType::Vaporize)` |
| Reaction Conduct | `ElementalSystem::React()` | `ParticleRenderer::EmitReaction(pos, ReactionType::Conduct)`, `GameView::Shake(5.0f, 0.3f)` |
| Reaction Overload | `ElementalSystem::React()` | `ParticleRenderer::EmitReaction(pos, ReactionType::Overload)`, `GameView::Shake(8.0f, 0.5f)` |
| Status thay đổi | `ElementalSystem::Update()` | `EnemyStatusRenderer::SetStatus(id, pos, burn, wet, shocked)` |
| DOT tick (Burn) | `ElementalSystem::Update()` | `FloatingTextManager::Emit(pos, burnDmgStr, RED, 1.0f)` |

**Trong frame loop:**
- `ElementalFX` (tint) được `CharacterRenderer::RenderAll()` tự động đọc và apply
- `EnemyStatusRenderer::Update(dt)` + `EnemyStatusRenderer::Render(camera)` — mỗi frame
- `ParticleSystem::Update(dt)` (Model) + `ParticleRenderer::RenderAll(particles, camera, dt)` — mỗi frame

**View tự đọc Model:**
| View class | Đọc từ Model | Field/Method |
|---|---|---|
| `CharacterRenderer` | `ElementalFX` | `GetTintForEntity(entityId)` — áp khi render |
| `EnemyStatusRenderer` | `ElementalSystem` | `GetStatus(entityId)` → `StatusEffect` (Burn/Wet/Shocked) |
| `ParticleRenderer` | `ParticleSystem` | `GetActiveParticles()` — vector particle đang active |

---

## 2.8 Effects & Particles

### Kiến trúc

```
ParticleSystem (Model)        ParticleRenderer (View)
├── std::vector<Particle>     ├── EmitBurst(pos, count, color)
├── Acquire() / Release()     ├── EmitBurst(pos, count, color)
│                             ├── EmitReaction(pos, type)
└── ObjectPool<Particle>      └── RenderAll(particles, camera, dt)

FloatingTextManager (View)
├── Emit(worldPos, text, color, lifetime)
├── Update(dt)
└── Render(camera)

Camera shake (GameView)
├── Shake(intensity, duration)
└── Tự fade theo thời gian
```

### Particle types

| Type | Trigger | Số lượng | Màu | View method |
|---|---|---|---|---|---|
| Hit | Enemy nhận damage | 5-8 | Trắng | `EmitBurst()` |
| Death | Enemy chết | 15-25 | Đỏ + Cam | `EmitBurst()` |
| Heal | Player hồi máu | 5 | Xanh lá | `EmitBurst()` |
| Reaction Vaporize | Fire + Water | 20 | Trắng xanh | `EmitReaction(pos, Vaporize)` |
| Reaction Conduct | Water + Thunder | 15 | Xanh dương + Vàng | `EmitReaction(pos, Conduct)` |
| Reaction Overload | Fire + Thunder | 25 | Đỏ + Vàng | `EmitReaction(pos, Overload)` |

### Các cách gọi View

**Trong frame loop (GameController::Update — mỗi frame):**
| Thứ tự | View method | Ghi chú |
|---|---|---|
| 1 | `ParticleSystem::Update(dt)` | Update particle positions + lifetimes |
| 2 | `FloatingTextManager::Update(dt)` | Tick lifetime + move up |
| 3 | `GameView::Update(dt)` | Camera shake fade |
| 4 | Trong `BeginMode2D(camera)` … `EndMode2D()`: | |
| 5 | `ParticleRenderer::RenderAll(particles, camera, dt)` | Particles trong world |
| 6 | `FloatingTextManager::Render(camera)` | Floating text trong world |

**Theo event:**
| Event | Ai gọi | View method |
|---|---|---|
| Entity hit | `GameController::OnHit()` | `ParticleRenderer::EmitBurst(pos, 8, WHITE)`, `FloatingTextManager::Emit(pos, "-N", RED, 1.5f)`, `GameView::Shake(3.0f, 0.15f)` |
| Entity die | `GameController::OnEntityRemoved()` | `ParticleRenderer::EmitBurst(pos, 20, RED)`, `GameView::Shake(5.0f, 0.3f)` |
| Player heal | `GameController::OnHeal()` | `ParticleRenderer::EmitBurst(pos, 5, GREEN)`, `FloatingTextManager::Emit(pos, "+N", GREEN, 1.5f)` |
| Reaction | `ElementalSystem::React()` | `ParticleRenderer::EmitReaction(pos, ReactionType::Vaporize)` |
| Nhặt coin | `GameController::OnItemCollect()` | `FloatingTextManager::Emit(pos, "+N", YELLOW, 1.5f)` |
| Boss heavy attack | `GameController::OnBossAttack()` | `GameView::Shake(8.0f, 0.5f)` |
| Ultimate cast | `GameController::OnUltimate()` | `GameView::Shake(10.0f, 0.6f)`, `ParticleRenderer::EmitBurst(pos, 30, WHITE)` |

**View tự đọc Model:**
| View class | Đọc từ Model |
|---|---|
| `ParticleRenderer` | `ParticleSystem::GetActiveParticles()` |
| `FloatingTextManager` | (tự quản lý nội bộ) |
| `GameView` | (tự quản lý shake timer) |

**ObjectPool wiring (Model → View reuse):**
```cpp
// Model pool
ObjectPool<Particle> m_particlePool;
auto* p = m_particlePool.Acquire();
p->Init(pos, vel, color, lifetime);
// Khi particle hết lifetime → ParticleSystem tự Release()
m_particlePool.Release(p);

// View chỉ render — không quản lý lifecycle
ParticleRenderer::RenderAll(m_particlePool.GetActive(), camera, dt);
```

---

## 2.9 UI Layer Stack

### Kiến trúc

```
UIStateManager (View)
├── Push(UILayer layer)       // mở modal layer
├── Pop()                     // đóng layer trên cùng
├── Clear()                   // đóng tất cả
├── IsOverlayActive() → bool  // đang có modal nào không?
├── RenderAll()               // render theo thứ tự
└── std::stack<UILayer> m_stack;
```

### Render order (dưới lên trên)

| Thứ tự | Layer | Modal? | Mô tả |
|---|---|---|---|
| 1 | HUD | ❌ | Luôn visible, không dim |
| 2 | SkillBar | ❌ | Toggle bằng phím, không dim |
| 3 | InteractPrompt | ❌ | Show/hide theo context |
| 4 | Menu (Pause) | ✅ | Dim các layer 1-3 |
| 5 | Inventory | ✅ | Dim các layer 1-3 |
| 6 | Result | ✅ | Dim tất cả |

### Các cách gọi View

**Trong frame loop (GameController::Update — gọi cuối cùng sau tất cả):**
- `UIStateManager::RenderAll()` — render tất cả layers theo thứ tự stack (dưới lên trên)

**Theo event — Push/Pop modal:**
| Event | Ai gọi | View method |
|---|---|---|
| Open inventory | `GameController::OnInputI()` | `UIStateManager::Push(UILayer::Inventory)`, `InventoryView::Open()` |
| Close inventory | `GameController::OnInputI()` (khi inventory open) | `UIStateManager::Pop()`, `InventoryView::Close()` |
| Pause | `GameController::OnInputESC()` | `UIStateManager::Push(UILayer::Menu)`, `MenuView::ShowPauseOverlay()` |
| Resume | `GameController::OnInputESC()` (khi paused) | `UIStateManager::Pop()`, `MenuView::HidePauseOverlay()` |
| Level complete | `GameController::OnLevelComplete()` | `UIStateManager::Push(UILayer::Result)`, `ResultView::Show(data)` |
| Game over | `GameController::OnGameOver()` | `UIStateManager::Push(UILayer::Result)`, `ResultView::ShowGameOver(data)` |
| Return to menu | Controller | `UIStateManager::Clear()` |
| Quest prompt | `GameController::OnNearNPC()` | `InteractPrompt::Show(text)` |

**Controller input routing (khi overlay active):**
```cpp
// Trong GameController::Update():
if (UIStateManager::GetInstance().IsOverlayActive()) {
    HandleMenuInput(input);        // Chỉ xử lý menu/inventory input
    return;                         // Không xử lý game input
}
// Xử lý game input bình thường
HandleGameInput(input);
```

**Init:**
| View method | Ai gọi |
|---|---|
| `UIStateManager::Init()` | `GameController::Init()` |
| `PreloadAtlas("assets/ui/dim_overlay.png")` | `GameView::Init()` |

---

## 2.10 Sound System

### Kiến trúc

SoundManager dùng **string key** (không phải enum). File `.wav` được load bằng key name, sau đó gọi bằng key đó.

```
SoundManager (singleton)
├── InitAudio()                // Init audio device
├── LoadSound(name, filepath)  // Load 1 SFX từ file
├── LoadMusic(name, filepath)  // Load 1 BGM từ file
├── PlaySound("key")           // Phát SFX 1 lần
├── PlayMusic("key")           // Phát BGM loop
├── StopMusic("key")
├── SetSFXVolume(float)
├── SetMusicVolume(float)
├── UnloadAll()                // Unload tất cả
└── CloseAudio()
```

### Controller Init — Load tất cả sounds

Trong `GameController::Init()`, gọi `LoadSound`/`LoadMusic` cho từng file:

```cpp
SoundManager& snd = SoundManager::GetInstance();
snd.InitAudio();
snd.LoadSound("player_attack",   "assets/sounds/sfx/player_attack.wav");
snd.LoadSound("enemy_death",     "assets/sounds/sfx/enemy_die.wav");
snd.LoadSound("coin_pickup",     "assets/sounds/sfx/coin_pickup.wav");
snd.LoadSound("chest_open",      "assets/sounds/sfx/chest_open.wav");
snd.LoadSound("ui_hover",        "assets/sounds/sfx/ui_hover.wav");
snd.LoadSound("ui_inventory_open",  "assets/sounds/sfx/ui_inventory_open.wav");
snd.LoadSound("ui_inventory_close", "assets/sounds/sfx/ui_inventory_close.wav");
// ... tương tự cho tất cả SFX
snd.LoadMusic("bgm_menu",     "assets/sounds/music/bgm_menu.wav");
snd.LoadMusic("bgm_gameplay", "assets/sounds/music/bgm_gameplay.wav");
snd.LoadMusic("bgm_boss",     "assets/sounds/music/bgm_boss.wav");
```

### Các cách gọi

**Theo event (Controller/Model gọi 1 lần):**
| Ai gọi | Code | Khi nào |
|---|---|---|
| `GameController::Init()` | `snd.InitAudio()` + các `LoadSound`/`LoadMusic` | Game start |
| `GameController::StartLevel()` | `snd.PlayMusic("bgm_gameplay")` | Level load |
| `GameController::OnBossSpawn()` | `snd.PlayMusic("bgm_boss")` | Boss fight bắt đầu |
| `GameController::OnBossDie()` | `snd.PlayMusic("bgm_gameplay")` | Boss chết, về BGM game |
| `GameController::OnLevelComplete()` | `snd.StopMusic("bgm_gameplay")` | Level kết thúc |
| `MenuController::ShowMainMenu()` | `snd.PlayMusic("bgm_menu")` | Vào main menu |
| `GameController::OnHit()` | `snd.PlaySound("player_hurt")` | Player nhận damage |
| `GameController::OnEntityRemoved()` | `snd.PlaySound("enemy_death")` | Enemy chết |
| `GameController::OnItemCollect()` | `snd.PlaySound("coin_pickup")` | Nhặt item |
| `GameController::OnInteract()` | `snd.PlaySound("chest_open")` | Mở chest |
| `GameController::OnBossPhase()` | `snd.PlaySound("boss_phase")` | Boss chuyển phase |
| `ElementalSystem::React()` | `snd.PlaySound("vaporize")` | Reaction |
| `MenuController::Update()` | `snd.PlaySound("ui_hover")` | UI hover |
| `InventoryView::Open()` | `snd.PlaySound("ui_inventory_open")` | Mở inventory |
| `InventoryView::Close()` | `snd.PlaySound("ui_inventory_close")` | Đóng inventory |

---

## 2.11 Wiring & Integration

### Data flow tổng quan

```
1. INIT SEQUENCE
   GameController::Init()
   ├── SoundManager::GetInstance().InitAudio()
   ├── SoundManager::GetInstance().LoadSound/LoadMusic   // load từng file .wav
   ├── GameView::GetInstance().Init()       // preload atlases
   ├── MenuController::GetInstance().Init()
   └── UIStateManager::GetInstance().Init()

2. LOAD LEVEL SEQUENCE
   GameController::StartLevel(levelNum, isCoop)
   ├── LevelFactory::LoadLevel(path, mode) → GameState
   │     • Parse .lvl → fill GameState::m_tiles, entities, scoring
   │     • Tạo Player với role phù hợp
   │     • Tạo Enemy (EnemyFactory), Boss, Chest, Checkpoint
   ├── Lấy entities từ GameState → for each: CharacterRenderer::Register(entity, atlasPath, "idle")
   ├── GameView::GetInstance().SetTiles(&state->GetTiles())  // pass tile data
   ├── GameView::LoadTileset(type, path, cols) × 6           // tilesheets
   ├── GameView::LoadBackgrounds()
   ├── HUDView::GetInstance().Init()            // reset bars
   └── SoundManager::GetInstance().PlayMusic("bgm_gameplay")

3. FRAME LOOP
   while (running) {
     ┌─ Input ──────────────────────────────┐
     │ InputController::Poll() → InputCommand│
     │   • moveLeft, attack, interact, ...   │
     └─────────── Controller ────────────────┘
           ↓
     ┌─ Update (Model) ─────────────────────┐
     │ GameState::Update(dt)                 │
     │   ├── Player::Update(dt)             │
     │   │     • Xử lý input → di chuyển     │
     │   │     • Skill cooldown tick         │
     │   ├── Entity::Update(dt) (mỗi entity) │
     │   │     • Enemy AI (Chase/Attack/...) │
     │   │     • Boss phase check            │
     │   ├── CollisionSystem::CheckAll()    │
     │   │     • Quadtree query + AABB test  │
     │   │     • Player vs Enemy → damage    │
     │   │     • Player vs Chest → loot      │
     │   │     • Player vs Checkpoint        │
     │   │     • Projectile vs Entity        │
     │   ├── ElementalSystem::Update(dt)    │
     │   │     • Tick DOT (Burn)             │
     │   │     • Check reactions             │
     │   └── VisibilitySystem::Update()     │
     │         • Fog of War (nếu Warrior)    │
     └──────────────────────────────────────┘
           ↓
      ┌─ Render (View) ──────────────────────┐
      │ // GameView::Update() internally gọi:  │
      │ //   CharacterRenderer::UpdateAll(dt)  │
      │ //   EntityRenderer::Update(dt)        │
      │ //   FloatingTextManager::Update(dt)   │
      │ //   EnemyStatusRenderer::Update(dt)   │
      │ //   ResultView::Update(dt) (nếu mở)   │
      │ //   Camera shake fade                 │
      │ GameView::Update(dt)                   │
      │                                        │
      │ // GameView::Render() internally gọi:   │
      │ //   RenderBackground(camera)          │
      │ //   BeginMode2D                       │
      │ //     RenderTilemap(m_tiles)          │
      │ //     CharacterRenderer::RenderAll()  │
      │ //     EntityRenderer::RenderAll()     │
      │ //     ParticleRenderer::RenderAll()   │
      │ //     EnemyStatusRenderer::Render()   │
      │ //   EndMode2D()                       │
      │ //   FloatingTextManager::Render()     │
      │ //   UIStateManager::RenderAll()       │
      │ GameView::Render(camera, particles, dt)│
      └──────────────────────────────────────┘
    }

4. LEVEL COMPLETE / GAME OVER
   GameController::OnLevelComplete()
   ├── Timer::Stop()
   ├── LevelScoring::Calculate()
   ├── SoundManager::GetInstance().StopMusic("bgm_gameplay")
   ├── UIStateManager::GetInstance().Push(UILayer::Result)
   └── View::ResultView::GetInstance().Show(snapshot)
```

### ObservableList pattern

```cpp
// Model định nghĩa list observable
class Player {
    ObservableList<BuffSlot> m_buffs;
    ObservableList<SkillSlotData> m_skillSlots;
public:
    ObservableList<BuffSlot>& GetBuffs() { return m_buffs; }
};

// View attach
HUDView::Init() {
    auto& buffs = m_player->GetBuffs();
    buffs.Attach([](const ObservableList<BuffSlot>& list){
        // View tự refresh khi Model thay đổi
        RenderBuffIcons(list);
    });
}

// Model thay đổi → View auto-refresh
m_player->GetBuffs().Add({StatusEffect::Burn, 3.0f, 3.0f, 1});
// → callback được gọi tự động
```

### Event flow — ví dụ "Player attack enemy"

```
1. Input: Player nhấn phím Attack
2. Controller: GameController::Update() phát hiện input.attack
3. Model:
   a. Player::Attack() → tạo Projectile (ObjectPool::Acquire)
   b. Projectile di chuyển, CollisionSystem phát hiện va chạm
   c. ElementalSystem::ApplyDamage(enemyId, packet)
      → Enemy::TakeDamage(50)
      → Nếu enemy HP ≤ 0 → Enemy::Die()
         → ObjectPool::Release(projectile)
4. Sound: SoundManager::GetInstance().PlaySound("player_attack")
5. View:
   a. CharacterRenderer::PlayAction(playerId, ACTION_ATTACK)
   b. ParticleRenderer::EmitBurst(enemyPos, 8, WHITE)
   c. FloatingTextManager::Emit(enemyPos, "-50", RED, 1.5f)
   d. GameView::Shake(2.0f, 0.1f)
   e. HUDView::Update() — không đổi (player không bị damage)
```

### Init sequence chi tiết

```cpp
bool GameController::Init() {
    // 1. Sound — init device + load all .wav files
    auto& snd = SoundManager::GetInstance();
    snd.InitAudio();
    snd.LoadSound("player_attack", "assets/sounds/sfx/player_attack.wav");
    snd.LoadSound("ui_hover", "assets/sounds/sfx/ui_hover.wav");
    // ... load tất cả SFX + BGM files

    // 2. Model
    m_gameState = std::make_unique<GameState>();

    // 3. View — preload atlases
    GameView::GetInstance().Init();

    // 4. Menu
    MenuController::GetInstance().Init();

    // 5. BGM
    snd.PlayMusic("bgm_menu");
    return true;
}
```

### Các cách gọi View — Tổng hợp tất cả View methods

**Init (gọi 1 lần):**
| View / System method | Ai gọi |
|---|---|
| `CharacterRenderer::PreloadAtlas(path)` | `GameView::Init()` internally |
| `GameView::Init()` | `GameController::Init()` |
| `GameView::LoadTileset(type, path, cols)` | `GameController::StartLevel()` |
| `GameView::LoadBackgrounds()` | `GameController::StartLevel()` |
| `GameView::SetTiles(&state->GetTiles())` | `GameController::StartLevel()` |
| `HUDView::Init()` | `GameController::StartLevel()` |
| `MenuView::LoadResources(path)` + `Init()` | `MenuController::Init()` |
| `UIStateManager::Init()` | `GameController::Init()` |
| `SoundManager::InitAudio()` + `LoadSound/LoadMusic` | `GameController::Init()` |
| `ResultView::LoadResources(path)` | `GameController::Init()` |

**Mỗi frame (trong GameController::Update):**
```cpp
// 1. Model update
m_gameState->Update(dt);
m_particleSystem.Update(dt);

// 2. View update
View::GameView::GetInstance().Update(dt);
View::HUDView::GetInstance().Update(dt, m_gameState->GetLocalPlayer());

// 3. Model → View data
auto& particles = m_particleSystem.GetActive();

// 4. Render (gọi tất cả trong 1 lần)
View::GameView::GetInstance().Render(m_camera, particles, dt);
```

**Theo event:**
| Event | Controller method | View method(s) |
|---|---|---|
| Level load | `StartLevel()` | `LoadTileset()` ×6, `LoadBackgrounds()`, `SetTiles()`, `Register()` (mỗi entity) |
| Entity die | `OnEntityRemoved()` | `CharacterRenderer::Unregister(id)`, `ParticleRenderer::EmitBurst()`, `FloatingTextManager::Emit()`, `GameView::Shake()` |
| Boss phase | `OnBossPhase()` | `CharacterRenderer::SwitchPhase(id, phase)`, `SoundManager::PlaySound("boss_phase")` |
| Open inventory | `OnInputI()` | `UIStateManager::Push(UILayer::Inventory)`, `InventoryView::Open()` |
| Pause | `OnInputESC()` | `UIStateManager::Push(UILayer::Menu)`, `MenuView::ShowPauseOverlay()` |
| Level complete | `OnLevelComplete()` | `UIStateManager::Push(UILayer::Result)`, `ResultView::Show(snapshot)` |
| Collect item | `OnItemCollect()` | `FloatingTextManager::Emit()`, `SoundManager::PlaySound("coin_pickup")` |

---

# PHẦN 3 — GAP ANALYSIS: NHỮNG GÌ CẦN BỔ SUNG

## 3.1 Model — Các thứ cần thêm/sửa

> `DualWorld` đã bị loại bỏ. Thay bằng single world + asymmetric co-op (Guide/Warrior).

### `GameState.h` — Cần thêm Tile struct + tile storage + role support

```cpp
// Tile struct NGAY TRONG GameState.h:
struct Tile {
    int x = 0, y = 0;
    int tileType = 1;
    int tileId   = -1;
    bool solid   = true;
};

// Thêm vào class GameState:
#include <functional>

std::vector<Tile> m_tiles;

std::unique_ptr<Player> m_remotePlayer;
MapType m_mapType = MapType::Flexible;

std::function<void(Entity*)> OnEntityAdded;
std::function<void(int)>     OnEntityRemoved;

void AddTile(const Tile& t) { m_tiles.push_back(t); }
const std::vector<Tile>& GetTiles() const { return m_tiles; }
void ClearTiles() { m_tiles.clear(); }

Player* GetRemotePlayer() const;
void    SetRemotePlayer(std::unique_ptr<Player>);
MapType GetMapType() const;
void    SetMapType(MapType);
int     GetTotalItems() const;

bool IsCoop() const;
PlayerRole GetLocalRole() const;
PlayerRole GetRemoteRole() const;
```

### `Player.h` — Cần thêm role + vision

```cpp
PlayerRole m_role = PlayerRole::Solo;
float m_visionRadius = 0.0f;     // 0 = full vision (Guide/Solo), >0 = fog radius (Warrior)
bool m_canAttack = true;         // Guide = false, Warrior/Solo = true

void SetRole(PlayerRole r) { m_role = r; }
PlayerRole GetRole() const { return m_role; }
void SetVisionRadius(float r) { m_visionRadius = r; }
float GetVisionRadius() const { return m_visionRadius; }
void SetCanAttack(bool v) { m_canAttack = v; }
bool GetCanAttack() const { return m_canAttack; }
```

### `Player.h` — HP/MP/SP/Ultimate cho HUD

HUDView cần các method sau để hiển thị thanh MP, SP, Ultimate. Hiện tại `Character.h` chỉ có `GetHealth()/GetMaxHealth()`.

```cpp
// Thêm vào class Player (hoặc Character nếu dùng chung):
int GetMP() const;
int GetMaxMP() const;
int GetSP() const;
int GetMaxSP() const;
float GetUltimateCharge() const;
float GetMaxUltimateCharge() const;

// Member variables:
int m_mp = 100;
int m_maxMp = 100;
int m_sp = 50;
int m_maxSp = 100;
float m_ultimateCharge = 0.0f;
float m_maxUltimateCharge = 100.0f;
```

**View method gọi:** `HUDView::Render()` đọc `GetMP()/GetMaxMP()`, `GetSP()/GetMaxSP()`, `GetUltimateCharge()/GetMaxUltimateCharge()` mỗi frame.

---

### `LevelScoring.h` — Cần thêm setter

```cpp
int GetTotalItems() const;
int GetTotalEnemies() const;
void SetTotalItems(int v);
void SetTotalEnemies(int v);
int GetCollectedItems() const;
int GetDefeatedEnemies() const;
```

**View method gọi:** `GameView::RenderTilemap(m_tiles)` đọc `GetTiles()`. `HUDView` đọc `GetClearTime()`. `ResultView` đọc `LevelScoring` snapshot.

---

## 3.2 Utils — `Types.h`

Cần thêm enum:

```cpp
enum class PlayerRole { Solo, Guide, Warrior };
enum class MapType { Flexible = 0, GuideSpecial = 1, WarriorSpecial = 2 };
enum class SkillType { None, Heal, Fireball, Lightning, Shield, Teleport };

// Mở rộng GameMode:
enum class GameMode {
    SinglePlayer,
    AsymCoopHost,    // Guide host
    AsymCoopClient   // Warrior client
};
```

**View method gọi:** Controller dùng enum để quyết định gọi View method nào — `MenuView::ShowRoleSelect()`, `VisibilitySystem` (fog of war cho Warrior).

---

## 3.3 Controller — Cần viết mới hoàn toàn (3 stubs rỗng)

### `InputController.h`

```cpp
struct InputCommand {
    bool moveLeft  = false;
    bool moveRight = false;
    bool jump      = false;
    bool attack    = false;   // chỉ Warrior + Solo
    bool parry     = false;
    bool skill1    = false;
    bool skill2    = false;
    bool ultimate  = false;
    bool interact  = false;   // chỉ Guide
    bool openInventory = false;
    bool pause         = false;
    int  menuDelta   = 0;
    bool menuConfirm = false;
};

class InputController {
public:
    static InputController& GetInstance();
    InputCommand Poll();
private:
    InputController() = default;
};
```

### `GameController.h` — skeleton

```cpp
class GameController {
public:
    static GameController& GetInstance();

    bool Init();
    void StartLevel(int levelNumber, bool isCoop = false);
    void Update(float dt);
    void Render();
    void Shutdown();

    bool IsRunning() const { return m_running; }
    void OnLevelComplete();
    void OnGameOver();

private:
    GameController() = default;

    std::unique_ptr<GameState>  m_gameState;
    Camera2D  m_camera{};
    bool      m_running = false;
    bool      m_paused  = false;
};
```

### `MenuController.h` — skeleton

```cpp
class MenuController {
public:
    static MenuController& GetInstance();

    bool Init();
    void Shutdown();
    void Update(float dt);

    void ShowMainMenu();
    void ShowPauseMenu();

private:
    MenuController() = default;
    void HandleMainMenuInput();
    void HandlePauseInput();
    void HandleLevelSelectInput();
    void HandleRoleSelectInput();

    int m_selected = 0;
    int m_selectedLevel = 1;
    GameMode m_selectedMode = GameMode::SinglePlayer;
};
```

**View method gọi:** `GameController::Init()` gọi `GameView::Init()`, `SoundManager::InitAudio()` + `LoadSound`. `GameController::Update()` gọi `GameView::Update(dt)`, `GameView::Render(camera, particles, dt)`. `MenuController` gọi `MenuView::ShowMainMenu()`, `MenuView::ShowPauseOverlay()`, `MenuView::ShowRoleSelect()`.

---

## 3.4 Factory — `LevelFactory.cpp` gần như rỗng

Cần implement parser cho format mới (single world, role spawn):

```cpp
std::unique_ptr<GameState> LevelFactory::LoadLevel(const std::string& filepath, GameMode mode) {
    auto state = std::make_unique<GameState>();
    std::ifstream file(filepath);
    if (!file.is_open()) return CreateDefaultLevel(1);

    bool hasLocalSpawn = false;
    std::string token;
    while (file >> token) {
        if (token == "maptype") {
            int t; file >> t;
            state->SetMapType(static_cast<MapType>(t));
        }
        else if (token == "width" || token == "height") {
            int v; file >> v;
        }
        else if (token == "tile") {
            Tile t; int solid;
            file >> t.x >> t.y >> t.tileType >> t.tileId >> solid;
            t.solid = (solid != 0);
            state->AddTile(t);
        }
        else if (token == "spawn_solo") {
            float tx, ty; file >> tx >> ty;
            if (mode == GameMode::SinglePlayer) {
                auto p = std::make_unique<Player>(Vector2{tx*TILE_SIZE, ty*TILE_SIZE});
                p->SetRole(PlayerRole::Solo);
                state->SetLocalPlayer(std::move(p));
                hasLocalSpawn = true;
            }
        }
        else if (token == "spawn_guide") {
            float tx, ty; file >> tx >> ty;
            if (mode == GameMode::AsymCoopHost) {
                auto p = std::make_unique<Player>(Vector2{tx*TILE_SIZE, ty*TILE_SIZE});
                p->SetRole(PlayerRole::Guide);
                p->SetCanAttack(false);
                state->SetLocalPlayer(std::move(p));
                hasLocalSpawn = true;
            }
        }
        else if (token == "spawn_warrior") {
            float tx, ty; file >> tx >> ty;
            if (mode == GameMode::AsymCoopClient) {
                auto p = std::make_unique<Player>(Vector2{tx*TILE_SIZE, ty*TILE_SIZE});
                p->SetRole(PlayerRole::Warrior);
                p->SetVisionRadius(120.0f);
                state->SetLocalPlayer(std::move(p));
                hasLocalSpawn = true;
            }
        }
        else if (token == "enemy") {
            std::string type; float tx, ty;
            file >> type >> tx >> ty;
            EnemyType et = (type=="melee") ? EnemyType::Melee
                         : (type=="ranged") ? EnemyType::Ranged : EnemyType::Flying;
            state->AddEntity(EnemyFactory::CreateEnemy({tx*TILE_SIZE, ty*TILE_SIZE}, et));
        }
        else if (token == "chest" || token == "checkpoint") {
            float tx, ty; file >> tx >> ty;
            if (token == "chest")
                state->AddEntity(std::make_unique<Chest>(Vector2{tx*TILE_SIZE, ty*TILE_SIZE}));
            else
                state->AddEntity(std::make_unique<Checkpoint>(Vector2{tx*TILE_SIZE, ty*TILE_SIZE}));
        }
        else if (token == "scoring") {
            int items, enemies; file >> items >> enemies;
            state->SetTotalItems(items);
            state->SetTotalEnemies(enemies);
        }
    }
    if (!hasLocalSpawn) return CreateDefaultLevel(1);
    return state;
}

> **Level file format (`.lvl`):**
> ```
> maptype 0              // 0=Flexible, 1=GuideSpecial, 2=WarriorSpecial
> width 40
> height 30
> tile 0 0 1 5 1         // x y tileType tileId solid
> tile 1 0 1 6 1
> spawn_solo 5 10
> spawn_guide 5 10
> spawn_warrior 15 10
> enemy melee 20 15
> chest 25 20
> checkpoint 30 25
> scoring 10 15          // totalItems totalEnemies
> ```

**View method gọi:** `LevelFactory` không gọi View — nó tạo `GameState`. `GameController::StartLevel()` đọc `GameState` và gọi `CharacterRenderer::Register()` cho mỗi entity, `GameView::LoadTileset()` + `GameView::LoadBackgrounds()`.

---

## 3.5 View — `GameView` API đã sửa

```cpp
// GameView đã sửa: RenderTilemap(const DualWorld*) → RenderTilemap(const std::vector<Tile>&)
// Đã xoá: SetActiveWorldLayer(), GetActiveWorldLayer(), LoadShadowShader()
// Đã xoá: m_activeLayer, m_shadowShader, m_shaderLoaded, m_lightWorldPos
```
**View method gọi:** `GameController::Update()` gọi `GameView::Update()`, `GameView::RenderBackground()`, `GameView::RenderTilemap()` mỗi frame.

---

## 3.6 Systems — `VisibilitySystem` cần tạo mới (Fog of War)

```cpp
// include/Systems/VisibilitySystem.h
class VisibilitySystem {
public:
    void Update(float dt, const Vector2& warriorWorldPos, float visionRadius);
    void Render(const Camera2D& camera);
    bool IsVisible(const Vector2& worldPos) const;
    void SetEnabled(bool enabled);
    bool IsEnabled() const;
};
```

> **Fog = dark overlay + circle hole** tại vị trí Warrior.  
> Chỉ active khi local player role = Warrior.  
> Guide luôn thấy toàn bộ map.

**View method gọi:** `VisibilitySystem::Render()` được `GameController::Update()` gọi trong `EndMode2D()` — vẽ dark overlay + circle hole.

---

## 3.7 Level files — 6 level mới

Thư mục `assets/levels/` cần tạo 6 file:

| File | Type | SP | Co-op roles |
|---|---|---|---|
| `level1.lvl` | Flexible | Solo | Warrior + Guide |
| `level2.lvl` | Flexible | Solo | Warrior + Guide |
| `level3.lvl` | Flexible | Solo | Warrior + Guide |
| `level4.lvl` | GuideSpecial | — | Guide (host) + Warrior |
| `level5.lvl` | GuideSpecial | — | Guide (host) + Warrior |
| `level6.lvl` | WarriorSpecial | — | Guide (host) + Warrior |

---

## 3.8 HUD — Player stats + HUDView

**Vấn đề:** `Player.h` hiện tại chưa có HP/MP/SP stats, coin count, ultimate charge. `HUDView` không có data để render.

**Cần thêm ở Model layer:**

| File | Thay đổi | View method gọi |
|---|---|---|
| `include/Model/Player.h` | Thêm `float m_hp, m_maxHp, m_mp, m_maxMp, m_sp, m_maxSp; int m_coinCount; float m_ultimateCharge; ObservableList<BuffSlot> m_buffs; ObservableList<SkillSlotData> m_skillSlots;` | `HUDView::Update()` đọc `GetHP()`, `GetMP()`, `GetCoinCount()` mỗi frame. `SkillBarView` attach `m_skillSlots`. |
| `src/Model/Player.cpp` | Implement getter/setter, constructor khởi tạo stats theo class | — |
| `include/Utils/Types.h` | Thêm struct `BuffSlot`, `SkillSlotData` (xem §2.6) | — |
| `src/View/HUDView.cpp` | Viết mới: render HP/MP/SP bars, timer, coin count, buff icons | `GameController::Update()` gọi mỗi frame |
| `src/View/SkillBarView.cpp` | Viết mới: attach ObservableList, render skill slots với cooldown | `GameController::Update()` gọi mỗi frame |

**Timer:**
| File | Thay đổi |
|---|---|
| `include/Model/GameState.h` | Thêm `float m_clearTime; void ResetTimer(); void StopTimer(); float GetClearTime() const;` |
| `src/Controller/GameController.cpp` | `Update()`: `if (!m_paused) m_gameState->m_clearTime += dt`. `OnLevelComplete()`: `m_gameState->StopTimer()` |

**Trạng thái:** Cần tạo mới hoàn toàn.

---

## 3.9 ElementalSystem — Reactions + Status Effects

**Vấn đề:** `ElementalSystem.h/.cpp` là stub — chưa có `ApplyDamage()`, `React()`, `DamagePacket`.

**Cần thêm:**

| File | Thay đổi | View method gọi |
|---|---|---|
| `include/Systems/ElementalSystem.h` | Thêm `struct DamagePacket`, `ApplyDamage(entityId, packet)`, `React(entityA, entityB)`, `GetStatus(entityId)`, `SetElementTint(entityId, type)` | `EnemyStatusRenderer::SetStatus()` đọc `GetStatus()`. `ElementalFX::SetElementTint()` gọi từ Controller. |
| `src/Systems/ElementalSystem.cpp` | Implement logic: áp status, DOT tick (Burn), stun (Shocked), reactions Vaporize/Conduct/Overload | — |
| `include/Utils/Types.h` | `StatusEffect` đã có (`None, Burn, Wet, Shocked`). `DamageType` đã có (`Physical, Fire, Water, Thunder`). | — |
| `src/View/ElementalFX.cpp` | Viết mới: tint sprite theo element, glow overlay cho reaction | `CharacterRenderer::RenderAll()` đọc `GetTintForEntity()` để apply tint |

**Dependencies:**
- `CollisionSystem` phát hiện va chạm → gọi `ElementalSystem::ApplyDamage()`
- `ParticleRenderer` nhận event reaction → emit particles
- `SoundManager` phát sound reaction

**Trạng thái:** Stub — cần implement toàn bộ.

---

## 3.10 Effects — ParticleSystem + FloatingText + Camera Shake

**Vấn đề:** `ParticleSystem.h/.cpp`, `FloatingText.h/.cpp`, camera shake trong `GameView` đều là stub hoặc chưa có.

**Cần thêm:**

| File | Thay đổi | View method gọi |
|---|---|---|
| `include/Systems/ParticleSystem.h` | Thêm `struct Particle`, `Emit()`, `Update(dt)`, pool | — |
| `src/Systems/ParticleSystem.cpp` | Implement particle lifecycle + pool | — |
| `include/Systems/ObjectPool.h` | Template pool cho Particle + Projectile | — |
| `src/View/ParticleRenderer.cpp` | Viết mới: `EmitBurst(pos, count, color)`, `RenderAll(particles, camera, dt)` | `GameController::Update()` gọi mỗi frame |
| `src/View/FloatingText.cpp` | Viết mới: `Emit(worldPos, text, color, lifetime)`, `Update(dt)`, `Render(camera)` | `GameController::Update()` gọi mỗi frame. `GameController::OnHit()` gọi `Emit()`. |
| `src/View/GameView.cpp` | Thêm `Shake(intensity, duration)`, offset camera khi render | `GameController::OnHit()`, `OnBossAttack()`, `OnUltimate()` gọi `GameView::Shake()` |

**Trạng thái:** Cần tạo mới hoàn toàn.

---

## 3.11 UI Layer — UIStateManager

**Vấn đề:** Không có cơ chế quản lý overlay stack — modal inventory và pause không thể coexist.

**Cần thêm:**

| File | Thay đổi | View method gọi |
|---|---|---|
| `include/View/UIStateManager.h` | Thêm `enum class UILayer`, `Push()`, `Pop()`, `Clear()`, `IsOverlayActive()`, `RenderAll()` | `GameController::OnInputI()` gọi `Push(Inventory)`. `OnInputESC()` gọi `Push(Menu)`. |
| `src/View/UIStateManager.cpp` | Implement stack + dimming + render ordering | `GameController::Update()` gọi `RenderAll()` cuối mỗi frame |
| `src/Controller/GameController.cpp` | Kiểm tra `IsOverlayActive()` trước khi xử lý game input | — |

**Quy tắc dim:**
- Layer modal (Menu/Inventory/Result) dim các layer dưới (alpha 0.5)
- Layer non-modal (HUD/SkillBar/InteractPrompt) không dim

**Trạng thái:** Cần tạo mới hoàn toàn.

---

## 3.12 SoundManager — Singleton (Đã code)

`SoundManager` đã được implement hoàn chỉnh với string-based API.

| File | Trạng thái | API |
|---|---|---|
| `include/Systems/SoundManager.h` | ✅ Đã code | `InitAudio()`, `CloseAudio()`, `LoadSound(name, path)`, `LoadMusic(name, path)`, `PlaySound("key")`, `PlayMusic("key")`, `StopMusic("key")`, `SetSFXVolume()`, `SetMusicVolume()`, `UnloadAll()` |
| `src/Systems/SoundManager.cpp` | ✅ Đã code | Các file .wav có sẵn trong `assets/sounds/sfx/` (23 file) và `assets/sounds/music/` (3 file). Cần Controller gọi `LoadSound`/`LoadMusic` trong `Init()` để load vào cache.

**Lưu ý:** API dùng string key, không phải enum. Xem §2.10 cho chi tiết.

---

## 3.13 ObjectPool — Template Pool

**Vấn đề:** Projectile, Particle, Effect được tạo mới mỗi lần → allocation overhead + memory fragmentation.

**Cần thêm:**

| File | Thay đổi | View method gọi |
|---|---|---|
| `include/Systems/ObjectPool.h` | Template class: `Acquire()`, `Release(T*)`, `Clear()`, `GetActiveCount()`, pool dùng `std::vector<std::unique_ptr<T>>` | `ParticleRenderer::RenderAll()` đọc `GetActive()` để render particles. `CharacterRenderer` không dùng pool. |
| `src/Systems/ObjectPool.cpp` | (Template — implementation trong .h, không cần .cpp) | — |

**Usage:**
```cpp
class ProjectilePool : public ObjectPool<Projectile> {};
// Hoặc dùng trực tiếp
ObjectPool<Projectile> m_projectilePool;

// Khi cần bắn đạn:
auto* proj = m_projectilePool.Acquire();
proj->Init(position, direction, speed);

// Khi đạn biến mất:
m_projectilePool.Release(proj);
```

**Trạng thái:** Cần tạo mới.

---

## 3.14 Network — Packet + Server/Client

**Vấn đề:** 4 file Network (`Packet.h/.cpp`, `Server.h/.cpp`, `Client.h/.cpp`, `NetworkManager.h/.cpp`) đều là stub.

**Cần thêm:**

| File | Thay đổi | View method gọi |
|---|---|---|
| `include/Network/Packet.h` | `Serialize(int/float/bool/string)`, `Deserialize()`, `GetData()`, `Clear()` | Không gọi View — NetworkManager nhận data → Controller update Model → View tự refresh |
| `src/Network/Packet.cpp` | Implement serialization | — |
| `include/Network/Server.h` | `Init(port)`, `Poll()`, `Broadcast(Packet)`, `Shutdown()` | — |
| `src/Network/Server.cpp` | Implement TCP server | — |
| `include/Network/Client.h` | `Connect(ip, port)`, `Send(Packet)`, `Receive()`, `Disconnect()` | — |
| `src/Network/Client.cpp` | Implement TCP client | — |
| `include/Network/NetworkManager.h` | Singleton: `Init()`, `Update()`, `SendGameState()`, `Shutdown()` | `NetworkManager::Update()` → Controller → View methods |
| `src/Network/NetworkManager.cpp` | Implement state sync | `MenuView::ShowConnectionStatus()`, `MenuView::ShowErrorDialog()` |

**State sync protocol:**
```
Server → Client: enemy positions, boss phase, damage results, chest/checkpoint states
Client → Server: player input (move, attack, skill)
Server-authoritative: combat reactions, loot, scoring
```

**Trạng thái:** Stub — cần implement toàn bộ.

---

## 3.15 Boss Phase — Per-boss config (Model layer cần sửa)

**Vấn đề:** Hiện tại `Boss` class là generic — tất cả boss dùng chung `m_phaseOrder` = `{Phase1, Phase2, Phase3, Enraged}` với stat buff cứng.

**Thực tế mỗi boss có số phase khác nhau:**
- **Boss1 (Knight):** 2 phase (Phase1 → Phase2)
- **Boss2 (Witch):** 3 phase (Phase1 → Phase2 → Phase3)
- **Boss3 (4-phase):** 4 phase (Phase1 → Phase2 → Phase3 → Enraged)

**Cần sửa ở Model layer:**

| File | Thay đổi | View method gọi |
|---|---|---|
| `Model/Boss.h` | Thêm field `BossType m_bossType` + setter/getter | `CharacterRenderer::SwitchPhase()` đọc `Boss::GetPhase()` để chọn phase atlas |
| `Model/Boss.cpp` | Constructor nhận `BossType`, phaseOrder theo từng loại | — |
| `Utils/Types.h` | Thêm `enum class BossType { Knight, Witch, FourPhase };` | — |

### Boss 2 Phase behavior (tham khảo từ master_builder_guide §2.2.4)

- **Phase1 (default):** Bắn đạn đơn, đứng xa player
- **Phase2 (HP < 60%):** Bắn đạn chùm + hồi máu định kỳ
- **Phase3 (HP < 30%):** Teleport né tránh + bắn liên tục (gồm cả projectile_attack2)

### View layer đã fix

- `CharacterRenderer::SwitchPhase()` — hỗ trợ per-boss path qua `SetBossAssetRoot()` + auto-detect từ atlas path trong `Register()`
- `GameView.cpp` — phase4 preloads đã chuyển thành phase3
- Assets `phase4/` → `phase3/` (projectile_attack2, dead)

---

## 3.16 Attack & Skill Specs

Tất cả attack đều có hướng trùng với **facing** của entity.

### Quy tắc chung
- **Ranged attack** (bắn đạn/projectile): hướng theo facing entity
- **Cận chiến** (melee): không cần spec đặc biệt

### Player Skills

#### Fighter
| Skill | Loại | Ghi chú |
|---|---|---|
| Attack 1 | Cận chiến | Chém |
| Attack 2 | Cận chiến | Chém |
| Attack 3 | Cận chiến | Chém |
| Ultimate | Tầm xa | Bắn tầm xa (giống bắn cung) |

#### Knight
| Skill | Loại | Ghi chú |
|---|---|---|
| Attack 1 | Cận chiến | |
| Attack 2 | Cận chiến | |
| Attack 3 | Cận chiến | |
| Ultimate | Cận chiến | |

#### Magic Caster
| Skill | Loại | Ghi chú |
|---|---|---|
| Attack 1 | Triệu hồi | Tại vị trí quái gần nhất |
| Attack 2 | Tầm xa | Bắn quả cầu lửa (giống bắn cung) |
| Attack 3 | Tầm xa | Bắn tầm xa (giống bắn cung) |
| Ultimate | Triệu hồi | Giống attack 1 |

#### Ninja
| Skill | Loại | Ghi chú |
|---|---|---|
| Attack 1 | Cận chiến | |
| Attack 2 | Tầm xa | Ném kunai (giống bắn cung) |
| Attack 3 | Đặc biệt | Teleport lại gần địch gần nhất |
| Ultimate | Tầm xa | Triệu hồi phân thân chạy tới (giống bắn cung) |

### Boss Attacks

#### Boss 2 — Witch (3 phases)
| Phase | Attack | Loại | Ghi chú |
|---|---|---|---|
| Phase 1 | Attack 1 | Tầm xa | Bắn đạn (giống bắn cung) |
| Phase 2 | Attack 1 | Tầm xa | Bắn đạn (giống bắn cung) |
| Phase 3 | Attack 1 | Tầm xa | Bắn đạn (giống bắn cung) |
| Phase 3 | Attack 2 | Đặc biệt | Xuất hiện tại vị trí player gần nhất |

#### Boss 3 — 4-phase
| Phase | Attack | Loại | Ghi chú |
|---|---|---|---|
| Phase 3 | Attack 2 | Tầm xa | Bắn đạn (giống bắn cung) |
| Phase 4 | Attack 1 | **Cận chiến** | Ground animate xuất hiện tại rìa boss |
| Phase 4 | Attack 2 | Đặc biệt | Tại vị trí player gần nhất |
| Phase 4 | Attack 3 | Tầm xa | Beam kéo dài — frame cuối xuất hiện thì xoá tất cả |

**View method gọi:** `CharacterRenderer::PlayAction(entityId, ACTION_ATTACK)` — Controller gọi cho mỗi attack. Clip name khớp với attack type (attack1, attack2, ...). Ranged attack dùng clip projectile_attack.

---

## 3.17 Tóm tắt bảng gap

| # | File/Class | Trạng thái | Việc cần làm | View method gọi |
|---|---|---|---|---|
| 1 | `Utils/Types.h` | Thiếu enum | Thêm `PlayerRole`, `MapType`, `SkillType`, `BossType`, update `GameMode` | — (enum được các View dùng để check) |
| 2 | `Model/GameState.h/.cpp` | Thiếu fields | Thêm Tile struct, tile storage, callbacks, remote player, mapType, role helpers | `GameView::RenderTilemap()` đọc `GetTiles()`. `HUDView` đọc `GetClearTime()`. |
| 3 | `Model/Player.h` | Thiếu fields | Thêm role, visionRadius, SetCanAttack(), skillSlots (ObservableList) | `HUDView::Update()` đọc HP/MP/SP. `SkillBarView` attach `m_skillSlots`. |
| 4 | `Model/LevelScoring.h` | Thiếu getter | Thêm GetTotalItems(), GetTotalEnemies(), SetTotalItems() | `ResultView::Show()` nhận snapshot, đọc scores. |
| 5a | `Model/DualWorld.h` + `.cpp` | **Còn tồn tại** | File vẫn trên disk (32+47 lines). Cần xoá hẳn — kiến trúc DualWorld đã loại bỏ. | — |
| 5b | `Model/GameState.h` + `Model/DualWorld.h` | **Đã sửa** | `DualWorld.h` giờ include `GameState.h` và dùng chung `struct Tile`. Không còn redefinition. | — |
| 6 | `Factories/LevelFactory.cpp` | Đã code (96 lines) | Có `LoadLevel()`, `SaveLevel()`, `LoadDualWorld()`, `SaveDualWorld()`. Cần sửa để dùng single world format mới. | Không gọi View — tạo GameState. Controller gọi View sau khi load. |
| 7 | `Controller/GameController.h/.cpp` | Stub | Viết mới: single world, fog, co-op camera, role-based input | Gọi tất cả View methods mỗi frame + theo event. |
| 8 | `Controller/MenuController.h/.cpp` | Stub | Viết mới: level select, co-op role selection | Gọi `MenuView::ShowMainMenu()`, `ShowPauseOverlay()`, `ShowRoleSelect()`. |
| 9 | `Controller/InputController.h/.cpp` | Stub | Viết mới + InputCommand struct | Không gọi View — Controller đọc InputCommand rồi quyết định View method. |
| 10 | `Systems/VisibilitySystem.h/.cpp` | Chưa có | Tạo mới: fog of war overlay | `VisibilitySystem::Render()` vẽ dark overlay. `GameController::Update()` gọi. |
| 11 | `View/GameView.h/.cpp` | Đã sửa | RenderTilemap(tiles), xoá layer switching, Boss 2 phase4→phase3 | `GameController::Update()` gọi Update/Render mỗi frame. |
| 12 | `assets/levels/level1-6.lvl` | Trống | Tạo 6 level files | Không gọi View — LevelFactory parse → GameState → View đọc. |
| 13 | `Model/Player.h` | Thiếu stats | Thêm MP/SP/Ultimate getter (xem §3.1 Player.h — HUD) | `HUDView::Render()` đọc `GetMP()/GetSP()/GetUltimateCharge()` mỗi frame. |
| 14 | `View/HUDView.cpp` | Đã code (235 lines) | Bỏ mock data MP/SP/Ult khi Model có API | `GameController::Update()` gọi mỗi frame. |
| 15 | `View/SkillBarView.cpp` | Đã code (232 lines) | — | `GameController::Update()` gọi mỗi frame. |
| 16 | `Systems/ElementalSystem.h/.cpp` | Đã code (187 lines) | DamagePacket, ApplyStatusEffect, Reaction xong. Cần thêm DOT tick timer. | `EnemyStatusRenderer` đọc `GetStatus()`. `ElementalFX` apply tint. |
| 17 | `Systems/ParticleSystem.h/.cpp` | Đã code | Particle struct, ObjectPool, Emit, Update, GetActive — đầy đủ. | `ParticleRenderer::RenderAll()` đọc `GetActiveParticles()`. |
| 18 | `View/ParticleRenderer.cpp` | Đã code (99 lines) | — | `GameController::OnHit()`, `OnEntityRemoved()` gọi `EmitBurst()`. |
| 19 | `View/FloatingText.cpp` | Đã code (43 lines) | — | `GameController` gọi `Emit()` theo event. `Update()` mỗi frame. |
| 20 | `View/UIStateManager.h/.cpp` | Đã code (112 lines) | — | `GameController::OnInputI()` gọi `Push(Inventory)`. `Update()` gọi `RenderAll()`. |
| 21 | `Systems/SoundManager.h/.cpp` | Đã code (175 lines) | Singleton, InitAudio, LoadSound/Music, PlaySound("key"), PlayMusic("key"), volume control — đầy đủ. | `GameController` gọi `PlaySound("key")` theo event. `MenuController` gọi `PlayMusic("key")`. |
| 22 | `Systems/ObjectPool.h` | Đã code | Template pool: Acquire, Release, ReleaseAll, Reserve, GetActiveCount — đầy đủ. | `ParticleRenderer::RenderAll()` đọc `GetActive()` để render. |
| 23 | `Network/Packet.h/.cpp` | Đã code | Append/Read int/float/bool/string — serialization đầy đủ. | Không gọi View — NetworkManager → Controller. |
| 24 | `Network/Server.h/.cpp` | Đã code | TCP server: Start, Stop, AcceptClient, SendToClient, Broadcast, ReceiveFromClient. | Không gọi View. |
| 25 | `Network/Client.h/.cpp` | Đã code | TCP client: Connect, Disconnect, Send, Receive. | Không gọi View. |
| 26 | `Network/NetworkManager.h/.cpp` | Đã code | Wrapper: StartServer, ConnectToServer, Broadcast, Receive, Update. | `MenuView::ShowConnectionStatus()`, `ShowErrorDialog()`. |
| 27 | `Model/CrossWorldManager.h/.cpp` | Đã code nhưng stale | Dùng DualWorld/DualWorldPlayer — cần sửa single world hoặc xoá. | — |
| 28 | `Model/DualWorldPlayer.h/.cpp` | Còn tồn tại | File stale từ DualWorld. Cần xoá. | — |
| 29 | `include/Model/Entity.h` | `EntityType::DualWorldPlayer` còn | Xoá enum value nếu không dùng. | — |
| 30 | `View/MenuView.h/.cpp` | **Đã code** | `ShowRoleSelect()`, `MenuMode::RoleSelect`, `RenderRoleSelect()`. | `MenuController` gọi khi vào Co-op mode. |
| 31 | `View/ParticleRenderer.h/.cpp` | **Đã code** | `EmitReaction(pos, ReactionType)` với `ReactionType` enum local. 3 reaction: Vaporize/Conduct/Overload. | `ElementalSystem::React()` gọi. |
| 32 | `View/UIStateManager.h/.cpp` | **Đã code** | `IsLayerActive(layer)` — duyệt stack kiểm tra. | Controller check trước khi handle input. |
| 33 | `Utils/Types.h` | Thiếu enum | Cần thêm `PlayerRole { Solo, Guide, Warrior }` và `ReactionType { Vaporize, Conduct, Overload }`. `GameMode` cần sửa thành `{ SinglePlayer, AsymCoopHost, AsymCoopClient }`. `BossType`, `MapType` cũng thiếu. | View method `MenuView::ShowRoleSelect()` cần `PlayerRole`. `ParticleRenderer::EmitReaction()` dùng enum local tạm. |


