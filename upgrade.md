# Nâng cấp Game – Apple Knight Multiplayer Adventure

## 1. Khu vực bí mật (Secret Areas)

### Vấn đề

Game gốc *Apple Knight* có các căn phòng bí mật ẩn sau tường giả, nơi người chơi tìm rương kho báu. Tài liệu thiết kế hiện tại chưa đề cập cơ chế này.

### Giải pháp

#### 1.1. Tường giả (Fake Wall)

- Entity `FakeWall` — hiển thị giống tường thường nhưng có thể phá được.
- Kích hoạt: tấn công (vũ khí cận chiến) hoặc đủ sát thương tích luỹ.
- Hiệu ứng: vỡ vụn / fade out, để lối đi vào phòng bí mật.

```cpp
class FakeWall : public Entity
{
private:
    int maxHP;
    int currentHP;
    bool destroyed;

public:
    void OnHit(int damage);
    bool IsDestroyed() const;
    void RevealSecret();
};
```

#### 1.2. Rương kho báu (Chest)

- Entity `Chest` — đặt trong phòng bí mật hoặc cuối khu vực.
- Trạng thái: đóng → mở (khi người chơi tương tác).
- Phần thưởng ngẫu nhiên từ pool:

| Loại       | Mô tả                    |
| ---------- | ------------------------ |
| Coins      | Tiền xu                   |
| Apples     | Hồi máu                   |
| Key        | Mở khoá cửa đặc biệt      |
| Equipment  | Trang bị (vũ khí/giáp)    |

```cpp
class Chest : public Entity
{
private:
    bool opened;
    std::vector<Item*> lootPool;

public:
    void Interact(Player* player);
    bool IsOpened() const;
};
```

#### 1.3. Tích hợp Level Design

- Công cụ Editor / JSON map cho phép đặt `FakeWall` và `Chest` trong tilemap.
- Minimap hint (tuỳ chọn): đánh dấu mờ khu vực chưa khám phá.

#### Lợi ích

- Tăng chiều sâu thám hiểm.
- Khuyến khích khám phá bản đồ.
- Giữ đúng tinh thần game gốc.

---

## 2. Hệ thống Đánh giá Màn chơi (Level Rating)

### Vấn đề

Game gốc đánh giá 1–3 sao dựa trên thành tích (bí mật, táo, quái). Dự án hiện có checkpoint nhưng thiếu cơ chế chấm điểm, giảm giá trị chơi lại.

### Giải pháp

#### 2.1. Tiêu chí tính điểm

| Tiêu chí                 | Tỉ trọng |
| ------------------------ | -------- |
| % khu vực bí mật tìm thấy | 30%      |
| % apples thu thập        | 25%      |
| % quái tiêu diệt          | 25%      |
| Thời gian hoàn thành      | 10%      |
| Mạng sống còn lại         | 10%      |

#### 2.2. Xếp hạng sao

| Điểm    | Sao |
| ------- | --- |
| < 50%   | 1   |
| 50–79%  | 2   |
| ≥ 80%   | 3   |

#### 2.3. Luồng hoạt động

```
Kết thúc màn
  ↓
Tính điểm theo tiêu chí
  ↓
Hiển thị màn hình kết quả
  ↓
Cập nhật điểm cao nhất (PlayerPrefs / file save)
  ↓
Quay lại Menu / Màn tiếp theo
```

```cpp
struct LevelRating
{
    float secretsFound;
    float applesCollected;
    float enemiesDefeated;
    float completionTime;
    int livesRemaining;
};

class LevelScoring
{
public:
    float CalculateScore(const LevelRating& rating);
    int CalculateStars(float score);
    void ShowResultScreen(int stars, float score);
    void SaveHighScore(int levelID, int stars);
};
```

#### 2.4. Replayability

- Mở khoá skin / vũ khí khi đạt đủ số sao (tuỳ chọn mở rộng).
- Bảng xếp hạng điểm cao (lưu local).

#### Lợi ích

- Tạo động lực chơi lại.
- Gắn kết các hệ thống: secret areas, inventory, combat.
- Tăng thời lượng chơi và giá trị học thuật.

---

## 3. Hệ thống Pet (Pet System)

### Vấn đề

Game gốc *Apple Knight* có thú cưng (pet) bay theo người chơi, tự động hỗ trợ chiến đấu. Tài liệu hiện tại chưa có cơ chế này.

### Giải pháp

#### 3.1. Kiến trúc Pet

`Pet` là entity kế thừa từ `Entity`, sở hữu bởi một `Player`. AI đơn giản: follow chủ + tự động tìm mục tiêu.

```cpp
enum class PetType { SKULL, GHOST, BABY_DRAGON, FAIRY };

class Pet : public Entity
{
private:
    Player* owner;
    PetType type;
    float attackCooldown;
    float attackTimer;
    int level;

public:
    Pet(Player* owner, PetType type);
    void Update(float dt) override;
    void Render() override;
    void LevelUp();
    void SetOwner(Player* newOwner);
};
```

#### 3.2. Các loại Pet

| Loại         | Hành vi                          | Sát thương | Đặc biệt              |
| ------------ | -------------------------------- | ---------- | --------------------- |
| Skull        | Bắn đạn xuyên thấu đơn giản       | Trung bình | Bắn xuyên tường       |
| Ghost        | Hồi máu định kỳ cho chủ          | Thấp       | Tăng tốc hồi HP       |
| Baby Dragon  | Phun lửa AOE ngắn                | Cao        | Đốt cháy kẻ địch      |
| Fairy        | Lượn lượm đồ tự động             | Không      | Hút item từ xa        |

#### 3.3. Nâng cấp Pet

- Pet tăng cấp khi chủ tiêu diệt quái / nhặt exp orb.
- Mỗi cấp: tăng sát thương / giảm cooldown / mở skill mới.

```cpp
struct PetStats
{
    float damage;
    float attackSpeed;
    float range;
    float specialPower;
};

PetStats GetPetStats(PetType type, int level);
```

#### 3.4. Multiplayer

- Mỗi player sở hữu pet riêng, đồng bộ qua server.
- Pet là entity đồng bộ (position, state, target).
- Client prediction cho pet AI để giảm độ trễ.

#### Lợi ích

- Giữ đúng tinh thần game gốc.
- Tăng chiều sâu chiến thuật (chọn pet phù hợp).
- Tương tác tốt với inventory (pet item).

---

## 4. Hệ thống Nguyên tố (Elemental Combat)

### Vấn đề

Game gốc chỉ có sát thương vật lý đơn thuần. Thiếu chiều sâu chiến thuật khi combo giữa 2 người chơi.

### Giải pháp

#### 4.1. Hệ nguyên tố

3 nguyên tố cốt lõi, quan hệ tương khắc:

```
          🔥 Lửa (Fire)
         /    \
   khắc /      \ khắc
       /        \
      ↓          ↓
  🌊 Nước (Water) ← khắc → ⚡ Sấm (Thunder)
         \        /
   sinh   \      / sinh
           \    /
            ↓  ↓
          🔥 Lửa (Fire)
```

#### 4.2. Gắn nguyên tố cho vũ khí

- Mỗi vũ khí có thể gắn 0–1 nguyên tố (qua enchantment / item).
- Đòn đánh thường gây sát thương vật lý + **Elemental Damage** kèm **Status Effect**.

```cpp
enum class Element { NONE, FIRE, WATER, THUNDER };
enum class StatusEffect { NONE, BURN, WET, SHOCKED, FROZEN, VAPORIZE, OVERLOAD, CONDUCT };

struct DamagePacket
{
    float physicalDamage;
    float elementalDamage;
    Element element;
};
```

#### 4.3. Status Effect

| Nguyên tố | Effect        | Mô tả                                      |
| ---------- | ------------- | ------------------------------------------ |
| Lửa        | Burn 🔥       | DOT mỗi giây trong 3s                      |
| Nước       | Wet 💧        | Giảm tốc 15%, tăng sát thương từ Thunder   |
| Sấm        | Shocked ⚡     | Làm choáng 0.5s                            |

#### 4.4. Elemental Reaction (Combo 2 người)

Khi một enemy đang có Status Effect, Player khác đánh bằng element khác → kích hoạt Reaction:

| Effect 1  | Effect 2  | Reaction        | Hiệu ứng                                              |
| ---------- | --------- | --------------- | ----------------------------------------------------- |
| Wet 💧     | Lửa 🔥     | Vaporize 💨     | x2.5 sát thương, xoá Wet                              |
| Wet 💧     | Sấm ⚡      | Conduct ⚡💧     | Stun AOE 4x4 ô, lan Shock sang kẻ địch gần            |
| Burn 🔥    | Sấm ⚡      | Overload 💥     | Nổ AOE lớn (x2 damage), knockback                     |
| Shocked ⚡ | Wet 💧     | Conduct ⚡💧     | (giống trên)                                          |
| Burn 🔥    | Nước 💧    | Vaporize 💨     | (giống trên)                                          |

Server kiểm tra reaction khi nhận damage:

```cpp
StatusEffect CheckReaction(StatusEffect existing, Element incoming)
{
    switch (existing)
    {
    case WET:
        if (incoming == FIRE) return VAPORIZE;
        if (incoming == THUNDER) return CONDUCT;
        break;
    case BURN:
        if (incoming == WATER) return VAPORIZE;
        if (incoming == THUNDER) return OVERLOAD;
        break;
    case SHOCKED:
        if (incoming == WATER) return CONDUCT;
        break;
    }
    return NONE;
}
```

#### 4.5. Tích hợp UI

- Thanh element bên cạnh thanh máu enemy (hiện trạng thái Burn/Wet/Shock).
- Icon nguyên tố trên vũ khí / skill.
- Combo indicator: "VAPORIZE!" hiện khi kích hoạt reaction.

#### 4.6. Cân bằng

| Nguyên tố | Mạnh nhất khi               | Yếu nhất khi             |
| ---------- | --------------------------- | ------------------------ |
| Lửa        | DOT diện rộng               | Kẻ địch miễn nhiễm Burn  |
| Nước       | Hỗ trợ team (tạo Wet setup) | Sát thương thấp đơn mục  |
| Sấm        | Stun lock, burst damage     | Cooldown dài             |

#### 4.7. Multiplayer Sync

- Server là nguồn thẩm quyền cho status effect & reaction.
- Client gửi: `{playerID, element, targetID}`.
- Server tính toán reaction, broadcast kết quả cho cả 2 client.
- Animation effect chạy local (VFX không ảnh hưởng gameplay).

#### Lợi ích

- Tăng chiều sâu combat.
- Khuyến khích phối hợp đồng đội.
- Tận dụng sẵn architecture multiplayer.
- Tạo identity riêng cho game (không phải bản sao Apple Knight thuần).

---

## 5. [Bonus] Song Thế Giới — Level Co-op Độc Quyền (Dual-World Co-op Level)

> ⚠️ **Mức ưu tiên:** Thấp — chỉ phát triển nếu hoàn thành các mục 1–4 trước hạn. Yêu cầu tối thiểu 2 người chơi, không có trong chế độ single.

### Vấn đề

Các màn chơi thông thường trong game gốc đều có thể chơi đơn. Chưa có level nào **tận dụng triệt để** cơ chế multiplayer để tạo trải nghiệm chỉ có thể chơi cùng bạn bè.

### Giải pháp

#### 5.1. Concept — Hai thế giới, một căn phòng

Hai người chơi spawn trong cùng một căn phòng, nhưng ngay khi bước qua **cổng dịch chuyển** (Portal) ở trung tâm — hoặc nếu không có asset portal, bước qua một **vạch phân cách Ánh Sáng / Bóng Tối** — họ bị tách vào 2 chiều không gian khác nhau:

| Người chơi A | Người chơi B |
|---|---|
| 🌞 **Thế Giới Ánh Sáng (Light World)** | 🌑 **Thế Giới Bóng Tối (Shadow World)** |
| Nhìn thấy môi trường đầy đủ ánh sáng | Màn hình tối, chỉ thấy được vùng nhỏ quanh nhân vật |
| Tilemap có nền đất đá, cây cỏ | Cùng tilemap nhưng tối màu, chướng ngại khác |
| Enemy là quái vật bình thường | Enemy là bóng ma chỉ xuất hiện khi đến gần |

**Nguyên tắc cốt lõi:** Cả 2 đứng trong cùng toạ độ map — chỉ khác `WorldLayer` — nên **hành động ở world này gây ra hậu quả vật lý ở world kia**.

#### 5.2. Cơ chế tương tác chéo (Cross-World Interaction)

| Hành động ở World A | Kết quả ở World B |
|---|---|
| Đập vỡ tường gạch | Gạch vụn rơi xuống → hình thành **bậc thang lơ lửng** cho B nhảy lên |
| Bước lên Pressure Plate | **Cầu / nền tảng tạm thời** mọc lên phía B |
| Kéo cần (Lever) | **Cửa / barrier** ở world B mở ra |
| Bật đuốc / kích hoạt Light Orb | Xoá vùng **bóng tối đặc biệt** trên đường đi của B |
| Đánh bại một enemy | Xác enemy hoá thành **khối đẩy (Push Block)** ở world B |
| Nhặt chìa khoá | **Key bản sao** xuất hiện trong rương ở world B |
| Đứng yên trên nền tảng đặc biệt | Nền tảng đó sáng lên → trở thành **điểm đáp an toàn** cho B |
| Rơi xuống hố | Tạo **chấn động địa hình** ở world B (đá rơi, đường tắt) |

```cpp
struct InteractionRule
{
    WorldLayer sourceLayer;
    std::string triggerEvent;   // "wall_destroyed", "lever_pulled", v.v.
    WorldLayer targetLayer;
    std::string effectEvent;    // "spawn_platform", "open_door", v.v.
    Vector2 effectPosition;
};

class CrossWorldManager
{
private:
    std::vector<InteractionRule> rules;
    DualWorldMap* map;

public:
    void RegisterRule(const InteractionRule& rule);
    void OnEvent(WorldLayer source, const std::string& event, Vector2 pos);
    void ApplyEffect(const InteractionRule& rule);
};
```

#### 5.3. Giao tiếp là chìa khoá (Communication-driven)

Đây là điểm đột phá của level này: **không có UI hint, không marker, không mini-map chỉ đường.** Hai người chơi **bắt buộc phải giao tiếp bằng giọng nói ngoài đời thực hoặc chat voice in-game** để vượt qua.

**Ví dụ gameplay loop điển hình:**

```
A: "Bên tao có cây cầu gỗ bên phải, mày thấy gì không?"
B: "Bên tao chỗ đó là vực sâu, chắc cầu vô hình. Tao nhảy thử... được rồi, qua được!"

B: "Bên tao thấy bức tường đen kịt."
A: "Bên tao cũng có tường ở toạ độ đó. Tao đập thử..."
  (CRASH — tường vỡ)
B: "Ô kìa, vụn gạch bay sang đây, xếp thành 3 bậc thang! Tao leo lên được rồi!"

A: "Mày thấy cái cần kéo bên trái không?"
B: "Thấy rồi, để tao kéo."
A: "Ổn, cửa bên tao mở ra rồi, đi tiếp!"
```

#### 5.4. Thiết kế level mẫu: "Hành lang hai chiều"

```
┌─────────────────────────────────────────────┐
│   [Spawn A] → [Cổng dịch chuyển] → [Spawn B] │
│                                            │
│   Khu vực 1: Phòng giao nhau              │
│   ┌────────────┐ ┌────────────┐          │
│   │ A: Cầu gỗ   │ │ B: Vực sâu  │         │
│   │ B: Cầu vô   │ │ A: Tường    │         │
│   │   hình       │ │   gạch     │         │
│   └────────────┘ └────────────┘          │
│         ↓            ↓                   │
│   ┌────────────┐ ┌────────────┐          │
│   │ A: Pressure │ │ B: Cầu mọc  │         │
│   │   Plate     │ │   lên      │         │
│   └────────────┘ └────────────┘          │
│         ↓            ↓                   │
│   ┌─────────────────────────────┐        │
│   │     Phòng Boss kết hợp       │        │
│   │  A đánh Boss quái thường    │        │
│   │  B đánh Boss bóng tối       │        │
│   │  Cả 2 hạ gần cùng lúc mới   │        │
│   │  mở cửa thoát               │        │
│   └─────────────────────────────┘        │
│         ↓            ↓                   │
│     [Exit A]     [Exit B]               │
│    (cần cả 2 đứng vào ô cuối cùng thời)  │
└─────────────────────────────────────────────┘
```

#### 5.5. Entry và Exit

| Giai đoạn | Cơ chế |
|---|---|
| **Entry** | Cả 2 spawn trong phòng chung → bước qua cổng dịch chuyển (Portal). Nếu không có asset portal: bước qua **vạch sáng/tối** phân cách ở giữa phòng và tự động assigned vào world layer. |
| **Kết nối** | Khi cả 2 đã vào đúng layer → thông báo "ĐÃ KẾT NỐI — hãy giao tiếp!" |
| **Exit** | Cuối level có 2 ô bệ thờ riêng cho mỗi player. Cả 2 phải đứng vào ô của mình **đồng thời** → cổng thoát hiện ra ở phòng chờ trung tâm. |

#### 5.6. Technical Design

```cpp
enum class WorldLayer { LIGHT, SHADOW };

class DualWorldMap
{
private:
    std::vector<std::vector<TilePair>> tiles;
    // TilePair = { lightTile, shadowTile }
    std::vector<Entity*> lightEntities;
    std::vector<Entity*> shadowEntities;

public:
    Tile GetTileAt(WorldLayer layer, int x, int y);
    Entity* GetEntityAt(WorldLayer layer, Vector2 pos);
};

class DualWorldPlayer : public Player
{
private:
    WorldLayer currentLayer;

public:
    WorldLayer GetLayer() const;
    void SwitchLayer();
    void Render() override;  // chỉ render layer của mình
};
```

- **Server** chạy cả 2 layers, quyết định cross-world interaction, là thẩm quyền duy nhất.
- **Client** chỉ render layer của player mình + effect từ world kia (khi có cross-world event).
- Cross-world event là packet đặc biệt: `{type, world, position, effect}`.

#### 5.7. Scaling

| Mức | Số phòng | Ghi chú |
|---|---|---|
| Mini | 1–2 | Proof of concept, 1 cơ chế (tường vỡ → bậc thang) |
| Medium | 3–4 | Đủ loop giao tiếp + mini-boss cuối |
| Full | 5–6 | Có plot twist: 2 world bắt đầu đảo lộn, player đổi chỗ |

#### Lợi ích

- **Unique selling point:** Không game nào trong lớp có cơ chế này.
- **Tận dụng tối đa multiplayer:** Level không thể chơi single, tạo động lực rủ bạn bè.
- **Kỷ niệm đáng nhớ:** Những khoảnh khắc "Ê đập tường đi!", "Tao thấy cầu rồi!" — là thứ người chơi nhớ mãi.
- **Thể hiện kỹ thuật:** Dual-layer map, cross-world event system, communication-driven design.
