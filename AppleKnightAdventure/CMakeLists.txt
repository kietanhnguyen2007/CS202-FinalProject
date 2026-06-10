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

# Build raylib from source via FetchContent
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
set(INSTALL_RAYLIB OFF CACHE BOOL "" FORCE)
set(RAYLIB_BUILD_TYPE STATIC CACHE STRING "")

FetchContent_Declare(
  raylib
  GIT_REPOSITORY https://github.com/raysan5/raylib.git
  GIT_TAG 5.5
)
FetchContent_MakeAvailable(raylib)

file(GLOB_RECURSE SOURCES src/*.cpp include/*.h)

add_executable(AppleKnightAdventure ${SOURCES})

target_include_directories(AppleKnightAdventure PRIVATE include)
target_link_libraries(AppleKnightAdventure PRIVATE raylib nlohmann_json::nlohmann_json)

# Test executable for Animator
add_executable(anim_test
  tests/anim_test.cpp
  src/Systems/AnimationSystem.cpp
  src/Systems/TextureAtlas.cpp
)
target_include_directories(anim_test PRIVATE include)
target_link_libraries(anim_test PRIVATE raylib nlohmann_json::nlohmann_json)
