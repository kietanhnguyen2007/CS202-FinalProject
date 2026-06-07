# Apple Knight Multiplayer Adventure

**Course:** Programming Systems (CS202)
**Instructor:** Mr. Ho Tuan Thanh
**Student:** Nguyen Trong Tien (25125037)

---

## Cấu trúc thư mục

```
AppleKnightAdventure/
├── include/
│   ├── Model/                     # Data layer - chứa dữ liệu và trạng thái game
│   │   ├── Entity.h               # Lớp cơ sở trừu tượng cho mọi đối tượng game
│   │   ├── Character.h            # Lớp cơ sở cho nhân vật (di chuyển, chiến đấu)
│   │   ├── Player.h               # Nhân vật người chơi, serialize qua mạng
│   │   ├── Enemy.h                # Kẻ địch với AI cơ bản (Melee/Ranged/Flying)
│   │   ├── Boss.h                 # Boss với nhiều phase và enrage mode
│   │   ├── Item.h                 # Vật phẩm thu thập (Coin, Apple, Key, Potion...)
│   │   ├── Projectile.h           # Đạn bắn (Arrow, Magic, BossAttack)
│   │   ├── Inventory.h            # Hệ thống túi đồ (thêm/xoá item, đếm coin/apple/key)
│   │   ├── Checkpoint.h           # Checkpoint respawn
│   │   └── GameState.h            # Trạng thái tổng thể game (player, enemy, mode...)
│   ├── View/                      # Visual layer - xử lý hiển thị và UI
│   │   ├── Renderer.h             # Renderer cơ bản (init frame, draw shape/text/texture)
│   │   ├── HUDView.h              # Hiển thị HUD (health bar, coin, skill cooldown)
│   │   ├── InventoryView.h        # Giao diện túi đồ
│   │   ├── MenuView.h             # Menu chính (Single Player, Host/Join LAN/Radmin)
│   │   └── GameView.h             # View tổng thể game (background, entity, pause overlay)
│   ├── Controller/                # Input & game flow control layer
│   │   ├── InputController.h      # Xử lý input, key binding, action mapping
│   │   ├── GameController.h       # Vòng lặp game chính (init → run → shutdown)
│   │   └── MenuController.h       # Vòng lặp menu chính
│   ├── Network/                   # Multiplayer networking
│   │   ├── NetworkManager.h       # Quản lý kết nối mạng (server/client)
│   │   ├── Server.h               # Server side (broadcast, send to client)
│   │   ├── Client.h               # Client side (connect, send, receive)
│   │   └── Packet.h               # Gói tin mạng (serialize int/float/bool/string)
│   ├── Systems/                   # Game systems
│   │   ├── ObservableList.h       # Generic observable collection (adapt từ skill C#)
│   │   ├── ObjectPool.h           # Pool tái sử dụng object (giảm allocation)
│   │   ├── Quadtree.h             # Spatial partitioning (tối ưu collision)
│   │   ├── CollisionSystem.h      # Hệ thống phát hiện va chạm
│   │   ├── ParticleSystem.h       # Hệ thống hiệu ứng hạt
│   │   └── SoundManager.h         # Quản lý âm thanh (singleton)
│   ├── Factories/                 # Factory pattern (từ spec §IV)
│   │   ├── EnemyFactory.h         # Tạo enemy theo loại
│   │   ├── ItemFactory.h          # Tạo item theo loại
│   │   └── LevelFactory.h         # Load/save level
│   └── Utils/
│       ├── Constants.h            # Hằng số game (screen, physics, network)
│       └── Types.h                # Type alias, enum, struct (WeaponType, SkillType...)
├── src/
│   ├── main.cpp                   # Entry point (menu → game)
│   ├── Model/                     # Implementation của Model headers
│   │   ├── Character.cpp
│   │   ├── Player.cpp
│   │   ├── Enemy.cpp
│   │   ├── Boss.cpp
│   │   ├── Item.cpp
│   │   ├── Projectile.cpp
│   │   ├── Inventory.cpp
│   │   ├── Checkpoint.cpp
│   │   └── GameState.cpp
│   ├── View/                      # Implementation của View headers
│   │   ├── Renderer.cpp
│   │   ├── HUDView.cpp
│   │   ├── InventoryView.cpp
│   │   ├── MenuView.cpp
│   │   └── GameView.cpp
│   ├── Controller/                # Implementation của Controller headers
│   │   ├── InputController.cpp
│   │   ├── GameController.cpp
│   │   └── MenuController.cpp
│   ├── Network/                   # Implementation của Network headers
│   │   ├── NetworkManager.cpp
│   │   ├── Server.cpp
│   │   ├── Client.cpp
│   │   └── Packet.cpp
│   ├── Systems/                   # Implementation của Systems headers
│   │   ├── CollisionSystem.cpp
│   │   ├── ParticleSystem.cpp
│   │   ├── Quadtree.cpp
│   │   └── SoundManager.cpp
│   └── Factories/                 # Implementation của Factories headers
│       ├── EnemyFactory.cpp
│       ├── ItemFactory.cpp
│       └── LevelFactory.cpp
├── assets/                        # Tài nguyên game
│   ├── textures/                  # Texture, sprite
│   ├── sounds/                    # Âm thanh, music
│   ├── fonts/                     # Font chữ
│   └── levels/                    # File cấu hình level
├── tests/                         # Unit tests
├── Makefile                       # Build script
└── README.md                      # Tài liệu này
```

## MVC Pattern Architecture

### Model (`include/Model/`)
Chứa toàn bộ dữ liệu và logic nghiệp vụ. Không phụ thuộc vào hệ thống hiển thị.

### View (`include/View/`)
Chỉ xử lý việc hiển thị dữ liệu. Các class là Passive View - nhận dữ liệu và render.

### Controller (`include/Controller/`)
Xử lý input và điều khiển luồng game.

**Luồng MVC thuần:**
```
Input → Controller → cập nhật Model → Controller gọi View → View đọc Model → Render
```

Controller là trung tâm điều phối: nhận input, thay đổi Model, sau đó yêu cầu View render từ Model.

## Key Systems (`include/Systems/`)

Các hệ thống kỹ thuật xuyên suốt game, không thuộc riêng Model/View/Controller nào:

| File | Chức năng |
|------|-----------|
| **ObservableList.h** | Generic container có cơ chế **thông báo khi dữ liệu thay đổi** (reactive). Adapt từ skill C# sang C++ template. Dùng trong Inventory, SkillBar để tự động cập nhật UI. |
| **ObjectPool.h** | **Pool tái sử dụng object** (Projectile, Particle, Effect). Giải quyết bài toán memory fragmentation (§VII.1): thay vì new/delete liên tục, lấy object từ pool có sẵn → predictable memory, zero allocation khi gameplay. |
| **Quadtree.h** | **Spatial partitioning** (§VII.2). Chia không gian game thành 4 phần tư đệ quy. Chỉ kiểm tra collision với entity ở gần → giảm O(n²) xuống O(n log n). |
| **CollisionSystem.h** | Hệ thống va chạm dùng Quadtree bên trong. `CheckCollision(a, b)` kiểm tra 2 rectangle. `GetNearbyEntities()` lấy entity trong vùng để tối ưu. |
| **ParticleSystem.h** | Hiệu ứng hạt (vụ nổ, cháy, máu...). Mỗi particle có position, velocity, color, lifetime. Update/Render hàng loạt, cleanup particle đã hết hạn. |
| **SoundManager.h** | Singleton quản lý âm thanh (load/play/stop Sound và Music) dùng raylib audio device. |

## Build

```bash
# Yêu cầu: raylib, compiler hỗ trợ C++17
make
./AppleKnightAdventure
```
