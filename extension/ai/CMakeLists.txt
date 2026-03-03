cmake_minimum_required(VERSION 3.15)

project(AIExtension)

# Include directories
include_directories(include)

# Source files for AI Extension
set(AI_EXTENSION_FILES
    src/ai_extension.cpp
    src/ai_filter.cpp
    src/ai_aggregate.cpp
    src/ai_similarity.cpp
    src/http_client.cpp
)

# Build static extension (for linking into DuckDB)
build_static_extension(ai ${AI_EXTENSION_FILES})

# Build loadable extension (standalone .duckdb_extension file)
set(PARAMETERS "-warnings")
build_loadable_extension(ai ${PARAMETERS} ${AI_EXTENSION_FILES})

# ============================================================================
# Google Mock Unit Tests
# ============================================================================

# Option to enable AI extension unit tests (default: OFF)
option(AI_EXTENSION_BUILD_UNITTESTS "Build AI extension Google Mock unit tests" OFF)

if(AI_EXTENSION_BUILD_UNITTESTS AND NOT WIN32)
    message(STATUS "AI Extension: Unit tests enabled - searching for Google Test...")

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
            message(STATUS "AI Extension: Found Google Test via manual search")
        endif()
    endif()

    if(GTEST_FOUND)
        message(STATUS "AI Extension: Building unit tests with Google Mock...")

        # Test source files - only test executor + http_client (not full ai_filter.cpp)
        # This avoids DuckDB dependencies
        set(AI_UNITTEST_SOURCES
            tests/gtest/ai_filter_test.cpp
            src/http_client.cpp
        )

        # Create test executable
        add_executable(ai_extension_unittest ${AI_UNITTEST_SOURCES})

        # Include directories for tests
        target_include_directories(ai_extension_unittest PRIVATE
            include
            ${GTEST_INCLUDE_DIRS}
            tests/gtest
        )

        # Link Google Test and Google Mock
        if(TARGET GTest::gtest)
            # Use CMake imported targets
            target_link_libraries(ai_extension_unittest
                GTest::gtest
                GTest::gmock
                pthread
            )
        else()
            # Use manual library paths
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
        include(CTest)
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

        message(STATUS "AI Extension: Unit tests configured:")
        message(STATUS "  - Build target: ai_extension_unittest")
        message(STATUS "  - Run target:   unittest_ai or make test")
        message(STATUS "  - CTest:        ctest -R ai_extension")
    else()
        message(WARNING "AI Extension: Google Test not found - unit tests disabled")
        message(WARNING "  Install with: brew install googletest")
        message(WARNING "  Or use: apt-get install libgtest-dev libgmock-dev")
    endif()
else()
    if(NOT WIN32)
        message(STATUS "AI Extension: Unit tests disabled (enable with -DAI_EXTENSION_BUILD_UNITTESTS=ON)")
    endif()
endif()

# Install extension
install(
  TARGETS ai_extension
  EXPORT "${DUCKDB_EXPORT_SET}"
  LIBRARY DESTINATION "${INSTALL_LIB_DIR}"
  ARCHIVE DESTINATION "${INSTALL_LIB_DIR}")
