# CMakeLists.txt for AI Extension Google Mock Unit Tests
# This is a standalone test that can be optionally built

cmake_minimum_required(VERSION 3.13)

project(ai_extension_unittest)

# Option to enable building these tests
option(BUILD_AI_UNITTESTS "Build AI extension unit tests" OFF)

if(NOT BUILD_AI_UNITTESTS)
    return()
endif()

message(STATUS "Test/AI: Unit tests enabled - searching for Google Test...")

# Try find_package first
find_package(GTest QUIET)

# If not found, try manual search for Homebrew paths
if(NOT GTEST_FOUND)
    # Include directories
    find_path(MANUAL_GTEST_INCLUDE_DIR
        NAMES gtest/gtest.h
        PATHS
            /opt/homebrew/include
            /usr/local/include
            /usr/include
        NO_DEFAULT_PATH
    )

    # Libraries
    find_path(MANUAL_GMOCK_INCLUDE_DIR
        NAMES gmock/gmock.h
        PATHS
            /opt/homebrew/include
            /usr/local/include
            /usr/include
        NO_DEFAULT_PATH
    )

    find_library(MANUAL_GTEST_LIB
        NAMES gtest libgtest
        PATHS
            /opt/homebrew/lib
            /usr/local/lib
            /usr/lib
        NO_DEFAULT_PATH
    )

    find_library(MANUAL_GMOCK_LIB
        NAMES gmock libgmock
        PATHS
            /opt/homebrew/lib
            /usr/local/lib
            /usr/lib
        NO_DEFAULT_PATH
    )

    if(MANUAL_GTEST_INCLUDE_DIR AND MANUAL_GMOCK_INCLUDE_DIR AND MANUAL_GTEST_LIB AND MANUAL_GMOCK_LIB)
        set(GTEST_FOUND TRUE)
        set(GTEST_INCLUDE_DIRS "${MANUAL_GTEST_INCLUDE_DIR};${MANUAL_GMOCK_INCLUDE_DIR}")
        set(GTEST_LIBRARIES "${MANUAL_GTEST_LIB};${MANUAL_GMOCK_LIB}")
        message(STATUS "Test/AI: Found Google Test via manual search")
    endif()
endif()

if(NOT GTEST_FOUND)
    message(WARNING "Test/AI: Google Test not found - unit tests disabled")
    message(WARNING "  Install with: brew install googletest")
    return()
endif()

message(STATUS "Test/AI: Building unit tests with Google Mock...")

# Source files - paths from test/ai_unittest directory
set(AI_EXT_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../extension/ai/include")
set(AI_EXT_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../extension/ai/src")
set(AI_UNITTEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../extension/ai/tests/gtest")

# Test source files - only test executor + http_client (not full ai_filter.cpp)
set(AI_UNITTEST_SOURCES
    ${AI_UNITTEST_DIR}/ai_filter_test.cpp
    ${AI_EXT_SRC_DIR}/http_client.cpp
)

# Create test executable
add_executable(ai_extension_unittest ${AI_UNITTEST_SOURCES})

# Include directories for tests
target_include_directories(ai_extension_unittest PRIVATE
    ${AI_EXT_INCLUDE_DIR}
    ${GTEST_INCLUDE_DIRS}
    ${AI_UNITTEST_DIR}
)

# Link Google Test and Google Mock
if(TARGET GTest::gtest)
    target_link_libraries(ai_extension_unittest
        GTest::gtest
        GTest::gmock
        pthread
    )
else()
    target_link_libraries(ai_extension_unittest
        ${GTEST_LIBRARIES}
        pthread
    )
endif()

# Enable coverage for unit tests (Debug builds only)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(ai_extension_unittest PRIVATE
        -fprofile-arcs
        -ftest-coverage
    )
    target_link_options(ai_extension_unittest PRIVATE
        -fprofile-arcs
    )
endif()

# Add test to CTest
enable_testing()
add_test(NAME ai_extension_unittest COMMAND ai_extension_unittest)
set_tests_properties(ai_extension_unittest PROPERTIES
    LABELS "ai;extension;unittest"
    TIMEOUT 30
)

# Custom target to run tests
add_custom_target(unittest_ai
    COMMAND ai_extension_unittest --gtest_color=yes
    DEPENDS ai_extension_unittest
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running AI Extension unit tests..."
)

message(STATUS "Test/AI: Unit tests configured:")
message(STATUS "  - Build target: ai_extension_unittest")
message(STATUS "  - Run target:   unittest_ai")
message(STATUS "  - CTest:        ctest -R ai_extension")
