# Cập nhật Boss Logic & Backend (Fullstack)

## Files Created / Modified
**Created:**
- `include/Model/Boss1.h`, `include/Model/Boss2.h`, `include/Model/Boss3.h`
- `src/Model/Boss1.cpp`, `src/Model/Boss2.cpp`, `src/Model/Boss3.cpp`
- `assets/textures/boss/boss2/phase1/hurt.json`
- `assets/textures/boss/boss2/phase2/hurt.json`

**Modified:**
- `include/Model/Boss.h` & `src/Model/Boss.cpp`
- `include/Model/Projectile.h` (Added `SetVelocity`)
- `src/Controller/GameController.cpp`
- `src/Controller/MapBuilderController.cpp`
- `src/Factories/LevelFactory.cpp`

## What was changed and why
- **Kiến trúc Boss FSM (State Machine):** Tách biệt logic của 3 Boss thành các class riêng (`Boss1`, `Boss2`, `Boss3`) kế thừa từ `Boss` (base) để dễ quản lý các Phase và logic riêng biệt của từng Boss.
- **Logic Chiêu thức & Edge Cases (Tường/Target):** Đã tích hợp hàm `CheckLineOfSight` (DDA Raycast) và `IsPointSolid` để đảm bảo không một chiêu thức nào (Projectiles, Targeted AoE, Beams) có thể xuyên tường. Đồng thời kiểm tra tầm nhìn để Boss không spawn đòn ngẫu nhiên hoặc sau vật cản. 
- **Đồng bộ Frontend & Backend:** `Boss::ChangeState` đồng thời cập nhật `BossState` nội bộ (cho AI) và `Character::State` (cho `CharacterRenderer`). Điều này giúp Controller không cần can thiệp quá sâu vào việc render của Boss.
- **Tích hợp LevelFactory & MapBuilder:** Cho phép game sinh ra đúng class (Boss1/2/3) thông qua `subType` được truyền từ file thiết kế bản đồ LDtk (subType = 1, 2, 3).
- **Vá lỗi và biên dịch thành công:** Đã xử lý triệt để các lỗi khi compile như thiếu method `GetCenter`, lỗi Constructor của `Projectile` hay conflict enum `State`.

## Current Status
- Game đã build thành công 100%. Các Boss đã được liên kết với hệ thống spawn, AI loop và Combat System. 
- Bạn có thể chuyển sang kiểm tra (Visual Test) bằng cách chạy game, gặp boss để test tính mượt mà của State/Phase cũng như hoạt ảnh animation (nhất là ảnh hurt của Boss 2 tôi vừa tạo).
