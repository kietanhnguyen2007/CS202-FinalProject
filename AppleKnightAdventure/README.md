# Apple Knight Adventure

Đây là project cuối kỳ học phần CS202.

## Hướng dẫn Build (Dành cho Giáo viên)

Project này sử dụng **CMake FetchContent**, toàn bộ thư viện cần thiết (Raylib, Nlohmann JSON) sẽ được tải tự động trong lúc cấu hình. Không cần phải cài đặt sẵn các thư viện này hoặc tải file binary thủ công.

### Yêu cầu hệ thống:
1. **CMake** (phiên bản 3.14 trở lên)
2. **Git** (để FetchContent có thể clone thư viện)
3. **Trình biên dịch C++** hỗ trợ C++17 (chẳng hạn như: GCC/MinGW, MSVC (Visual Studio), hoặc Clang)
4. **Kết nối mạng** (chỉ yêu cầu ở lần chạy CMake đầu tiên để tải thư viện)

### Các bước Build bằng dòng lệnh:

Mở Terminal / PowerShell / Command Prompt ở thư mục chứa file `CMakeLists.txt` này:

```bash
# 1. Cấu hình project (Tự động tải thư viện và detect compiler)
cmake -B build

# 2. Biên dịch
cmake --build build

# 3. Chạy game (tùy theo HĐH/compiler, đường dẫn có thể khác biệt đôi chút)
# Windows:
.\build\AppleKnightAdventure.exe
```

**Hoặc dùng CMakePresets (nếu dùng CMake 3.20+):**
```bash
cmake --preset default
cmake --build --preset default
```

### Build bằng VS Code:
- Cài extension **CMake Tools**
- Mở thư mục bằng VS Code
- Chuyển `CMake: Build Target` sang `AppleKnightAdventure`
- Nhấn **Build** trên thanh trạng thái (hoặc dùng tổ hợp phím F7)
- Nhấn **Launch** để chạy

## Lưu ý về Backend Server
Game bao gồm một tính năng `Survival3D` có gọi tới một server backend (`AegisRiftServer`) và kết nối thông qua Windows HTTP (`winhttp`).
Server này chỉ hỗ trợ và sẽ tự động được biên dịch khi đang dùng hệ điều hành **Windows**.
