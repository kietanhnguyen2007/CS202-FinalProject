# 1. Project Setup
cmake_minimum_required(VERSION 3.10)
project(AppleKnightAdventure)

# 2. C++ Standard Configuration
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # Helpful for VS Code IntelliSense

# 3. Third-Party Dependencies (Remote)
include(FetchContent)

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

# 6. Add include directories (Project headers + Local Raylib headers)
target_include_directories(AppleKnightAdventure PRIVATE 
    include 
    raylib/include
)

# 7. Add library directories (Local Raylib library files)
target_link_directories(AppleKnightAdventure PRIVATE raylib/lib)

# 8. Link your libraries (JSON, Local Raylib, and required Windows libraries)
target_link_libraries(AppleKnightAdventure PRIVATE 
    nlohmann_json::nlohmann_json 
    raylib 
    opengl32 
    gdi32 
    winmm
    comdlg32
    winhttp
)

# Fix for MinGW 14.2.0 / UCRT stat64i32 linking error with older raylib binary
if(MINGW)
    target_link_options(AppleKnightAdventure PRIVATE "-Wl,--defsym,stat64i32=_stat64i32")
    # Bundle the GCC/C++ runtime into the executable so it launches without
    # C:\msys64\...\bin on PATH (double-click, VS Code, PowerShell, other PCs).
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
