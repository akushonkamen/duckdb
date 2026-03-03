# CMakeLists.txt for Google Test based unit tests
# For DuckDB AI Extension with dependency injection
#
# Usage:
#   cd /path/to/duckdb
#   mkdir -p build && cd build
#   cmake .. -DBUILD_AI_UNITTESTS=ON
#   make ai_filter_gtest
#   ctest -R ai_filter

cmake_minimum_required(VERSION 3.13)

project(ai_filter_unittest)

# Option to enable AI extension unit tests
option(BUILD_AI_UNITTESTS "Build AI extension unit tests" OFF)

if(NOT BUILD_AI_UNITTESTS)
    return()
endif()

# ============================================================================
# Find Google Test (Homebrew on macOS, system packages on Linux)
# ============================================================================

# Try find_package first (works with CMake's FindGTest.cmake)
find_package(GTest)

# If not found, try manual search
if(NOT GTEST_FOUND)
    # Include directories
    find_path(GTEST_INCLUDE_DIR
        NAMES gtest/gtest.h
        PATHS
            /opt/homebrew/include
            /usr/local/include
            /usr/include
            $ENV{HOME}/include
        NO_DEFAULT_PATH
    )

    find_path(GMOCK_INCLUDE_DIR
        NAMES gmock/gmock.h
        PATHS
            /opt/homebrew/include
            /usr/local/include
            /usr/include
            $ENV{HOME}/include
        NO_DEFAULT_PATH
    )

    # Libraries
    find_library(GTEST_LIB
        NAMES gtest libgtest
        PATHS
            /opt/homebrew/lib
            /usr/local/lib
            /usr/lib
            $ENV{HOME}/lib
        NO_DEFAULT_PATH
    )

    find_library(GMOCK_LIB
        NAMES gmock libgmock
        PATHS
            /opt/homebrew/lib
            /usr/local/lib
            /usr/lib
            $ENV{HOME}/lib
        NO_DEFAULT_PATH
    )

    if(GTEST_INCLUDE_DIR AND GMOCK_INCLUDE_DIR AND GTEST_LIB AND GMOCK_LIB)
        set(GTEST_FOUND TRUE)
        set(GTEST_INCLUDE_DIRS "${GTEST_INCLUDE_DIR};${GMOCK_INCLUDE_DIR}")
        set(GTEST_LIBRARIES "${GTEST_LIB};${GMOCK_LIB}")
        message(STATUS "Found Google Test: ${GTEST_INCLUDE_DIR}")
    endif()
endif()

if(NOT GTEST_FOUND)
    message(WARNING "Google Test not found - AI extension unit tests disabled")
    message(WARNING "  Install with: brew install googletest")
    return()
endif()

message(STATUS "Building AI Extension Google Mock unit tests...")

# ============================================================================
# Test Configuration
# ============================================================================

set(AI_EXT_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../include")
set(AI_EXT_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../src")

# Test source files - only test executor + http_client (not full ai_filter.cpp)
# This avoids DuckDB dependencies
set(AI_UNITTEST_SOURCES
    ai_filter_test.cpp
    ${AI_EXT_SRC_DIR}/http_client.cpp
)

# Create test executable
add_executable(ai_filter_gtest ${AI_UNITTEST_SOURCES})

# Include directories
target_include_directories(ai_filter_gtest PRIVATE
    ${AI_EXT_INCLUDE_DIR}
    ${GTEST_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}  # For mock headers
)

# Link libraries
if(TARGET GTest::gtest)
    # Use CMake imported targets
    target_link_libraries(ai_filter_gtest
        GTest::gtest
        GTest::gmock
        pthread
    )
else()
    # Use manual library paths
    target_link_libraries(ai_filter_gtest
        ${GTEST_LIBRARIES}
        pthread
    )
endif()

# Compiler flags for coverage
target_compile_options(ai_filter_gtest PRIVATE
    -fPIC
    -O0
    -g
    --coverage
)

# Link options for coverage
target_link_options(ai_filter_gtest PRIVATE
    --coverage
)

# ============================================================================
# Test Integration
# ============================================================================

include(CTest)

# Add test to CTest
add_test(NAME ai_filter_gtest COMMAND ai_filter_gtest)

# Set test properties
set_tests_properties(ai_filter_gtest PROPERTIES
    LABELS "ai;extension;unittest"
    TIMEOUT 30
)

# Custom target to run tests
add_custom_target(run_ai_unittest
    COMMAND ai_filter_gtest --gtest_color=yes
    DEPENDS ai_filter_gtest
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running AI Extension unit tests..."
)

# Custom target for coverage report
add_custom_target(coverage_ai_unittest
    COMMAND ${CMAKE_COMMAND} -E env LC_ALL=C gcov ${AI_UNITTEST_SOURCES} | grep -E "^File.*ai_function_executor|^File.*http_client"
    DEPENDS ai_filter_gtest
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating coverage report for AI unit tests..."
)

message(STATUS "AI Extension unit tests configured:")
message(STATUS "  Build target: ai_filter_gtest")
message(STATUS "  Run target:   run_ai_unittest")
message(STATUS "  Coverage:     coverage_ai_unittest")
message(STATUS "  CTest:        ctest -R ai_filter")
