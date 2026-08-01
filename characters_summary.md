# Apple Knight Adventure — Tóm Tắt Nhân Vật, Kẻ Địch & Thực Thể

> **Trạng thái codebase**: Tất cả logic Model đã được implement đầy đủ (không còn stub).  
> Các file `.cpp` trong `src/Model/`, `src/Systems/`, `src/Factories/` đều có code thực.

---

## 1. 🧙 PLAYER — Nhân Vật Người Chơi

### Cây kế thừa
```
Entity
 └── Character  (HP, speed, direction, state machine, attack cooldown, gravity)
      └── Player  (inventory, score, dash, sprint, KnightSkillSet)
           └── DualWorldPlayer  (WorldLayer: Light / Shadow)
```

### `Player` — Lớp chính

| Thuộc tính | Giá trị |
|---|---|
| HP tối đa | 100 |
| Tốc độ cơ bản | 200 px/s |
| Kích thước hitbox | 0.5 × 0.9 tile |
| Jump force | −600 px/s |
| Trọng lực | 980 px/s² |

**Cơ chế di chuyển:**
- **Sprint** (`IsSprinting`) — nhân tốc độ × 1.4
- **Dash** — lunge (có di chuyển) hoặc dodge (tại chỗ):
  - Thời gian: 0.22s, tốc độ lunge: 520 px/s, cooldown: 1.0s
  - **Invincible** trong suốt thời gian dash

**State machine Player:**
`Idle → Walk → Jump → Fall → Attack / Attack2 / Attack3 → Dead`

---

### ⚔️ `KnightSkillSet` — 3 Chiêu Thức Knight

| Slot | Tên chiêu | Phím | Damage | Charge | Active | Cooldown | Ghi chú |
|---|---|---|---|---|---|---|---|
| Attack 1 | **Quick Slash** | `J` | 20 | Không | 0.20s | 0.35s | Instant, hitbox trước mặt |
| Attack 2 | **Heavy Strike** | `K` | 45 | 0.30s | 0.25s | 0.75s | Phải charge; hitbox rộng × 2.2, slam xuống |
| Attack 3 | **Lunge Thrust** | `U` | 32 | Không | 0.45s | 1.25s | Lao nhanh (1000 px/s) về phía trước |

> **Attack2 HitBox**: rộng `playerSize.x × 2.2`, cao `playerSize.y × 0.9`, xuất hiện phía trước + hơi thấp (cảm giác đập xuống).

**Các class character khác** (`Fighter`, `Ninja`, `MagicCaster`) được định nghĩa trong enum `CharacterClass` và `SkillType` nhưng **chưa có logic riêng** — dùng chung `Player` base.

---

### `DualWorldPlayer` — Co-op Dual World

Kế thừa hoàn toàn từ `Player`, bổ sung:
- `WorldLayer m_layer` — `Light` hoặc `Shadow`
- `SwitchLayer()` — đổi layer ngay lập tức (dùng trong cơ chế DualWorld co-op)

---

## 2. 👾 ENEMIES — Kẻ Thù

### Cây kế thừa
```
Entity
 └── Character
      └── Enemy  (EnemyType, EnemyState, AI state machine)
```

### 3 Loại Enemy (được tạo bởi `EnemyFactory`)

| Loại | HP | Damage | Speed | Detection | Attack Range | Patrol Range | Cơ chế đặc biệt |
|---|---|---|---|---|---|---|---|
| **Melee** | 50 | 15 | 120 px/s | 200 px | 100 px | 100 px | Chỉ di chuyển ngang, có wind-up 0.75s |
| **Ranged** | 30 | 10 | 80 px/s | 300 px | 300 px | 120 px | Bắn đạn từ xa, spawn projectile |
| **Flying** | 40 | 12 | 100 px/s | 250 px | 100 px | 80 px | Di chuyển 2D toàn hướng, tấn công không cần wind-up |

### AI State Machine

```
Idle ──(detect player)──► Chase ──(in range)──► WindUp ──(0.75s)──► Attack
  ▲                         │                      │
  │                    (out of range)         (player escapes)
  │                         ▼                      │
  └─────────────────── Patrol ◄────────────────────┘
                            │
                       (take damage)
                            ▼
                          Hurt ──(0.4s)──► Chase
                            │
                         (HP = 0)
                            ▼
                           Dead ──(0.5s)──► deactivate
```

**Chi tiết hành vi:**
- **Idle**: chờ 2s rồi tự chuyển sang Patrol
- **Patrol**: dao động sin quanh spawn position, Flying thêm float về chiều Y
- **Chase**: Ground chỉ MoveX; Flying dùng Move 2D  
- **WindUp** (0.75s): đứng yên, dùng animation Attack nhưng **chưa gây damage**
- **Attack** (0.6s): damage window; Flying rút lui về sau khi xong
- **Hurt**: đứng yên 0.4s rồi Chase lại
- **Dead**: đứng yên 0.5s rồi `m_active = false`

---

## 3. 👑 BOSS

### Cây kế thừa
```
Entity
 └── Character
      └── Boss  (BossPhase, enrage threshold, phase order)
```

### Thống số Boss (base)

| Thuộc tính | Giá trị |
|---|---|
| HP tối đa | 500 |
| Speed | 60 px/s |
| Damage | 20 (Phase1) |
| Detection range | 400 px |
| Attack range | 80 px |
| Attack cooldown | 1.5s |
| Enrage threshold | 20% HP |

### Phase System — 4 Pha

| Phase | Kích hoạt | Damage | Speed | Cooldown | Scale |
|---|---|---|---|---|---|
| **Phase 1** | Bắt đầu | 20 | 60 | 1.5s | 1.0 |
| **Phase 2** | HP giảm | +10 (→30) | ×1.2 (→72) | — | — |
| **Phase 3** | HP giảm | +15 (→45) | — | ×0.8 (→1.2s) | — |
| **Enraged** | HP ≤ 20% | ×2 (→90) | ×1.5 (→108) | ×0.6 (→0.72s) | 1.3× |

> **`ExecutePhaseBehavior()`** là `virtual` — được override ở subclass cụ thể để thêm behavior riêng từng phase.

### Boss AI
- Tiếp cận player khi trong detection range
- Tấn công khi `dist ≤ attackRange && CanAttack()`
- Mỗi lần `TakeDamage()`: tự kiểm tra enrage threshold → `TransitionToNextPhase()`

---

## 4. 🐾 PETS — Thú Cưng AI

### 4 Loại Pet

| Pet | Damage | AI Role | Cơ chế |
|---|---|---|---|
| **Skull** | 8 | — | Chỉ follow (chưa implement AI riêng) |
| **Ghost** | 0 | Healer | Heal player khi **không combat**, budget 70 HP/level, 8 HP/s, tick 0.3s |
| **BabyDragon** | 18 | Ranged DPS | Detect enemy ≤ 320px, charge 0.6s, bắn homing projectile mỗi 1.2s |
| **Fairy** | 4 | — | Chỉ follow (chưa implement AI riêng) |

### Follow Behavior (tất cả Pet)
- Bay vòng tròn (circular hover): radius 25px, speed 2.0
- Offset: `(-60, -40)` từ player + sin/cos oscillation
- Spring-follow: `speed = baseSpeed + dist × 4.0`

### BabyDragon Projectile
- **Homing**: `m_isHoming = true`, steering force 280 px/s
- Tự tìm target gần nhất trong range 320px
- Spawn bởi `GameController` khi `WantsToFire() == true`

### Ghost Heal Logic
- `CanHeal()`: `healBudget > 0 && playerHP < maxHP`
- `HealPlayer()`: tick mỗi 0.3s, heal `8 × 0.3 = 2.4 HP/tick ≈ 2 HP/tick`
- Khi combat: chuyển `Idle`, không heal

---

## 5. 🎯 PROJECTILES — Đạn

| Type | Dùng bởi | Ghi chú |
|---|---|---|
| `Arrow` | Ranged enemy | Thẳng theo hướng |
| `Magic` | Player / Boss | Thẳng |
| `BossAttack` | Boss | Damage cao |
| `RangedBomb` | Ranged enemy | Có trọng lực (`GRAVITY_PROJECTILE = 200`) |
| `FlyingProjectile` | Flying enemy | Hướng theo 2D |

**Homing** (BabyDragon): `SetHoming(true)`, update `m_homingTargetPos` mỗi frame từ Controller.

---

## 6. 🔥 ELEMENTAL SYSTEM — Hệ Thống Nguyên Tố

### Status Effects

| Effect | Gây bởi |
|---|---|
| `Burn` | Fire damage |
| `Wet` | Water damage |
| `Shocked` | Thunder damage |

### Elemental Reactions (đã implement)

| Existing + Incoming | Reaction | Bonus Damage | Kết quả |
|---|---|---|---|
| Burn + Water | **Vaporize** | +100% (= base) | Xóa Burn |
| Wet + Thunder | **Conduct** | +150% | → Shocked 2s |
| Wet + Fire | **Vaporize** | +120% | Xóa Wet |
| Shocked + Fire | **Overload** | +200% | → Burn 3s |
| Burn + Thunder | **Overload** | +180% | Xóa Burn |

---

## 7. 🗺️ WORLD ENTITIES — Các Entity Khác

### `Chest` — Hộp Báu Vật
- Mở được 1 lần (`IsOpened()`)
- Tạo 1–3 item random khi mở (`CHEST_MIN_LOOT=1`, `CHEST_MAX_LOOT=3`)
- Loot hiện tại: **Coin** (giá trị 1–5), có `physicsEnabled` để coin văng ra

### `Item` — Vật Phẩm

| Type | Mô tả |
|---|---|
| `Coin` | Tiền, có physics khi văng từ rương |
| `Apple` | Hồi máu |
| `Key` | Mở khóa |
| `Potion` | Thuốc |
| `Equipment` | Trang bị |

### `Checkpoint` — Điểm Hồi Sinh
- `Activate()` / `Deactivate()`
- `m_isEndGame`: nếu `true` → kết thúc màn chơi khi chạm

### `FakeWall` — Tường Giả
- HP = 3 (`FAKE_WALL_HEALTH`)
- Có thể phá bằng vũ khí

---

## 8. 📊 TỔNG QUAN TRẠNG THÁI IMPLEMENTATION

| Hạng mục | Trạng thái | Ghi chú |
|---|---|---|
| `Entity` base | ✅ Đầy đủ | transform, id, active, bounding box |
| `Character` base | ✅ Đầy đủ | HP, speed, direction, gravity, state, attack |
| `Player` | ✅ Đầy đủ | Sprint, Dash (invincible), 3 attack slots, score, inventory |
| `DualWorldPlayer` | ✅ Đầy đủ | WorldLayer switch |
| `KnightSkillSet` | ✅ Đầy đủ | 3 chiêu với charge/active/cooldown logic |
| `Enemy` (Melee/Ranged/Flying) | ✅ Đầy đủ | AI state machine, patrol, chase, windup, attack |
| `Boss` | ✅ Đầy đủ | 4 phase, enrage, phase transitions |
| `Pet` (Ghost/BabyDragon) | ✅ Đầy đủ | Heal + homing dragon AI |
| `Pet` (Skull/Fairy) | ⚠️ Partial | Chỉ follow, chưa có AI đặc biệt |
| `Projectile` | ✅ Đầy đủ | 5 loại, homing support |
| `ElementalSystem` | ✅ Đầy đủ | 5 reactions: Vaporize, Conduct, Overload |
| `Chest` | ✅ Đầy đủ | Random loot (Coin) |
| `Item` | ✅ Đầy đủ | 5 loại, physics flag |
| `Checkpoint` | ✅ Đầy đủ | Activate/Deactivate, EndGame flag |
| `FakeWall` | ✅ Đầy đủ | HP=3, có thể phá |
| Fighter/Ninja/MagicCaster class | ❌ Chưa có | Chỉ có enum, chưa có class/skill riêng |
| `ExecutePhaseBehavior` (Boss) | ❌ Placeholder | Thân hàm rỗng — cần override ở subclass |
| Pet AI (Skull/Fairy) | ❌ Chưa có | Chỉ follow player |

---

## 9. 🔗 File References

| File | Link |
|---|---|
| Entity.h | [Entity.h](file:///c:/Code/CS202/CS202-FinalProject/AppleKnightAdventure/include/Model/Entity.h) |
| Character.h | [Character.h](file:///c:/Code/CS202/CS202-FinalProject/AppleKnightAdventure/include/Model/Character.h) |
| Player.h | [Player.h](file:///c:/Code/CS202/CS202-FinalProject/AppleKnightAdventure/include/Model/Player.h) |
| Player.cpp | [Player.cpp](file:///c:/Code/CS202/CS202-FinalProject/AppleKnightAdventure/src/Model/Player.cpp) |
| KnightSkillSet.h | [KnightSkillSet.h](file:///c:/Code/CS202/CS202-FinalProject/AppleKnightAdventure/include/Model/KnightSkillSet.h) |
| KnightSkillSet.cpp | [KnightSkillSet.cpp](file:///c:/Code/CS202/CS202-FinalProject/AppleKnightAdventure/src/Model/KnightSkillSet.cpp) |
| DualWorldPlayer.h | [DualWorldPlayer.h](file:///c:/Code/CS202/CS202-FinalProject/AppleKnightAdventure/include/Model/DualWorldPlayer.h) |
| Enemy.h | [Enemy.h](file:///c:/Code/CS202/CS202-FinalProject/AppleKnightAdventure/include/Model/Enemy.h) |
| Enemy.cpp | [Enemy.cpp](file:///c:/Code/CS202/CS202-FinalProject/AppleKnightAdventure/src/Model/Enemy.cpp) |
| Boss.h | [Boss.h](file:///c:/Code/CS202/CS202-FinalProject/AppleKnightAdventure/include/Model/Boss.h) |
| Boss.cpp | [Boss.cpp](file:///c:/Code/CS202/CS202-FinalProject/AppleKnightAdventure/src/Model/Boss.cpp) |
| Pet.h | [Pet.h](file:///c:/Code/CS202/CS202-FinalProject/AppleKnightAdventure/include/Model/Pet.h) |
| Pet.cpp | [Pet.cpp](file:///c:/Code/CS202/CS202-FinalProject/AppleKnightAdventure/src/Model/Pet.cpp) |
| Projectile.h | [Projectile.h](file:///c:/Code/CS202/CS202-FinalProject/AppleKnightAdventure/include/Model/Projectile.h) |
| ElementalSystem.h | [ElementalSystem.h](file:///c:/Code/CS202/CS202-FinalProject/AppleKnightAdventure/include/Systems/ElementalSystem.h) |
| ElementalSystem.cpp | [ElementalSystem.cpp](file:///c:/Code/CS202/CS202-FinalProject/AppleKnightAdventure/src/Systems/ElementalSystem.cpp) |
| EnemyFactory.cpp | [EnemyFactory.cpp](file:///c:/Code/CS202/CS202-FinalProject/AppleKnightAdventure/src/Factories/EnemyFactory.cpp) |
| Types.h | [Types.h](file:///c:/Code/CS202/CS202-FinalProject/AppleKnightAdventure/include/Utils/Types.h) |
| Constants.h | [Constants.h](file:///c:/Code/CS202/CS202-FinalProject/AppleKnightAdventure/include/Utils/Constants.h) |
