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
    ws2_32
    comdlg32
    winhttp
)

# Fix for MinGW 14.2.0 / UCRT stat64i32 linking error with older raylib binary
if(MINGW)
    target_link_options(AppleKnightAdventure PRIVATE "-Wl,--defsym,stat64i32=_stat64i32")
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

# 10. Pure C++17 local service for the Survival3D M6 REST contract.
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

    add_executable(AegisRiftServerTests
        backend/survival3d/tests/SurvivalServerTests.cpp
        backend/survival3d/SurvivalServerCore.cpp
    )
    target_include_directories(AegisRiftServerTests PRIVATE backend/survival3d)
    target_link_libraries(AegisRiftServerTests PRIVATE nlohmann_json::nlohmann_json)

    enable_testing()
    add_test(NAME AegisRiftServerTests COMMAND AegisRiftServerTests)

    add_executable(Survival3DConfigTests tests/Survival3DConfigTests.cpp)
    target_link_libraries(Survival3DConfigTests PRIVATE nlohmann_json::nlohmann_json)
    add_test(NAME Survival3DConfigTests
        COMMAND Survival3DConfigTests "${CMAKE_CURRENT_SOURCE_DIR}/../assets/survival3d")

    add_executable(AnimationGraphTests
        tests/AnimationGraphTests.cpp
        src/Survival3D/Animation/AnimationGraph.cpp
    )
    target_include_directories(AnimationGraphTests PRIVATE include)
    add_test(NAME AnimationGraphTests COMMAND AnimationGraphTests)

    add_executable(AnimationEventsIKTests
        tests/AnimationEventsIKTests.cpp
        src/Survival3D/Animation/AnimationEvents.cpp
        src/Survival3D/Systems/RuntimeIK.cpp
    )
    target_include_directories(AnimationEventsIKTests PRIVATE include)
    add_test(NAME AnimationEventsIKTests COMMAND AnimationEventsIKTests)

    add_executable(Step10ProductionTests
        tests/Step10ProductionTests.cpp
        src/Survival3D/Animation/AnimationGraph.cpp
        src/Survival3D/Animation/AnimationEvents.cpp
        src/Survival3D/Systems/RuntimeIK.cpp
    )
    target_include_directories(Step10ProductionTests PRIVATE include)
    add_test(NAME Step10ProductionTests
        COMMAND Step10ProductionTests "${CMAKE_CURRENT_SOURCE_DIR}/..")

    add_executable(VfxRuntimeTests
        tests/VfxRuntimeTests.cpp
        src/Survival3D/Vfx/VfxRuntime.cpp
        src/Survival3D/Vfx/SkillVfxPackages.cpp
    )
    target_include_directories(VfxRuntimeTests PRIVATE include)
    add_test(NAME VfxRuntimeTests COMMAND VfxRuntimeTests)

    foreach(SURVIVAL_TEST_TARGET
            AnimationGraphTests AnimationEventsIKTests Step10ProductionTests
            VfxRuntimeTests)
        if(MSVC)
            target_compile_options(${SURVIVAL_TEST_TARGET} PRIVATE /W4)
        else()
            target_compile_options(${SURVIVAL_TEST_TARGET} PRIVATE
                -Wall -Wextra -Wpedantic -Werror)
        endif()
    endforeach()
endif()
