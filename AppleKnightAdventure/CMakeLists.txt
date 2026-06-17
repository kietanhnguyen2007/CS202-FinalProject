cmake_minimum_required(VERSION 3.16)
project(AppleKnightAdventure VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Bring in nlohmann_json via FetchContent
include(FetchContent)
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(nlohmann_json)

# Link local raylib (dynamic)
add_library(raylib SHARED IMPORTED)
set_target_properties(raylib PROPERTIES
  IMPORTED_IMPLIB "${CMAKE_SOURCE_DIR}/raylib/lib/libraylibdll.a"
  IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/raylib/lib/raylib.dll"
  INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/raylib/include"
)

file(GLOB_RECURSE SOURCES src/*.cpp include/*.h)

add_executable(AppleKnightAdventure ${SOURCES})
set_target_properties(AppleKnightAdventure PROPERTIES WIN32_EXECUTABLE FALSE)

target_include_directories(AppleKnightAdventure PRIVATE include)
target_link_libraries(AppleKnightAdventure PRIVATE raylib nlohmann_json::nlohmann_json winmm ws2_32)
target_link_options(AppleKnightAdventure PRIVATE -mconsole)

add_custom_command(TARGET AppleKnightAdventure POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_SOURCE_DIR}/raylib/lib/raylib.dll"
    $<TARGET_FILE_DIR:AppleKnightAdventure>
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/assets"
    "$<TARGET_FILE_DIR:AppleKnightAdventure>/assets"
)

# Test Visual Target
# Exclude main.cpp from SOURCES to prevent multiple main definitions
set(TEST_SOURCES ${SOURCES})
list(FILTER TEST_SOURCES EXCLUDE REGEX "src/main.cpp")

add_executable(test_visual
    tests/main_test.cpp
    tests/TestController.cpp
    tests/TestScene.cpp
    tests/scenes/TestAssetLoad.cpp
    tests/scenes/TestBackgroundTilemap.cpp
    tests/scenes/TestBossAnim.cpp
    tests/scenes/TestVFX.cpp
    tests/scenes/TestRangedBomb.cpp
    tests/scenes/TestPetScene.cpp
    tests/scenes/TestPlayerAnim.cpp
    tests/scenes/TestEnemyAnim.cpp
    tests/scenes/TestHUDView.cpp
    tests/scenes/TestInventoryView.cpp
    tests/scenes/TestMenuView.cpp
    tests/scenes/TestResultView.cpp
    tests/scenes/TestSkillBarView.cpp
    ${TEST_SOURCES}
)

target_include_directories(test_visual PRIVATE include tests)
target_link_libraries(test_visual PRIVATE raylib nlohmann_json::nlohmann_json winmm ws2_32)
set_target_properties(test_visual PROPERTIES WIN32_EXECUTABLE FALSE)
target_link_options(test_visual PRIVATE -mconsole)

add_custom_command(TARGET test_visual POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_SOURCE_DIR}/raylib/lib/raylib.dll"
    $<TARGET_FILE_DIR:test_visual>
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/assets"
    "$<TARGET_FILE_DIR:test_visual>/assets"
)
