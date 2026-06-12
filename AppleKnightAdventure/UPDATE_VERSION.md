# UPDATE_VERSION

## Files modified (11 files + 1 new)

| # | File | Change |
|---|------|--------|
| 1 | `src/View/UIStateManager.cpp` | Xoá `\|\| true` khiến `IsLayerVisible(Menu)` luôn true |
| 2 | `src/View/HUDView.cpp` | Thêm `#include <cstdio>` cho `snprintf` |
| 3 | `src/View/ParticleRenderer.cpp` | Thêm `#include <cstdlib>` cho `rand()` |
| 4 | `src/View/EnemyStatusRenderer.cpp` | Đổi `Layer::UI` → `Layer::Foreground` (3 SubmitSprite); thêm `GetWorldToScreen2D` cho fallback `DrawText` |
| 5 | `src/View/TextureAtlas.cpp` | Đổi `#include <nlohmann/json.hpp>` → `"nlohmann/json.hpp"` |
| 6 | `include/View/SkillBarView.h` | Thêm `operator==` cho `SkillSlotData` |
| 7 | `src/View/GameView.cpp` | Thêm `EnemyStatusRenderer::Update(dt)` vào `GameView::Update` |
| 8 | `src/View/MenuView.cpp` | Guard `PlaySound` sau khi check `IsAudioInitialized()` |
| 9 | `src/View/InventoryView.cpp` | Guard `PlaySound` sau khi check `IsAudioInitialized()` |
| 10 | `src/View/EntityRenderer.cpp` | Thêm `assert(rawId >= 0)` cho entity ID; xoá comment outdated; xoá no-op line |
| 11 | `src/View/CharacterRenderer.cpp` | Thêm `assert(rawId >= 0)` cho entity ID |
| — | `third_party/nlohmann/json.hpp` | **Mới**: header-only JSON library v3.12.0 |

## What was changed and why

- **UIStateManager**: `IsLayerVisible(Menu)` luôn trả về `true` do `|| true` làm menu đè lên gameplay
- **HUDView/ParticleRenderer**: Thiếu include chuẩn C++ gây lỗi biên dịch trên toolchain strict
- **EnemyStatusRenderer**: Status icon dùng `Layer::UI` + world coords → icon không bám entity khi camera di chuyển. Đổi sang `Layer::Foreground` + `GetWorldToScreen2D` cho text fallback
- **TextureAtlas**: `nlohmann/json.hpp` là external dependency không có trong repo. Thêm vào `third_party/` để build được
- **SkillBarView**: `SkillSlotData` thiếu `operator==` → `ObservableList::Remove/Contains` sẽ lỗi nếu gọi
- **GameView**: `EnemyStatusRenderer::Update(dt)` không được gọi → future-proofing
- **MenuView/InventoryView**: `PlaySound` gọi trước khi audio device init → thêm guard với `IsAudioInitialized()`
- **EntityRenderer/CharacterRenderer**: ID `int` cast xuống `uint32_t` không kiểm tra âm → thêm assert

## Current status

All 10 View bugs đã được sửa. Cần cập nhật CMakeLists.txt để thêm `-I third_party/` vào include paths trước khi build.
