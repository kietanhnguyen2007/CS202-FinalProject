# UPDATE_VERSION — feature/character-skills

## Branch
`feature/character-skills` (tách từ `Tien`)

## Files Created (NEW)

| File | Mô tả |
|------|-------|
| `include/Model/CharacterSkillSet.h` | Abstract base class cho tất cả skill set |
| `include/Model/FighterSkillSet.h` | Skill set cho Fighter (J/K/U melee, H projectile, P parry) |
| `src/Model/FighterSkillSet.cpp` | Implementation Fighter skills |
| `include/Model/MagicCasterSkillSet.h` | Skill set Magic Caster (J lightning, K fireball, U wave, H ult lightning) |
| `src/Model/MagicCasterSkillSet.cpp` | Implementation Magic Caster skills |
| `include/Model/NinjaSkillSet.h` | Skill set Ninja (J slash, K blade rush, U teleport, H shadow clone) |
| `src/Model/NinjaSkillSet.cpp` | Implementation Ninja skills |
| `assets/textures/player/ninja/run.png` | Atlas 1350×160 — 9 frames dash animation Ninja (từ ASU_19) |
| `assets/textures/player/ninja/run.json` | JSON mô tả clip "run" cho atlas trên |

## Files Modified

| File | Thay đổi |
|------|----------|
| `include/Model/Character.h` | Thêm `State::Run`, `State::Dash`, `State::Parry`, `State::Ultimate`; `TakeDamage` → virtual |
| `include/Model/KnightSkillSet.h` | Kế thừa `CharacterSkillSet`; thêm `SkillData ultimate`, `SkillData parry`; lunge speed giảm 1000→650px/s |
| `src/Model/KnightSkillSet.cpp` | `TryUltimate()`, `TryParry()`, `GetUltimateHitBox()`; fix lunge duration 0.15s |
| `include/Model/Player.h` | Generic `m_skills (unique_ptr<CharacterSkillSet>)`; `IsParrying()`, `DoUltimate()`, `TakeDamage` override |
| `src/Model/Player.cpp` | Rewrite: `InitSkills()` theo CharacterClass, state machine: Dash/Parry/Hurt/Run/Ultimate, parry 70% reduction |
| `include/Controller/InputController.h` | Thêm `parryBlock` (P), đổi comment ultimate → H |
| `src/Controller/InputController.cpp` | Map `KEY_H → ultimate`, `KEY_P → parryBlock` |
| `include/Controller/GameController.h` | Thêm includes SkillSet; `SpawnPlayerProjectile`, `SpawnLightningAt`, `UpdatePlayerProjectiles`, `UpdateNinjaTeleport`, `m_playerProjectiles` |
| `src/Controller/GameController.cpp` | `RegisterPlayerVisuals`: parry/ultimate/run/teleport JSONs (optional load); `HandlePlayerInput`: routing 4 class (Knight/Fighter/MagicCaster/Ninja); `UpdateCombat`: hitboxes + projectile spawning theo class; `UpdatePlayerProjectiles`; `UpdateNinjaTeleport` snap position; clear `m_playerProjectiles` khi StartLevel |

## Thay đổi và lý do

### 1. CharacterSkillSet base class
Cho phép `Player` chứa bất kỳ skill set nào qua `std::unique_ptr<CharacterSkillSet>`, và `GameController` dispatch bằng `dynamic_cast` — không cần template, không cần enum switch trong Model.

### 2. Knight: fix lunge + thêm Ultimate
- Lunge giảm từ 1000px/s × 0.45s (450px) → 650px/s × 0.15s (~97px): hợp lý hơn với kích thước tile.
- Ultimate (H): hitbox 3× player width, damage 80, cooldown 5s, short charge 0.20s.
- Parry (P): giảm damage nhận 70%, active 0.30s, cooldown 1.0s.

### 3. Player.cpp: parry reduction trong TakeDamage
`TakeDamage` override check `IsParrying()` (poll các SkillSet cụ thể), reduce damage × 0.3 trước khi gọi `Character::TakeDamage()`.

### 4. Ninja run.png
9 frames từ `ASU_19/dash_ ninja/` được resize xuống 150×160 (căn đáy, giữ tỉ lệ), pack thành atlas 1350×160.

### 5. MagicCaster: lightning instant-at-target
`SpawnLightningAt` đặt projectile tại vị trí enemy gần nhất trong range 500px và deal damage ngay lập tức. Fireball/Wave bay thẳng theo velocity.

### 6. Ninja: teleport + projectile
`UpdateNinjaTeleport` snap nhân vật đến 80px trước enemy sau khi `TELEPORT_START_DURATION` (0.30s). Blade Rush và Shadow Clone dùng fire signal pattern (set flag → GameController spawn 1 frame sau).

## Current Status
- ✅ Syntax check sạch (tất cả file mới và đã sửa)
- ⚠️ Build đầy đủ cần thực hiện bởi user (build cache ninja bị lock trên máy)
- ⚠️ `SetPosition()` và `SetVelocity()` của `Player` phải tồn tại trong `Entity.h/.cpp` (đã được dùng trước đó trong codebase)
- 📝 Animation State → Clip mapping (trong `CharacterRenderer.cpp`) chưa được cập nhật cho `State::Dash`, `State::Parry`, `State::Ultimate` — cần làm khi integrate View layer

## Lệnh để commit (chạy bởi user)
```bash
git add .
git commit -m "feat: add full character skill system (Knight/Fighter/MagicCaster/Ninja), parry/dash/ultimate, ninja run atlas"
```
