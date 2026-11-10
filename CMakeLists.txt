# Spintacular's CMake project. The engine is an Automap composition:
# cmake/automap/automap.cmake brings in the libraries and the glue.
cmake_minimum_required(VERSION 3.25)
project(spintacular LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# Everything lands beside the executable, so the game finds its assets and DLLs.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

include(cmake/automap/automap.cmake)

if(MSVC)
    add_compile_options(/W3 /utf-8)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS NOMINMAX)
else()
    add_compile_options(-Wall -Wextra)
endif()

# The rules core: the C++ standard library only (docs/design/tech.md).
add_library(spin_rules STATIC game/rules/run.cpp game/rules/tables.cpp)
target_include_directories(spin_rules PUBLIC game)

# The data layer: tables, saves and the profile, as JSON.
add_library(spin_data STATIC game/data/tables_json.cpp game/data/save.cpp game/data/profile.cpp)
target_link_libraries(spin_data PUBLIC spin_rules nlohmann_json::nlohmann_json)

# The balance simulation: bots play the real rules and check the session rubric.
find_package(Threads REQUIRED)
add_executable(spintacular-sim game/sim/main.cpp game/sim/sim.cpp game/sim/bots.cpp)
target_link_libraries(spintacular-sim PRIVATE spin_data Threads::Threads)

add_executable(spintacular-tests tests/rules_tests.cpp)
target_link_libraries(spintacular-tests PRIVATE spin_data)

# The game.
file(GLOB SPIN_APP_SOURCES CONFIGURE_DEPENDS game/app/*.cpp)
add_executable(spintacular ${SPIN_APP_SOURCES})
target_link_libraries(spintacular PRIVATE automap_glue spin_data)
if(MSVC)
    # No console window in a release build; main() stays the entry point.
    target_link_options(spintacular PRIVATE $<$<CONFIG:Release>:/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup>)
endif()
add_custom_command(TARGET spintacular POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets ${CMAKE_BINARY_DIR}/assets
    COMMENT "Copying assets beside the executable")

enable_testing()
add_test(NAME rules COMMAND spintacular-tests ${CMAKE_SOURCE_DIR}/assets/data/tables.json)
add_test(NAME selftest COMMAND spintacular-sim selftest --tables ${CMAKE_SOURCE_DIR}/assets/data/tables.json)
