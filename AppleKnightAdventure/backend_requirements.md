# Yêu cầu cập nhật cho Backend

Tài liệu này liệt kê các thay đổi cần thiết trong thư mục `Model` (cụ thể là `Player`) để hỗ trợ giao diện (HUD) hiển thị các thanh chỉ số mới: **SP, MP, và Ultimate Charge**. Hiện tại HUD đang sử dụng *mock data* để vẽ giao diện.

## File cần sửa: `include/Model/Player.h`
Cần khai báo thêm các biến trạng thái và hàm Getter/Setter:

```cpp
// Trong phần protected:
int m_mp;
int m_maxMp;

int m_sp;
int m_maxSp;

float m_ultimateCharge;
float m_maxUltimateCharge;

// Trong phần public (Getter/Setter):
int GetMP() const;
void SetMP(int mp);
int GetMaxMP() const;
void SetMaxMP(int maxMp);

int GetSP() const;
void SetSP(int sp);
int GetMaxSP() const;
void SetMaxSP(int maxSp);

float GetUltimateCharge() const;
void SetUltimateCharge(float charge);
float GetMaxUltimateCharge() const;
void SetMaxUltimateCharge(float maxCharge);
```

## File cần sửa: `src/Model/Player.cpp`
Cần khởi tạo giá trị và thực thi các hàm Getter/Setter:

- **Constructor**: Cần khởi tạo `m_mp = 100`, `m_maxMp = 100`, `m_sp = 100`, `m_maxSp = 100`, `m_ultimateCharge = 0.0f`, `m_maxUltimateCharge = 100.0f` (hoặc con số phù hợp với thiết kế game).
- **Hàm Getter/Setter**: Trả về và gán giá trị tương ứng. Cần đảm bảo giá trị không vượt quá Max khi Set.
- **Update Logic**: Cập nhật logic để hồi SP/MP tự động theo thời gian nếu cần thiết, hoặc logic tăng `m_ultimateCharge` khi đánh trúng kẻ thù.

## Tích hợp vào HUD
Khi Backend đã bổ sung xong, vui lòng mở file `src/View/HUDView.cpp`:
1. Xóa các dòng khai báo mock data (ví dụ `int mockMP = 50;`).
2. Thay thế bằng các lệnh gọi hàm thực tế từ `m_player`, ví dụ: `int mp = m_player->GetMP();`.
