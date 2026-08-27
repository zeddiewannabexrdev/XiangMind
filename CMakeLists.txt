cmake_minimum_required(VERSION 3.28.2)
project(xiangqi-zeddieengine)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

include(FetchContent)
FetchContent_Declare(
  raylib
  GIT_REPOSITORY https://github.com/raysan5/raylib.git
  GIT_TAG 5.5
)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(raylib)

add_executable(${PROJECT_NAME}
    src/types.cpp
    src/tables.cpp
    src/position.cpp
    src/eval.cpp
    src/see.cpp
    src/tt.cpp
    src/smp.cpp
    src/search.cpp
    src/uci.cpp
    src/main.cpp
    src/datagen.cpp
    src/gui/gui_viewer.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE src src/gui)
target_link_libraries(${PROJECT_NAME} PRIVATE raylib)
