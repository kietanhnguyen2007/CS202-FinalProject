# View API Integration Guide

Tài liệu này dành cho team **Controller**, **System**, **Model** — hướng dẫn sử dụng View APIs và quy ước khi thêm feature mới.

---

## 1. Tổng quan

- **View là passive layer**: chỉ đọc dữ liệu từ Model, không mutate. Controller là người gọi View APIs.
- **Tất cả View classes** đều là singleton: `ClassName::GetInstance().Method(...)`.
- **Coordinate system**: View dùng world coordinates cho in-game entities, screen coordinates cho UI.
- **Asset path**: `assets/` root directory. Atlas frame names theo convention trong từng class.

---

## 2. Initialization Flow

Controller phải gọi theo thứ tự này **một lần** khi game khởi động (sau `InitWindow()`).

**Quan trọng**: Tất cả hàm `Init()` đều trả về `bool`. Controller **bắt buộc kiểm tra** và gọi `ShowErrorDialog` nếu thất bại.

```cpp
// 1. Core renderer
if (!View::Renderer::GetInstance().Init()) {
    // Critical failure — cannot render anything
    return;
}

// 2. Game view (tilesets, shaders, particles)
if (!View::GameView::GetInstance().Init()) {
    View::MenuView::GetInstance().ShowErrorDialog("Failed to init GameView");
    return;
}

// 3. UI components
if (!View::HUDView::GetInstance().Init()) {
    View::MenuView::GetInstance().ShowErrorDialog("Failed to init HUD");
    return;
}
View::HUDView::GetInstance().LoadResources("assets/ui/ui_atlas.json");

if (!View::MenuView::GetInstance().Init()) {
    // Fallback: game can still run without menu
}
View::MenuView::GetInstance().LoadResources("assets/ui/ui_atlas.json");

if (!View::InventoryView::GetInstance().Init()) { /* non-critical */ }
View::InventoryView::GetInstance().LoadResources("assets/ui/ui_atlas.json");

if (!View::SkillBarView::GetInstance().Init()) { /* non-critical */ }
View::SkillBarView::GetInstance().LoadResources("assets/ui/ui_atlas.json");

if (!View::ResultView::GetInstance().Init()) { /* non-critical */ }

View::UIStateManager::GetInstance().Init();

View::EnemyStatusRenderer::GetInstance().LoadResources("assets/ui/ui_atlas.json");

// 4. Load tilesheets (gọi sau InitWindow, trước game loop):
// if (!GameView::GetInstance().LoadTileset(0, "assets/tilesets/ground.png", 8)) { error }
```

**Shutdown** (khi thoát game):
```cpp
View::UIStateManager::GetInstance().Shutdown();
View::GameView::GetInstance().Shutdown();
View::ResultView::GetInstance().Shutdown();
View::InventoryView::GetInstance().Shutdown();
View::MenuView::GetInstance().Shutdown();
View::HUDView::GetInstance().Shutdown();
View::SkillBarView::GetInstance().Shutdown();
View::Renderer::GetInstance().Shutdown();
```

---

## 3. Per-Frame Render Order

Mỗi frame Controller gọi theo thứ tự:

```cpp
// A. World pass
GameView::GetInstance().Update(dt);
// Nếu có DualWorld:
// GameView::GetInstance().RenderTilemap(dualWorldPtr);
GameView::GetInstance().Render(camera, particleSystem.GetActive(), dt);

// B. UI pass — single call, UIStateManager handles order + dimming
UIStateManager::GetInstance().RenderAll();
```

`UIStateManager::RenderAll()` tự động:
- Render HUD → SkillBar → InteractPrompt (luôn visible nếu active)
- Render các modal layer (Menu → Inventory → Result) theo stack order
- Tự động vẽ dim overlay cho layer dưới top layer
- Controller **không cần** gọi SetVisible/Hide thủ công cho từng component

---

## 4. View API Reference — Controller Gọi Gì Và Khi Nào

### 4.1 GameView

| API | Khi nào gọi | Ghi chú |
|-----|-------------|---------|
| `Init()` | Startup | Sau Renderer::Init |
| `Shutdown()` | Shutdown | Giải phóng tilesets + shader |
| `LoadTileset(tileType, path, cols)` | Startup, sau InitWindow | Mỗi tileType một tilesheet riêng |
| `RenderTilemap(DualWorld*)` | Mỗi frame, trước Render | Vẽ Background layer trước, Shadow layer nếu active |
| `SetActiveWorldLayer(layer, lightPos)` | Khi DualWorld switch layer | Light/shadow shader |
| `GetActiveWorldLayer()` | Bất kỳ | Query current layer |
| `Shake(intensity, duration)` | Khi player nhận damage | Random offset camera |
| `Render(camera, particles, dt)` | Mỗi frame | World rendering |

### 4.2 HUDView

| API | Khi nào gọi | Ghi chú |
|-----|-------------|---------|
| `Init()` / `Shutdown()` | Startup / Shutdown | |
| `LoadResources(path)` | Startup | Load UI atlas |
| `Update(dt, playerPtr)` | Mỗi frame | Cập nhật HP, coins từ Player |
| `Render()` | Mỗi frame, sau GameView::Render | HP bar, coins counter |
| `SetVisible(bool)` | Khi inventory/pause mở | Ẩn HUD khi UI overlay |

### 4.3 CharacterRenderer

| API | Khi nào gọi | Ghi chú |
|-----|-------------|---------|
| `Register(entity, atlasPath, defaultClip)` | Khi entity spawn | Trả về false nếu atlas load fail |
| `Unregister(entityId)` | Khi entity bị destroy | Bắt buộc — tránh dangling pointer |
| `Clear()` | Khi chuyển level | Xoá tất cả entity |
| `SetActionClipMap(type, {action→clip})` | Startup, one-time per type | Map ACTION_ATTACK → "attack" |
| `SetInferFunction(type, fn)` | Startup, one-time per type | Tự động switch clip theo action |
| `UpdateAll(dt)` | Mỗi frame | Advance animators |
| `RenderAll()` | Mỗi frame (trong GameView::Render) | Draw all entities |
| `PlayAction(entityId, ACTION_ATTACK/HURT/SKILL)` | Khi entity attack/hurt | Force play one-shot clip |
| `SetBossPhase(entityId, BossPhase)` | Khi boss chuyển phase | Phase overlay (glow + tint) |
| `ClearBossPhase(entityId)` | Khi boss die | |

### 4.4 EntityRenderer

| API | Khi nào gọi | Ghi chú |
|-----|-------------|---------|
| `Register(entity, tex, src, origin, flipX)` | Khi entity spawn | Chest, FakeWall, Checkpoint |
| `Unregister(entityId)` | Khi entity destroy | |
| `Clear()` | Khi chuyển level | |
| `UpdateSpriteRect(entityId, rect)` | Chest open, animation frame | |
| `SetEntityVisible(entityId, false)` | FakeWall destroyed | |
| `GetEntityPtr(entityId)` | Bất kỳ | Query registered entity pointer |

### 4.5 ParticleRenderer

| API | Khi nào gọi | Ghi chú |
|-----|-------------|---------|
| `RenderAll(particles, camera, dt)` | Mỗi frame (trong GameView::Render) | |
| `EmitBurst(pos, count=8)` | FakeWall break, Chest open | View-side debris (không qua ParticleSystem) |

### 4.6 FloatingTextManager

| API | Khi nào gọi | Ghi chú |
|-----|-------------|---------|
| `Emit(worldPos, text, color, lifetime)` | Damage, reaction, heal | "VAPORIZE!", "-15", "+20 HP" |
| `Update(dt)` | Mỗi frame | tích hợp trong GameView::Update |
| `Render(camera)` | Mỗi frame | tích hợp trong GameView::Render |

### 4.7 EnemyStatusRenderer

| API | Khi nào gọi | Ghi chú |
|-----|-------------|---------|
| `LoadResources(path)` | Startup | |
| `SetStatus(entityId, pos, burn, wet, shocked)` | Khi ElementalSystem cập nhật | | |
| `ClearStatus(entityId)` | Khi entity die | |
| `Render(camera)` | Mỗi frame | tích hợp trong GameView::Render |

### 4.8 ElementalFX

| API | Khi nào gọi | Ghi chú |
|-----|-------------|---------|
| `SetElementTint(entityId, DamageType)` | Khi weapon imbued | Tự động áp dụng trong CharacterRenderer::RenderAll |
| `ClearElementTint(entityId)` | Khi effect expires | |

### 4.9 InteractPrompt

| API | Khi nào gọi | Ghi chú |
|-----|-------------|---------|
| `Show("Press E to open")` | Khi player đứng gần Chest (chưa open) / Checkpoint (chưa activated) | |
| `Hide()` | Khi player rời khỏi range / Chest đã open | |
| `Render()` | Mỗi frame (UI pass) | |
| `IsVisible()` | Bất kỳ | |

### 4.10 MenuView

| API | Khi nào gọi | Ghi chú |
|-----|-------------|---------|
| `Init()` / `Shutdown()` | Startup / Shutdown | |
| `LoadResources(path)` | Startup | |
| `Update(dt, selectedIndex)` | Mỗi frame | Input selection index |
| `Render()` | Mỗi frame | Render theo mode hiện tại |
| `SetVisible(bool)` | Khi cần ẩn/hiện | |
| `ShowMainMenu()` | Khi game ở main menu | Mode Main |
| `ShowPauseOverlay()` | Khi nhấn Escape | Mode Pause (backdrop mờ) |
| `ShowErrorDialog(msg)` | Khi network lỗi / load fail | Mode Error (modal) |
| `ShowConnectionStatus(ip, connected)` | Khi multiplayer connect/disconnect | Mode Connection |

### 4.11 InventoryView

| API | Khi nào gọi | Ghi chú |
|-----|-------------|---------|
| `Init()` / `Shutdown()` | Startup / Shutdown | |
| `LoadResources(path)` | Startup | |
| `Open()` | Khi player nhấn I | Phát SoundManager "ui_inventory_open" |
| `Close()` | Khi nhấn I hoặc Escape | Phát SoundManager "ui_inventory_close" |
| `IsOpen()` | Bất kỳ | Check trước khi update input |
| `Update(dt)` | Mỗi frame (nếu open) | |
| `Render()` | Mỗi frame (nếu open) | Grid 6x4 |
| `SetInventorySnapshot(const Inventory&)` | Khi inventory thay đổi | Copy dữ liệu từ Model |
| `SetSelectionIndex(index)` | Khi input điều hướng | Row-major |
| `AttachObservable(ObservableList<const Item*>*)` | Khi gắn Inventory vào ObservableList | Auto-update snapshot |
| `DetachObservable()` | Khi shutdown / chuyển level | |
| `RegisterOnRequestUseItem(callback)` | Startup | View gọi callback khi player chọn dùng item |

### 4.12 SkillBarView

| API | Khi nào gọi | Ghi chú |
|-----|-------------|---------|
| `Init()` / `Shutdown()` | Startup / Shutdown | |
| `LoadResources(path)` | Startup | |
| `Update(dt, playerPtr)` | Mỗi frame | Tick cooldown |
| `Render()` | Mỗi frame (UI pass) | 4 skill slots + cooldown overlay |
| `Open()` / `Close()` | Khi skill bar cần ẩn/hiện | |
| `SetSelection(index)` | Khi input chọn skill | Highlight + cooldown check |
| `AttachObservable(ObservableList<SkillSlotData>*)` | Startup | Auto-sync skill slots từ Model |
| `DetachObservable()` | Shutdown | |

### 4.13 ResultView

| API | Khi nào gọi | Ghi chú |
|-----|-------------|---------|
| `Init()` / `Shutdown()` | Startup / Shutdown | |
| `LoadResources(path)` | Startup | |
| `Show(snapshot)` | Khi level complete | Màn hình win (sao, time, kills) |
| `ShowGameOver(snapshot)` | Khi player chết | Màn hình game over (đỏ, retry) |
| `Dismiss()` | Khi player chọn continue/retry | |
| `Update(dt)` | Khi visible | Animation |
| `Render()` | Khi visible | |
| `IsVisible()` | Bất kỳ | |
| `IsGameOver()` | Bất kỳ | Phân biệt win/lose |

---

### 4.14 UIStateManager

| API | Khi nào gọi | Ghi chú |
|-----|-------------|---------|
| `Init()` / `Shutdown()` | Startup / Shutdown | Trả về bool |
| `Push(layer)` | Khi mở modal UI (Inventory, Menu, Result) | Push lên stack, tự động dim layer dưới |
| `Pop()` | Khi đóng modal UI | Pop top layer |
| `Clear()` | Khi chuyển scene | Pop tất cả |
| `GetTopLayer()` | Bất kỳ | Layer nào đang nhận input |
| `IsOverlayActive()` | Bất kỳ | Có modal nào đang mở không |
| `RenderAll()` | Mỗi frame (UI pass) | Render tất cả layer với dimming |

**UILayer enum** (thứ tự render từ dưới lên):
```
HUD(0) → SkillBar(1) → InteractPrompt(2) → Menu(3) → Inventory(4) → Result(5)
```

**Modal layers** (cần Push/Pop): `Menu`, `Inventory`, `Result`
**Non-modal layers** (luôn render khi visible): `HUD`, `SkillBar`, `InteractPrompt`

Controller flow với UIStateManager:
```cpp
// Mở Inventory (HUD tự động dim)
UIStateManager::GetInstance().Push(UILayer::Inventory);
InventoryView::GetInstance().SetInventorySnapshot(player.GetInventory());
InventoryView::GetInstance().SetSelectionIndex(0);

// Đóng Inventory
UIStateManager::GetInstance().Pop();

// Pause game
UIStateManager::GetInstance().Push(UILayer::Menu);
MenuView::GetInstance().ShowPauseOverlay();

// Resume
UIStateManager::GetInstance().Pop();

// Level complete
UIStateManager::GetInstance().Push(UILayer::Result);
ResultView::GetInstance().Show(snapshot);
```

## 5. Event Flow Diagrams

### 5.1 Elemental Reaction

```
ElementalSystem::ApplyReaction(entityId, damagePacket)
  → Controller nhận ReactionResult { bonusDamage, resultingEffect, reactionName }
    → EnemyStatusRenderer::SetStatus(entityId, pos, isBurn, isWet, isShocked)
    → FloatingTextManager::Emit(pos, reactionName, GOLD, 1.2f)
    → ElementalFX::SetElementTint(weaponId, damagePacket.damageType)
```

### 5.2 Chest Open

```
Controller: player gần Chest + nhấn E
  → Chest::Open()                          // Model
    → (loot generated)
  → EntityRenderer::UpdateSpriteRect(chestId, "chest/open")
  → ParticleRenderer::EmitBurst(chestPos, 6)
  → InteractPrompt::Hide()

Controller: player gần Chest (chưa open)
  → InteractPrompt::Show("Press E to open")
```

### 5.3 Boss Phase Change

```
Controller: Boss::GetPhase() thay đổi
  → CharacterRenderer::SetBossPhase(bossId, BossPhase::Phase2)
    → (CharacterRenderer::RenderAll tự vẽ phase glow overlay)

Controller: Boss::IsEnraged()
  → CharacterRenderer::SetBossPhase(bossId, BossPhase::Enraged)
    → (overlay đỏ + scale 1.3x)
```

### 5.4 Level Complete

```
LevelScoring (Model)
  → Controller đọc LevelScoring getters
  → Build LevelResultSnapshot
    → ResultView::Show(snap)              // win
    // hoặc:
    → ResultView::ShowGameOver(snap)      // lose (player chết)
```

### 5.5 FakeWall Destroyed

```
Controller: FakeWall::IsDestroyed()
  → EntityRenderer::SetEntityVisible(wallId, false)
  → ParticleRenderer::EmitBurst(wallPos, 12)
```

### 5.6 Checkpoint Activated

```
Controller: player gần Checkpoint + nhấn E
  → Checkpoint::Activate()
  → InteractPrompt::Hide()
  // (sprite do Controller cập nhật qua EntityRenderer)
```

---

## 6. Data Contracts (View reads from Model)

| View Class | Đọc từ Model | Trường |
|-----------|-------------|--------|
| EntityRenderer | `Entity` | `GetPosition()`, `GetScale()`, `GetRotation()`, `IsActive()`, `GetType()` |
| CharacterRenderer | `Entity` | `GetPosition()`, `GetScale()`, `GetRotation()`, `IsActive()`, `GetType()`, `GetVelocity()` |
| HUDView | `Player` (qua `Character`) | `GetHealth()`, `GetMaxHealth()`, `GetInventory().GetCoins()` |
| InventoryView | `Inventory` | `GetItemCount()`, `GetItem(index)`, `GetItemName()`, `GetAmount()` |
| SkillBarView | `Player` | `GetSkillPoints()` (optional hiển thị) |
| GameView | `DualWorld` | `GetTiles(layer)`, `GetActiveLayer()` |
| EnemyStatusRenderer | `Entity` (qua GetEntityPtr) | `GetPosition()` |
| ParticleRenderer | `Particle*` | `position`, `color`, `size`, `active` |
| ResultView | `LevelResultSnapshot` | `stars`, `clearTime`, `enemiesKilled`, `applesPercent`, `score` |

---

## 7. Contract: Khi Backend Teams Thêm Feature Mới(Tạo file md hoặc team view phải đọc lại)

### 7.1 Model team thêm:

| Thay đổi | Hành động View cần |
|---------|-------------------|
| **EntityType mới** | Thêm case trong `CharacterRenderer::RenderAll` hoặc `EntityRenderer::RenderAll` |
| **Field mới trên Entity** | Nếu cần hiển thị → thêm getter + render code trong View component tương ứng |
| **Enum value mới** (ItemType, SkillType, v.v.) | Nếu có visual khác → thêm sprite/color mapping |
| **Class mới kế thừa Entity** | Nếu cần render riêng → `CharacterRenderer::Register` hoặc `EntityRenderer::Register` |

**⚠️ Memory Safety — BẮT BUỘC**: `CharacterRenderer::Register` và `EntityRenderer::Register` lưu raw `const Entity*`. Nếu Entity bị destroy mà không gọi `Unregister`, View có dangling pointer → crash.

| Vấn đề | Giải pháp (Model team) |
|--------|----------------------|
| Entity destroy không báo View | Model team thêm `std::function<void(int)> m_onDestroyed` trong `Entity.h`. Destructor của Entity gọi callback này. Controller gọi `CharacterRenderer::Unregister(id)` trong callback. |
| Quên Unregister thủ công | Model team implement lifecycle hook trong GameState: mỗi khi `RemoveEntity(id)` hoặc entity die, tự động gọi `EntityRenderer::Unregister(id)` và `CharacterRenderer::Unregister(id)`. |

**View team đã hỗ trợ**:
- `CharacterRenderer::SetOnEntityRemovedCallback(entityId, cb)` — đăng ký callback được gọi khi Unregister
- `EntityRenderer::SetOnEntityRemovedCallback(entityId, cb)` — tương tự
- `CharacterRenderer::IsRegistered(entityId)` — kiểm tra entity đã register chưa

**Controller team nên**:
```cpp
// Khi entity spawn:
CharacterRenderer::GetInstance().Register(entity, "assets/char.json");
// Đăng ký callback cleanup (nếu Model chưa có auto-cleanup):
// Khi entity destroy TRONG Controller:
CharacterRenderer::GetInstance().Unregister(entityId);
```

### 7.2 Systems team thêm:

| Thay đổi | Hành động View cần |
|---------|-------------------|
| **Hệ thống mới có visual output** | Tạo View component mới (singleton pattern) |
| **Effect type mới** | Thêm particle / overlay / tint trong View |
| **ObservableList mới** | View có thể `AttachObservable` để auto-update UI |

### 7.3 Controller team thêm:

| Thay đổi | Hành động View cần |
|---------|-------------------|
| **Game state mới** | Cần View component mới hoặc mở rộng View hiện tại |
| **Input action mới** | Cần API mới trong View (setter/trigger) |

### 7.4 Rule chung

> **Nếu feature có output hiển thị (sprite, text, particle, effect), View phải được thông báo TRƯỚC khi backend implement.**

Quy trình:
1. Backend team xác định nhu cầu visual mới
2. Báo View team qua issue / discussion
3. View team implement component / API
4. Backend team tích hợp sau khi View API có sẵn

### 7.5 Ví dụ cụ thể

**Model muốn thêm EntityType::Totem:**
```
1. Model: Thêm Totem : public Entity
2. View:  Thêm clip map trong CharacterRenderer cho EntityType::Totem
3. Controller: Gọi CharacterRenderer::Register(totem, "assets/totem.json")
```

**Systems muốn thêm hệ thống Weather:**
```
1. Systems: WeatherSystem với Rain/Snow/Sun
2. View:    Thêm WeatherOverlay component (particle overlay fullscreen)
3. Controller: Gọi WeatherOverlay::SetWeather(rain) mỗi khi weather change
```

---

## 8. Asset Convention

View mong đợi asset paths và frame names theo convention:

| Asset | Path | Frame name convention |
|-------|------|----------------------|
| UI atlas | `assets/ui/ui_atlas.json` | `"status/burn"`, `"status/wet"`, `"status/shocked"` |
| Character atlas | tuỳ entity | Clip names: `"idle"`, `"walk"`, `"attack"`, `"hurt"`, `"dead"` |
| Boss glow overlay | tuỳ boss atlas | `"boss/phase2_glow"`, `"boss/phase3_glow"`, `"boss/enraged_glow"` |
| Status icons | UI atlas | `"status/burn"`, `"status/wet"`, `"status/shocked"` |
| Aura effects | entity atlas | `"aura/fire"`, `"aura/water"`, `"aura/thunder"` |
| Tilesheets | `assets/tilesets/*.png` | Tile grid dùng tileId index (cols configurable) |

---

## 9. Notes & Cảnh báo

- **Init() return bool**: Tất cả `Init()` đều trả về `bool`. Controller **phải kiểm tra** — nếu false, gọi `MenuView::ShowErrorDialog(msg)` thay vì tiếp tục khởi tạo.
- **UIStateManager**: Controller nên dùng `UIStateManager::Push/Pop` thay vì gọi SetVisible/Open/Close thủ công. Tránh spaghetti khi nhiều lớp UI chồng lên nhau.
- **Memory Safety**: `CharacterRenderer::Register` và `EntityRenderer::Register` lưu raw pointer. Model team chưa có auto-cleanup → Controller **bắt buộc** gọi `Unregister(entityId)` trước khi Entity destroy. View team đã thêm `SetOnEntityRemovedCallback` và `IsRegistered` để hỗ trợ defensive programming.
- `Renderer::SubmitSprite` trả về `bool`. Nếu false (capacity exceeded), submission bị drop.
- `SoundManager` trong View chỉ dùng cho UI feedback. Gameplay sounds (attack, coin, damage) do Controller gọi.
- `InteractPrompt` chỉ render text + backdrop. Controller quyết định khi nào show/hide dựa trên Model state.
- `SkillBarView` cooldown timers mặc định (2s/5s/1.5s/4s) là placeholder — Controller cần set lại.
