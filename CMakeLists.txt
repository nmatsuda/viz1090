cmake_minimum_required(VERSION 3.14)
project(viz1090
  VERSION 0.2.0
  DESCRIPTION "ADSB aircraft visualization"
  LANGUAGES CXX
)

# C++17 required
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for IDE integration
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build type defaults to Release
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Profiling option
option(ENABLE_PROFILING "Enable performance profiling output" OFF)
if(ENABLE_PROFILING)
  add_compile_definitions(PROFILING_ENABLED)
  message(STATUS "Profiling enabled - performance stats will be output to stderr")
endif()

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)
pkg_check_modules(SDL2_GFX REQUIRED SDL2_gfx)

# Fetch standalone ASIO (header-only)
include(FetchContent)
FetchContent_Declare(
  asio
  GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
  GIT_TAG asio-1-28-0
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(asio)

# Create ASIO interface library
add_library(asio INTERFACE)
target_include_directories(asio INTERFACE ${asio_SOURCE_DIR}/asio/include)
target_compile_definitions(asio INTERFACE ASIO_STANDALONE)

# Source files organized by module
set(APP_SOURCES
  src/app/viz1090.cpp
  src/app/AppData.cpp
  src/app/Config.cpp
)

set(CORE_SOURCES
  src/core/Aircraft.cpp
  src/core/AircraftList.cpp
)

set(DECODER_SOURCES
  src/decoder/ModeSDecoder.cpp
)

set(NETWORK_SOURCES
  src/network/BeastClient.cpp
  src/network/ConnectionManager.cpp
)

set(UI_SOURCES
  src/ui/View.cpp
  src/ui/Input.cpp
  src/ui/Map.cpp
  src/ui/Label.cpp
  src/ui/AircraftLabel.cpp
  src/ui/AircraftViewState.cpp
  src/ui/MapView.cpp
  src/ui/AircraftRenderer.cpp
  src/ui/UIOverlay.cpp
  src/ui/InputFeedback.cpp
  src/ui/Button.cpp
  src/ui/MenuPanel.cpp
)

set(STYLE_SOURCES
  src/style/StyleManager.cpp
)

# Main executable
add_executable(viz1090
  ${APP_SOURCES}
  ${CORE_SOURCES}
  ${DECODER_SOURCES}
  ${NETWORK_SOURCES}
  ${UI_SOURCES}
  ${STYLE_SOURCES}
)

target_include_directories(viz1090 PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${SDL2_INCLUDE_DIRS}
  ${SDL2_TTF_INCLUDE_DIRS}
  ${SDL2_GFX_INCLUDE_DIRS}
)

target_link_libraries(viz1090 PRIVATE
  ${SDL2_LIBRARIES}
  ${SDL2_TTF_LIBRARIES}
  ${SDL2_GFX_LIBRARIES}
  pthread
  m
)

# GCC 8 requires explicit linking of stdc++fs for std::filesystem
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
  target_link_libraries(viz1090 PRIVATE stdc++fs)
endif()

# Link ASIO for future network migration
target_link_libraries(viz1090 PRIVATE asio)

# Copy resources to build directory
add_custom_command(TARGET viz1090 POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/font
    $<TARGET_FILE_DIR:viz1090>/font
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/themes
    $<TARGET_FILE_DIR:viz1090>/themes
  COMMENT "Copying resources to build output"
)

# Copy map data to build directory if it exists
set(MAPDATA_FILES
  ${CMAKE_SOURCE_DIR}/mapdata.bin
  ${CMAKE_SOURCE_DIR}/mapnames
  ${CMAKE_SOURCE_DIR}/airportdata.bin
  ${CMAKE_SOURCE_DIR}/airportnames
)

foreach(MAPFILE ${MAPDATA_FILES})
  if(EXISTS ${MAPFILE})
    get_filename_component(MAPFILE_NAME ${MAPFILE} NAME)
    add_custom_command(TARGET viz1090 POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${MAPFILE}
        $<TARGET_FILE_DIR:viz1090>/${MAPFILE_NAME}
      COMMENT "Copying ${MAPFILE_NAME} to build output"
    )
  endif()
endforeach()

# Custom target to download and generate map data
# Usage: cmake --build . --target mapdata
# Requires: Python 3 with fiona, shapely, numpy, tqdm
# The script handles missing data sources gracefully
# Output goes directly to build directory so executable can find it
add_custom_target(mapdata
  COMMAND bash ${CMAKE_SOURCE_DIR}/src/tools/getmap.sh $<TARGET_FILE_DIR:viz1090>
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  COMMENT "Downloading and generating map data (requires Python 3 with fiona, shapely, numpy, tqdm)"
  VERBATIM
)

# Installation
install(TARGETS viz1090 RUNTIME DESTINATION bin)
install(DIRECTORY font/ DESTINATION share/viz1090/font)
install(DIRECTORY themes/ DESTINATION share/viz1090/themes)

# Testing
option(BUILD_TESTS "Build unit tests" ON)

if(BUILD_TESTS)
  # Fetch Google Test
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
    GIT_SHALLOW TRUE
  )
  # Prevent overriding parent project's compiler/linker settings on Windows
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)

  enable_testing()

  # Test sources
  set(TEST_SOURCES
    tests/test_crc.cpp
    tests/test_modes_decoder.cpp
    tests/test_cpr_decoder.cpp
    tests/test_mode_ac_decoder.cpp
  )

  # Test executable
  add_executable(viz1090_tests
    ${TEST_SOURCES}
    ${DECODER_SOURCES}
    src/app/Config.cpp
  )

  target_include_directories(viz1090_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  )

  target_link_libraries(viz1090_tests PRIVATE
    GTest::gtest_main
    pthread
  )

  include(GoogleTest)
  gtest_discover_tests(viz1090_tests)
endif()
