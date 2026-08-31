# KỊCH BẢN QUAY VIDEO MINH CHỨNG DỰ ÁN APPLE KNIGHT ADVENTURE

## 1. Mục tiêu và quy tắc đếm

Tài liệu này là kịch bản quay, thu lời và kiểm tra coverage cho bản demo dự án. Video phải chứng minh ba tập nội dung **tách biệt**:

- **130 feature độc lập:** `F001` đến `F130`.
- **6 level campaign 2D:** `L2D-01` đến `L2D-06`.
- **50 wave Survival3D:** `W3D-01` đến `W3D-50`.

Sáu level 2D và 50 wave 3D **không được cộng vào 130 feature**. Bảng kết video bắt buộc hiện ba bộ đếm riêng: `130/130 FEATURES`, `6/6 2D LEVELS`, `50/50 3D WAVES`.

### Hai tuyến nội dung bắt buộc trong bản bàn giao

Video cuối phải chứa đủ **cả hai tuyến** dưới đây. Một tuyến không được dùng để thay thế tuyến còn lại:

1. **FULL FEATURES — 130/130:** một phần showcase chứng minh lần lượt `F001–F130`. Mỗi feature phải hiện mã `Fxxx`, tên feature, cảnh chứng minh và lời thuyết minh. Có thể cắt lại cảnh phù hợp từ footage chạy full level/wave; chỉ quay insert mới khi footage full run chưa chứng minh rõ feature đó.
2. **FULL LEVEL/WAVE RUN — 56/56:** giữ footage chạy trọn `L2D-01–L2D-06` và toàn bộ tiến trình `W3D-01–W3D-50`. Phần này chứng minh nội dung level/wave hoàn chỉnh và vẫn được đếm riêng, không thay cho showcase feature.

Vì vậy, một cảnh gameplay có thể được sử dụng hai lần trong bản dựng: một lần dưới mã `Fxxx` trong tuyến **FULL FEATURES**, và một lần nằm trong footage chạy trọn của level/wave tương ứng. Không bắt buộc quay lại nếu footage gốc đã rõ, nhưng cả hai tuyến đầu ra vẫn phải hiện diện.

### Phân công cố định

- **Nguyễn Trọng Tiến — tuyến FULL FEATURES:** dựng và thuyết minh `F001–F075`. Trước hết rà sáu footage full level đã có, cắt các cảnh đủ rõ vào đúng mã feature, sau đó chỉ quay bổ sung những feature còn thiếu.
- **Nguyễn Trọng Tiến — tuyến FULL LEVEL RUN:** phụ trách **toàn bộ** `L2D-01–L2D-06`. **Trạng thái hiện tại: đã quay xong 6/6 full level 2D.**
- **Nguyễn Anh Kiệt — tuyến FULL FEATURES:** quay, dựng và thuyết minh `F076–F130`.
- **Nguyễn Anh Kiệt — tuyến FULL WAVE RUN:** phụ trách **toàn bộ** tiến trình `W3D-01–W3D-50`.
- Hai người có thể cùng xuất hiện ở mở đầu/kết thúc, nhưng sáu level 2D chỉ ghi Nguyễn Trọng Tiến là người phụ trách; 50 wave 3D chỉ ghi Nguyễn Anh Kiệt là người phụ trách.

### Block quay FULL FEATURES

Mỗi block là một buổi/setup quay độc lập. Hoàn thành và kiểm tra đủ mã trong block trước khi đổi class, save hoặc mode.

| Block Tiến | Feature | Setup chính |
|---|---:|---|
| `T1` | `F001–F005` | Startup, resize, Main Menu, profile progression và Prepare |
| `T2` | `F006–F011` | Bốn class, pet, solo/local co-op và hai bộ input |
| `T3` | `F012–F020` | Một map 2D phù hợp để quay movement, collision, dash, parry và respawn |
| `T4` | `F021–F025` | Knight 2D |
| `T5` | `F026–F030` | Fighter 2D |
| `T6` | `F031–F035` | Magic Caster 2D |
| `T7` | `F036–F040` | Ninja 2D |
| `T8` | `F041–F045` | Coin, Apple, animation, Potion và Skill Bar |
| `T9` | `F046–F050` | Chest, checkpoint restore, portal, tutorial và Cup |
| `T10` | `F051–F055` | End flag, scoring, Result, minimap và achievement |
| `T11` | `F056–F060` | Leaderboard, Options, parallax, combat feedback và MapBuilder entry |
| `T12` | `F061–F065` | Enemy thường, projectile, FSM, telegraph, LOS và ledge avoidance |
| `T13` | `F066–F070` | Boss phase/navigation, bốn pet, boon và core |
| `T14` | `F071–F075` | Element, reaction, local co-op, dynamic spawn và boss portal |

| Block Kiệt | Feature | Setup chính |
|---|---:|---|
| `K1` | `F076–F080` | Survival3D Character Select, movement, camera, target lock và dash |
| `K2` | `F081–F085` | Knight 3D |
| `K3` | `F086–F090` | Magic Caster 3D |
| `K4` | `F091–F094` | Enemy, boss, director và upgrade draft 3D |
| `K5` | `F095–F100` | Fixed step, pooling, animation/VFX, accessibility và run result |
| `K6` | `F101–F105` | Audio manifest, variation, cooldown, spatial sound và BGM |
| `K7` | `F106–F110` | Save, asset loading, renderer và MVC |
| `K8` | `F111–F115` | OOD, Singleton, Factory, Adapter và Command/Composite |
| `K9` | `F116–F120` | Pool, Quadtree, animation event, offline queue và backend |
| `K10` | `F121–F125` | Shop, custom level và các thao tác MapBuilder |
| `K11` | `F126–F130` | Editor I/O/playtest, UI states, build và configuration |

### Việc còn lại của Nguyễn Trọng Tiến sau khi đã quay 6 full level

Tiến **không cần quay lại sáu level từ đầu**. Phần việc còn lại là hoàn thành lần lượt các block `T1–T14` của tuyến `FULL FEATURES F001–F075` theo ba bước:

1. Cắt các cảnh feature đã xuất hiện rõ trong sáu footage full level và gắn slate `Fxxx` tương ứng.
2. Quay insert riêng cho các feature chưa xuất hiện hoặc chưa đủ rõ, đặc biệt là menu/Prepare, resize, bốn class và toàn bộ skill, local co-op, pet, boon/core, elemental reaction, achievement, leaderboard, Options và MapBuilder entry.
3. Thu lời thuyết minh, chèn bằng chứng code khi kịch bản yêu cầu và xác nhận bảng coverage đạt `F001–F075 = 75/75`.

### Những phát biểu bị cấm

- Không gọi AI là machine learning; đây là rule-based AI/FSM/pathfinding.
- Không gọi backend là production-grade security; guest ID và HTTP localhost không phải xác thực/mã hóa mạnh.

## 2. Chuẩn bị trước khi quay

### 2.1. Thiết lập kỹ thuật

1. Quay ở 1920×1080, 60 FPS, định dạng MKV để giảm rủi ro mất file khi phần mềm quay bị dừng bất ngờ.
2. Tách ba track: game audio, micro Nguyễn Trọng Tiến, micro Nguyễn Anh Kiệt. Khi dựng mới cân loudness và khử ồn.
3. Tắt thông báo hệ điều hành; đóng ứng dụng chứa thông tin cá nhân; dùng con trỏ chuột dễ nhìn khi quay code/UI.
4. Giữ một bản quay gameplay gốc liên tục. Bản dựng có thể tăng tốc đoạn lặp, nhưng không được làm mất mã feature, level/wave banner, boss phase, Wave Clear hoặc Result.
5. Sao lưu `save.json`/file save thực tế trước khi quay vì level progress, coin, options, achievement và Survival record sẽ thay đổi.
6. Chuẩn bị hai profile/save: một profile sạch để quay progression; một profile demo đã mở đủ level để không phải lặp lại quá trình unlock khi quay lại cảnh lỗi.
7. Chuẩn bị IDE với font tối thiểu 18 pt, tắt minimap editor nếu làm code quá nhỏ, ghim sẵn các file bằng chứng được liệt kê trong từng feature.
8. Khi minh họa backend tùy chọn, dùng bản sao cấu hình test trỏ tới localhost và giữ nguyên `services.json` của bản chính.

### 2.2. Quy ước tên clip

- Feature của Tiến: `AKA_F001_TIEN_<slug>_T01.mkv`.
- Feature của Kiệt: `AKA_F076_KIET_<slug>_T01.mkv`.
- Level 2D: `AKA_L2D_01_TIEN_FULL_T01.mkv` và các clip phụ `AKA_L2D_01_TIEN_<shot>_T01.mkv`.
- Wave 3D: `AKA_W3D_01_KIET_T01.mkv`.
- Code walkthrough: thêm hậu tố `_CODE`; gameplay: thêm `_GAME`; UI: thêm `_UI`.
- Không ghi đè take cũ. Take mới tăng `T02`, `T03`.

### 2.3. Slate/overlay bắt buộc

Mỗi clip mở đầu 1–2 giây bằng overlay:

```text
[Fxxx | FEATURE] Tên feature
Người phụ trách: Nguyễn ...
Gameplay/Code evidence: đường/dẫn/file.cpp
```

Với level hoặc wave, đổi dòng đầu thành `[L2D-xx | LEVEL — KHÔNG TÍNH FEATURE]` hoặc `[W3D-xx | WAVE — KHÔNG TÍNH FEATURE]`.

## 3. Timeline bản dựng đề xuất

Thời lượng là mốc dựng, không phải giới hạn file raw. Nếu footage W01–W50 dài, giữ thêm một video phụ “Full 50-wave run” không cắt.

- `00:00–02:30`: Hai thành viên giới thiệu, nêu rõ phép đếm 130 + 6 + 50.
- `02:30–11:00`: Tiến — block `T1–T3`, F001–F020.
- `11:00–24:00`: Tiến — block `T4–T7`, bốn bộ kỹ năng 2D F021–F040.
- `24:00–39:00`: Tiến — block `T8–T11`, F041–F060.
- `39:00–53:00`: Tiến — block `T12–T14`, F061–F075.
- `53:00–75:00`: Tiến — L2D-01–L2D-06.
- `75:00–92:00`: Kiệt — block `K1–K5`, F076–F100.
- `92:00–150:00`: Kiệt — W3D-01–W3D-50; boss wave để tốc độ thật, wave thường có thể tăng tốc nhưng phải đọc được số wave.
- `150:00–166:00`: Kiệt — block `K6–K9`, F101–F120.
- `166:00–179:00`: Kiệt — block `K10–K11`, F121–F130.
- `179:00–181:00`: Tổng kết ba bộ đếm và credit phân công.

---

# PHẦN A — KỊCH BẢN 130 FEATURE

## Block T1 — Nguyễn Trọng Tiến — F001–F005: startup, menu, progression và Prepare

### F001 — Progressive startup loading

- **Clip:** `AKA_F001_TIEN_PROGRESSIVE_LOADING_UI_T01.mkv`.
- **Cảnh quay:** Khởi động executable từ trạng thái tắt hoàn toàn; giữ trọn loading background, thanh tiến độ và thời điểm chuyển sang menu.
- **Thao tác:** Mở game một lần ở bản build có đầy đủ assets; không cắt qua màn loading. Nếu tải quá nhanh, chèn replay 50% tốc độ thay vì giả lập lỗi.
- **Lời thoại:** “Game duyệt asset, decode ảnh ở worker và cập nhật tiến độ thật; chỉ chuyển vào menu sau khi atlas và asset khởi động hoàn tất.”
- **Bằng chứng:** `AppleKnightAdventure/src/main.cpp:61-260`; `AppleKnightAdventure/src/View/AssetManager.cpp`.

### F002 — Resizable window và responsive UI

- **Clip:** `AKA_F002_TIEN_RESPONSIVE_WINDOW_UI_T01.mkv`.
- **Cảnh quay:** Menu và HUD ở ít nhất ba kích thước cửa sổ; chữ, button và vùng game vẫn căn đúng.
- **Thao tác:** Kéo cửa sổ từ rộng sang hẹp rồi phóng lớn; thử fullscreen trong Options và quay lại windowed.
- **Lời thoại:** “WindowManager theo dõi resize và DPI để UI tính lại tỷ lệ thay vì cố định ở một độ phân giải.”
- **Bằng chứng:** `AppleKnightAdventure/src/Systems/WindowManager.cpp`; `AppleKnightAdventure/include/Systems/WindowManager.h`; `AppleKnightAdventure/src/main.cpp:44-59`.

### F003 — Main menu điều hướng bằng bàn phím và chuột

- **Clip:** `AKA_F003_TIEN_MAIN_MENU_UI_T01.mkv`.
- **Cảnh quay:** Toàn bộ main menu, hover bằng chuột, đổi lựa chọn bằng bàn phím và hiệu ứng xác nhận.
- **Thao tác:** Đi lần lượt qua Start, Survival3D, Custom Map, Map Builder, Shop, Leaderboard, Achievements, Options; chưa cần mở sâu từng màn.
- **Lời thoại:** “Main menu có điều hướng chuột và bàn phím, âm thanh hover/xác nhận và chuyển tới các mode riêng.”
- **Bằng chứng:** `AppleKnightAdventure/src/Controller/MenuController.cpp`; `AppleKnightAdventure/src/View/MenuView.cpp:304-367`.

### F004 — Level Select mở khóa tuần tự

- **Clip:** `AKA_F004_TIEN_LEVEL_UNLOCK_UI_T01.mkv`.
- **Cảnh quay:** Một save mới chỉ mở Level 1; sau khi hoàn thành, quay lại để thấy Level 2 được mở và sao tốt nhất được hiển thị.
- **Thao tác:** Thử chọn level đang khóa để nghe error; chọn level hợp lệ để chuyển sang Prepare.
- **Lời thoại:** “Campaign luôn mở từ Level 1 và chỉ mở level kế tiếp khi level trước đã có score hoặc star hợp lệ.”
- **Bằng chứng:** `AppleKnightAdventure/src/Controller/MenuController.cpp:473-527`.

### F005 — Prepare/loadout trước màn

- **Clip:** `AKA_F005_TIEN_PREPARE_LOADOUT_UI_T01.mkv`.
- **Cảnh quay:** Prepare screen hiển thị class, pet hoặc P2, mô tả và nút bắt đầu.
- **Thao tác:** Đổi class; đổi pet; bật/tắt hai người; xác nhận vào level rồi Back để chứng minh luồng hai chiều.
- **Lời thoại:** “Prepare tách việc chọn nhân vật, companion và số người chơi khỏi game loop, sau đó truyền cấu hình vào GameController.”
- **Bằng chứng:** `AppleKnightAdventure/src/Controller/PrepareController.cpp`; `AppleKnightAdventure/src/View/PrepareView.cpp`.

## Block T2 — Nguyễn Trọng Tiến — F006–F011: class, pet, solo/co-op và hai bộ input

### F006 — Bốn class 2D có bộ kỹ năng riêng

- **Clip:** `AKA_F006_TIEN_FOUR_CLASSES_GAME_T01.mkv`.
- **Cảnh quay:** Montage Knight, Fighter, Magic Caster, Ninja đứng cạnh cùng một mốc map, mỗi class tung một đòn cơ bản.
- **Thao tác:** Bắt đầu lại một level bốn lần với bốn class; overlay tên class ở mỗi lần chuyển.
- **Lời thoại:** “Bốn class dùng bốn implementation CharacterSkillSet khác nhau, không chỉ thay sprite.”
- **Bằng chứng:** `AppleKnightAdventure/include/Model/CharacterSkillSet.h`; `AppleKnightAdventure/src/Model/KnightSkillSet.cpp`; `FighterSkillSet.cpp`; `MagicCasterSkillSet.cpp`; `NinjaSkillSet.cpp`.

### F007 — Pet selection và companion runtime

- **Clip:** `AKA_F007_TIEN_PET_SELECTION_GAME_T01.mkv`.
- **Cảnh quay:** Chọn từng pet trong Prepare, sau đó vào solo level để thấy pet đi theo player.
- **Thao tác:** Tối thiểu quay một pet follow; footage chi tiết bốn hành vi nằm ở F068.
- **Lời thoại:** “Ở solo mode, pet được chọn từ Prepare và được tạo cùng player; local co-op chủ động tắt pet để dành slot cho P2.”
- **Bằng chứng:** `AppleKnightAdventure/src/Controller/PrepareController.cpp`; `AppleKnightAdventure/src/Controller/GameController.cpp:542-600`; `AppleKnightAdventure/src/Model/Pet.cpp`.

### F008 — Chuyển solo/local co-op

- **Clip:** `AKA_F008_TIEN_SOLO_COOP_TOGGLE_UI_T01.mkv`.
- **Cảnh quay:** Prepare hiển thị `1 PLAYER`, sau đó chuyển `2 PLAYERS`; vùng pet đổi thành class P2.
- **Thao tác:** Bắt đầu cùng một level ở solo rồi local co-op; quay cảnh P2 xuất hiện cạnh P1.
- **Lời thoại:** “Đây là local co-op hai người trên cùng máy; không phải multiplayer qua mạng.”
- **Bằng chứng:** `AppleKnightAdventure/src/View/PrepareView.cpp:199-300`; `AppleKnightAdventure/src/Controller/GameController.cpp:145,542-548`.

### F009 — Bộ input solo có keyboard/mouse alternative

- **Clip:** `AKA_F009_TIEN_SOLO_INPUT_GAME_T01.mkv`.
- **Cảnh quay:** Overlay key viewer khi player di chuyển bằng A/D rồi phím mũi tên; tấn công bằng J rồi chuột trái; tương tác bằng F/O.
- **Thao tác:** Thực hiện từng input ở nơi an toàn để action nhìn rõ.
- **Lời thoại:** “InputController gom nhiều phím thay thế vào cùng InputCommand, giúp gameplay không phải kiểm tra từng phím trực tiếp.”
- **Bằng chứng:** `AppleKnightAdventure/src/Controller/InputController.cpp:9-35`; `AppleKnightAdventure/include/Controller/InputController.h`.

### F010 — Bộ điều khiển độc lập Player 1

- **Clip:** `AKA_F010_TIEN_P1_CONTROLS_GAME_T01.mkv`.
- **Cảnh quay:** Key overlay P1: A/D, W hoặc Space, J/K/U/H, P, F, Shift, L.
- **Thao tác:** Lần lượt đi, nhảy, bốn attack slot, block, interact, sprint và dash.
- **Lời thoại:** “P1 có input state đầy đủ cho di chuyển, bốn kỹ năng, block, tương tác, sprint và dash.”
- **Bằng chứng:** `AppleKnightAdventure/src/Controller/InputController.cpp:37-53`.

### F011 — Bộ điều khiển độc lập Player 2

- **Clip:** `AKA_F011_TIEN_P2_CONTROLS_GAME_T01.mkv`.
- **Cảnh quay:** Hai người cùng màn hình; P1 đứng yên, P2 lần lượt dùng Arrow, KP1/KP2/KP3/KP0, KP4/KP5/KP6/KP7.
- **Thao tác:** Chỉ điều khiển P2 trong lượt này để chứng minh input không bị dùng chung.
- **Lời thoại:** “P2 có mapping numpad và phím mũi tên độc lập, được poll thành InputCommand thứ hai trong cùng frame.”
- **Bằng chứng:** `AppleKnightAdventure/src/Controller/InputController.cpp:55-72`; `AppleKnightAdventure/src/Controller/GameController.cpp:2033-2036`.

## Block T3 — Nguyễn Trọng Tiến — F012–F020: movement, collision, dash, parry và respawn

### F012 — Movement và facing

- **Clip:** `AKA_F012_TIEN_MOVEMENT_FACING_GAME_T01.mkv`.
- **Cảnh quay:** Đi trái/phải, dừng, đổi hướng sát một sign/chest để thấy sprite facing và interaction hitbox đổi theo hướng.
- **Thao tác:** Chuyển hướng nhanh nhiều lần, sau đó chạy liên tục để chứng minh cập nhật theo delta time.
- **Lời thoại:** “Vận tốc ngang và hướng nhìn được cập nhật từ InputCommand; animation/hitbox lấy cùng trạng thái facing.”
- **Bằng chứng:** `AppleKnightAdventure/src/Controller/GameController.cpp:801-900`; `AppleKnightAdventure/src/Model/Player.cpp`.

### F013 — Jump và gravity platformer

- **Clip:** `AKA_F013_TIEN_JUMP_GRAVITY_GAME_T01.mkv`.
- **Cảnh quay:** Nhảy từ nền phẳng, rơi xuống platform thấp hơn và tiếp đất.
- **Thao tác:** Nhấn jump trên đất; thử nhấn lại khi đang trên không để cho thấy điều kiện grounded; bật slow-motion ở phần tiếp đất khi dựng.
- **Lời thoại:** “Player chỉ bắt đầu jump khi được phép đứng đất, sau đó chịu gravity và được tile collision đặt lại grounded khi tiếp đất.”
- **Bằng chứng:** `AppleKnightAdventure/src/Controller/GameController.cpp:660-800`; `AppleKnightAdventure/include/Utils/Constants.h`; `AppleKnightAdventure/src/Model/Player.cpp`.

### F014 — Sprint tăng tốc

- **Clip:** `AKA_F014_TIEN_SPRINT_GAME_T01.mkv`.
- **Cảnh quay:** Split-screen cùng quãng đường: đi thường và giữ sprint; chèn đồng hồ thời gian.
- **Thao tác:** Chạy giữa hai mốc map giống nhau, không dash.
- **Lời thoại:** “Giữ sprint nhân tốc độ di chuyển khoảng 1,4 lần nhưng vẫn dùng collision và animation run bình thường.”
- **Bằng chứng:** `AppleKnightAdventure/src/Controller/GameController.cpp:801-900`; `AppleKnightAdventure/include/Model/Player.h`.

### F015 — Dash có vận tốc, thời lượng, cooldown và invulnerability

- **Clip:** `AKA_F015_TIEN_DASH_GAME_T01.mkv`.
- **Cảnh quay:** Dash qua hitbox enemy/projectile, hiển thị cooldown; thử dash lại ngay để thấy bị chặn, rồi dash sau khi hồi.
- **Thao tác:** Canh một projectile dễ nhìn; giữ HP trước và sau cửa sổ dash.
- **Lời thoại:** “Dash là state khoảng 0,22 giây, tốc độ 520, hồi một giây và bỏ qua damage trong cửa sổ invulnerability.”
- **Bằng chứng:** `AppleKnightAdventure/include/Model/Player.h`; `AppleKnightAdventure/src/Model/Player.cpp`; `AppleKnightAdventure/src/Controller/GameController.cpp:801-900`.

### F016 — Block/parry giảm sát thương

- **Clip:** `AKA_F016_TIEN_PARRY_GAME_T01.mkv`.
- **Cảnh quay:** Cùng một enemy đánh player khi không block và khi giữ block; đặt hai số damage cạnh nhau.
- **Thao tác:** Đưa HP về cùng mức hoặc dùng hai lượt restart; không gắn core Bulwark trong cảnh baseline.
- **Lời thoại:** “Parry mặc định chỉ nhận 30% damage, tương đương giảm 70%; đây không phải bất tử tuyệt đối.”
- **Bằng chứng:** `AppleKnightAdventure/src/Model/Player.cpp:170-190`; `AppleKnightAdventure/include/Model/Player.h`.

### F017 — Cast lock và skill cooldown

- **Clip:** `AKA_F017_TIEN_CAST_LOCK_GAME_T01.mkv`.
- **Cảnh quay:** Dùng một skill có charge, spam các nút khác trong wind-up; animation vẫn hoàn tất đúng đòn và skill chỉ dùng lại khi cooldown hết.
- **Thao tác:** Bật key viewer để thấy input spam nhưng action không bị restart.
- **Lời thoại:** “BeginCast khóa action qua wind-up/contact/recovery; CanStartSkill và cooldown ngăn hủy chiêu hoặc spam ngoài ý muốn.”
- **Bằng chứng:** `AppleKnightAdventure/include/Model/Player.h`; `AppleKnightAdventure/src/Model/Player.cpp`; các file `*SkillSet.cpp`.

### F018 — Tile AABB collision

- **Clip:** `AKA_F018_TIEN_TILE_COLLISION_GAME_T01.mkv`.
- **Cảnh quay:** Player chạy vào tường, nhảy chạm trần và đáp lên sàn; không xuyên qua tile.
- **Thao tác:** Thử tiếp cận tường từ cả hai phía và rơi lên platform từ trên xuống.
- **Lời thoại:** “Game giải quyết AABB theo vị trí frame trước và trục xuyên nhỏ nhất, đồng thời đặt trạng thái đứng đất khi va chạm từ trên.”
- **Bằng chứng:** `AppleKnightAdventure/src/Controller/GameController.cpp:671-800`.

### F019 — FakeWall tham gia collision rồi bị phá

- **Clip:** `AKA_F019_TIEN_FAKE_WALL_GAME_T01.mkv`.
- **Cảnh quay:** Đâm vào FakeWall khi còn nguyên; đánh phá tường; sau đó đi xuyên vị trí cũ, kèm particle/sound.
- **Thao tác:** Chọn level có FakeWall; giữ cùng góc camera trước và sau để so sánh.
- **Lời thoại:** “FakeWall được xem là solid trong collision cho tới khi trạng thái destroyed được đặt; sau đó vật cản bị loại.”
- **Bằng chứng:** `AppleKnightAdventure/src/Model/FakeWall.cpp`; `AppleKnightAdventure/src/Controller/GameController.cpp:671-800`.

### F020 — Fall death và checkpoint respawn

- **Clip:** `AKA_F020_TIEN_FALL_RESPAWN_GAME_T01.mkv`.
- **Cảnh quay:** Kích hoạt checkpoint, cố ý rơi khỏi đáy map, rồi xuất hiện lại tại checkpoint; projectile cũ không tiếp tục gây damage.
- **Thao tác:** Quay cả checkpoint flag trước khi rơi và tọa độ hồi sinh sau khi chết.
- **Lời thoại:** “Khi player rơi quá giới hạn map, GameController chạy quy trình chết và khôi phục ở checkpoint gần nhất thay vì luôn về spawn đầu.”
- **Bằng chứng:** `AppleKnightAdventure/src/Controller/GameController.cpp:1821-1880,2260-2284`.

## Block T4 — Nguyễn Trọng Tiến — F021–F025: bộ kỹ năng Knight 2D

### F021 — Knight Quick Slash

- **Clip:** `AKA_F021_TIEN_KNIGHT_QUICK_SLASH_GAME_T01.mkv`.
- **Cảnh quay:** Knight dùng J lên một melee enemy; hiển thị active slash, damage text và cooldown ngắn.
- **Thao tác:** Đứng trong tầm gần, dùng đúng một đòn rồi lùi ra.
- **Lời thoại:** “Quick Slash là đòn cơ bản nhanh của Knight, damage 20 và cooldown khoảng 0,35 giây.”
- **Bằng chứng:** `AppleKnightAdventure/src/Model/KnightSkillSet.cpp`.

### F022 — Knight Heavy Strike có charge

- **Clip:** `AKA_F022_TIEN_KNIGHT_HEAVY_GAME_T01.mkv`.
- **Cảnh quay:** Giữ/nhấn K, thấy wind-up rồi hitbox nặng trúng enemy; so sánh damage với Quick Slash.
- **Thao tác:** Bật slow-motion ở charge/contact, không cắt bỏ wind-up.
- **Lời thoại:** “Heavy Strike đổi tốc độ lấy sát thương và hitbox lớn hơn: khoảng 45 damage, charge 0,30 giây.”
- **Bằng chứng:** `AppleKnightAdventure/src/Model/KnightSkillSet.cpp`.

### F023 — Knight Lunge

- **Clip:** `AKA_F023_TIEN_KNIGHT_LUNGE_GAME_T01.mkv`.
- **Cảnh quay:** Knight dùng U từ ngoài tầm melee, lao tới và đánh trúng mục tiêu.
- **Thao tác:** Đặt marker vị trí đầu/cuối; tránh dùng dash để không nhầm cơ chế.
- **Lời thoại:** “Lunge vừa là skill gây damage vừa dịch chuyển Knight tiến về trước, khác với dash phòng thủ dùng chung.”
- **Bằng chứng:** `AppleKnightAdventure/src/Model/KnightSkillSet.cpp`.

### F024 — Knight Ultimate diện rộng

- **Clip:** `AKA_F024_TIEN_KNIGHT_ULTIMATE_GAME_T01.mkv`.
- **Cảnh quay:** Gom nhiều enemy, dùng H và giữ toàn bộ charge, VFX, hit-stop, damage nhiều mục tiêu.
- **Thao tác:** Chờ đầy cooldown trước clip; không che skill bar.
- **Lời thoại:** “Ultimate Knight gây khoảng 80 damage trên hitbox rộng, có charge và cooldown dài để kiểm soát nhịp dùng.”
- **Bằng chứng:** `AppleKnightAdventure/src/Model/KnightSkillSet.cpp`; `AppleKnightAdventure/src/Controller/GameController.cpp`.

### F025 — Knight parry animation/logic riêng

- **Clip:** `AKA_F025_TIEN_KNIGHT_PARRY_GAME_T01.mkv`.
- **Cảnh quay:** Knight block đúng lúc trước melee hit; giữ close-up animation parry và HP.
- **Thao tác:** Dùng Knight, không dùng montage chung F016.
- **Lời thoại:** “Ngoài multiplier damage chung, KnightSkillSet cung cấp action và animation parry đúng với class.”
- **Bằng chứng:** `AppleKnightAdventure/src/Model/KnightSkillSet.cpp`; `AppleKnightAdventure/src/View/CharacterRenderer.cpp`.

## Block T5 — Nguyễn Trọng Tiến — F026–F030: bộ kỹ năng Fighter 2D

### F026 — Fighter Punch

- **Clip:** `AKA_F026_TIEN_FIGHTER_PUNCH_GAME_T01.mkv`.
- **Cảnh quay:** Fighter dùng J ở cự ly gần; hiển thị hitbox ngắn và nhịp đánh nhanh.
- **Thao tác:** Đánh một enemy duy nhất để damage dễ đọc.
- **Lời thoại:** “Punch là đòn cơ bản 20 damage của Fighter với cooldown khoảng 0,35 giây.”
- **Bằng chứng:** `AppleKnightAdventure/src/Model/FighterSkillSet.cpp`.

### F027 — Fighter Combo

- **Clip:** `AKA_F027_TIEN_FIGHTER_COMBO_GAME_T01.mkv`.
- **Cảnh quay:** Dùng K, giữ trọn chuỗi animation và active window dài hơn Punch.
- **Thao tác:** Không xen input skill khác trong chuỗi.
- **Lời thoại:** “Combo có active window dài hơn đòn thường và gây khoảng 35 damage, phù hợp áp sát liên tục.”
- **Bằng chứng:** `AppleKnightAdventure/src/Model/FighterSkillSet.cpp`.

### F028 — Fighter Charged Energy Punch

- **Clip:** `AKA_F028_TIEN_FIGHTER_CHARGED_PUNCH_GAME_T01.mkv`.
- **Cảnh quay:** Charge U rồi contact; hiện damage khoảng 50 và cooldown.
- **Thao tác:** Đặt enemy trong hitbox, giữ wind-up trong bản dựng.
- **Lời thoại:** “Energy Punch có charge khoảng 0,40 giây, damage cao hơn và cooldown khoảng 1,5 giây.”
- **Bằng chứng:** `AppleKnightAdventure/src/Model/FighterSkillSet.cpp`.

### F029 — Fighter Energy Orb ultimate

- **Clip:** `AKA_F029_TIEN_FIGHTER_ORB_GAME_T01.mkv`.
- **Cảnh quay:** Fighter dùng H, orb rời tay, bay qua không gian và trúng enemy ở xa.
- **Thao tác:** Quay cả player và quỹ đạo projectile; tránh đứng quá gần mục tiêu.
- **Lời thoại:** “Ultimate Fighter tạo Energy Orb độc lập, tốc độ khoảng 400, damage khoảng 70 và cooldown sáu giây.”
- **Bằng chứng:** `AppleKnightAdventure/src/Model/FighterSkillSet.cpp`; `AppleKnightAdventure/src/Model/Projectile.cpp`.

### F030 — Fighter parry riêng

- **Clip:** `AKA_F030_TIEN_FIGHTER_PARRY_GAME_T01.mkv`.
- **Cảnh quay:** Fighter parry melee hit, close-up animation và damage giảm.
- **Thao tác:** Lặp đúng setup F025 nhưng đổi class Fighter.
- **Lời thoại:** “Fighter có implementation parry riêng trong CharacterSkillSet, đồng bộ animation với trạng thái block.”
- **Bằng chứng:** `AppleKnightAdventure/src/Model/FighterSkillSet.cpp`; `AppleKnightAdventure/src/View/CharacterRenderer.cpp`.

## Block T6 — Nguyễn Trọng Tiến — F031–F035: bộ kỹ năng Magic Caster 2D

### F031 — Magic Caster Lightning Thunder có target/LOS

- **Clip:** `AKA_F031_TIEN_MAGE_LIGHTNING_GAME_T01.mkv`.
- **Cảnh quay:** Mage dùng J lên enemy gần nhất đang nhìn thấy; thử thêm một enemy bị tường che để chứng minh không khóa xuyên tường.
- **Thao tác:** Quay hai take: line-of-sight trống và bị tile chặn.
- **Lời thoại:** “Lightning chọn enemy hợp lệ gần nhất trong vùng nhìn và kiểm tra line-of-sight trước khi gây damage.”
- **Bằng chứng:** `AppleKnightAdventure/src/Model/MagicCasterSkillSet.cpp`; `AppleKnightAdventure/src/Controller/GameController.cpp`.

### F032 — Magic Caster Fireball

- **Clip:** `AKA_F032_TIEN_MAGE_FIREBALL_GAME_T01.mkv`.
- **Cảnh quay:** Fireball bay từ caster đến enemy, va chạm và gắn nguyên tố Fire/Burn.
- **Thao tác:** Dùng K ở hành lang trống để thấy rõ projectile.
- **Lời thoại:** “Fireball là projectile hệ Fire, khoảng 35 damage, có tốc độ, tầm và lifetime riêng.”
- **Bằng chứng:** `AppleKnightAdventure/src/Model/MagicCasterSkillSet.cpp`; `AppleKnightAdventure/src/Model/Projectile.cpp`.

### F033 — Magic Caster Water Wave

- **Clip:** `AKA_F033_TIEN_MAGE_WATER_WAVE_GAME_T01.mkv`.
- **Cảnh quay:** Water Wave đi qua hành lang, trúng mục tiêu và tạo trạng thái Wet.
- **Thao tác:** Dùng U; giữ icon/status trên enemy trong khung hình.
- **Lời thoại:** “Water Wave là projectile hệ Water khoảng 30 damage, dùng để tạo Wet và chuẩn bị elemental reaction.”
- **Bằng chứng:** `AppleKnightAdventure/src/Model/MagicCasterSkillSet.cpp`; `AppleKnightAdventure/src/Systems/ElementalSystem.cpp`.

### F034 — Magic Caster Void ultimate

- **Clip:** `AKA_F034_TIEN_MAGE_VOID_ULTIMATE_GAME_T01.mkv`.
- **Cảnh quay:** Dùng H lên mục tiêu có LOS; giữ charge, hit effect, Corroded và cooldown.
- **Thao tác:** Đợi skill bar sẵn sàng trước take.
- **Lời thoại:** “Void ultimate gây khoảng 80 damage và áp dụng hệ Void; Corroded làm mục tiêu nhận thêm damage.”
- **Bằng chứng:** `AppleKnightAdventure/src/Model/MagicCasterSkillSet.cpp`; `AppleKnightAdventure/src/Systems/ElementalSystem.cpp`.

### F035 — Magic Caster parry riêng

- **Clip:** `AKA_F035_TIEN_MAGE_PARRY_GAME_T01.mkv`.
- **Cảnh quay:** Mage block một projectile hoặc melee hit; giữ animation và HP bar.
- **Thao tác:** Không dùng dash trong cửa sổ va chạm để tránh nhầm nguồn miễn damage.
- **Lời thoại:** “Magic Caster vẫn có action parry class-specific dù vai trò chính là đánh tầm xa.”
- **Bằng chứng:** `AppleKnightAdventure/src/Model/MagicCasterSkillSet.cpp`; `AppleKnightAdventure/src/View/CharacterRenderer.cpp`.

## Block T7 — Nguyễn Trọng Tiến — F036–F040: bộ kỹ năng Ninja 2D

### F036 — Ninja Slash

- **Clip:** `AKA_F036_TIEN_NINJA_SLASH_GAME_T01.mkv`.
- **Cảnh quay:** Ninja dùng J, quay cận tốc độ animation và damage text.
- **Thao tác:** Đứng trong tầm melee, chỉ đánh một lần.
- **Lời thoại:** “Slash là đòn cơ bản nhanh của Ninja, khoảng 25 damage và cooldown 0,4 giây.”
- **Bằng chứng:** `AppleKnightAdventure/src/Model/NinjaSkillSet.cpp`.

### F037 — Ninja Blade Rush projectile

- **Clip:** `AKA_F037_TIEN_NINJA_BLADE_RUSH_GAME_T01.mkv`.
- **Cảnh quay:** Dùng K, cho thấy projectile được sinh ở cuối animation rồi bay tới enemy.
- **Thao tác:** Đặt camera đủ rộng để thấy cả spawn và contact.
- **Lời thoại:** “Blade Rush tạo projectile khoảng 40 damage, tốc độ 480 và tầm khoảng 550.”
- **Bằng chứng:** `AppleKnightAdventure/src/Model/NinjaSkillSet.cpp`; `AppleKnightAdventure/src/Model/Projectile.cpp`.

### F038 — Ninja Teleport Strike

- **Clip:** `AKA_F038_TIEN_NINJA_TELEPORT_GAME_T01.mkv`.
- **Cảnh quay:** Take A có enemy trong 500 pixel: Ninja teleport gần mục tiêu; take B không có mục tiêu: teleport tiến về trước.
- **Thao tác:** Đặt marker vị trí trước/sau cho cả hai take.
- **Lời thoại:** “Teleport ưu tiên enemy gần nhất trong phạm vi; nếu không có mục tiêu hợp lệ, Ninja dịch chuyển theo hướng nhìn.”
- **Bằng chứng:** `AppleKnightAdventure/src/Model/NinjaSkillSet.cpp`.

### F039 — Ninja Shadow Clone projectile

- **Clip:** `AKA_F039_TIEN_NINJA_SHADOW_CLONE_GAME_T01.mkv`.
- **Cảnh quay:** Dùng H, giữ trọn clone/projectile, quỹ đạo và hit effect.
- **Thao tác:** Quay trên nền ít chi tiết để silhouette dễ thấy.
- **Lời thoại:** “Shadow Clone ultimate tạo projectile riêng khoảng 60 damage với charge và cooldown dài.”
- **Bằng chứng:** `AppleKnightAdventure/src/Model/NinjaSkillSet.cpp`; `AppleKnightAdventure/src/Model/Projectile.cpp`.

### F040 — Ninja parry riêng

- **Clip:** `AKA_F040_TIEN_NINJA_PARRY_GAME_T01.mkv`.
- **Cảnh quay:** Ninja parry đúng trước enemy attack; close-up animation và số damage giảm.
- **Thao tác:** Dùng cùng enemy baseline của F025/F030/F035 để so sánh.
- **Lời thoại:** “Ninja hoàn thiện bộ class bằng parry implementation và animation riêng, vẫn dùng multiplier phòng thủ chung của Player.”
- **Bằng chứng:** `AppleKnightAdventure/src/Model/NinjaSkillSet.cpp`; `AppleKnightAdventure/src/View/CharacterRenderer.cpp`.

## Block T8 — Nguyễn Trọng Tiến — F041–F045: item, animation và Skill Bar

### F041 — Coin pickup và persistent currency

- **Clip:** `AKA_F041_TIEN_COIN_PICKUP_GAME_T01.mkv`.
- **Cảnh quay:** Nhặt coin đặt sẵn và coin văng ra từ Chest; HUD/score/coin tăng, kèm sound/particle.
- **Thao tác:** Ghi số coin trước và sau; kết thúc level hoặc về menu để cho thấy persistent coin được lưu.
- **Lời thoại:** “Coin cập nhật item score và persistent currency; trong local co-op, cả hai người chơi dùng chung số dư của profile.”
- **Bằng chứng:** `AppleKnightAdventure/src/Controller/GameController.cpp:1550-1595`; `AppleKnightAdventure/src/Factories/ItemFactory.cpp`; `AppleKnightAdventure/src/Model/SaveManager.cpp`.

### F042 — Apple hồi 25 HP

- **Clip:** `AKA_F042_TIEN_APPLE_HEAL_GAME_T01.mkv`.
- **Cảnh quay:** Player bị mất máu, nhặt Apple và HP tăng; floating heal text nếu xuất hiện.
- **Thao tác:** Ghi HP trước/sau, tránh nhặt khi đầy máu vì khó chứng minh tác dụng.
- **Lời thoại:** “Apple là consumable hồi 25 HP và luôn clamp ở max health.”
- **Bằng chứng:** `AppleKnightAdventure/src/Controller/GameController.cpp:1550-1595`; `AppleKnightAdventure/src/Factories/ItemFactory.cpp`.

### F043 — Hệ animation sprite 2D theo atlas và state

- **Clip:** `AKA_F043_TIEN_2D_ANIMATION_STATES_GAME_CODE_T01.mkv`.
- **Cảnh quay:** Một take liên tục idle → walk/run → jump/fall → dash → attack → parry → hurt → death/respawn; đổi hướng trái/phải; chèn atlas JSON và state-to-clip mapping.
- **Thao tác:** Giữ player trong khung, có thể slow-motion ở hai chuyển state nhưng vẫn giữ đoạn tốc độ thật.
- **Lời thoại:** “TextureAtlas đọc clip và thời lượng frame từ JSON; Animator phát frame, còn CharacterRenderer ánh xạ state gameplay và hướng nhìn sang clip tương ứng.”
- **Bằng chứng:** `AppleKnightAdventure/src/Model/Player.cpp:115-145`; `AppleKnightAdventure/src/View/CharacterRenderer.cpp:206-299`; `AppleKnightAdventure/src/View/Animator.cpp`; `AppleKnightAdventure/src/View/TextureAtlas.cpp`.

### F044 — Potion hồi 50 HP, SFX và achievement hook

- **Clip:** `AKA_F044_TIEN_POTION_GAME_T01.mkv`.
- **Cảnh quay:** Player còn dưới 50 HP thiếu, nhặt Potion; quay HP tăng, potion SFX và achievement lần đầu nếu chưa mở.
- **Thao tác:** Dùng save sạch cho take achievement; take gameplay chính chỉ cần chứng minh hồi máu.
- **Lời thoại:** “Potion hồi 50 HP, phát event âm thanh riêng và cập nhật tiến độ achievement.”
- **Bằng chứng:** `AppleKnightAdventure/src/Controller/GameController.cpp:1586-1589`; `AppleKnightAdventure/src/Systems/AchievementManager.cpp`; `assets/sounds/audio_manifest.json`.

### F045 — Skill Bar 2D hiển thị cooldown, charge và trạng thái active

- **Clip:** `AKA_F045_TIEN_SKILL_BAR_GAME_T01.mkv`.
- **Cảnh quay:** Dùng đủ bốn skill của ít nhất hai class; zoom thanh skill khi charge/active/cooldown/ready; quay màu nguyên tố Magic Caster, dash/parry badge và một take local co-op có hai bar.
- **Thao tác:** Giữ input viewer và Skill Bar trong cùng khung; chờ ít nhất một cooldown chạy về 0 ở tốc độ thật.
- **Lời thoại:** “SkillBarView lấy icon theo class và đồng bộ timer, charge, active, element cùng phím điều khiển; layout tự đổi giữa solo và local co-op.”
- **Bằng chứng:** `AppleKnightAdventure/src/View/SkillBarView.cpp:80-336`; `AppleKnightAdventure/src/Controller/GameController.cpp:159-160,460,612,2336`.

## Block T9 — Nguyễn Trọng Tiến — F046–F050: Chest, checkpoint, portal, tutorial và Cup

### F046 — Chest mở một lần, coin scatter và item physics

- **Clip:** `AKA_F046_TIEN_CHEST_LOOT_GAME_T01.mkv`.
- **Cảnh quay:** Chest closed → tương tác → animation open → nhiều coin bay lên rồi rơi xuống → nhặt coin.
- **Thao tác:** Thử tương tác lại để thấy không tạo loot lần hai; quay slow-motion quỹ đạo coin.
- **Lời thoại:** “Chest chỉ mở một lần, sinh số coin ngẫu nhiên với vận tốc ban đầu; UpdateItemPhysics xử lý trọng lực và va chạm cho coin sau khi văng ra.”
- **Bằng chứng:** `AppleKnightAdventure/src/Model/Chest.cpp`; `AppleKnightAdventure/src/Controller/GameController.cpp:1612-1680,2422-2480`.

### F047 — Checkpoint và encounter restore

- **Clip:** `AKA_F047_TIEN_CHECKPOINT_RESTORE_GAME_T01.mkv`.
- **Cảnh quay:** Kích hoạt checkpoint; hạ hoặc làm thay đổi một encounter sau checkpoint; chết; respawn và quan sát enemy/projectile được dựng lại theo snapshot.
- **Thao tác:** Giữ một khung trước chết và một khung sau respawn có cùng vùng camera để so sánh.
- **Lời thoại:** “Checkpoint lưu vị trí và snapshot encounter; khi respawn, game xóa projectile cũ và khôi phục enemy phù hợp mà không cho farm lại kill score đã tính.”
- **Bằng chứng:** `AppleKnightAdventure/src/Controller/GameController.cpp:1821-1880`; `AppleKnightAdventure/include/Controller/GameController.h`.

### F048 — Color-linked local portals

- **Clip:** `AKA_F048_TIEN_COLOR_PORTAL_GAME_T01.mkv`.
- **Cảnh quay:** Đi vào portal màu thứ nhất và xuất hiện ở portal cùng ColorId; quay partner được kéo theo trong một take co-op ngắn.
- **Thao tác:** Overlay ColorId của cặp portal; nếu map có nhiều màu, thử hai cặp để chứng minh mapping.
- **Lời thoại:** “Portal ghép cặp bằng ColorId và target level; portal nội bộ dịch chuyển trong cùng map, co-op còn đưa partner theo để không tách camera.”
- **Bằng chứng:** `AppleKnightAdventure/src/Model/TeleportPortal.cpp`; `AppleKnightAdventure/src/Controller/GameController.cpp:1705-1810`.

### F049 — Signboard và tutorial dialog

- **Clip:** `AKA_F049_TIEN_SIGNBOARD_TUTORIAL_GAME_T01.mkv`.
- **Cảnh quay:** Tiếp cận signboard, nhấn interact, đọc panel hướng dẫn; đóng panel và gameplay tiếp tục.
- **Thao tác:** Quay ít nhất hai bảng có nội dung khác nhau nếu Level 1 cung cấp.
- **Lời thoại:** “Signboard là world entity tương tác, mở tutorial dialog theo dữ liệu của level thay vì hard-code một màn hướng dẫn duy nhất.”
- **Bằng chứng:** `AppleKnightAdventure/src/Model/Signboard.cpp`; `AppleKnightAdventure/src/Model/InMapGuide.cpp`; `AppleKnightAdventure/src/View/TutorialRenderer.cpp`; `AppleKnightAdventure/src/Controller/GameController.cpp`.

### F050 — Trophy/Cup completion

- **Clip:** `AKA_F050_TIEN_TROPHY_COMPLETION_GAME_T01.mkv`.
- **Cảnh quay:** Đứng gần cúp, nhấn interact, quay particle/sound/camera shake và chuyển Result.
- **Thao tác:** Trước khi nhấn, đứng cạnh vài giây để chứng minh không tự hoàn thành chỉ vì chạm hitbox.
- **Lời thoại:** “LevelCompleteCup là một đường hoàn thành: player phải tương tác với cúp, sau đó GameController đặt completion và chạy pipeline kết quả.”
- **Bằng chứng:** `AppleKnightAdventure/src/Model/LevelCompleteCup.cpp`; `AppleKnightAdventure/src/Controller/GameController.cpp:1644-1651`; `AppleKnightAdventure/src/Model/GameState.cpp:302-321`.

## Block T10 — Nguyễn Trọng Tiến — F051–F055: completion, scoring, Result, minimap và achievement

### F051 — End checkpoint và cờ ba trạng thái

- **Clip:** `AKA_F051_TIEN_END_FLAG_GAME_T01.mkv`.
- **Cảnh quay:** Cờ end checkpoint từ uncaptured sang flag_out rồi captured loop; sau đó Result xuất hiện.
- **Thao tác:** Dùng slow-motion cho hai lần chuyển animation, giữ interaction key overlay.
- **Lời thoại:** “End checkpoint là nhánh hoàn thành khác với Cup; cờ trình bày ba giai đoạn hình ảnh uncaptured, flag-out và captured.”
- **Bằng chứng:** `AppleKnightAdventure/src/Controller/GameController.cpp:1696-1704,2520-2575`; `AppleKnightAdventure/src/Model/Checkpoint.cpp`; `AppleKnightAdventure/src/Model/GameState.cpp:302-321`.

### F052 — Performance scoring và hệ thống sao

- **Clip:** `AKA_F052_TIEN_SCORING_STARS_GAME_CODE_T01.mkv`.
- **Cảnh quay:** Result của một lượt chơi có enemy/item/time khác nhau; chèn graphic công thức và code.
- **Thao tác:** Nếu có thể, quay hai result: một lượt chậm/ít item và một lượt tốt hơn để sao thay đổi.
- **Lời thoại:** “Performance gồm 40% tỷ lệ hạ enemy, 35% tỷ lệ item và 25% thời gian; hoàn thành có một sao, từ 0,60 có hai sao, từ 0,85 có ba sao.”
- **Bằng chứng:** `AppleKnightAdventure/src/Model/LevelScoring.cpp:24-40`; `AppleKnightAdventure/src/Controller/GameController.cpp:1889-1952`.

### F053 — Result: Continue, Retry, Level Select

- **Clip:** `AKA_F053_TIEN_RESULT_ACTIONS_UI_T01.mkv`.
- **Cảnh quay:** Result star reveal/confetti/new record; lần lượt minh họa Retry, Level Select và Continue bằng ba take ngắn.
- **Thao tác:** Dùng cùng Result snapshot để các nút dễ so sánh.
- **Lời thoại:** “ResultView không chỉ hiển thị điểm; nó điều phối ba đường tiếp tục, chơi lại hoặc về chọn level.”
- **Bằng chứng:** `AppleKnightAdventure/include/View/ResultView.h`; `AppleKnightAdventure/src/View/ResultView.cpp`; `AppleKnightAdventure/src/Controller/GameController.cpp:1889-1952`.

### F054 — Minimap fog và exploration persistence

- **Clip:** `AKA_F054_TIEN_MINIMAP_EXPLORATION_GAME_T01.mkv`.
- **Cảnh quay:** Bản đồ nhỏ trước khi khám phá, khi đi qua khu vực mới và sau khi restart/load lại.
- **Thao tác:** Chọn một nhánh map chưa mở; đi qua; thoát an toàn để Save; mở lại level.
- **Lời thoại:** “Minimap chỉ mở vùng đã khám phá, thể hiện hướng/player và lưu exploration vào SaveManager.”
- **Bằng chứng:** `AppleKnightAdventure/src/View/MinimapView.cpp:62-220`; `AppleKnightAdventure/src/Model/SaveManager.cpp`.

### F055 — 15 achievement và popup tiến độ

- **Clip:** `AKA_F055_TIEN_ACHIEVEMENTS_UI_GAME_T01.mkv`.
- **Cảnh quay:** Achievement screen có 15 mục; kích hoạt một achievement dễ quay và giữ popup mở khóa.
- **Thao tác:** Dùng save sạch cho achievement “Potion”/“First completion” phù hợp; không giả lập bằng chỉnh file.
- **Lời thoại:** “AchievementManager định nghĩa 15 thành tựu, cập nhật từ sự kiện gameplay và hiển thị popup khi đạt điều kiện.”
- **Bằng chứng:** `AppleKnightAdventure/src/Systems/AchievementManager.cpp:39-200`; `AppleKnightAdventure/src/View/MenuView.cpp`.

## Block T11 — Nguyễn Trọng Tiến — F056–F060: leaderboard, Options, presentation và MapBuilder entry

### F056 — Local leaderboard theo score và time

- **Clip:** `AKA_F056_TIEN_LOCAL_LEADERBOARD_UI_T01.mkv`.
- **Cảnh quay:** Hoàn thành level, sau đó mở Leaderboard và chuyển giữa score/time; quay record có class, sao và co-op flag nếu có.
- **Thao tác:** Tạo hai run khác nhau nếu cần để thấy thứ tự sắp xếp.
- **Lời thoại:** “SaveManager giữ tối đa năm record score và năm record time cho mỗi level, hoàn toàn local.”
- **Bằng chứng:** `AppleKnightAdventure/src/Model/SaveManager.cpp:429-467`; `AppleKnightAdventure/src/Controller/GameController.cpp:1929-1949`; `AppleKnightAdventure/src/View/MenuView.cpp`.

### F057 — Options cho music, SFX và fullscreen

- **Clip:** `AKA_F057_TIEN_OPTIONS_AUDIO_FULLSCREEN_UI_T01.mkv`.
- **Cảnh quay:** Kéo music slider, SFX slider, bật fullscreen; quay sự thay đổi nghe/nhìn và trạng thái vẫn giữ sau restart.
- **Thao tác:** Phát một menu event trước/sau khi đổi SFX; để BGM chạy khi chỉnh music.
- **Lời thoại:** “Options điều khiển độc lập music, SFX và fullscreen, sau đó ghi thiết lập vào save.”
- **Bằng chứng:** `AppleKnightAdventure/src/View/OptionsView.cpp:115-350`; `AppleKnightAdventure/src/Systems/SoundManager.cpp:315-340`; `AppleKnightAdventure/src/Model/SaveManager.cpp`.

### F058 — Parallax background theo theme

- **Clip:** `AKA_F058_TIEN_PARALLAX_GAME_T01.mkv`.
- **Cảnh quay:** Camera di chuyển ngang trong Forest và ColdCorridor; dùng crop/guide để thấy các layer nền trượt với tốc độ khác nhau.
- **Thao tác:** Đi đều, không dash; nếu dựng split-screen, giữ cùng tốc độ playback.
- **Lời thoại:** “Nền được chia thành nhiều layer có hệ số camera khác nhau, tạo chiều sâu parallax theo theme của level.”
- **Bằng chứng:** `AppleKnightAdventure/src/View/GameView.cpp:233-346`; `AppleKnightAdventure/src/View/MenuView.cpp`.

### F059 — Particle, floating text, camera shake và hit-stop

- **Clip:** `AKA_F059_TIEN_COMBAT_FEEDBACK_GAME_T01.mkv`.
- **Cảnh quay:** Một heavy/ultimate hit nhiều enemy; giữ particle burst, số damage nổi, rung camera và freeze-frame ngắn.
- **Thao tác:** Quay 60 FPS rồi chèn replay 25% tốc độ có nhãn kỹ thuật; không nhầm lag với hit-stop.
- **Lời thoại:** “Combat feedback kết hợp particle pool, floating text, camera shake và hit-stop có giới hạn để đòn mạnh có trọng lượng.”
- **Bằng chứng:** `AppleKnightAdventure/src/Systems/ParticleSystem.cpp`; `AppleKnightAdventure/src/View/FloatingText.cpp`; `AppleKnightAdventure/src/View/GameView.cpp`; `AppleKnightAdventure/src/Controller/GameController.cpp`.

### F060 — MapBuilder là mode tích hợp trong ứng dụng

- **Clip:** `AKA_F060_TIEN_MAPBUILDER_ENTRY_UI_T01.mkv`.
- **Cảnh quay:** Từ Main Menu mở Map Builder; workspace editor xuất hiện với toolbar, palette, canvas, minimap; Back trở về menu.
- **Thao tác:** Chỉ chứng minh mode và vòng đời tích hợp; tool chi tiết được quay ở F123–F127 bởi Nguyễn Anh Kiệt.
- **Lời thoại:** “MapBuilder chạy như một mode của executable chính, có controller/view riêng và chuyển trạng thái an toàn với menu/gameplay.”
- **Bằng chứng:** `AppleKnightAdventure/src/main.cpp:383-430`; `AppleKnightAdventure/src/Controller/MapBuilderController.cpp`; `AppleKnightAdventure/src/View/MapBuilderView.cpp`.

## Block T12 — Nguyễn Trọng Tiến — F061–F065: enemy archetype, FSM, LOS và ledge avoidance

### F061 — Melee enemy

- **Clip:** `AKA_F061_TIEN_MELEE_ENEMY_GAME_T01.mkv`.
- **Cảnh quay:** Melee enemy patrol, phát hiện player, chase, wind-up, đánh, hurt và dead.
- **Thao tác:** Đứng ngoài detection, bước vào, lùi ra khỏi attack range trong một take và để trúng trong take khác.
- **Lời thoại:** “Melee enemy dùng FSM đầy đủ thay vì luôn chạy thẳng; nó tuần tra, đuổi, báo trước đòn đánh rồi mới gây damage.”
- **Bằng chứng:** `AppleKnightAdventure/src/Model/Enemy.cpp`; `AppleKnightAdventure/src/Factories/EnemyFactory.cpp`.

### F062 — Ranged enemy và projectile

- **Clip:** `AKA_F062_TIEN_RANGED_ENEMY_GAME_T01.mkv`.
- **Cảnh quay:** Ranged enemy giữ khoảng cách, vào attack state và bắn bomb/projectile; projectile va chạm player/tile.
- **Thao tác:** Dùng hành lang dài để quỹ đạo nhìn rõ; dash qua một projectile nếu muốn liên kết F015.
- **Lời thoại:** “Ranged archetype dùng cùng FSM nhưng tạo projectile riêng thay vì hitbox melee.”
- **Bằng chứng:** `AppleKnightAdventure/src/Model/Enemy.cpp`; `AppleKnightAdventure/src/Model/Projectile.cpp`; `AppleKnightAdventure/src/Controller/GameController.cpp`.

### F063 — Flying enemy

- **Clip:** `AKA_F063_TIEN_FLYING_ENEMY_GAME_T01.mkv`.
- **Cảnh quay:** Flying enemy đổi cả X/Y, bám theo player trên platform khác độ cao và tấn công.
- **Thao tác:** Dẫn enemy qua khu vực có platform cao/thấp; không đứng sau tường trong take đầu.
- **Lời thoại:** “Flying enemy không bị ràng buộc bởi gravity mặt đất; AI điều chỉnh hai trục để tiếp cận mục tiêu.”
- **Bằng chứng:** `AppleKnightAdventure/src/Model/Enemy.cpp`; `AppleKnightAdventure/src/Controller/GameController.cpp:969-1150`.

### F064 — Enemy FSM và attack telegraph

- **Clip:** `AKA_F064_TIEN_ENEMY_FSM_TELEGRAPH_GAME_CODE_T01.mkv`.
- **Cảnh quay:** Overlay state khi enemy đổi Idle/Patrol/Chase/WindUp/Attack/Hurt/Dead; lùi khỏi range trong wind-up để thấy attack bị hủy.
- **Thao tác:** Ghép gameplay với enum/code và sơ đồ state đơn giản.
- **Lời thoại:** “Enemy có bảy state; ground attack dùng telegraph khoảng 0,75 giây và có thể bị hủy nếu mục tiêu thoát phạm vi trước contact.”
- **Bằng chứng:** `AppleKnightAdventure/include/Utils/Types.h`; `AppleKnightAdventure/src/Model/Enemy.cpp:118-330`.

### F065 — Line-of-sight và ledge avoidance

- **Clip:** `AKA_F065_TIEN_ENEMY_LOS_LEDGE_GAME_T01.mkv`.
- **Cảnh quay:** Take A: flying enemy bị tường che không chọn đường bắn trực tiếp; take B: ground enemy tới mép vực rồi đổi hướng.
- **Thao tác:** Giữ geometry rõ ràng; thêm overlay ray/probe minh họa trong hậu kỳ, ghi rõ overlay không phải debug render của game.
- **Lời thoại:** “AI dùng DDA line-of-sight cho mục tiêu bay và probe nền phía trước để ground enemy không tự bước khỏi platform.”
- **Bằng chứng:** `AppleKnightAdventure/src/Controller/GameController.cpp:969-1150`; `AppleKnightAdventure/src/Model/Boss.cpp:154-227`.

## Block T13 — Nguyễn Trọng Tiến — F066–F070: boss AI, pet, boon và core

### F066 — Ba boss 2D và nhiều phase

- **Clip:** `AKA_F066_TIEN_THREE_2D_BOSSES_GAME_T01.mkv`.
- **Cảnh quay:** Montage Boss1, Boss2, Boss3; mỗi boss phải hiện ít nhất một lần đổi phase và một kỹ năng đặc trưng.
- **Thao tác:** Overlay `Boss1: 2 phase`, `Boss2: 3 phase`, `Boss3: 4 phase`; footage Level 6 có thể tái sử dụng nhưng mã feature vẫn tách.
- **Lời thoại:** “Ba boss có state machine riêng; Boss1 có hai phase, Boss2 ba phase và Boss3 bốn phase với chính sách chiến đấu khác nhau.”
- **Bằng chứng:** `AppleKnightAdventure/src/Model/Boss1.cpp`; `Boss2.cpp`; `Boss3.cpp`; `AppleKnightAdventure/include/Model/Boss.h`.

### F067 — Boss navigation, retreat và stuck recovery

- **Clip:** `AKA_F067_TIEN_BOSS_NAVIGATION_AI_GAME_CODE_T01.mkv`.
- **Cảnh quay:** Boss đi vòng platform/hành lang tới player, lùi khi cần, nhảy/leap qua gap hoặc tự thoát vị trí kẹt; chèn code BFS/LOS.
- **Thao tác:** Dẫn boss qua geometry có chênh cao; không chỉ quay arena phẳng.
- **Lời thoại:** “Boss dùng DDA line-of-sight và BFS xét footprint, có retreat, gap leap, recheck đường đi và unstick khi không tiến triển.”
- **Bằng chứng:** `AppleKnightAdventure/src/Model/Boss.cpp:154-560`.

### F068 — Bốn pet behavior

- **Clip:** `AKA_F068_TIEN_FOUR_PETS_GAME_T01.mkv`.
- **Cảnh quay:** Bốn đoạn có nhãn: Skull/Dragon chọn enemy rồi bắn; Fairy tự tìm và nhặt item; Ghost hồi máu ngoài combat.
- **Thao tác:** Mỗi pet quay đúng điều kiện kích hoạt; với Ghost phải mất máu và thoát combat; với Fairy đặt item trong bán kính.
- **Lời thoại:** “Pet AI là rule-based theo vai trò: hai pet tấn công, Fairy thu thập và Ghost hồi phục có ngân sách.”
- **Bằng chứng:** `AppleKnightAdventure/src/Model/Pet.cpp:55-180`; `AppleKnightAdventure/src/Controller/GameController.cpp`.

### F069 — 12 timed boon và lựa chọn ba card

- **Clip:** `AKA_F069_TIEN_BOON_DRAFT_GAME_T01.mkv`.
- **Cảnh quay:** Trong boss arena, chờ boon draft; game dừng, ba card khác nhau xuất hiện; chọn X/Y/Z và quay hiệu ứng buff trên HUD/gameplay.
- **Thao tác:** Chèn trang tổng hợp tên 12 boon; gameplay chỉ cần minh họa một instant boon và một timed boon.
- **Lời thoại:** “BuffSystem định nghĩa 12 boon; draft chọn ba option không trùng theo trọng số và áp dụng tức thời hoặc trong thời lượng cấu hình.”
- **Bằng chứng:** `AppleKnightAdventure/src/Systems/BuffSystem.cpp`; `AppleKnightAdventure/src/Controller/GameController.cpp:4009-4070`.

### F070 — 21 run-long core, rarity và class lock

- **Clip:** `AKA_F070_TIEN_CORE_SYSTEM_GAME_CODE_T01.mkv`.
- **Cảnh quay:** Core draft ba card sau kill/boss, rarity màu, chọn core, mở loadout; chèn danh sách 21 CoreId và một core class-specific.
- **Thao tác:** Quay một draft thường và một boss reward nếu thời gian; nêu rõ core kéo dài trong run.
- **Lời thoại:** “CoreSystem có 21 core, stack cap, rarity, class lock, pity và các hiệu ứng build như revive, pierce, chain spark.”
- **Bằng chứng:** `AppleKnightAdventure/include/Systems/CoreSystem.h`; `AppleKnightAdventure/src/Systems/CoreSystem.cpp:41-220`; `AppleKnightAdventure/src/Controller/GameController.cpp:3790-3880`.

## Block T14 — Nguyễn Trọng Tiến — F071–F075: element, co-op, dynamic spawn và boss portal

### F071 — Bốn elemental aura/status

- **Clip:** `AKA_F071_TIEN_FOUR_ELEMENTS_GAME_T01.mkv`.
- **Cảnh quay:** Montage Fire/Burn, Water/Wet, Thunder/Shocked, Void/Corroded; mỗi status có icon/tint/timer.
- **Thao tác:** Dùng skill/infusion tạo từng nguyên tố trên enemy còn đủ HP để status tồn tại lâu.
- **Lời thoại:** “Bốn aura tạo bốn status: Burn gây DOT, Wet làm chậm, Shocked vừa DOT vừa slow, Corroded tăng damage nhận vào.”
- **Bằng chứng:** `AppleKnightAdventure/src/Systems/ElementalSystem.cpp`; `AppleKnightAdventure/src/View/EnemyStatusRenderer.cpp`; `AppleKnightAdventure/src/View/ElementalFX.cpp`.

### F072 — 12 elemental reaction và live Codex

- **Clip:** `AKA_F072_TIEN_ELEMENT_REACTIONS_CODEX_GAME_T01.mkv`.
- **Cảnh quay:** Mở Codex bằng C, cuộn/đọc 12 cặp; đóng bảng rồi kích hoạt ít nhất hai reaction theo thứ tự khác nhau.
- **Thao tác:** Giữ tên reaction, multiplier/damage text và particle trong frame; nói rõ cùng nguyên tố chỉ refresh status.
- **Lời thoại:** “Ma trận có 12 phản ứng có hướng; aura sẵn có kết hợp nguyên tố đánh vào để tạo reaction, còn đánh lại cùng hệ chỉ làm mới thời gian.”
- **Bằng chứng:** `AppleKnightAdventure/src/Systems/ElementalSystem.cpp`; `AppleKnightAdventure/src/Controller/GameController.cpp:3090-3375`.

### F073 — Local co-op: dual HUD, shared camera, nearest targeting và shared upgrades

- **Clip:** `AKA_F073_TIEN_LOCAL_COOP_FULL_GAME_T01.mkv`.
- **Cảnh quay:** P1/P2 di chuyển tách nhau, camera zoom; dual HP/skill bars; enemy đổi sang người gần hơn; chọn boon/core và thấy cả hai nhận; portal kéo partner.
- **Thao tác:** Quay liên tục một encounter; để key overlay cho cả hai bộ điều khiển.
- **Lời thoại:** “Đây là local co-op hai người: hai input và HUD riêng, camera chung, AI chọn mục tiêu gần nhất, tài nguyên/upgrade được chia theo logic của run.”
- **Bằng chứng:** `AppleKnightAdventure/src/Controller/GameController.cpp:145,542-548,2033-2336,3830,4050`; `AppleKnightAdventure/src/View/HUDView.cpp`; `AppleKnightAdventure/src/View/SkillBarView.cpp`.

### F074 — Dynamic random enemy spawning

- **Clip:** `AKA_F074_TIEN_DYNAMIC_ENEMY_SPAWN_GAME_T01.mkv`.
- **Cảnh quay:** Đứng/di chuyển đủ lâu trong campaign để enemy động xuất hiện tại vị trí hợp lệ; overlay timer, khoảng cách và active count.
- **Thao tác:** Quay đoạn chưa có enemy rồi thời điểm spawn; nếu cần tăng tốc phần chờ nhưng để spawn ở tốc độ thật.
- **Lời thoại:** “Campaign director có thể sinh thêm enemy sau khoảng thời gian ngẫu nhiên, giới hạn số active và kiểm tra nền/khoảng trống trước khi tạo.”
- **Bằng chứng:** `AppleKnightAdventure/src/Controller/GameController.cpp` — `UpdateRandomEnemySpawns`; `AppleKnightAdventure/include/Controller/GameController.h`.

### F075 — Boss-arena portal lock và snapshot/restore player

- **Clip:** `AKA_F075_TIEN_BOSS_ARENA_PORTAL_GAME_T01.mkv`.
- **Cảnh quay:** Ghi HP, score, skill points, max HP và Core loadout trước khi vào boss arena; vào portal; exit portal còn khóa khi boss sống; hạ boss, portal mở; trở về level cũ và đối chiếu state.
- **Thao tác:** Không chỉnh save giữa hai cảnh; quay trọn transition vào/ra.
- **Lời thoại:** “Game snapshot state trước khi đổi sang boss arena, khóa exit khi còn enemy và restore state khi quay lại level trước.”
- **Bằng chứng:** `AppleKnightAdventure/include/Controller/GameController.h:34-48,102-107,281-284`; `AppleKnightAdventure/src/Controller/GameController.cpp:1749-1805,1964-2025`.

## Block K1 — Nguyễn Anh Kiệt — F076–F080: Survival3D setup, movement và camera

### F076 — Survival3D Character Select với hai hero

- **Clip:** `AKA_F076_KIET_3D_CHARACTER_SELECT_UI_T01.mkv`.
- **Cảnh quay:** Mở Aegis Rift/Survival3D, chuyển giữa Knight và Magic Caster; model, tên, weapon/skill labels đổi; bắt đầu run bằng cả hai hero trong hai take.
- **Thao tác:** Dùng A/D hoặc click card; giữ màn skill list đủ lâu để đọc.
- **Lời thoại:** “Survival3D là mode single-player riêng với hai hero, mỗi hero có weapon, animation track và bộ năm action khác nhau.”
- **Bằng chứng:** `AppleKnightAdventure/include/Survival3D/Model/SurvivalTypes.h:10-35`; `AppleKnightAdventure/src/Survival3D/Controller/SurvivalController.cpp:609-645`; `AppleKnightAdventure/src/Survival3D/View/SurvivalView.cpp:3709-3740`.

### F077 — Di chuyển tự do trên mặt phẳng XZ

- **Clip:** `AKA_F077_KIET_3D_XZ_MOVEMENT_GAME_T01.mkv`.
- **Cảnh quay:** Đi tiến/lùi/trái/phải, chéo và xoay hướng trong arena; camera cho thấy đây là mặt phẳng 3D chứ không phải side-scroller.
- **Thao tác:** Dùng WASD và Arrow; đi vòng quanh một pillar để làm mốc không gian.
- **Lời thoại:** “Input được chiếu theo hướng camera lên mặt phẳng XZ, chuẩn hóa vector chéo và cập nhật facing độc lập với tọa độ màn hình.”
- **Bằng chứng:** `AppleKnightAdventure/src/Survival3D/Controller/SurvivalController.cpp:790-995`.

### F078 — Third-person orbit/zoom camera và camera collision

- **Clip:** `AKA_F078_KIET_3D_CAMERA_GAME_T01.mkv`.
- **Cảnh quay:** Orbit quanh hero, zoom gần/xa, đi sát pillar/tường để camera tự điều chỉnh không xuyên geometry.
- **Thao tác:** Kéo chuột theo cả yaw/pitch; cuộn wheel; lùi player sát vật cản.
- **Lời thoại:** “Camera third-person có orbit, zoom và kiểm tra vật cản từ target tới vị trí camera để tránh nhìn xuyên arena.”
- **Bằng chứng:** `AppleKnightAdventure/src/Survival3D/Controller/SurvivalController.cpp:2896-2990`; `AppleKnightAdventure/include/Survival3D/Controller/SurvivalController.h:86-89`.

### F079 — Target lock và aim assist

- **Clip:** `AKA_F079_KIET_TARGET_LOCK_GAME_T01.mkv`.
- **Cảnh quay:** Có nhiều enemy; nhấn T hoặc middle mouse để khóa mục tiêu; camera/reticle/facing bám mục tiêu, sau đó tắt lock và aim tự do.
- **Thao tác:** Đổi vị trí để target gần nhất thay đổi; không gọi đây là aimbot mạng.
- **Lời thoại:** “Target lock chọn enemy hợp lệ và cung cấp aim direction cho combat/camera; người chơi có thể bật tắt bằng T hoặc chuột giữa.”
- **Bằng chứng:** `AppleKnightAdventure/src/Survival3D/Controller/SurvivalController.cpp:790-835,2933-2990`; `AppleKnightAdventure/include/Survival3D/Controller/SurvivalController.h:70`.

### F080 — Jump, dash và invulnerability 3D

- **Clip:** `AKA_F080_KIET_3D_JUMP_DASH_GAME_T01.mkv`.
- **Cảnh quay:** Nhảy bằng Space; dash bằng L qua một attack/projectile; skill bar/cooldown và HP cho thấy cửa sổ tránh damage.
- **Thao tác:** Thử dash lại ngay để chứng minh cooldown; không nhầm dash Knight và Mage là cùng VFX.
- **Lời thoại:** “Survival3D có grounded jump và dash action riêng; dash đặt invulnerable timer ngắn và dùng cooldown theo hero.”
- **Bằng chứng:** `AppleKnightAdventure/src/Survival3D/Controller/SurvivalController.cpp:836-997,2123-2133,2630-2642,2757-2775`; `AppleKnightAdventure/include/Survival3D/Model/SurvivalTypes.h:178-210`.

## Block K2 — Nguyễn Anh Kiệt — F081–F085: bộ kỹ năng Knight 3D

### F081 — Knight Violet Edge và combo ba nhịp

- **Clip:** `AKA_F081_KIET_VIOLET_EDGE_GAME_T01.mkv`.
- **Cảnh quay:** Bấm J/left mouse ba lần đúng nhịp; hiện ba slash, đòn thứ ba mạnh hơn, cone hitbox/VFX/audio.
- **Thao tác:** Đánh một nhóm enemy để thấy arc và combo damage `1.0/1.1/1.6`.
- **Lời thoại:** “Violet Edge là basic combo ba bước; hit thứ ba tăng damage và vẫn kiểm tra phạm vi, hướng nhìn cùng line-of-sight arena.”
- **Bằng chứng:** `AppleKnightAdventure/src/Survival3D/Controller/SurvivalController.cpp:2455-2508`; `AppleKnightAdventure/src/Survival3D/Vfx/SkillVfxPackages.cpp`.

### F082 — Knight Aegis Counter/Guard

- **Clip:** `AKA_F082_KIET_AEGIS_COUNTER_GAME_T01.mkv`.
- **Cảnh quay:** Dùng K/right mouse trước đòn boss; shield/rune xuất hiện, damage giảm; nếu có Royal Bulwark, quay một take thời lượng guard dài hơn.
- **Thao tác:** So sánh cùng một attack không guard và có guard.
- **Lời thoại:** “Aegis Counter mở guard timer khoảng 0,65 giây; Royal Bulwark có thể kéo dài lên khoảng một giây.”
- **Bằng chứng:** `AppleKnightAdventure/src/Survival3D/Controller/SurvivalController.cpp:2511-2522,2757-2770`; `AppleKnightAdventure/src/Survival3D/Vfx/SkillVfxPackages.cpp`.

### F083 — Knight Shield Rush

- **Clip:** `AKA_F083_KIET_SHIELD_RUSH_GAME_T01.mkv`.
- **Cảnh quay:** Dùng U/Q xuyên qua hàng enemy; thấy root-motion travel, swept-path hit, knockback và invulnerability ngắn.
- **Thao tác:** Xếp ít nhất ba enemy theo đường thẳng; overlay đường đi start→end.
- **Lời thoại:** “Shield Rush lấy chuyển vị từ root-motion curve và gây damage theo toàn đoạn swept path, không teleport rồi kiểm tra một điểm.”
- **Bằng chứng:** `AppleKnightAdventure/src/Survival3D/Controller/SurvivalController.cpp:2547-2596`; `AppleKnightAdventure/src/Survival3D/Animation/AnimationGraph.cpp`.

### F084 — Knight Bastion Breaker

- **Clip:** `AKA_F084_KIET_BASTION_BREAKER_GAME_T01.mkv`.
- **Cảnh quay:** Đủ ultimate charge, dùng H/R giữa nhóm enemy; giữ full wind-up/contact, AoE, knockback, camera shake và spatial sound.
- **Thao tác:** Không che ultimate meter; đặt pillar ngoài phạm vi để minh họa LOS nếu có thể.
- **Lời thoại:** “Bastion Breaker là ultimate AoE bán kính khoảng 5,5 nhân area multiplier, base damage khoảng 85 trước multiplier build.”
- **Bằng chứng:** `AppleKnightAdventure/src/Survival3D/Controller/SurvivalController.cpp:2599-2627`; `AppleKnightAdventure/src/Survival3D/Vfx/SkillVfxPackages.cpp`.

### F085 — Knight Steel Step

- **Clip:** `AKA_F085_KIET_STEEL_STEP_GAME_T01.mkv`.
- **Cảnh quay:** Dash Knight, giữ afterimage/ribbon/dust, quỹ đạo và âm thanh Steel Step.
- **Thao tác:** Quay góc xiên để VFX không bị hero che; thêm một dodge thành công.
- **Lời thoại:** “Steel Step là presentation package riêng của dash Knight, đồng bộ animation event, VFX, camera cue và spatial audio.”
- **Bằng chứng:** `AppleKnightAdventure/src/Survival3D/Controller/SurvivalController.cpp:2123-2133,2630-2642`; `AppleKnightAdventure/src/Survival3D/Vfx/SkillVfxPackages.cpp:136-153`.

## Block K3 — Nguyễn Anh Kiệt — F086–F090: bộ kỹ năng Magic Caster 3D

### F086 — Mage Arc Bolt

- **Clip:** `AKA_F086_KIET_ARC_BOLT_GAME_T01.mkv`.
- **Cảnh quay:** J/left mouse tạo projectile mesh, trail và impact; nếu có Forked Bolt, quay hai bolt phụ.
- **Thao tác:** Bắn từ xa, giữ cả spawn và va chạm trong frame.
- **Lời thoại:** “Arc Bolt là basic projectile của Mage; Forked Bolt nâng cấp nó thành hai nhánh phụ yếu hơn.”
- **Bằng chứng:** `AppleKnightAdventure/src/Survival3D/Controller/SurvivalController.cpp:2047-2075,2458-2469`; `AppleKnightAdventure/src/Survival3D/Vfx/SkillVfxPackages.cpp`.

### F087 — Mage Frost Ring

- **Clip:** `AKA_F087_KIET_FROST_RING_GAME_T01.mkv`.
- **Cảnh quay:** K/right mouse khi nhiều enemy áp sát; vòng băng lan ra, gây damage và slow ba giây.
- **Thao tác:** Giữ một enemy sống để quay tốc độ trước/sau slow.
- **Lời thoại:** “Tên hiển thị là Frost Ring; kỹ năng đánh AoE khoảng bốn đơn vị nhân area multiplier và đặt slow timer ba giây.”
- **Bằng chứng:** `AppleKnightAdventure/src/Survival3D/Controller/SurvivalController.cpp:2525-2544`; `AppleKnightAdventure/src/Survival3D/View/SurvivalView.cpp:3725-3726`.

### F088 — Mage Gravity Well

- **Clip:** `AKA_F088_KIET_GRAVITY_WELL_GAME_T01.mkv`.
- **Cảnh quay:** Dùng U/Q, chọn điểm trong arena; vortex tồn tại bốn giây và kéo/ảnh hưởng enemy; quay Event Horizon nếu có để thấy projectile địch bị hủy.
- **Thao tác:** Đặt well gần nhóm enemy nhưng không xuyên pillar; giữ tâm và bán kính trong frame.
- **Lời thoại:** “Gravity Well clamp điểm đặt trong arena, dừng trước pillar, tồn tại bốn giây; Event Horizon bổ sung khả năng phá projectile địch.”
- **Bằng chứng:** `AppleKnightAdventure/src/Survival3D/Controller/SurvivalController.cpp:2547-2561`; `AppleKnightAdventure/src/Survival3D/Vfx/SkillVfxPackages.cpp`; `UpgradeId::EventHorizon` trong `SurvivalTypes.h`.

### F089 — Mage Astral Tempest

- **Clip:** `AKA_F089_KIET_ASTRAL_TEMPEST_GAME_T01.mkv`.
- **Cảnh quay:** Đủ charge, dùng H/R giữa arena; giữ sigil/meteor/impact, bán kính lớn và nhiều enemy nhận damage.
- **Thao tác:** Quay từ camera hơi cao để thấy AoE khoảng tám đơn vị.
- **Lời thoại:** “Astral Tempest là ultimate Mage có vùng rộng hơn Bastion Breaker, base damage khoảng 65 và kiểm tra line-of-sight tới từng enemy.”
- **Bằng chứng:** `AppleKnightAdventure/src/Survival3D/Controller/SurvivalController.cpp:2599-2627`; `AppleKnightAdventure/src/Survival3D/Vfx/SkillVfxPackages.cpp:217-238`.

### F090 — Mage Phase Blink

- **Clip:** `AKA_F090_KIET_PHASE_BLINK_GAME_T01.mkv`.
- **Cảnh quay:** Dash Mage qua projectile, giữ afterimage/motes/distortion và HP không đổi trong cửa sổ hợp lệ.
- **Thao tác:** Dựng cạnh Steel Step để thấy presentation khác nhưng không gộp mã feature.
- **Lời thoại:** “Phase Blink là dash package của Mage, có animation/audio/VFX riêng và cùng hệ invulnerable timer ngắn.”
- **Bằng chứng:** `AppleKnightAdventure/src/Survival3D/Controller/SurvivalController.cpp:2123-2133,2630-2642`; `AppleKnightAdventure/src/Survival3D/Vfx/SkillVfxPackages.cpp:240-260`.

## Block K4 — Nguyễn Anh Kiệt — F091–F094: enemy, boss, director và upgrade draft 3D

### F091 — Tám archetype đối thủ 3D

- **Clip:** `AKA_F091_KIET_EIGHT_ARCHETYPES_GAME_T01.mkv`.
- **Cảnh quay:** Title card và combat clip cho Riftling, Hex Archer, Obsidian Brute, Brood Warden, Hexeye Artillerist, Ironroot Colossus, Eclipse Chimera, Void Sovereign.
- **Thao tác:** Mỗi archetype phải hiện model, tên overlay và ít nhất một hành động đặc trưng; boss footage có thể lấy từ W10/20/30/40/50.
- **Lời thoại:** “Survival3D định nghĩa ba archetype thường và năm boss chính, mỗi loại có chỉ số và policy cập nhật riêng.”
- **Bằng chứng:** `AppleKnightAdventure/include/Survival3D/Model/SurvivalTypes.h:37-47`; `AppleKnightAdventure/src/Survival3D/Controller/SurvivalController.cpp:1215-2020`.

### F092 — Năm boss 3D có phase/special riêng

- **Clip:** `AKA_F092_KIET_FIVE_3D_BOSSES_GAME_T01.mkv`.
- **Cảnh quay:** Montage năm boss; giữ boss title/HP/phase, một phase transition và một special của mỗi boss.
- **Thao tác:** Dùng footage boss wave nguyên bản, không chỉ quay model ở Character Select/debug.
- **Lời thoại:** “Năm mốc boss không phải reskin: Brood Warden triệu hồi đàn, Hexeye bắn volley, Ironroot dùng slam/fissure, Chimera đổi policy, Void Sovereign có ba phase và summon/projectile pattern.”
- **Bằng chứng:** `AppleKnightAdventure/src/Survival3D/Controller/SurvivalController.cpp:1680-2020`; `AppleKnightAdventure/src/Survival3D/View/SurvivalView.cpp`.

### F093 — Director data-driven: budget, mix, cap và scaling

- **Clip:** `AKA_F093_KIET_WAVE_DIRECTOR_GAME_CODE_T01.mkv`.
- **Cảnh quay:** Chèn `waves.json` và `balance.json`, sau đó gameplay W01, W25, W48 để thấy mật độ/tốc độ/độ bền tăng.
- **Thao tác:** Overlay công thức budget `5 + 0.65w + 0.012w²`, interval giảm tới 0,5 giây và HP/damage scale.
- **Lời thoại:** “Wave thường dùng threat budget chứ không phải số quái cố định; mix chọn Swarm/Ranger/Tanker, active cap và HP/damage cùng tăng theo wave.”
- **Bằng chứng:** `assets/survival3d/config/waves.json`; `assets/survival3d/config/balance.json`; `AppleKnightAdventure/src/Survival3D/Controller/SurvivalController.cpp:338-434,1015-1061,1215-1325,3031-3040`.

### F094 — Draft ba card với 18 upgrade, rarity, stack và pity

- **Clip:** `AKA_F094_KIET_3D_UPGRADES_UI_GAME_T01.mkv`.
- **Cảnh quay:** Wave Clear → ba card → đổi lựa chọn bằng phím/chuột → apply; montage Common/Rare/Epic/Legendary và một hero-specific upgrade.
- **Thao tác:** Chèn danh sách 18 UpgradeId; quay stack count thay đổi nếu chọn lại upgrade stackable.
- **Lời thoại:** “Survival3D có 18 upgrade, rarity/weight, stack cap, hero restriction và pity để giảm chuỗi draft không có đồ hiếm.”
- **Bằng chứng:** `AppleKnightAdventure/include/Survival3D/Model/SurvivalTypes.h:57-83`; `AppleKnightAdventure/src/Survival3D/Controller/SurvivalController.cpp:292-319,1070-1172`.

## Block K5 — Nguyễn Anh Kiệt — F095–F100: fixed step, pooling, animation/VFX và run result

### F095 — Fixed timestep 1/60 và catch-up cap

- **Clip:** `AKA_F095_KIET_FIXED_TIMESTEP_GAME_CODE_T01.mkv`.
- **Cảnh quay:** Bật performance HUD, chơi lúc FPS ổn định và khi kéo/resize cửa sổ; chèn code accumulator.
- **Thao tác:** Không cố gây crash; chỉ tạo chậm ngắn rồi quan sát simulation phục hồi.
- **Lời thoại:** “Simulation chạy bước cố định 1/60 giây và tối đa sáu catch-up tick mỗi frame, giảm phụ thuộc render FPS và tránh spiral-of-death.”
- **Bằng chứng:** `AppleKnightAdventure/src/Survival3D/Controller/SurvivalController.cpp:766-777`.

### F096 — Pool enemy và projectile

- **Clip:** `AKA_F096_KIET_3D_POOLS_GAME_CODE_T01.mkv`.
- **Cảnh quay:** Wave đông/projectile dày; performance HUD ổn định; chèn code `AcquireEnemy`/`AcquireProjectile` và capacity.
- **Thao tác:** Chọn W40+ hoặc boss projectile pattern; không tuyên bố “zero allocation toàn game”.
- **Lời thoại:** “SurvivalController tái sử dụng slot enemy và projectile, với pool khoảng 144 enemy và 384 projectile, thay vì tạo/xóa object cho từng spawn.”
- **Bằng chứng:** `AppleKnightAdventure/include/Survival3D/Controller/SurvivalController.h`; `AppleKnightAdventure/src/Survival3D/Controller/SurvivalController.cpp` — `AcquireEnemy`, `AcquireProjectile`.

### F097 — Animation graph, contact event, combo, root motion và IK presentation state

- **Clip:** `AKA_F097_KIET_3D_ANIMATION_PIPELINE_GAME_CODE_T01.mkv`.
- **Cảnh quay:** Basic combo/contact đúng pose; Shield Rush dịch chuyển theo root curve; aiming/foot placement; chèn graph/event code và runtime IK targets.
- **Thao tác:** Dựng split-screen animation chậm với event marker; không khẳng định mọi IK target đã biến dạng đầy đủ mọi bone trong renderer.
- **Lời thoại:** “Gameplay action tồn tại qua wind-up/contact/recovery; animation event phát damage đúng contact, root motion cấp chuyển vị, còn RuntimeIK tính target tay/chân và trạng thái trình bày.”
- **Bằng chứng:** `AppleKnightAdventure/src/Survival3D/Animation/AnimationGraph.cpp`; `AnimationEvents.cpp`; `AppleKnightAdventure/src/Survival3D/Systems/RuntimeIK.cpp`; `AppleKnightAdventure/src/Survival3D/Controller/SurvivalController.cpp:1350-1495,2225-2452`.

### F098 — GLB model, weapon socket và VFX package

- **Clip:** `AKA_F098_KIET_3D_ASSETS_VFX_GAME_CODE_T01.mkv`.
- **Cảnh quay:** Hero model với greatsword/staff gắn đúng tay; close-up projectile/skill GLB; VFX nhiều layer, trail, particles, distortion, light và camera cue.
- **Thao tác:** Quay Knight và Mage; chèn danh sách asset path/package definition.
- **Lời thoại:** “View tải arena, actor, weapon và skill GLB; socket đặt weapon theo pose, còn mỗi skill dùng package VFX khai báo nhiều layer.”
- **Bằng chứng:** `AppleKnightAdventure/src/Survival3D/View/SurvivalView.cpp`; `AppleKnightAdventure/src/Survival3D/Vfx/SkillVfxPackages.cpp`; `AppleKnightAdventure/src/Survival3D/Vfx/VfxRuntime.cpp`; `assets/survival3d/models`.

### F099 — Accessibility và performance controls

- **Clip:** `AKA_F099_KIET_ACCESSIBILITY_UI_T01.mkv`.
- **Cảnh quay:** F3 bật performance; F4 high contrast; F5 reduced motion; F6 chuyển các mức UI scale; restart để chứng minh lưu trạng thái.
- **Thao tác:** Mỗi phím có before/after rõ ràng; reduced motion nên dùng cùng một skill nhiều VFX để so sánh.
- **Lời thoại:** “Survival3D có bốn toggle/hotkey hỗ trợ kiểm tra hiệu năng, tương phản cao, giảm chuyển động và scale UI; thiết lập được lưu.”
- **Bằng chứng:** `AppleKnightAdventure/src/Survival3D/Controller/SurvivalController.cpp:599-606`; `AppleKnightAdventure/src/Model/SaveManager.cpp`; `AppleKnightAdventure/src/Survival3D/View/SurvivalView.cpp`.

### F100 — Run result, coin reward và local records

- **Clip:** `AKA_F100_KIET_3D_RUN_RESULT_UI_T01.mkv`.
- **Cảnh quay:** RunFailed hoặc RunVictory có wave/time/score/grade/reward; mở records bằng Tab; trở về menu và kiểm tra coin.
- **Thao tác:** Ưu tiên footage thắng W50; nếu chưa có, dùng RunFailed để quay UI rồi thay bằng victory khi hoàn tất full run.
- **Lời thoại:** “Kết thúc run tạo result snapshot, tính reward coin, lưu best/local record và cho phép restart, xem records hoặc về menu.”
- **Bằng chứng:** `AppleKnightAdventure/src/Survival3D/Controller/SurvivalController.cpp:652-761,1008-1035`; `AppleKnightAdventure/src/Survival3D/Systems/SurvivalRunService.cpp`; `AppleKnightAdventure/src/Survival3D/View/SurvivalView.cpp`.

## Block K6 — Nguyễn Anh Kiệt — F101–F105: hệ thống audio

### F101 — Audio manifest data-driven

- **Clip:** `AKA_F101_KIET_AUDIO_MANIFEST_CODE_T01.mkv`.
- **Cảnh quay:** Mở `audio_manifest.json`, lần lượt chỉ `samples`, `events`, `music`; chạy game để log báo số asset đã load.
- **Thao tác:** Dùng tìm kiếm tới một event 2D và một event Survival3D; không cần mở từng file WAV.
- **Lời thoại:** “SoundManager không hard-code mọi đường dẫn: manifest hiện khai báo 83 sample, 75 logical event và bốn music track.”
- **Bằng chứng:** `assets/sounds/audio_manifest.json`; `AppleKnightAdventure/src/Systems/SoundManager.cpp:67-132`.

### F102 — Random sample không lặp ngay và random pitch

- **Clip:** `AKA_F102_KIET_AUDIO_VARIATION_GAME_CODE_T01.mkv`.
- **Cảnh quay:** Lặp cùng action tám đến mười lần, waveform/tai nghe cho thấy variation; chèn event có nhiều sample và khoảng pitch.
- **Thao tác:** Chọn sword hit/footstep có đủ variation; giữ volume không đổi để so sánh công bằng.
- **Lời thoại:** “Mỗi logical event có thể chọn ngẫu nhiên trong nhiều sample, tránh lặp ngay sample trước và random pitch trong khoảng manifest.”
- **Bằng chứng:** `AppleKnightAdventure/src/Systems/SoundManager.cpp:166-218`; `assets/sounds/audio_manifest.json`.

### F103 — Sound event cooldown và layered playback

- **Clip:** `AKA_F103_KIET_AUDIO_COOLDOWN_LAYER_GAME_CODE_T01.mkv`.
- **Cảnh quay:** Một multi-hit/ultimate có nhiều nguồn tác động; âm thanh không spam vô hạn, event layered phát nhiều thành phần; chèn cấu hình `layered`/`cooldown`.
- **Thao tác:** Dùng Bastion Breaker hoặc một boss attack; hiển thị event name trong hậu kỳ.
- **Lời thoại:** “Cooldown chặn cùng event phát quá dày; layered event phát nhiều sample đồng thời để tạo âm tổng hợp.”
- **Bằng chứng:** `AppleKnightAdventure/src/Systems/SoundManager.cpp:138-218`; `assets/sounds/audio_manifest.json`.

### F104 — Spatial attenuation và stereo pan

- **Clip:** `AKA_F104_KIET_SPATIAL_AUDIO_GAME_T01.mkv`.
- **Cảnh quay:** Một enemy/skill phát âm bên trái, giữa, bên phải và ở xa/gần; meter stereo hoặc headphone capture thể hiện pan/gain.
- **Thao tác:** Giữ camera/listener ổn định, di chuyển nguồn quanh player; dùng Survival3D vì có call site `PlaySoundAt` thực tế.
- **Lời thoại:** “PlaySoundAt suy hao gain theo khoảng cách và pan theo vị trí ngang của nguồn so với listener.”
- **Bằng chứng:** `AppleKnightAdventure/src/Systems/SoundManager.cpp:154-165`; `AppleKnightAdventure/src/Survival3D/Controller/SurvivalController.cpp:2466-2639`.

### F105 — Contextual BGM

- **Clip:** `AKA_F105_KIET_CONTEXTUAL_BGM_GAME_T01.mkv`.
- **Cảnh quay:** Menu → tutorial/gameplay → boss → victory/menu; tên track hiện bằng overlay và chuyển không sai ngữ cảnh.
- **Thao tác:** Giữ 2–3 giây âm thanh mỗi trạng thái; dùng crossfade khi dựng chỉ để nối clip, không nói SoundManager có crossfade nếu code không có.
- **Lời thoại:** “Music state chọn giữa menu, tutorial, gameplay và boss; boss wave chủ động đổi sang `bgm_boss`.”
- **Bằng chứng:** `assets/sounds/audio_manifest.json`; `AppleKnightAdventure/src/Systems/SoundManager.cpp:251-314`; `AppleKnightAdventure/src/Controller/GameController.cpp`; `AppleKnightAdventure/src/Survival3D/Controller/SurvivalController.cpp:1038-1053`.

## Block K7 — Nguyễn Anh Kiệt — F106–F110: persistence, asset loading, renderer và MVC

### F106 — Save version 2, ghi tạm và backup

- **Clip:** `AKA_F106_KIET_ATOMIC_SAVE_CODE_FILE_T01.mkv`.
- **Cảnh quay:** Trong IDE chỉ schema version và luồng `.tmp`/`.bak`; trong File Explorer quay file save, thực hiện một thay đổi Options/progress rồi Save.
- **Thao tác:** Dùng bản sao save demo; không cố làm hỏng save chính của nhóm.
- **Lời thoại:** “Save schema version 2 ghi dữ liệu vào file tạm, tạo backup rồi thay file chính để giảm rủi ro mất toàn bộ tiến độ khi ghi dở.”
- **Bằng chứng:** `AppleKnightAdventure/src/Model/SaveManager.cpp:258-361`.

### F107 — Save load fallback, clamp và normalize dữ liệu

- **Clip:** `AKA_F107_KIET_SAVE_VALIDATION_CODE_T01.mkv`.
- **Cảnh quay:** Code walkthrough đường đọc primary rồi `.bak`, default và clamp; nếu demo file lỗi, chỉ dùng bản sao trong thư mục tạm/branch quay.
- **Thao tác:** Chứng minh app vẫn khởi động với fallback; phục hồi save sau take.
- **Lời thoại:** “Load không tin tuyệt đối dữ liệu JSON: nó fallback backup/default và clamp các trường như level, volume, scale và Survival record.”
- **Bằng chứng:** `AppleKnightAdventure/src/Model/SaveManager.cpp:97-257`.

### F108 — Async asset decode và main-thread GPU upload

- **Clip:** `AKA_F108_KIET_ASYNC_ASSET_LOADER_CODE_UI_T01.mkv`.
- **Cảnh quay:** Loading screen chạy mượt; chèn AssetManager worker queue và đoạn main thread chỉ upload số atlas giới hạn mỗi frame.
- **Thao tác:** Dùng sơ đồ Worker CPU decode → Ready Queue → Main GPU upload; không nói GPU API chạy ở worker.
- **Lời thoại:** “Worker xử lý phần CPU/image decode; GPU texture upload vẫn được đưa về main thread và giới hạn theo frame.”
- **Bằng chứng:** `AppleKnightAdventure/src/View/AssetManager.cpp:70-190`; `AppleKnightAdventure/include/View/AssetManager.h`; `AppleKnightAdventure/src/main.cpp:61-260`.

### F109 — Layered renderer, stable z-sort và MPSC queue

- **Clip:** `AKA_F109_KIET_RENDERER_ARCH_GAME_CODE_T01.mkv`.
- **Cảnh quay:** Gameplay có background/tile/entity/foreground/UI chồng đúng; chèn enum layer, command buffer, stable sort và off-main submission queue.
- **Thao tác:** Pause ở frame có player đi sau foreground nhưng HUD luôn trên cùng.
- **Lời thoại:** “Renderer gom lệnh theo Background, World, Foreground, UI, sắp xếp z ổn định và nhận submission qua MPSC queue.”
- **Bằng chứng:** `AppleKnightAdventure/include/View/RenderTypes.h`; `AppleKnightAdventure/include/View/Renderer.h`; `AppleKnightAdventure/src/View/Renderer.cpp:60-380`.

### F110 — Kiến trúc MVC-like

- **Clip:** `AKA_F110_KIET_MVC_ARCH_CODE_T01.mkv`.
- **Cảnh quay:** IDE tree ba thư mục Model/View/Controller; theo một flow InputController → GameController → Player/GameState → GameView/HUD.
- **Thao tác:** Vẽ sơ đồ một chiều cho cả 2D và Survival3D; không tuyên bố strict MVC tuyệt đối.
- **Lời thoại:** “Dự án tách dữ liệu/hành vi domain, trình bày và điều phối theo MVC-like; main.cpp đóng vai trò orchestration giữa các screen.”
- **Bằng chứng:** `AppleKnightAdventure/include/Model`; `include/View`; `include/Controller`; `src/main.cpp`; `AppleKnightAdventure/include/Survival3D`.

## Block K8 — Nguyễn Anh Kiệt — F111–F115: OOD, Factory, Adapter và Command/Composite

### F111 — Inheritance, composition và runtime polymorphism

- **Clip:** `AKA_F111_KIET_OOD_CODE_T01.mkv`.
- **Cảnh quay:** Sơ đồ `Entity → Character → Player/Enemy/Boss/Pet`, `Boss → Boss1/2/3`; mở Player composition với `unique_ptr<CharacterSkillSet>`, active buffs và Core loadout.
- **Thao tác:** Chỉ một call qua pointer/interface và implementation được dispatch theo class; nêu cả ưu/nhược điểm.
- **Lời thoại:** “OOD kết hợp kế thừa cho is-a, composition cho has-a và virtual interface CharacterSkillSet cho hành vi class-specific; GameController vẫn lớn nên không khẳng định SOLID tuyệt đối.”
- **Bằng chứng:** `AppleKnightAdventure/include/Model/Entity.h`; `Character.h`; `Player.h`; `Boss.h`; `CharacterSkillSet.h`.

### F112 — Singleton cho shared service/controller/view

- **Clip:** `AKA_F112_KIET_SINGLETON_CODE_T01.mkv`.
- **Cảnh quay:** Mở `GetInstance()` của SaveManager, SoundManager, GameController; chỉ deleted copy constructor ở service tài nguyên.
- **Thao tác:** Theo một call thực tế từ gameplay tới SoundManager/SaveManager.
- **Lời thoại:** “Các service dùng chung có một instance truy cập tập trung; những lớp quản lý tài nguyên quan trọng cấm copy để tránh nhân đôi ownership.”
- **Bằng chứng:** `AppleKnightAdventure/include/Model/SaveManager.h`; `AppleKnightAdventure/include/Systems/SoundManager.h`; `AppleKnightAdventure/include/Controller/GameController.h` và các file `.cpp` tương ứng.

### F113 — Factory cho Enemy, Item và Level

- **Clip:** `AKA_F113_KIET_FACTORY_PATTERN_CODE_GAME_T01.mkv`.
- **Cảnh quay:** Level load tạo nhiều enemy/item khác type; chèn `EnemyFactory`, `ItemFactory`, `LevelFactory` và call site.
- **Thao tác:** Theo dữ liệu type từ file level đến concrete object; tránh chỉ đọc định nghĩa class.
- **Lời thoại:** “Factory gom logic khởi tạo concrete enemy, item và level, giúp loader/controller không tự gọi constructor cho mọi subtype.”
- **Bằng chứng:** `AppleKnightAdventure/src/Factories/EnemyFactory.cpp`; `ItemFactory.cpp`; `LevelFactory.cpp`; `AppleKnightAdventure/src/Controller/GameController.cpp`.

### F114 — Adapter cho `.lvl` và `.ldtk`

- **Clip:** `AKA_F114_KIET_LEVEL_ADAPTER_CODE_GAME_T01.mkv`.
- **Cảnh quay:** Mở một legacy `.lvl` và một `.ldtk`; cả hai được đưa về `GameState`; chèn selector adapter.
- **Thao tác:** Dùng MapBuilder import hoặc start level phù hợp để chứng minh runtime, không chỉ extension text.
- **Lời thoại:** “LegacyLevelAdapter và LDtkLevelAdapter cùng implement ILevelSourceAdapter, đưa hai nguồn file về model GameState thống nhất.”
- **Bằng chứng:** `AppleKnightAdventure/include/Factories/LevelSourceAdapter.h`; `AppleKnightAdventure/src/Factories/LevelSourceAdapter.cpp`; `AppleKnightAdventure/src/Factories/LevelFactory.cpp:102-154,493+`.

### F115 — Command và Composite pattern cho edit transaction

- **Clip:** `AKA_F115_KIET_COMMAND_PATTERN_CODE_T01.mkv`.
- **Cảnh quay:** Code `Command`, `Execute`, `Undo`, `CompositeCommand`, `CommandManager`; minh họa một thao tác batch được undo một lần.
- **Thao tác:** Đây là walkthrough pattern; hành vi copy/paste chi tiết quay lại ở F125.
- **Lời thoại:** “Mỗi edit là command có execute/undo; Composite gom nhiều command thành một transaction để lịch sử không bị vỡ thành hàng chục bước.”
- **Bằng chứng:** `AppleKnightAdventure/include/Model/Command.h`; `AppleKnightAdventure/src/Model/Command.cpp`; `AppleKnightAdventure/src/Controller/MapBuilderController.cpp:40-94`.

## Block K9 — Nguyễn Anh Kiệt — F116–F120: Pool, Quadtree, animation event, service và backend

### F116 — Object Pool pattern

- **Clip:** `AKA_F116_KIET_OBJECT_POOL_CODE_GAME_T01.mkv`.
- **Cảnh quay:** Particle burst dày trong 2D và enemy/projectile pool trong 3D; chèn acquire/release/reuse code.
- **Thao tác:** Bật performance overlay; nói rõ có generic pool cho Particle và pool chuyên dụng trong Survival3D.
- **Lời thoại:** “Object Pool tái sử dụng object sống/chết, giảm cấp phát liên tục ở particle và các đối tượng spawn dày của Survival3D.”
- **Bằng chứng:** `AppleKnightAdventure/include/Systems/ObjectPool.h`; `AppleKnightAdventure/include/Systems/ParticleSystem.h`; `AppleKnightAdventure/src/Systems/ParticleSystem.cpp`; `AppleKnightAdventure/include/Survival3D/Controller/SurvivalController.h`.

### F117 — Quadtree broad-phase collision

- **Clip:** `AKA_F117_KIET_QUADTREE_CODE_GAME_T01.mkv`.
- **Cảnh quay:** Combat với nhiều enemy; chèn quadtree subdivision/query và `RebuildSpatialIndex`/`QueryEntitiesInRect`.
- **Thao tác:** Vẽ overlay vùng query quanh hitbox; ghi rõ overlay hậu kỳ.
- **Lời thoại:** “Quadtree giảm tập ứng viên va chạm: combat truy vấn vùng hitbox thay vì so sánh với mọi entity trong level.”
- **Bằng chứng:** `AppleKnightAdventure/include/Systems/Quadtree.h`; `AppleKnightAdventure/src/Systems/Quadtree.cpp`; `AppleKnightAdventure/src/Systems/CollisionSystem.cpp`; `AppleKnightAdventure/src/Controller/GameController.cpp:3067-3090`.

### F118 — Animation-event callback tại contact frame

- **Clip:** `AKA_F118_KIET_ANIMATION_EVENT_CALLBACK_CODE_GAME_T01.mkv`.
- **Cảnh quay:** Skill 3D chậm với event marker; damage/VFX/SFX chỉ xảy ra ở contact, không ở lúc bấm; chèn callback controller.
- **Thao tác:** Dùng Violet Edge hoặc Shield Rush; hiển thị monotonic serial chống consume event hai lần.
- **Lời thoại:** “Animation system phát callback/event ở frame tác giả định nghĩa; controller resolve gameplay contact và view/audio consume presentation event một lần.”
- **Bằng chứng:** `AppleKnightAdventure/src/Survival3D/Animation/AnimationEvents.cpp`; `AppleKnightAdventure/src/Survival3D/Controller/SurvivalController.cpp:1350-1495,2225-2650`.

### F119 — Offline queue, background worker và exponential retry

- **Clip:** `AKA_F119_KIET_OFFLINE_QUEUE_CODE_T01.mkv`.
- **Cảnh quay:** Code/service log khi remote service tắt; run vẫn được finalize/local-persist, request nằm trong queue; chỉ worker/retry/backoff.
- **Thao tác:** Không cần bật Internet. Nếu quay retry, dùng localhost service không chạy và cắt bớt thời gian chờ.
- **Lời thoại:** “SurvivalRunService tách gửi mạng khỏi game loop, giữ offline queue và tăng khoảng retry; gameplay và local record vẫn hoạt động khi dịch vụ tắt.”
- **Bằng chứng:** `AppleKnightAdventure/src/Survival3D/Systems/SurvivalRunService.cpp:112-355,392-461`; `assets/survival3d/config/services.json`.

### F120 — Backend validation, idempotency và leaderboard từ run hợp lệ

- **Clip:** `AKA_F120_KIET_BACKEND_INTEGRITY_CODE_TERMINAL_T01.mkv`.
- **Cảnh quay:** Code/server terminal localhost: gửi run hợp lệ; gửi lại cùng idempotency key; thử payload wave ngoài 1–50 hoặc victory trước W50 và ghi response từ chối.
- **Thao tác:** Chỉ dùng dữ liệu test; không đưa secret. Nêu rõ server bind loopback và HTTP.
- **Lời thoại:** “Backend kiểm tra range, boss/wave consistency, victory W50, tính lại score và dùng idempotency chống ghi trùng. Đây là integrity localhost, không phải production authentication/security.”
- **Bằng chứng:** `AppleKnightAdventure/backend/survival3d/SurvivalServerCore.cpp:178-313`; `AppleKnightAdventure/backend/survival3d/main.cpp:52-129,276`; `assets/survival3d/config/services.json`.

## Block K10 — Nguyễn Anh Kiệt — F121–F125: Shop, custom level và MapBuilder tools

### F121 — Shop: khóa, mua, mở khóa và equip nhân vật/pet

- **Clip:** `AKA_F121_KIET_SHOP_PURCHASE_EQUIP_UI_GAME_T01.mkv`.
- **Cảnh quay:** Profile sạch: Knight mở sẵn, các item có giá còn khóa; thử mua khi thiếu coin; sau đó đủ coin, mua một character hoặc pet, thấy coin bị trừ và `PURCHASED & EQUIPPED`; đóng/mở Shop hoặc restart để chứng minh unlock/equip được lưu.
- **Thao tác:** Ghi rõ coin trước/sau và chọn đúng một item chưa sở hữu; sau khi mua bấm lại để chứng minh nhánh equip không trừ coin lần hai.
- **Lời thoại:** “Shop nối currency campaign với customization lâu dài: kiểm tra khóa và số dư, trừ đúng giá, ghi unlock, equip, phát feedback/achievement và lưu profile.”
- **Bằng chứng:** `AppleKnightAdventure/src/Controller/ShopController.cpp:121-249`; `AppleKnightAdventure/src/Model/SaveManager.cpp:94-169,269-271,370-389`; `AppleKnightAdventure/src/View/ShopView.cpp`.

### F122 — Custom-level browser và adapter loading

- **Clip:** `AKA_F122_KIET_CUSTOM_MAP_BROWSER_UI_GAME_T01.mkv`.
- **Cảnh quay:** Tạo/lưu ít nhất hai custom `.lvl`; mở `PLAY CUSTOM MAP`, thấy danh sách alphabet, chọn một map, Play và vào gameplay; quay dialog Delete/Cancel nhưng không xóa file quan trọng.
- **Thao tác:** Browser bỏ qua `temp_playtest`, alias `custom_map` và campaign `levelN`; map chọn được copy an toàn sang alias rồi LevelFactory load qua adapter.
- **Lời thoại:** “Custom browser liệt kê named `.lvl`, cho play/delete có xác nhận; selected map đi qua LevelFactory/Legacy adapter. LDtk được import trong Builder ở F126, không được browser này liệt kê trực tiếp.”
- **Bằng chứng:** `AppleKnightAdventure/src/Controller/MenuController.cpp:258-375`; `AppleKnightAdventure/src/View/MenuView.cpp`; `AppleKnightAdventure/src/Factories/LevelFactory.cpp:102-154`.

### F123 — MapBuilder: Brush, Eraser, Bucket Fill, Select và Box Select

- **Clip:** `AKA_F123_KIET_MAPBUILDER_TOOLS_UI_T01.mkv`.
- **Cảnh quay:** Trên map test, dùng Brush vẽ hình, Eraser xóa, Bucket Fill tô vùng liên thông, Select chọn một entity, Box Select khoanh vùng.
- **Thao tác:** Mỗi tool có slate 2 giây và before/after; không dùng cùng màu tile cho tất cả vì khó thấy kết quả.
- **Lời thoại:** “Editor có sáu tool mode gồm Brush, Eraser, BucketFill, Select, BoxSelect và MoveCamera; feature này minh họa năm tool chỉnh sửa chính.”
- **Bằng chứng:** `AppleKnightAdventure/include/View/MapBuilderView.h:15-20`; `AppleKnightAdventure/src/Controller/MapBuilderController.cpp:334-567`; `AppleKnightAdventure/src/View/MapBuilderView.cpp:280-282`.

### F124 — MapBuilder: ba layer, multi-tileset, entity palette và properties

- **Clip:** `AKA_F124_KIET_MAPBUILDER_PALETTES_UI_T01.mkv`.
- **Cảnh quay:** Vẽ Background/Main/Foreground rồi thay visibility/lock; chuyển qua sáu tileset; đặt player/enemy/boss/item/chest/checkpoint/portal; chọn entity để mở Properties panel.
- **Thao tác:** Chứng minh Main có solid behavior trong playtest; không cố đặt loại entity không có trong palette.
- **Lời thoại:** “MapBuilder tách ba layer, hỗ trợ nhiều tileset, gameplay entity palette và property panel; layer còn có visible/locked state.”
- **Bằng chứng:** `AppleKnightAdventure/include/View/MapBuilderView.h:55-120`; `AppleKnightAdventure/src/View/MapBuilderView.cpp:323-711`; `AppleKnightAdventure/src/Controller/MapBuilderController.cpp:434-521`.

### F125 — Copy/paste vùng, CompositeCommand, undo và redo

- **Clip:** `AKA_F125_KIET_MAPBUILDER_COPY_UNDO_UI_T01.mkv`.
- **Cảnh quay:** Box-select vùng có nhiều tile/entity, Ctrl+C, Ctrl+V tại offset mới; nhấn Ctrl+Z một lần để toàn bộ paste biến mất; Ctrl+Y để khôi phục.
- **Thao tác:** Hiển thị history action name nếu UI có; giữ selection rectangle và offset rõ ràng.
- **Lời thoại:** “Copy lưu tọa độ tương đối; paste tạo CompositeCommand nên một Undo hoàn tác toàn batch và Redo dựng lại đúng vùng.”
- **Bằng chứng:** `AppleKnightAdventure/src/Controller/MapBuilderController.cpp:40-94,334-410`; `AppleKnightAdventure/include/Model/Command.h`; `AppleKnightAdventure/src/Model/Command.cpp`.

## Block K11 — Nguyễn Anh Kiệt — F126–F130: editor I/O/playtest, UI, build và configuration

### F126 — Save/load, filename sanitization và LDtk import

- **Clip:** `AKA_F126_KIET_MAPBUILDER_SAVE_IMPORT_UI_FILE_T01.mkv`.
- **Cảnh quay:** Nhập tên có ký tự/path không an toàn và lưu; quan sát safe `.lvl`; Open File chọn `.lvl`; lần khác chọn `.ldtk`, status báo import và sau đó lưu thành editable `.lvl`.
- **Thao tác:** Chỉ dùng file test; không ghi đè campaign. Quay code chặn tên `levelN`, `temp_playtest` và đường dẫn ngoài `assets/levels`.
- **Lời thoại:** “Builder lọc filename, tránh tên campaign/temp, lưu named `.lvl`, duy trì alias playable và dùng adapter để import `.ldtk` thành dữ liệu editable.”
- **Bằng chứng:** `AppleKnightAdventure/src/Controller/MapBuilderController.cpp:95-176,191-276`; `AppleKnightAdventure/src/Platform/FileDialog.cpp`; `AppleKnightAdventure/src/Factories/LevelSourceAdapter.cpp`.

### F127 — Temporary playtest và quay lại editor

- **Clip:** `AKA_F127_KIET_MAPBUILDER_PLAYTEST_GAME_T01.mkv`.
- **Cảnh quay:** Trong Builder nhấn Playtest; file `temp_playtest.lvl` được tạo; gameplay thật khởi động với HUD playtest; thoát và trở về đúng editor/map.
- **Thao tác:** Đặt một platform, spawn, enemy/item trước playtest để chứng minh nội dung vừa chỉnh được load.
- **Lời thoại:** “Playtest serialize map tạm, gọi game bằng level id riêng và main loop nhớ nguồn playtest để trả người dùng về editor thay vì menu.”
- **Bằng chứng:** `AppleKnightAdventure/src/Controller/MapBuilderController.cpp:177-190`; `AppleKnightAdventure/src/main.cpp:409-430`; `AppleKnightAdventure/include/View/HUDView.h`.

### F128 — UI state và modal layer cho menu, pause, HUD và result

- **Clip:** `AKA_F128_KIET_UI_STATES_MODAL_GAME_T01.mkv`.
- **Cảnh quay:** Montage Main, Shop, Level Select, Custom Maps, Leaderboard, Achievements, Options; trong level mở/đóng Pause; hoàn thành level để Result nằm trên gameplay.
- **Thao tác:** Quay một lần thử input gameplay khi Pause mở, sau đó resume; giữ transition/back action của từng màn.
- **Lời thoại:** “MenuController, MenuView và UIStateManager điều phối screen và layer modal; Pause/Result chặn luồng gameplay phù hợp trong khi HUD và Skill Bar được quản lý theo layer riêng.”
- **Bằng chứng:** `AppleKnightAdventure/include/View/UIStateManager.h`; `AppleKnightAdventure/src/View/UIStateManager.cpp`; `AppleKnightAdventure/src/View/MenuView.cpp`; `AppleKnightAdventure/src/Controller/GameController.cpp:2070-2185`.

### F129 — C++17 build, asset sync và optional server target

- **Clip:** `AKA_F129_KIET_BUILD_SYSTEM_TERMINAL_CODE_T01.mkv`.
- **Cảnh quay:** Mở CMakeLists, chỉ C++17, nlohmann/raylib/link libraries, SyncAssets target và optional AegisRiftServer; chạy build incremental rồi kiểm tra assets có trong build directory.
- **Thao tác:** Dùng command build chuẩn của repo; không xóa build/cache đang dùng. Server target chỉ build/launch localhost nếu môi trường cho phép.
- **Lời thoại:** “CMake dùng C++17, đồng bộ assets kể cả khi chỉ dữ liệu thay đổi và có target server tùy chọn; client không phụ thuộc việc server phải chạy.”
- **Bằng chứng:** `AppleKnightAdventure/CMakeLists.txt:5-88`.

### F130 — Data-driven config, fallback và diagnostic boundaries

- **Clip:** `AKA_F130_KIET_CONFIG_FALLBACK_CODE_LOG_T01.mkv`.
- **Cảnh quay:** So sánh `audio_manifest.json`, `waves.json`, `balance.json`, `victory_grades.json`, LDtk; chèn các `try/catch`, default/fallback và `TraceLog` warning khi file thiếu/sai trong bản sao test.
- **Thao tác:** Không phá assets bản chính; nếu cần demo lỗi, chạy từ bản copy hoặc đổi đường dẫn test có thể phục hồi. Chỉ ra boundary: JSON parse → validate/clamp → fallback/log.
- **Lời thoại:** “Nội dung được data-drive qua JSON/LDtk nhưng code vẫn đặt biên an toàn: schema/range check, default, fallback và diagnostic log khi asset/config không hợp lệ.”
- **Bằng chứng:** `AppleKnightAdventure/src/Systems/SoundManager.cpp:67-132`; `AppleKnightAdventure/src/Survival3D/Controller/SurvivalController.cpp:338-434`; `AppleKnightAdventure/src/Systems/AchievementManager.cpp:15-31`; `AppleKnightAdventure/src/Factories/LevelFactory.cpp:102-118`; các file trong `assets/config` và `assets/survival3d/config`.

---

# PHẦN B — SÁU LEVEL 2D, KHÔNG TÍNH VÀO FEATURE

## Quy tắc quay chung cho L2D

- Người duy nhất phụ trách gameplay và lời dẫn sáu mục này là **Nguyễn Trọng Tiến**.
- Mỗi level phải có một file raw liên tục từ Level Select/Prepare tới Result. Bản dựng có thể dùng montage, nhưng phải lưu raw để đối chiếu.
- Không khẳng định cúp tự kiểm tra đã diệt toàn bộ enemy. Code completion cho phép tương tác Cup/End checkpoint; việc đường map buộc chiến đấu là thiết kế bố cục, không phải predicate tổng quát enemy-clear.
- Mỗi Result phải đọc được level number, score, clear time, enemy/item ratio và stars.
- Nếu chết/respawn trong raw run vẫn giữ footage; điều này chứng minh checkpoint, không làm mất tính hợp lệ của level.

## L2D-01 — Level 1: Tutorial

- **Người phụ trách:** Nguyễn Trọng Tiến.
- **File raw:** `AKA_L2D_01_TIEN_FULL_T01.mkv`.
- **Dữ liệu:** `assets/levels/tutorial.ldtk`, level identifier `Tutorial`, kích thước 7360×320 px; 4 melee, 22 InMapGuide, 6 Signboard, 10 Portal, 1 checkpoint statue và 1 LevelCompleteCup.
- **Shot checklist riêng:**
  - [ ] Slate `[L2D-01 | LEVEL — KHÔNG TÍNH FEATURE]` và card Level 1 ở Level Select.
  - [ ] Prepare screen, class/pet đã chọn và cảnh spawn đầu tutorial.
  - [ ] Ít nhất hai InMapGuide cùng hai Signboard có nội dung điều khiển khác nhau.
  - [ ] Thực hiện movement, jump, attack, interact theo hướng dẫn thay vì chạy lướt bỏ tutorial.
  - [ ] Quay ít nhất một cặp trong 10 Portal, gồm lúc vào và vị trí ra.
  - [ ] Quay encounter với bốn melee; ít nhất một enemy phải thể hiện patrol → chase → telegraph → death.
  - [ ] Kích hoạt checkpoint statue và giữ animation captured.
  - [ ] Tương tác LevelCompleteCup; giữ particle/sound/chuyển trạng thái.
  - [ ] Result đọc được score/time/stars; thử Continue hoặc Level Select.
- **Lời dẫn gợi ý:** “Level 1 là tutorial ngang dài, đưa người chơi qua guide, signboard, portal, combat cơ bản, checkpoint rồi cúp hoàn thành. Đây là level riêng, không được tính thành feature.”
- **Bằng chứng:** `assets/levels/tutorial.ldtk`; `AppleKnightAdventure/src/Controller/GameController.cpp:184-203,479-495`.

## L2D-02 — Level 2: Forest

- **Người phụ trách:** Nguyễn Trọng Tiến.
- **File raw:** `AKA_L2D_02_TIEN_FULL_T01.mkv`.
- **Dữ liệu:** `assets/levels/lvl2.ldtk`, runtime dùng index 0 `Forest`, 3280×3472 px; 53 enemy thường + Boss1; 6 chest, 18 coin entity, 17 potion, 6 checkpoint, 2 portal, 1 FakeWall và 1 Cup.
- **Shot checklist riêng:**
  - [ ] Slate L2D-02 và card Level 2 đã được unlock sau Level 1.
  - [ ] Spawn và wide shot cho cấu trúc Forest nhiều tầng theo chiều dọc.
  - [ ] Quay đủ ba archetype thường: melee, ranged và flying.
  - [ ] Mở một Chest, quan sát coin scatter, nhặt coin và Potion.
  - [ ] Tìm và phá FakeWall tại khu vực phù hợp; đi qua phần tường đã phá.
  - [ ] Dùng cặp portal ColorId 1; quay đầy đủ đầu vào/đầu ra.
  - [ ] Kích hoạt tối thiểu hai trong sáu checkpoint, trong đó có một lần respawn nếu thuận tiện.
  - [ ] Quay Boss1: boss title/HP, phase 1, chuyển phase 2 và ít nhất dash/combo đặc trưng.
  - [ ] Đi tới Cup và tương tác; không nói code bắt buộc phải diệt toàn bộ 54 mục tiêu để Cup hoạt động.
  - [ ] Result có score/time/stars và enemy/item ratios.
- **Lời dẫn gợi ý:** “Level 2 mở rộng tutorial thành Forest dọc, kết hợp đủ ba loại enemy, loot, FakeWall, portal, nhiều checkpoint và Boss1 hai phase.”
- **Bằng chứng:** `assets/levels/lvl2.ldtk`; `AppleKnightAdventure/src/Factories/LevelFactory.cpp:493+`; `AppleKnightAdventure/src/Model/Boss1.cpp`.

## L2D-03 — Level 3: Forest mở rộng

- **Người phụ trách:** Nguyễn Trọng Tiến.
- **File raw:** `AKA_L2D_03_TIEN_FULL_T01.mkv`.
- **Dữ liệu:** `assets/levels/lvl3.ldtk`, runtime dùng index 0 `Forest`, 4128×1488 px; 172 enemy thường + Boss1; 12 chest, 17 coin, 30 potion, 12 checkpoint, 6 portal, 1 FakeWall và 1 Cup.
- **Shot checklist riêng:**
  - [ ] Slate L2D-03 và card Level 3.
  - [ ] Quay spawn cùng một pan ngang thể hiện map rộng 258 ô.
  - [ ] Encounter có melee/ranged/flying đồng thời để chứng minh mật độ cao hơn Level 2.
  - [ ] Mở Chest, nhặt Coin/Potion và quay HUD tăng.
  - [ ] Phá FakeWall; ghi before/after collision.
  - [ ] Quay đủ ba màu/cặp portal ở dạng montage: Color 1, 2 và 3.
  - [ ] Kích hoạt checkpoint ở ít nhất đầu, giữa và gần cuối map.
  - [ ] Quay Boss1 phase transition và một lần player dùng core/boon build chống boss.
  - [ ] Tương tác Cup và chuyển Result.
  - [ ] Result đọc được score/time/stars; so sánh performance với Level 2.
- **Lời dẫn gợi ý:** “Level 3 nhấn mạnh quy mô ngang và mật độ encounter: 173 mục tiêu tính điểm, ba cặp portal, nhiều loot/checkpoint và Boss1.”
- **Bằng chứng:** `assets/levels/lvl3.ldtk`; `AppleKnightAdventure/src/Controller/GameController.cpp:184-203`; `AppleKnightAdventure/src/Factories/LevelFactory.cpp`.

## L2D-04 — Level 4: Flying gauntlet và Boss2

- **Người phụ trách:** Nguyễn Trọng Tiến.
- **File raw:** `AKA_L2D_04_TIEN_FULL_T01.mkv`.
- **Dữ liệu:** `assets/levels/lvl4.ldtk`, index 0 `Forest`, 1456×1376 px; 234 flying + Boss2; 48 potion, 12 checkpoint, 2 portal, cụm 16 FakeWall và 1 Cup; không có chest/coin entity authored.
- **Shot checklist riêng:**
  - [ ] Slate L2D-04 và card Level 4.
  - [ ] Spawn/wide shot cho bố cục 91×86 ô.
  - [ ] Quay một màn hình có nhiều flying enemy cùng lúc và AI di chuyển cả X/Y.
  - [ ] Nhặt Potion trong lúc xử lý gauntlet; nói rõ map không có Chest/Coin entity authored.
  - [ ] Phá hoặc mở đường qua cụm FakeWall 2×8; giữ số lượng/cấu trúc bằng overview.
  - [ ] Dùng cặp portal Color 1.
  - [ ] Kích hoạt checkpoint giữa gauntlet và quay một lần hồi sinh nếu có.
  - [ ] Quay Boss2 đủ ba phase; bắt ít nhất spread projectile, telegraphed AoE, teleport/kite hoặc self-heal.
  - [ ] Tương tác Cup và chuyển Result.
  - [ ] Result đọc được score/time/stars.
- **Lời dẫn gợi ý:** “Level 4 có bản sắc flying gauntlet rất rõ: 234 flying enemy, cụm FakeWall lớn, nhiều Potion và Boss2 ba phase thiên ranged/kiting.”
- **Bằng chứng:** `assets/levels/lvl4.ldtk`; `AppleKnightAdventure/src/Model/Boss2.cpp`; `AppleKnightAdventure/src/Model/FakeWall.cpp`.

## L2D-05 — Level 5: Forest quy mô lớn và Boss2

- **Người phụ trách:** Nguyễn Trọng Tiến.
- **File raw:** `AKA_L2D_05_TIEN_FULL_T01.mkv`.
- **Dữ liệu:** `assets/levels/lvl5.ldtk`, index 0 `Forest`, 2720×2000 px; 298 enemy thường + Boss2; 12 chest, 31 potion, 17 checkpoint, 4 portal, 12 FakeWall và 1 Cup.
- **Shot checklist riêng:**
  - [ ] Slate L2D-05 và card Level 5.
  - [ ] Spawn và minimap/wide shot thể hiện map 170×125 ô.
  - [ ] Encounter hỗn hợp melee/ranged/flying; giữ một đoạn crowd combat dùng AoE/core/reaction.
  - [ ] Mở Chest và nhặt coin văng ra; ghi rõ file không đặt coin entity nhưng Chest vẫn có thể sinh coin.
  - [ ] Quay cụm FakeWall 3×4 trước/sau khi phá.
  - [ ] Quay hai cặp portal Color 1/2.
  - [ ] Kích hoạt checkpoint ở ba vùng xa nhau để chứng minh đường dài.
  - [ ] Quay Boss2 phase/special; tránh dùng nguyên footage Level 4 mà không có slate Level 5.
  - [ ] Tương tác Cup và chuyển Result.
  - [ ] Result đọc được score/time/stars và par-time context.
- **Lời dẫn gợi ý:** “Level 5 tăng mạnh quy mô và số encounter, bổ sung 12 Chest, hai cặp portal, cụm FakeWall và một trận Boss2 mới trong bố cục khác.”
- **Bằng chứng:** `assets/levels/lvl5.ldtk`; `AppleKnightAdventure/src/Model/Boss2.cpp`; `AppleKnightAdventure/src/Controller/GameController.cpp`.

## L2D-06 — Level 6: ColdCorridor và chuỗi ba boss

- **Người phụ trách:** Nguyễn Trọng Tiến.
- **File raw:** `AKA_L2D_06_TIEN_FULL_T01.mkv`.
- **Dữ liệu:** `assets/levels/lvl6.ldtk`, identifier `Level_0`, theme `ColdCorridor`, 2720×2000 px; 84 enemy thường + Boss1/Boss2/Boss3; 16 chest, 91 coin entity tổng Amount 240, 18 potion, 7 checkpoint, 4 portal, 1 Cup.
- **Shot checklist riêng:**
  - [ ] Slate L2D-06 và card Level 6.
  - [ ] Spawn, ColdCorridor parallax và minimap/wide shot.
  - [ ] Encounter có melee/ranged/flying; nhặt một chuỗi coin để thấy bố cục loot dày.
  - [ ] Mở Chest, quan sát coin scatter và nhặt Potion.
  - [ ] Quay hai cặp portal Color 1/2 và ít nhất ba checkpoint dọc hành trình.
  - [ ] Boss1 tại vùng đầu: đủ hai phase và một dash/combo.
  - [ ] Boss2 tại vùng giữa: đủ ba phase và một ranged/AoE/teleport behavior.
  - [ ] Boss3 tại vùng cuối: đủ bốn phase; quay melee, energy sphere và ít nhất ground AoE hoặc beam bị giới hạn bởi tường.
  - [ ] Tương tác Cup sau chuỗi nội dung và giữ transition.
  - [ ] Result đọc được score/time/stars; overlay `3 bosses = 9 boss phases`.
- **Lời dẫn gợi ý:** “Level 6 là màn tổng kết ColdCorridor, có 87 mục tiêu tính điểm và chuỗi Boss1 hai phase, Boss2 ba phase, Boss3 bốn phase. Đây vẫn chỉ là một level, không phải chín feature.”
- **Bằng chứng:** `assets/levels/lvl6.ldtk`; `AppleKnightAdventure/src/Model/Boss1.cpp`; `Boss2.cpp`; `Boss3.cpp`.

---

# PHẦN C — 50 WAVE SURVIVAL3D, KHÔNG TÍNH VÀO FEATURE

## Quy tắc quay chung cho W3D

- Người duy nhất phụ trách 50 wave là **Nguyễn Anh Kiệt**.
- Bắt buộc giữ một raw file liên tục từ Character Select đến RunVictory W50. Có thể đồng thời tách marker/clip theo từng wave để dựng.
- Tỷ lệ `Swarm/Ranger/Tanker` là trọng số mix, **không phải số lượng enemy tuyệt đối**. Số spawn wave thường đến từ threat budget; Brute tốn budget cao hơn Riftling.
- Mỗi wave thường phải có: wave banner, ít nhất một spawn/encounter, HUD wave number, khoảnh khắc quái cuối chết, `WAVE CLEAR` và upgrade transition.
- Mỗi boss wave phải có: boss name/HP, ít nhất một normal attack, special, phase transition nếu có, death, boss reward và next-wave transition.
- Nếu mix tỷ lệ thấp không sinh đủ archetype trong một seed cụ thể, không dựng giả; giữ gameplay thật và chèn inset dòng cấu hình từ `waves.json`.
- Các con số budget/cap/interval dưới đây được tính từ `balance.json`. Budget là threat budget; boss wave spawn boss trực tiếp nên không dùng budget thường để suy ra số quái.

## C1. W3D-01 đến W3D-10

### W3D-01 — Khởi đầu thuần Swarm

- **File:** `AKA_W3D_01_KIET_T01.mkv`.
- **Cấu hình:** Swarm/Ranger/Tanker `100/0/0`; threat budget khoảng 6, active cap 10, spawn interval khoảng 1,10 giây.
- **Shot checklist riêng:**
  - [ ] Banner `WAVE 1` và HUD `1/50` đọc được.
  - [ ] Riftling đầu tiên spawn; quay movement và một basic combo không cắt.
  - [ ] Quái cuối chết và counter về 0.
  - [ ] `WAVE CLEAR` cùng draft đầu tiên/transition W02.
- **Lời dẫn:** “Wave 1 dùng toàn bộ trọng số Swarm để giới thiệu nhịp arena và basic combat; budget 6 là threat budget, không phải lời hứa đúng sáu model trên sân.”
- **Bằng chứng:** `assets/survival3d/config/waves.json` entry wave 1; `assets/survival3d/config/balance.json`; `SurvivalController.cpp:1015-1061`.

### W3D-02 — Swarm thuần, nhịp spawn nhanh hơn nhẹ

- **File:** `AKA_W3D_02_KIET_T01.mkv`.
- **Cấu hình:** `100/0/0`; budget khoảng 6, cap 10, interval khoảng 1,07 giây.
- **Shot checklist riêng:**
  - [ ] Banner W02 và build/upgrade vừa chọn từ W01.
  - [ ] Ít nhất hai Riftling tiếp cận từ hai hướng; dùng jump/dash né một hit.
  - [ ] Giữ HP/wave counter trong suốt đoạn combat đại diện.
  - [ ] Quái cuối, Wave Clear và transition W03.
- **Lời dẫn:** “Wave 2 giữ mix thuần Swarm nhưng giảm interval; đây là bước kiểm tra upgrade đầu tiên trước khi Ranger xuất hiện.”
- **Bằng chứng:** `waves.json` wave 2; `balance.json`; `SurvivalController::BeginWave`.

### W3D-03 — Ranger xuất hiện lần đầu

- **File:** `AKA_W3D_03_KIET_T01.mkv`.
- **Cấu hình:** `85/15/0`; budget khoảng 7, cap 11, interval khoảng 1,04 giây.
- **Shot checklist riêng:**
  - [ ] Banner W03.
  - [ ] Quay Riftling và Hex Archer trong cùng encounter hoặc inset config nếu seed không sinh Archer.
  - [ ] Né/guard một ranged projectile; thử target lock vào Archer.
  - [ ] Wave Clear và upgrade transition.
- **Lời dẫn:** “Wave 3 thêm 15% Ranger, buộc người chơi ưu tiên mục tiêu tầm xa thay vì chỉ xử lý Swarm trước mặt.”
- **Bằng chứng:** `waves.json` wave 3; `SurvivalController.cpp:1215-1325,1550+`.

### W3D-04 — Tăng tỷ trọng Ranger

- **File:** `AKA_W3D_04_KIET_T01.mkv`.
- **Cấu hình:** `80/20/0`; budget khoảng 8, cap 11, interval khoảng 1,01 giây.
- **Shot checklist riêng:**
  - [ ] Banner W04 và wave counter.
  - [ ] Hex Archer telegraph/bắn khi Riftling gây áp lực gần.
  - [ ] Dùng pillar/camera orbit để phá line-of-sight ranged.
  - [ ] Quái cuối, Wave Clear và card choice.
- **Lời dẫn:** “Wave 4 tăng Ranger lên 20%, cho thấy arena pillar và camera không chỉ là trang trí mà ảnh hưởng cách né projectile.”
- **Bằng chứng:** `waves.json` wave 4; `SurvivalController.cpp` AI HexArcher và arena LOS.

### W3D-05 — Mốc giữa block đầu

- **File:** `AKA_W3D_05_KIET_T01.mkv`.
- **Cấu hình:** `75/25/0`; budget khoảng 9, cap 12, interval khoảng 0,98 giây.
- **Shot checklist riêng:**
  - [ ] Banner W05.
  - [ ] Encounter có áp lực melee + projectile; dùng Skill One đúng hero.
  - [ ] Quay thanh cooldown và ultimate charge tăng.
  - [ ] Wave Clear, chụp build summary ngắn trước W06.
- **Lời dẫn:** “Wave 5 đạt 25% Ranger và cap 12; người chơi bắt đầu kết hợp skill khống chế/phòng thủ thay vì chỉ basic.”
- **Bằng chứng:** `waves.json` wave 5; `balance.json`.

### W3D-06 — Trở lại 85/15 để kiểm tra sức mạnh build

- **File:** `AKA_W3D_06_KIET_T01.mkv`.
- **Cấu hình:** `85/15/0`; budget khoảng 9, cap 12, interval khoảng 0,96 giây.
- **Shot checklist riêng:**
  - [ ] Banner W06 và các upgrade stack hiện có.
  - [ ] Dùng Skill Two dọn một cụm Swarm.
  - [ ] Giữ một ranged shot và một dodge/parry trong clip.
  - [ ] Wave Clear và transition W07.
- **Lời dẫn:** “Wave 6 giảm tỷ lệ Ranger so với W05 nhưng giữ budget cao hơn đầu game, giúp nhìn rõ hiệu quả build sau nhiều draft.”
- **Bằng chứng:** `waves.json` wave 6; `SurvivalController::BeginWave`.

### W3D-07 — Ranger đạt 30%

- **File:** `AKA_W3D_07_KIET_T01.mkv`.
- **Cấu hình:** `70/30/0`; budget khoảng 10, cap 12, interval khoảng 0,93 giây.
- **Shot checklist riêng:**
  - [ ] Banner W07.
  - [ ] Ít nhất một tình huống bị tấn công từ xa và gần cùng lúc.
  - [ ] Target lock/aim assist chuyển sang Hex Archer ưu tiên.
  - [ ] Quái cuối, Wave Clear, upgrade.
- **Lời dẫn:** “W07 là lần đầu Ranger chiếm 30%, kiểm tra khả năng chọn ưu tiên và quản lý hướng camera.”
- **Bằng chứng:** `waves.json` wave 7; `SurvivalController.cpp:790-835`.

### W3D-08 — Xen kẽ mix 80/20

- **File:** `AKA_W3D_08_KIET_T01.mkv`.
- **Cấu hình:** `80/20/0`; budget khoảng 11, cap 13, interval khoảng 0,91 giây.
- **Shot checklist riêng:**
  - [ ] Banner W08.
  - [ ] Quay cap cao hơn với nhiều enemy active nếu seed cho phép.
  - [ ] Dùng ultimate lần đầu trong chuỗi wave thường và giữ contact/VFX.
  - [ ] Wave Clear và lựa chọn card.
- **Lời dẫn:** “W08 tăng budget/cap dù tỷ lệ Ranger về 20%; độ khó đến từ tổng threat chứ không chỉ phần trăm archetype.”
- **Bằng chứng:** `waves.json` wave 8; `balance.json`; `SurvivalController.cpp:1048-1061`.

### W3D-09 — Wave chuẩn bị boss

- **File:** `AKA_W3D_09_KIET_T01.mkv`.
- **Cấu hình:** `70/30/0`; budget khoảng 12, cap 13, interval khoảng 0,89 giây.
- **Shot checklist riêng:**
  - [ ] Banner W09 và build trước boss.
  - [ ] Combat Swarm/Ranger hoàn chỉnh, giữ HP sau quái cuối.
  - [ ] Wave Clear và draft; quay quyết định chọn upgrade chuẩn bị W10.
  - [ ] Transition/BGM báo boss W10.
- **Lời dẫn:** “Wave 9 là kiểm tra tổng hợp Swarm/Ranger cuối block đầu; lựa chọn upgrade sau wave ảnh hưởng trực tiếp trận Brood Warden.”
- **Bằng chứng:** `waves.json` wave 9; `SurvivalController.cpp:1070-1172`.

### W3D-10 — Boss Brood Warden

- **File:** `AKA_W3D_10_KIET_T01.mkv`.
- **Cấu hình:** Boss `brood_warden`; HP scale khoảng 0,648 và damage scale khoảng 0,377 theo cấu hình easy-submission.
- **Shot checklist riêng:**
  - [ ] Banner/boss title `BROOD WARDEN`, nhạc boss và HP bar.
  - [ ] Normal claw/sweep hoặc contact attack.
  - [ ] Phase/special summon Brood/Riftling; quay ít nhất một lần đàn con xuất hiện.
  - [ ] Dùng build/ultimate xử lý boss cùng adds.
  - [ ] Boss death, boss count tăng, reward/draft hiếm.
  - [ ] Transition sang W11 và BGM trở lại đúng ngữ cảnh.
- **Lời dẫn:** “W10 là boss đầu: Brood Warden chuyển nhịp từ xử lý một mục tiêu sang boss kèm đàn triệu hồi.”
- **Bằng chứng:** `waves.json` wave 10; `SurvivalController.cpp:341-359,1215-1325,1680-1755`.

## C2. W3D-11 đến W3D-20

### W3D-11 — Tanker xuất hiện lần đầu

- **File:** `AKA_W3D_11_KIET_T01.mkv`.
- **Cấu hình:** `60/30/10`; budget khoảng 14, cap 14, interval khoảng 0,85 giây.
- **Shot checklist riêng:**
  - [ ] Banner W11.
  - [ ] Riftling, Hex Archer và Obsidian Brute cùng được nhận diện; nếu RNG thiếu Brute, inset config bắt buộc.
  - [ ] Đánh Brute để thấy HP/poise cao hơn; né ground slam.
  - [ ] Wave Clear và upgrade.
- **Lời dẫn:** “W11 mở Tanker 10%; Obsidian Brute tiêu tốn threat budget lớn hơn và làm thay đổi ưu tiên damage.”
- **Bằng chứng:** `waves.json` wave 11; `SurvivalController.cpp:1215-1325,1600+`.

### W3D-12 — Ba archetype 55/30/15

- **File:** `AKA_W3D_12_KIET_T01.mkv`.
- **Cấu hình:** `55/30/15`; budget khoảng 15, cap 14, interval khoảng 0,83 giây.
- **Shot checklist riêng:**
  - [ ] Banner W12.
  - [ ] Archer đứng xa trong lúc Brute chặn đường và Swarm áp sát.
  - [ ] Dùng AoE/control skill để xử lý ba vai trò.
  - [ ] Quái cuối, Wave Clear, card selection.
- **Lời dẫn:** “W12 tăng Tanker lên 15%, tạo đội hình ba vai trò: áp sát nhanh, bắn xa và chắn chịu damage.”
- **Bằng chứng:** `waves.json` wave 12; `balance.json`.

### W3D-13 — Swarm tăng lại 65%

- **File:** `AKA_W3D_13_KIET_T01.mkv`.
- **Cấu hình:** `65/20/15`; budget khoảng 15, cap 15, interval khoảng 0,81 giây.
- **Shot checklist riêng:**
  - [ ] Banner W13.
  - [ ] Quay lượng Swarm dày hơn W12 và một Brute giữ tuyến.
  - [ ] Dùng dash/root-motion skill thoát vòng vây.
  - [ ] Wave Clear và transition.
- **Lời dẫn:** “W13 đổi trọng tâm về Swarm 65% nhưng vẫn giữ Tanker, kiểm tra mobility khi đường thoát bị che.”
- **Bằng chứng:** `waves.json` wave 13; `SurvivalController::BeginWave`.

### W3D-14 — Ranger tăng 35%

- **File:** `AKA_W3D_14_KIET_T01.mkv`.
- **Cấu hình:** `50/35/15`; budget khoảng 16, cap 15, interval khoảng 0,79 giây.
- **Shot checklist riêng:**
  - [ ] Banner W14.
  - [ ] Nhiều projectile từ các hướng; dùng camera/target lock chọn Archer.
  - [ ] Có ít nhất một Brute ground slam trong lúc né ranged.
  - [ ] Wave Clear và draft.
- **Lời dẫn:** “W14 đưa Ranger lên 35%; người chơi phải đọc telegraph ở nhiều khoảng cách thay vì tunnel vision vào Tanker.”
- **Bằng chứng:** `waves.json` wave 14; AI archetype trong `SurvivalController.cpp`.

### W3D-15 — Tanker đạt 20%

- **File:** `AKA_W3D_15_KIET_T01.mkv`.
- **Cấu hình:** `55/25/20`; budget khoảng 17, cap 16, interval khoảng 0,77 giây.
- **Shot checklist riêng:**
  - [ ] Banner W15.
  - [ ] Quay hai target bền hoặc Brute tồn tại qua nhiều combo.
  - [ ] Thể hiện upgrade damage/execute/pierce nếu đã có.
  - [ ] Quái cuối, Wave Clear, build summary giữa run.
- **Lời dẫn:** “W15 tăng Tanker lên 20% và cap 16, là mốc rõ để đánh giá damage scaling của build.”
- **Bằng chứng:** `waves.json` wave 15; `SurvivalController.cpp:2825+` damage/upgrade effects.

### W3D-16 — Swarm 60%, Tanker 20%

- **File:** `AKA_W3D_16_KIET_T01.mkv`.
- **Cấu hình:** `60/20/20`; budget khoảng 18, cap 16, interval khoảng 0,76 giây.
- **Shot checklist riêng:**
  - [ ] Banner W16.
  - [ ] Một cụm Swarm quanh Brute; dùng AoE trúng nhiều mục tiêu.
  - [ ] Quay wave-heal trước/đầu wave nếu HP tăng từ clear trước.
  - [ ] Wave Clear và card.
- **Lời dẫn:** “W16 giữ Tanker 20% nhưng tăng Swarm, khuyến khích area damage và quản lý vị trí quanh Brute.”
- **Bằng chứng:** `waves.json` wave 16; `balance.json` `waveHealFraction`; `SurvivalController.cpp:1038-1047`.

### W3D-17 — Ranger 40%

- **File:** `AKA_W3D_17_KIET_T01.mkv`.
- **Cấu hình:** `45/40/15`; budget khoảng 20, cap 16, interval khoảng 0,74 giây.
- **Shot checklist riêng:**
  - [ ] Banner W17.
  - [ ] Projectile pattern dày; dùng pillar/guard/dash hợp lý.
  - [ ] Target lock chuyển mục tiêu sau khi một Archer chết.
  - [ ] Wave Clear và upgrade.
- **Lời dẫn:** “W17 đạt 40% Ranger và budget khoảng 20, biến không gian né projectile thành thử thách chính.”
- **Bằng chứng:** `waves.json` wave 17; `SurvivalController.cpp` HexArcher update.

### W3D-18 — Mix cân bằng hơn

- **File:** `AKA_W3D_18_KIET_T01.mkv`.
- **Cấu hình:** `55/25/20`; budget khoảng 21, cap 17, interval khoảng 0,73 giây.
- **Shot checklist riêng:**
  - [ ] Banner W18.
  - [ ] Ba archetype xuất hiện trong đoạn đại diện.
  - [ ] Dùng một upgrade hero-specific và chỉ rõ effect.
  - [ ] Quái cuối, Wave Clear, card transition.
- **Lời dẫn:** “W18 hạ Ranger nhưng tăng tổng budget/cap; đây là wave kiểm tra build toàn diện thay vì một loại threat.”
- **Bằng chứng:** `waves.json` wave 18; `SurvivalController.cpp:1070-1172`.

### W3D-19 — Wave chuẩn bị boss thứ hai

- **File:** `AKA_W3D_19_KIET_T01.mkv`.
- **Cấu hình:** `45/35/20`; budget khoảng 22, cap 17, interval khoảng 0,71 giây.
- **Shot checklist riêng:**
  - [ ] Banner W19 và build trước W20.
  - [ ] Combat ba archetype với Ranger/Tanker cùng gây áp lực.
  - [ ] Giữ HP và ultimate charge sau quái cuối.
  - [ ] Wave Clear/draft và boss transition W20.
- **Lời dẫn:** “W19 là vòng kiểm tra trước Hexeye; giữ tài nguyên và chọn upgrade chống projectile quan trọng hơn dọn wave thật nhanh.”
- **Bằng chứng:** `waves.json` wave 19; `SurvivalController::BeginWave`.

### W3D-20 — Boss Hexeye Artillerist

- **File:** `AKA_W3D_20_KIET_T01.mkv`.
- **Cấu hình:** Boss `hexeye_artillerist`; HP scale khoảng 0,778, damage scale khoảng 0,413.
- **Shot checklist riêng:**
  - [ ] Banner/title `HEXEYE ARTILLERIST`, BGM và HP bar.
  - [ ] Normal ranged attack.
  - [ ] Targeting volley/special, quay telegraph và nhiều projectile.
  - [ ] Phase transition và thay đổi số shot/cooldown.
  - [ ] Boss death, reward/draft, record boss kill.
  - [ ] Transition W21.
- **Lời dẫn:** “W20 là boss pháo kích tầm xa; phase sau tăng mật độ volley nên camera, dash và line-of-sight quanh pillar trở thành trọng tâm.”
- **Bằng chứng:** `waves.json` wave 20; `SurvivalController.cpp:1755-1815`.

## C3. W3D-21 đến W3D-25

### W3D-21 — Ranger 40% sau boss

- **File:** `AKA_W3D_21_KIET_T01.mkv`.
- **Cấu hình:** `45/40/15`; budget khoảng 24, cap 18, interval khoảng 0,69 giây.
- **Shot checklist riêng:**
  - [ ] Banner W21 và boss reward vừa nhận.
  - [ ] Test reward trên nhóm có nhiều Ranger.
  - [ ] Né một chuỗi projectile mà không mất lock mục tiêu.
  - [ ] Wave Clear và upgrade.
- **Lời dẫn:** “Ngay sau boss, W21 trở lại với 40% Ranger và budget 24 để kiểm tra sức mạnh reward mới.”
- **Bằng chứng:** `waves.json` wave 21; `balance.json`.

### W3D-22 — 50/30/20

- **File:** `AKA_W3D_22_KIET_T01.mkv`.
- **Cấu hình:** `50/30/20`; budget khoảng 25, cap 18, interval khoảng 0,67 giây.
- **Shot checklist riêng:**
  - [ ] Banner W22.
  - [ ] Đoạn combat đại diện có ba vai trò.
  - [ ] Dùng combo/skill/ultimate theo một rotation hoàn chỉnh.
  - [ ] Quái cuối, Wave Clear, card.
- **Lời dẫn:** “W22 cân bằng ba archetype và là cảnh phù hợp để trình bày rotation đầy đủ của hero.”
- **Bằng chứng:** `waves.json` wave 22; action queue trong `SurvivalController.cpp:836-1001`.

### W3D-23 — Ranger/Tanker đạt tổng 60%

- **File:** `AKA_W3D_23_KIET_T01.mkv`.
- **Cấu hình:** `40/40/20`; budget khoảng 26, cap 19, interval khoảng 0,66 giây.
- **Shot checklist riêng:**
  - [ ] Banner W23.
  - [ ] Archer đứng sau Brute; chứng minh target priority.
  - [ ] Dùng pierce/Forked Bolt/AoE nếu build có.
  - [ ] Wave Clear và transition.
- **Lời dẫn:** “W23 chỉ còn 40% Swarm; tuyến Brute + Archer khiến damage xuyên/AoE và chọn mục tiêu có giá trị hơn.”
- **Bằng chứng:** `waves.json` wave 23; upgrade definitions trong `SurvivalController.cpp:301-319`.

### W3D-24 — Swarm quay lại 55%

- **File:** `AKA_W3D_24_KIET_T01.mkv`.
- **Cấu hình:** `55/25/20`; budget khoảng 28, cap 19, interval khoảng 0,65 giây.
- **Shot checklist riêng:**
  - [ ] Banner W24.
  - [ ] Quay spawn nhanh tạo áp lực Swarm quanh player.
  - [ ] Dùng dash/Gravity Well/Shield Rush để tái định vị.
  - [ ] Quái cuối, Wave Clear, card.
- **Lời dẫn:** “W24 tăng lại Swarm trong khi budget đạt 28; khả năng gom hoặc xuyên đám đông quyết định tốc độ clear.”
- **Bằng chứng:** `waves.json` wave 24; `balance.json`.

### W3D-25 — Tanker 25%, mốc nửa block

- **File:** `AKA_W3D_25_KIET_T01.mkv`.
- **Cấu hình:** `45/30/25`; budget khoảng 29, cap 20, interval khoảng 0,64 giây.
- **Shot checklist riêng:**
  - [ ] Banner W25 và build summary ngắn.
  - [ ] Obsidian Brute chiếm tỷ trọng cao; quay ground slam/độ bền.
  - [ ] Dùng execute/damage upgrade để kết liễu tanker nếu có.
  - [ ] Wave Clear và chọn upgrade chuẩn bị nửa sau run.
- **Lời dẫn:** “W25 đưa Tanker lên 25% và cap 20, là mốc đánh giá liệu build có đủ single-target damage cho nửa sau.”
- **Bằng chứng:** `waves.json` wave 25; `SurvivalController.cpp:1260-1290`.

## C4. W3D-26 đến W3D-30

### W3D-26 — Ranger đạt 45%

- **File:** `AKA_W3D_26_KIET_T01.mkv`.
- **Cấu hình:** `40/45/15`; budget khoảng 30, cap 20, interval khoảng 0,63 giây.
- **Shot checklist riêng:**
  - [ ] Banner W26.
  - [ ] Quay nhiều Hex Archer cùng bắn/đổi vị trí.
  - [ ] Sử dụng pillar, target lock và dash để phá vòng projectile.
  - [ ] Quái cuối, Wave Clear và upgrade.
- **Lời dẫn:** “W26 có 45% Ranger — tỷ lệ cao nhất tới thời điểm này — nên khả năng quản lý projectile và line-of-sight quan trọng hơn đứng yên gây damage.”
- **Bằng chứng:** `waves.json` wave 26; HexArcher AI trong `SurvivalController.cpp`.

### W3D-27 — Swarm 50%, Tanker 25%

- **File:** `AKA_W3D_27_KIET_T01.mkv`.
- **Cấu hình:** `50/25/25`; budget khoảng 31, cap 20, interval khoảng 0,62 giây.
- **Shot checklist riêng:**
  - [ ] Banner W27.
  - [ ] Riftling vây quanh một hoặc nhiều Brute.
  - [ ] Dùng AoE trước rồi single-target kết liễu Tanker.
  - [ ] Wave Clear và card transition.
- **Lời dẫn:** “W27 đổi từ projectile-heavy sang Swarm/Tanker; rotation hiệu quả là dọn quái nhỏ rồi dành đòn mạnh cho Brute.”
- **Bằng chứng:** `waves.json` wave 27; `balance.json`.

### W3D-28 — Mix 45/35/20

- **File:** `AKA_W3D_28_KIET_T01.mkv`.
- **Cấu hình:** `45/35/20`; budget khoảng 33, cap 21, interval khoảng 0,61 giây.
- **Shot checklist riêng:**
  - [ ] Banner W28.
  - [ ] Ba archetype trong cùng camera; HUD vẫn đọc được ở cap 21.
  - [ ] Quay một proc upgrade như Burn/Chain Spark/Emergency Barrier nếu build có.
  - [ ] Quái cuối, Wave Clear, upgrade.
- **Lời dẫn:** “W28 tăng budget lên 33 và cap 21, thích hợp chứng minh proc upgrade hoạt động trong encounter đông.”
- **Bằng chứng:** `waves.json` wave 28; `SurvivalController.cpp:2770-2860`.

### W3D-29 — Wave chuẩn bị Ironroot

- **File:** `AKA_W3D_29_KIET_T01.mkv`.
- **Cấu hình:** `35/40/25`; budget khoảng 34, cap 21, interval khoảng 0,60 giây.
- **Shot checklist riêng:**
  - [ ] Banner W29 và build trước boss.
  - [ ] Ranger + Tanker chiếm 65%; quay áp lực projectile sau tuyến Brute.
  - [ ] Giữ HP/ultimate charge sau quái cuối.
  - [ ] Wave Clear, boss-oriented upgrade và transition W30.
- **Lời dẫn:** “W29 giảm Swarm còn 35% nhưng tăng threat bền/tầm xa, ép người chơi chuẩn bị vị trí và hồi chiêu trước Ironroot.”
- **Bằng chứng:** `waves.json` wave 29; `SurvivalController::BeginWave`.

### W3D-30 — Boss Ironroot Colossus

- **File:** `AKA_W3D_30_KIET_T01.mkv`.
- **Cấu hình:** Boss `ironroot_colossus`; HP scale khoảng 0,930, damage scale khoảng 0,455.
- **Shot checklist riêng:**
  - [ ] Banner/title `IRONROOT COLOSSUS`, BGM và HP bar.
  - [ ] Normal heavy/contact attack.
  - [ ] Ground slam và fissure/projectile pattern; quay telegraph trước damage.
  - [ ] Phase transition và nhịp attack thay đổi.
  - [ ] Boss death, reward/draft và boss kill counter.
  - [ ] Transition W31.
- **Lời dẫn:** “W30 là Tanker boss: Ironroot telegraph ground slam, tạo fissure và tăng áp lực ở phase sau; đọc vùng nguy hiểm quan trọng hơn áp sát liên tục.”
- **Bằng chứng:** `waves.json` wave 30; `SurvivalController.cpp:1815-1885`.

## C5. W3D-31 đến W3D-40

### W3D-31 — Khởi đầu block bốn

- **File:** `AKA_W3D_31_KIET_T01.mkv`.
- **Cấu hình:** `45/35/20`; budget khoảng 37, cap 22, interval khoảng 0,58 giây.
- **Shot checklist riêng:**
  - [ ] Banner W31 và boss reward từ W30.
  - [ ] Test reward trên encounter ba archetype.
  - [ ] Quay ít nhất một damage spike nhưng player phản ứng kịp.
  - [ ] Wave Clear và upgrade.
- **Lời dẫn:** “W31 quay lại wave thường với budget 37; boss reward phải chứng minh giá trị ngay trong encounter dày hơn.”
- **Bằng chứng:** `waves.json` wave 31; `balance.json`.

### W3D-32 — Swarm 50%, Tanker 25%

- **File:** `AKA_W3D_32_KIET_T01.mkv`.
- **Cấu hình:** `50/25/25`; budget khoảng 38, cap 22, interval khoảng 0,57 giây.
- **Shot checklist riêng:**
  - [ ] Banner W32.
  - [ ] Đám Swarm che chắn Brute; quay root-motion/dash thoát vòng.
  - [ ] Dùng ultimate trúng nhiều target.
  - [ ] Quái cuối, Wave Clear và card.
- **Lời dẫn:** “W32 dùng 50% Swarm và 25% Tanker, tạo cơ hội kiểm tra AoE/ultimate trên tuyến địch nhiều lớp.”
- **Bằng chứng:** `waves.json` wave 32; skill contact trong `SurvivalController.cpp:2455-2627`.

### W3D-33 — Ranger 45%, Swarm chỉ 35%

- **File:** `AKA_W3D_33_KIET_T01.mkv`.
- **Cấu hình:** `35/45/20`; budget khoảng 40, cap 23, interval khoảng 0,56 giây.
- **Shot checklist riêng:**
  - [ ] Banner W33.
  - [ ] Nhiều Archer phân tán quanh arena.
  - [ ] Camera orbit/target lock đổi mục tiêu tối thiểu hai lần.
  - [ ] Quái cuối, Wave Clear, upgrade.
- **Lời dẫn:** “W33 có 45% Ranger và budget 40; camera awareness cùng target switching là nội dung chính của wave.”
- **Bằng chứng:** `waves.json` wave 33; camera/lock code trong `SurvivalController.cpp`.

### W3D-34 — Tanker đạt 30%

- **File:** `AKA_W3D_34_KIET_T01.mkv`.
- **Cấu hình:** `45/25/30`; budget khoảng 41, cap 23, interval khoảng 0,55 giây.
- **Shot checklist riêng:**
  - [ ] Banner W34.
  - [ ] Quay nhiều Brute/tanker pressure hoặc inset config nếu seed không sinh đủ.
  - [ ] Thể hiện single-target damage/Execution dưới 25% HP.
  - [ ] Wave Clear và card.
- **Lời dẫn:** “W34 lần đầu Tanker đạt 30%, buộc build vượt qua health pool cao thay vì chỉ kiểm soát số lượng.”
- **Bằng chứng:** `waves.json` wave 34; `UpgradeId::Execution`; enemy base stats trong `SurvivalController.cpp:1260-1290`.

### W3D-35 — 40/35/25

- **File:** `AKA_W3D_35_KIET_T01.mkv`.
- **Cấu hình:** `40/35/25`; budget khoảng 42, cap 24, interval khoảng 0,54 giây.
- **Shot checklist riêng:**
  - [ ] Banner W35 và build stack overview.
  - [ ] Ba archetype, tối thiểu một projectile và ground slam cùng thời điểm.
  - [ ] Emergency Barrier/Second Wind nếu proc trong run; không dựng giả nếu chưa có.
  - [ ] Wave Clear và upgrade.
- **Lời dẫn:** “W35 cân bằng áp lực tầm gần, tầm xa và Tanker ở active cap 24; defensive upgrade bắt đầu có giá trị rõ.”
- **Bằng chứng:** `waves.json` wave 35; upgrade handling trong `SurvivalController.cpp:1134-1168,2757-2825`.

### W3D-36 — Swarm tăng lại 50%

- **File:** `AKA_W3D_36_KIET_T01.mkv`.
- **Cấu hình:** `50/30/20`; budget khoảng 44, cap 24, interval khoảng 0,54 giây.
- **Shot checklist riêng:**
  - [ ] Banner W36.
  - [ ] Quay spawn cadence nhanh và cụm Swarm lớn.
  - [ ] Dùng Gravity Well/Frost Ring hoặc Shield Rush/Violet combo tùy hero.
  - [ ] Quái cuối, Wave Clear, card.
- **Lời dẫn:** “W36 tăng lại Swarm để kiểm tra area control khi spawn interval đã gần sàn 0,5 giây.”
- **Bằng chứng:** `waves.json` wave 36; `balance.json`.

### W3D-37 — Ranger/Tanker chiếm 65%

- **File:** `AKA_W3D_37_KIET_T01.mkv`.
- **Cấu hình:** `35/40/25`; budget khoảng 45, cap 24, interval khoảng 0,53 giây.
- **Shot checklist riêng:**
  - [ ] Banner W37.
  - [ ] Archer đứng sau tuyến Brute trong một góc camera.
  - [ ] Dùng line-of-sight pillar rồi vòng ra tấn công.
  - [ ] Wave Clear và upgrade.
- **Lời dẫn:** “W37 ưu tiên Ranger/Tanker; chiến thuật hiệu quả là cắt LOS, vòng qua tuyến chắn và hạ nguồn projectile.”
- **Bằng chứng:** `waves.json` wave 37; arena LOS/camera collision trong `SurvivalController.cpp`.

### W3D-38 — Tanker 30%, budget 47

- **File:** `AKA_W3D_38_KIET_T01.mkv`.
- **Cấu hình:** `45/25/30`; budget khoảng 47, cap 25, interval khoảng 0,52 giây.
- **Shot checklist riêng:**
  - [ ] Banner W38.
  - [ ] Quay một chuỗi combat dài với Brute còn sống qua nhiều action.
  - [ ] Giữ cooldown/ultimate/resource management rõ ràng.
  - [ ] Quái cuối, Wave Clear, card.
- **Lời dẫn:** “W38 giữ 30% Tanker nhưng tăng budget/cap; người chơi phải quản lý cooldown thay vì dùng hết skill ở spawn đầu.”
- **Bằng chứng:** `waves.json` wave 38; action cooldown code trong `SurvivalController.cpp`.

### W3D-39 — Wave chuẩn bị Eclipse Chimera

- **File:** `AKA_W3D_39_KIET_T01.mkv`.
- **Cấu hình:** `35/40/25`; budget khoảng 49, cap 25, interval khoảng 0,51 giây.
- **Shot checklist riêng:**
  - [ ] Banner W39 và build trước boss.
  - [ ] Combat với Ranger/Tanker dày, giữ HP sau quái cuối.
  - [ ] Wave Clear/draft; chọn upgrade boss-oriented.
  - [ ] Transition và BGM boss W40.
- **Lời dẫn:** “W39 là wave thường gần đạt sàn spawn interval; giữ hồi chiêu và defensive resource cho Chimera là mục tiêu.”
- **Bằng chứng:** `waves.json` wave 39; `balance.json`.

### W3D-40 — Boss Eclipse Chimera

- **File:** `AKA_W3D_40_KIET_T01.mkv`.
- **Cấu hình:** Boss `eclipse_chimera`; HP scale khoảng 1,103, damage scale khoảng 0,502.
- **Shot checklist riêng:**
  - [ ] Banner/title `ECLIPSE CHIMERA`, BGM và HP bar.
  - [ ] Quay normal/contact attack.
  - [ ] Quay ranged/fissure/special pattern đại diện.
  - [ ] Phase/policy switch và thay đổi hành vi cận chiến/tầm xa.
  - [ ] Boss death, reward/draft và boss counter.
  - [ ] Transition W41.
- **Lời dẫn:** “W40 là hybrid boss; Eclipse Chimera đổi policy giữa phase thay vì chỉ giảm cooldown của cùng một đòn.”
- **Bằng chứng:** `waves.json` wave 40; `SurvivalController.cpp:1860-1925`.

## C6. W3D-41 đến W3D-50

### W3D-41 — Spawn interval chạm sàn

- **File:** `AKA_W3D_41_KIET_T01.mkv`.
- **Cấu hình:** `40/35/25`; budget khoảng 52, cap 26, interval 0,50 giây.
- **Shot checklist riêng:**
  - [ ] Banner W41 và reward từ W40.
  - [ ] Quay spawn cadence 0,5 giây cùng ba archetype.
  - [ ] Test boss reward trong encounter thường.
  - [ ] Wave Clear và upgrade.
- **Lời dẫn:** “Từ W41, spawn interval đạt sàn 0,5 giây; các wave sau tăng chủ yếu qua budget, cap và scaling thay vì spawn nhanh hơn nữa.”
- **Bằng chứng:** `waves.json` wave 41; `balance.json`; `SurvivalController.cpp:1054-1061`.

### W3D-42 — 45/30/25

- **File:** `AKA_W3D_42_KIET_T01.mkv`.
- **Cấu hình:** `45/30/25`; budget khoảng 53, cap 26, interval 0,50 giây.
- **Shot checklist riêng:**
  - [ ] Banner W42.
  - [ ] Combat ba archetype ở cadence tối đa.
  - [ ] Quay area damage và crowd-control chain.
  - [ ] Quái cuối, Wave Clear, card.
- **Lời dẫn:** “W42 giữ mix tương đối cân bằng nhưng budget 53, kiểm tra khả năng clear liên tục khi spawn không còn khoảng nghỉ dài.”
- **Bằng chứng:** `waves.json` wave 42; `balance.json`.

### W3D-43 — Ranger 45% cuối game

- **File:** `AKA_W3D_43_KIET_T01.mkv`.
- **Cấu hình:** `35/45/20`; budget khoảng 55, cap 27, interval 0,50 giây.
- **Shot checklist riêng:**
  - [ ] Banner W43.
  - [ ] Projectile từ nhiều hướng ở cap cao.
  - [ ] Né bằng dash/guard/camera orbit và hạ ít nhất hai Archer liên tiếp.
  - [ ] Wave Clear và upgrade.
- **Lời dẫn:** “W43 ghép Ranger 45% với cap 27; đây là bài kiểm tra đọc màn hình và phản xạ né projectile ở late game.”
- **Bằng chứng:** `waves.json` wave 43; HexArcher projectile logic trong `SurvivalController.cpp`.

### W3D-44 — Tanker 30% cuối game

- **File:** `AKA_W3D_44_KIET_T01.mkv`.
- **Cấu hình:** `45/25/30`; budget khoảng 57, cap 27, interval 0,50 giây.
- **Shot checklist riêng:**
  - [ ] Banner W44.
  - [ ] Brute chịu nhiều combo; quay HP scaling late-game.
  - [ ] Dùng execute/single-target burst/ultimate hợp lý.
  - [ ] Quái cuối, Wave Clear, card.
- **Lời dẫn:** “W44 đối xứng W43 bằng 30% Tanker; thử thách chuyển từ né dày sang vượt health pool lớn.”
- **Bằng chứng:** `waves.json` wave 44; `SurvivalController::HpScale`.

### W3D-45 — Mix 40/35/25, cap 28

- **File:** `AKA_W3D_45_KIET_T01.mkv`.
- **Cấu hình:** `40/35/25`; budget khoảng 59, cap 28, interval 0,50 giây.
- **Shot checklist riêng:**
  - [ ] Banner W45 và full build summary ngắn.
  - [ ] Ba archetype cùng gây áp lực; HUD/performance vẫn đọc được.
  - [ ] Quay một defensive proc hoặc revive nếu xảy ra thật.
  - [ ] Wave Clear và upgrade.
- **Lời dẫn:** “W45 là bài kiểm tra build hoàn chỉnh với cap 28; mọi lớp damage, defense, cooldown và mobility đều tham gia.”
- **Bằng chứng:** `waves.json` wave 45; upgrade application trong `SurvivalController.cpp`.

### W3D-46 — Swarm/Ranger 45/35

- **File:** `AKA_W3D_46_KIET_T01.mkv`.
- **Cấu hình:** `45/35/20`; budget khoảng 60, cap 28, interval 0,50 giây.
- **Shot checklist riêng:**
  - [ ] Banner W46.
  - [ ] Spawn cadence liên tục; xử lý Swarm mà không bỏ quên Archer.
  - [ ] Dùng target switch và AoE trong cùng rotation.
  - [ ] Quái cuối, Wave Clear, card.
- **Lời dẫn:** “W46 vượt threat budget 60; người chơi phải vừa dọn đám đông vừa cắt nguồn ranged damage.”
- **Bằng chứng:** `waves.json` wave 46; `balance.json`.

### W3D-47 — Ranger/Tanker 65%

- **File:** `AKA_W3D_47_KIET_T01.mkv`.
- **Cấu hình:** `35/40/25`; budget khoảng 62, cap 28, interval 0,50 giây.
- **Shot checklist riêng:**
  - [ ] Banner W47.
  - [ ] Brute chắn Archer; quay reposition quanh pillar.
  - [ ] Giữ HP/defensive cooldown cho W48–W50.
  - [ ] Wave Clear và upgrade.
- **Lời dẫn:** “W47 giảm Swarm, tăng tuyến Ranger/Tanker; sai target priority dễ kéo dài wave và bào mòn tài nguyên trước ba mốc cuối.”
- **Bằng chứng:** `waves.json` wave 47; `SurvivalController.cpp` AI/target lock.

### W3D-48 — Budget spike 1,15×

- **File:** `AKA_W3D_48_KIET_T01.mkv`.
- **Cấu hình:** `40/30/30`, `budgetMultiplier=1.15`; threat budget khoảng 73 thay vì khoảng 64, cap 29, interval 0,50 giây.
- **Shot checklist riêng:**
  - [ ] Banner W48 và overlay `1.15× BUDGET`.
  - [ ] Quay thời lượng/mật độ wave cao hơn W47; ba archetype hiện diện.
  - [ ] Dùng ultimate/area control nhiều lần theo cooldown thật.
  - [ ] Quái cuối, Wave Clear và upgrade.
- **Lời dẫn:** “W48 là spike được author trực tiếp: mix có 30% Tanker và toàn budget nhân 1,15, nên tổng threat vượt cả W49.”
- **Bằng chứng:** `waves.json` wave 48; `SurvivalController.cpp:407-434,1048-1056`.

### W3D-49 — Wave cuối trước Void Sovereign

- **File:** `AKA_W3D_49_KIET_T01.mkv`.
- **Cấu hình:** `35/40/25`; budget khoảng 66, cap 29, interval 0,50 giây.
- **Shot checklist riêng:**
  - [ ] Banner W49 và full build trước final boss.
  - [ ] Combat Ranger/Tanker-heavy; không cắt mất quái cuối.
  - [ ] Ghi HP, revive/defensive state và ultimate charge sau clear.
  - [ ] Wave Clear, lựa chọn upgrade cuối và transition/BGM W50.
- **Lời dẫn:** “W49 có budget thấp hơn spike W48 nhưng là bước quản lý tài nguyên cuối cùng; upgrade sau wave là lựa chọn chốt build cho Void Sovereign.”
- **Bằng chứng:** `waves.json` wave 49; `SurvivalController::BeginWave` và upgrade draft.

### W3D-50 — Final boss Void Sovereign và RunVictory

- **File:** `AKA_W3D_50_KIET_T01.mkv`.
- **Cấu hình:** Boss `void_sovereign`; HP scale khoảng 1,299, damage scale khoảng 0,554; victory chỉ hợp lệ khi wave 50 hoàn tất.
- **Shot checklist riêng:**
  - [ ] Banner/title `VOID SOVEREIGN`, final-boss BGM và full HP bar.
  - [ ] Phase 1 normal pattern.
  - [ ] Phase 2 transition, summon và special/projectile pattern.
  - [ ] Phase 3 transition, nhịp attack nhanh hơn và volley lớn.
  - [ ] Final hit, death animation/VFX, boss counter và `RUN VICTORY`.
  - [ ] Result có score/time/grade/coin reward; mở local record.
  - [ ] Giữ frame xác nhận `50/50 WAVES` và không cộng thành feature.
- **Lời dẫn:** “W50 kết thúc run bằng Void Sovereign ba phase. Sau final hit, controller chuyển RunVictory; backend chỉ chấp nhận cờ victory khi wave hoàn tất đúng 50.”
- **Bằng chứng:** `waves.json` wave 50; `SurvivalController.cpp:1919-2020,1032`; `SurvivalServerCore.cpp:178-230`.

---

# PHẦN D — KẾ HOẠCH BUỔI QUAY VÀ KIỂM TRA COVERAGE

## D1. Chia buổi quay thực tế

### Nguyễn Trọng Tiến

- **T1–T3 — F001–F020:** quay thành ba block riêng đúng các heading T1, T2 và T3 ở trên. Ưu tiên F001 ngay sau boot sạch; các clip input bật key viewer.
- **T4–T7 — F021–F040:** mỗi bộ kỹ năng của một class là một block riêng. Chuẩn bị level ngắn có enemy HP đủ cao và giữ camera nhất quán giữa bốn block.
- **T8–T11 — F041–F060:** quay bốn block item/completion/UI riêng; dùng save sạch cho achievement và chuẩn bị đúng trạng thái animation/Skill Bar.
- **T12–T14 — F061–F075:** quay ba block AI/combat systems riêng; chuẩn bị geometry cho LOS, ledge, boss navigation và local co-op input thật.
- **Sáu full campaign run độc lập:** mỗi level L2D-01 đến L2D-06 có một raw full run riêng. Tên file phải có `FULL`; tuyệt đối không thay full run bằng tập hợp highlight rời.

### Nguyễn Anh Kiệt

- **K1–K3 — F076–F090:** quay thành ba block riêng cho setup/movement, Knight và Magic Caster. Tên skill phải dùng `FROST RING`, không dùng tên cue nội bộ Frost Nova.
- **K4–K5 — F091–F100:** tách riêng block enemy/director/upgrade và block fixed-step/pooling/presentation/result.
- **K6–K9 — F101–F120:** quay bốn block riêng cho audio, persistence/renderer, OOD/design patterns và service/backend. Với cảnh code/terminal, giữ font đủ lớn để đọc.
- **K10–K11 — F121–F130:** tách Shop/custom level/MapBuilder tools khỏi editor I/O/UI/build/configuration. Tất cả thao tác file dùng map/save test, không ghi đè campaign.
- **Một full 50-wave run độc lập:** quay liên tục từ Character Select đến RunVictory. Nếu phần mềm tự split file theo dung lượng, các phần phải nối thời gian, không thiếu frame và tên `FULL_PART01`, `PART02`.

## D2. Checklist 130 feature

Mỗi checkbox chỉ được đánh dấu sau khi tồn tại: (1) file raw, (2) slate đúng mã, (3) cảnh/thao tác theo mục feature, (4) voice hoặc transcript, (5) evidence file nhìn được hay được trích trong overlay.

### Nguyễn Trọng Tiến — 75 feature

- [ ] F001  - [ ] F002  - [ ] F003  - [ ] F004  - [ ] F005
- [ ] F006  - [ ] F007  - [ ] F008  - [ ] F009  - [ ] F010
- [ ] F011  - [ ] F012  - [ ] F013  - [ ] F014  - [ ] F015
- [ ] F016  - [ ] F017  - [ ] F018  - [ ] F019  - [ ] F020
- [ ] F021  - [ ] F022  - [ ] F023  - [ ] F024  - [ ] F025
- [ ] F026  - [ ] F027  - [ ] F028  - [ ] F029  - [ ] F030
- [ ] F031  - [ ] F032  - [ ] F033  - [ ] F034  - [ ] F035
- [ ] F036  - [ ] F037  - [ ] F038  - [ ] F039  - [ ] F040
- [ ] F041  - [ ] F042  - [ ] F043  - [ ] F044  - [ ] F045
- [ ] F046  - [ ] F047  - [ ] F048  - [ ] F049  - [ ] F050
- [ ] F051  - [ ] F052  - [ ] F053  - [ ] F054  - [ ] F055
- [ ] F056  - [ ] F057  - [ ] F058  - [ ] F059  - [ ] F060
- [ ] F061  - [ ] F062  - [ ] F063  - [ ] F064  - [ ] F065
- [ ] F066  - [ ] F067  - [ ] F068  - [ ] F069  - [ ] F070
- [ ] F071  - [ ] F072  - [ ] F073  - [ ] F074  - [ ] F075

### Nguyễn Anh Kiệt — 55 feature

- [ ] F076  - [ ] F077  - [ ] F078  - [ ] F079  - [ ] F080
- [ ] F081  - [ ] F082  - [ ] F083  - [ ] F084  - [ ] F085
- [ ] F086  - [ ] F087  - [ ] F088  - [ ] F089  - [ ] F090
- [ ] F091  - [ ] F092  - [ ] F093  - [ ] F094  - [ ] F095
- [ ] F096  - [ ] F097  - [ ] F098  - [ ] F099  - [ ] F100
- [ ] F101  - [ ] F102  - [ ] F103  - [ ] F104  - [ ] F105
- [ ] F106  - [ ] F107  - [ ] F108  - [ ] F109  - [ ] F110
- [ ] F111  - [ ] F112  - [ ] F113  - [ ] F114  - [ ] F115
- [ ] F116  - [ ] F117  - [ ] F118  - [ ] F119  - [ ] F120
- [ ] F121  - [ ] F122  - [ ] F123  - [ ] F124  - [ ] F125
- [ ] F126  - [ ] F127  - [ ] F128  - [ ] F129  - [ ] F130

## D3. Checklist full-run 6 level 2D — riêng, không tính feature

- [ ] `L2D-01`: có raw `FULL`; bắt đầu từ Level Select/Prepare; spawn; guide/sign/portal/enemy/checkpoint/Cup; kết thúc ở Result.
- [ ] `L2D-02`: có raw `FULL`; spawn; ba enemy type; Chest/Potion/FakeWall/portal/checkpoint; Boss1; Cup; Result.
- [ ] `L2D-03`: có raw `FULL`; spawn; ba enemy type; ba cặp portal/FakeWall/loot/checkpoint; Boss1; Cup; Result.
- [ ] `L2D-04`: có raw `FULL`; spawn; flying gauntlet/Potion/FakeWall/portal/checkpoint; Boss2 ba phase; Cup; Result.
- [ ] `L2D-05`: có raw `FULL`; spawn; encounter/Chest/FakeWall/hai cặp portal/checkpoint; Boss2; Cup; Result.
- [ ] `L2D-06`: có raw `FULL`; spawn; ColdCorridor/loot/portal/checkpoint; Boss1 + Boss2 + Boss3 đủ phase; Cup; Result.

## D4. Checklist full-run 50 wave 3D — riêng, không tính feature

- [ ] Raw liên tục bắt đầu ở Character Select, không bắt đầu từ save-state giữa run.
- [ ] W01  - [ ] W02  - [ ] W03  - [ ] W04  - [ ] W05
- [ ] W06  - [ ] W07  - [ ] W08  - [ ] W09  - [ ] W10 Brood Warden
- [ ] W11  - [ ] W12  - [ ] W13  - [ ] W14  - [ ] W15
- [ ] W16  - [ ] W17  - [ ] W18  - [ ] W19  - [ ] W20 Hexeye Artillerist
- [ ] W21  - [ ] W22  - [ ] W23  - [ ] W24  - [ ] W25
- [ ] W26  - [ ] W27  - [ ] W28  - [ ] W29  - [ ] W30 Ironroot Colossus
- [ ] W31  - [ ] W32  - [ ] W33  - [ ] W34  - [ ] W35
- [ ] W36  - [ ] W37  - [ ] W38  - [ ] W39  - [ ] W40 Eclipse Chimera
- [ ] W41  - [ ] W42  - [ ] W43  - [ ] W44  - [ ] W45
- [ ] W46  - [ ] W47  - [ ] W48 budget spike  - [ ] W49  - [ ] W50 Void Sovereign
- [ ] Raw kết thúc sau RunVictory/Result/local record; không dừng ngay ở final hit.
- [ ] Mọi banner/bộ đếm wave đều đọc được; không có số wave bị bỏ qua khi montage.

## D5. QA nội dung và kỹ thuật trước khi xuất bản

### Kiểm tra nội dung

- [ ] Có đúng 130 slate feature, không trùng mã và không thiếu mã.
- [ ] Không có `L2D-*` hoặc `W3D-*` nào xuất hiện trong tổng feature.
- [ ] Sáu level 2D đều ghi Nguyễn Trọng Tiến; không gán một level 2D cho Nguyễn Anh Kiệt.
- [ ] 50 wave 3D đều ghi Nguyễn Anh Kiệt.
- [ ] Local co-op luôn được gọi là “local co-op/cùng máy”.
- [ ] AI luôn được gọi là “rule-based AI/FSM/pathfinding”, không gọi ML.
- [ ] F043 có đủ các state animation 2D và đổi hướng trái/phải.
- [ ] F045 có charge/active/cooldown/ready, element và layout local co-op.
- [ ] F121 quay profile sạch, thiếu coin, mua thành công, trừ coin đúng và persistence.
- [ ] F119/F120 quay đủ queue/retry, validation, idempotency và leaderboard localhost.
- [ ] F120 nói integrity/resilience localhost, không nói production security.
- [ ] F128 có Main, Shop, Level Select, Custom Maps, Leaderboard, Achievements, Options, Pause và Result.

### Kiểm tra hình/tiếng

- [ ] 1920×1080/60 FPS hoặc cao hơn; không stretch sai tỉ lệ.
- [ ] Game không bị che bởi cửa sổ notification, taskbar hoặc overlay nhạy cảm.
- [ ] Voice rõ, không clipping; BGM nhỏ hơn lời; sound effect vẫn nghe được khi feature âm thanh được trình bày.
- [ ] Code evidence dùng font đủ lớn; mỗi file giữ trên màn hình ít nhất ba giây.
- [ ] Mọi replay slow-motion có nhãn `REPLAY 0.25×/0.5×`; không để người xem hiểu nhầm tốc độ game thật.
- [ ] Mọi debug graphic thêm ở hậu kỳ có nhãn `MINH HỌA`, không giả là debug renderer tích hợp.
- [ ] Raw file và project dựng được sao lưu ở ít nhất hai ổ/thư mục.

## D6. Lời mở đầu và kết thúc mẫu

### Mở đầu

> “Nhóm trình bày Apple Knight Adventure theo ba tập minh chứng độc lập. Tập thứ nhất gồm 130 feature, đánh mã F001 đến F130. Tập thứ hai gồm sáu level campaign 2D và tập thứ ba gồm 50 wave Survival3D. Sáu level và 50 wave không được cộng vào số feature. Nguyễn Trọng Tiến phụ trách F001 đến F075 và toàn bộ sáu level 2D; Nguyễn Anh Kiệt phụ trách F076 đến F130 và toàn bộ 50 wave 3D.”

### Kết thúc

> “Video đã đối chiếu đủ 130 trên 130 feature. Ngoài các feature này, nhóm đã lưu và trình bày full run sáu trên sáu level 2D cùng một full run liên tục 50 trên 50 wave Survival3D. Phần campaign có local co-op; dịch vụ tùy chọn đồng bộ kết quả run và leaderboard.”

### Final verification card

```text
APPLE KNIGHT ADVENTURE — COVERAGE VERIFIED

FEATURES:       130 / 130
2D LEVELS:        6 / 6     (not counted as features)
3D WAVES:        50 / 50    (not counted as features)

NGUYỄN TRỌNG TIẾN: F001–F075 + L2D-01–L2D-06
NGUYỄN ANH KIỆT:   F076–F130 + W3D-01–W3D-50
```
