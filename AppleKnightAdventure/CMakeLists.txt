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
)
