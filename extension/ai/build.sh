#!/bin/bash
# Build script for AI Extension (MVP)
# This script compiles the AI extension as a loadable extension for testing

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"
EXT_BUILD_DIR="${BUILD_DIR}/extension"
DUCKDB_LIB="${BUILD_DIR}/src/libduckdb_static.a"

echo "=== DuckDB AI Extension Build Script ==="
echo "Project root: ${PROJECT_ROOT}"
echo ""

# Check if DuckDB library exists
if [ ! -f "${DUCKDB_LIB}" ]; then
    echo "❌ DuckDB library not found at: ${DUCKDB_LIB}"
    echo "Please build DuckDB first:"
    echo "  cd ${BUILD_DIR} && make"
    exit 1
fi

# Create build directory for extension
mkdir -p "${EXT_BUILD_DIR}"

# Compilation flags
CXX="clang++"
CXXFLAGS="-std=c++17 -fPIC -O2 -DDUCKDB_EXTENSION_PLATFORM_NAME=osx_arm64"
INCLUDES="-I${PROJECT_ROOT}/extension/ai/include -I${PROJECT_ROOT}/src/include -I${PROJECT_ROOT}/third_party/mbedtls/include"

# Source files
SOURCES="${PROJECT_ROOT}/extension/ai/src/ai_extension.cpp ${PROJECT_ROOT}/extension/ai/src/ai_filter.cpp"

# Output file
OUTPUT="${EXT_BUILD_DIR}/ai.duckdb_extension"

echo "Compiling AI Extension..."
${CXX} ${CXXFLAGS} ${INCLUDES} -shared ${SOURCES} ${DUCKDB_LIB} -o ${OUTPUT}

if [ $? -eq 0 ]; then
    echo ""
    echo "✅ Build successful!"
    echo "Extension location: ${OUTPUT}"
    ls -lh "${OUTPUT}"
    echo ""
    echo "To load in DuckDB:"
    echo "  LOAD '${OUTPUT}';"
else
    echo ""
    echo "❌ Build failed"
    exit 1
fi
