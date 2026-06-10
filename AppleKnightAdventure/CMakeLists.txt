cmake_minimum_required(VERSION 3.16)
project(AppleKnightAdventure VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(raylib REQUIRED)

# Bring in nlohmann_json via FetchContent
include(FetchContent)
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(nlohmann_json)

file(GLOB_RECURSE SOURCES src/*.cpp include/*.h)

add_executable(AppleKnightAdventure ${SOURCES})

target_include_directories(AppleKnightAdventure PRIVATE include)
target_include_directories(AppleKnightAdventure PRIVATE raylib/include)
target_link_libraries(AppleKnightAdventure PRIVATE raylib nlohmann_json::nlohmann_json)

# Test executable for Animator
add_executable(anim_test tests/anim_test.cpp)
target_include_directories(anim_test PRIVATE include)
target_include_directories(anim_test PRIVATE raylib/include)
target_link_libraries(anim_test PRIVATE raylib nlohmann_json::nlohmann_json)
