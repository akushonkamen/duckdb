#!/bin/bash
# Build script for AI Extension (Loadable) - MVP
# Follows the pattern of test/extension/loadable_extension_demo

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"
EXT_BUILD_DIR="${BUILD_DIR}/extension"

echo "=== DuckDB AI Extension Build (Loadable) ==="
echo "Project root: ${PROJECT_ROOT}"
echo ""

# Create build directory for extension
mkdir -p "${EXT_BUILD_DIR}"

# Use DuckDB's build_loadable_extension_directory function
cd "${PROJECT_ROOT}"

# Build the extension using the build system
cmake --build build --target ai_loadable_extension 2>&1 || {
    echo ""
    echo "❌ Build failed - extension not in main build targets"
    echo "Trying alternative: manual compilation..."
    ""

    # Fallback: Manual compilation following test/extension pattern
    CXX="clang++"
    CXXFLAGS="-std=c++17 -fPIC -O2 -DDUCKDB_EXTENSION_PLATFORM_NAME=osx_arm64 -DDUCKDB_BUILD_LOADABLE_EXTENSION"
    INCLUDES="-I${PROJECT_ROOT}/src/include -I${PROJECT_ROOT}/third_party/mbedtls/include"
    LDFLAGS="-undefined dynamic_lookup"

    OUTPUT="${EXT_BUILD_DIR}/ai.duckdb_extension"

    echo "Compiling: ai_extension_loadable.cpp"
    ${CXX} ${CXXFLAGS} ${INCLUDES} ${LDFLAGS} -shared \
        extension/ai/ai_extension_loadable.cpp \
        -o ${OUTPUT}

    if [ $? -eq 0 ]; then
        echo ""
        echo "✅ Manual build successful!"
        echo "Extension: ${OUTPUT}"
        ls -lh "${OUTPUT}"
    else
        echo "❌ Manual build failed"
        exit 1
    fi

    exit 0
}

echo ""
echo "✅ Build successful!"
echo "Extension location: ${EXT_BUILD_DIR}/ai.duckdb_extension"
