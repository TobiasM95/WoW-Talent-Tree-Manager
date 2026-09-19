# Portable build for the TTM engine and its headless CLI.
#
# This exists alongside "WoW Talent Manager.sln", which still builds the native
# Dear ImGui client on Windows. CMake deliberately covers ONLY Engine + CLI: those
# are what run server-side as the solver worker. The GUI and AppUpdater stay
# MSVC-only.
#
#   cmake -B build -DCMAKE_BUILD_TYPE=Release
#   cmake --build build -j
#
# Produces the `ttm-solver` binary.

cmake_minimum_required(VERSION 3.16)

project(ttm
    VERSION 1.4.2
    DESCRIPTION "WoW Talent Tree Manager engine and solver CLI"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to Release; an unspecified build type yields an unoptimised solver,
# which for this workload is the difference between seconds and minutes.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    message(STATUS "CMAKE_BUILD_TYPE not set, defaulting to Release")
endif()

find_package(Threads REQUIRED)

# ---------------------------------------------------------------------------
# ttm_engine -- tree model, serialisation, and the variant-counting solver
# ---------------------------------------------------------------------------

add_library(ttm_engine STATIC
    Engine/src/TalentTrees.cpp
    Engine/src/TreeSolver.cpp
    Engine/src/TTMEnginePresets.cpp)

target_include_directories(ttm_engine PUBLIC Engine/src)
target_link_libraries(ttm_engine PUBLIC Threads::Threads)

if(MSVC)
    target_compile_options(ttm_engine PRIVATE /W3)
else()
    # -Wno-unused-result: the engine ignores some stream return values by design.
    target_compile_options(ttm_engine PRIVATE -Wall -Wextra
        -Wno-unused-parameter -Wno-unused-result -Wno-sign-compare)
endif()

# ---------------------------------------------------------------------------
# ttm-solver -- headless entry point, invoked as a worker process
# ---------------------------------------------------------------------------

add_executable(ttm-solver CLI/CLI.cpp)
target_include_directories(ttm-solver PRIVATE CLI)
target_link_libraries(ttm-solver PRIVATE ttm_engine)

set_target_properties(ttm-solver PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

install(TARGETS ttm-solver RUNTIME DESTINATION bin)
