cmake_minimum_required(VERSION 3.28.2)
project(xiangqi-zeddieengine)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

add_executable(${PROJECT_NAME}
    src/types.cpp
    src/tables.cpp
    src/position.cpp
    src/eval.cpp
    src/search.cpp
    src/uci.cpp
    src/main.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE src)
