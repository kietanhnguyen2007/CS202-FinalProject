cmake_minimum_required(VERSION 3.16)
project(AppleKnightAdventure VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(raylib REQUIRED)

file(GLOB_RECURSE SOURCES src/*.cpp include/*.h)

add_executable(AppleKnightAdventure ${SOURCES})

target_include_directories(AppleKnightAdventure PRIVATE include)
target_link_libraries(AppleKnightAdventure PRIVATE raylib)
