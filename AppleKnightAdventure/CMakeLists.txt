# 1. Project Setup
cmake_minimum_required(VERSION 3.10)
project(AppleKnightAdventure)

# 2. C++ Standard Configuration
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # Helpful for VS Code IntelliSense

# 3. Third-Party Dependencies (Remote)
include(FetchContent)

# Fetch nlohmann_json
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

# Fetch raylib (fixes MinGW stat64i32 undefined reference by building from source)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.0
)
FetchContent_MakeAvailable(raylib)

# 4. Find all source files safely
# Watches the src tree recursively. If you add a new .cpp file, CMake will auto-reconfigure!
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

# 5. Create the main executable
add_executable(AppleKnightAdventure ${SRC_FILES})

# 6. Add include directories (Project headers)
target_include_directories(AppleKnightAdventure PRIVATE 
    include 
)

# Note: raylib include and lib directories are automatically handled by target_link_libraries

# 8. Link your libraries (JSON, Local Raylib, and required Windows libraries)
target_link_libraries(AppleKnightAdventure PRIVATE 
    nlohmann_json::nlohmann_json 
    raylib 
    opengl32 
    gdi32 
    winmm
    ws2_32
)

# 9. Create symlink/junction for assets in the build folder
if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/assets")
    if(WIN32)
        # Convert forward slashes to native backslashes for cmd compatibility
        file(TO_NATIVE_PATH "${CMAKE_CURRENT_BINARY_DIR}/assets" NATIVE_LINK)
        file(TO_NATIVE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/assets" NATIVE_TARGET)
        # Use a directory junction on Windows (no admin privileges required)
        execute_process(COMMAND cmd /c mklink /j "${NATIVE_LINK}" "${NATIVE_TARGET}")
    else()
        execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink "${CMAKE_CURRENT_SOURCE_DIR}/assets" "${CMAKE_CURRENT_BINARY_DIR}/assets")
    endif()
endif()