# Survival3D C++ service

REST server thuần C++17 cho M6. Server dùng Winsock, `nlohmann::json` và một file JSON ghi nguyên tử; không cần Python, FastAPI, SQLite hay runtime riêng.

```powershell
cmake --build build --target AegisRiftServer AegisRiftServerTests
.\build\AegisRiftServerTests.exe
.\build\AegisRiftServer.exe 8080 .\survival3d_data.json
```

Các route giữ nguyên contract với `SurvivalRunService` WinHTTP: guest auth, profile, idempotent run completion và score leaderboard. Client vẫn mặc định offline; đổi `enabled` thành `true` trong `assets/survival3d/config/services.json` để nối server local.
