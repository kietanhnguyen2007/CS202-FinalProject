# 1. Project Setup
cmake_minimum_required(VERSION 3.10)
project(AppleKnightAdventure)

# 2. C++ Standard Configuration
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # Helpful for VS Code IntelliSense

# 3. Third-Party Dependencies (Remote)
include(FetchContent)

# Tắt build shared lib và examples của raylib
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# Fetch raylib 6.0
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 6.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(raylib)

# Fetch nlohmann_json (Fixed Version)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

# 4. Find all source files safely
# Watches the src tree recursively. If you add a new .cpp file, CMake will auto-reconfigure!
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

# 5. Create the main executable
add_executable(AppleKnightAdventure ${SRC_FILES})

# 6. Thêm include directories (Project headers)
target_include_directories(AppleKnightAdventure PRIVATE include)

# 7. Cấu hình compiler options (Cross-platform)
if(MSVC)
    target_compile_options(AppleKnightAdventure PRIVATE /W3)
else()
    target_compile_options(AppleKnightAdventure PRIVATE -Wall -Wextra)
endif()

# 8. Link thư viện
target_link_libraries(AppleKnightAdventure PRIVATE 
    nlohmann_json::nlohmann_json 
    raylib
    winhttp
)

# 9. Tùy chỉnh riêng cho MinGW (nếu dùng MinGW)
if(MINGW)
    # Bundle runtime để chạy không cần MSYS2/MinGW trong PATH
    target_link_options(AppleKnightAdventure PRIVATE -static -static-libgcc -static-libstdc++)
endif()

# 9. Keep runtime assets in sync even when only an LDtk/image/audio file changes.
# A POST_BUILD command alone does not run when the executable is already up to date.
file(GLOB_RECURSE GAME_ASSET_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/../assets/*"
)
set(ASSET_SYNC_STAMP "${CMAKE_BINARY_DIR}/assets_sync.stamp")
add_custom_command(
    OUTPUT "${ASSET_SYNC_STAMP}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/../assets"
            "${CMAKE_BINARY_DIR}/assets"
    COMMAND ${CMAKE_COMMAND} -E touch "${ASSET_SYNC_STAMP}"
    DEPENDS ${GAME_ASSET_FILES}
    COMMENT "Syncing runtime assets"
    VERBATIM
)
add_custom_target(SyncAssets ALL DEPENDS "${ASSET_SYNC_STAMP}")
add_dependencies(AppleKnightAdventure SyncAssets)

# 10. Optional local service for Survival3D run validation and leaderboard sync.
if(WIN32)
    add_executable(AegisRiftServer
        backend/survival3d/main.cpp
        backend/survival3d/SurvivalServerCore.cpp
    )
    target_include_directories(AegisRiftServer PRIVATE backend/survival3d)
    target_link_libraries(AegisRiftServer PRIVATE
        nlohmann_json::nlohmann_json
        ws2_32
    )
    if(MINGW)
        target_link_options(AegisRiftServer PRIVATE -static -static-libgcc -static-libstdc++)
    endif()
endif()
