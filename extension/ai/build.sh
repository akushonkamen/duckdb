#!/bin/bash
# Build script for AI Extension (MVP)
# This script compiles the AI extension as a loadable extension for testing

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"
EXT_BUILD_DIR="${BUILD_DIR}/extension/ai"
DUCKDB="${BUILD_DIR}/duckdb"

echo "=== DuckDB AI Extension Build Script ==="
echo "Project root: ${PROJECT_ROOT}"
echo ""

# Check if DuckDB binary exists
if [ ! -f "${DUCKDB}" ]; then
    echo "❌ DuckDB binary not found at: ${DUCKDB}"
    echo "Please build DuckDB first:"
    echo "  cd ${BUILD_DIR} && make"
    exit 1
fi

# Create build directory for extension
mkdir -p "${EXT_BUILD_DIR}"

# Detect platform
PLATFORM="osx_arm64"
if [[ "$(uname)" == "Linux" ]]; then
    if [[ "$(uname -m)" == "x86_64" ]]; then
        PLATFORM="linux_amd64"
    else
        PLATFORM="linux_arm64"
    fi
fi

# Compilation flags for loadable extension
# IMPORTANT: Don't link against libduckdb_static.a for loadable extensions
# The symbols will be resolved when the extension is loaded
CXX="clang++"
CXXFLAGS="-std=c++17 -fPIC -O2 -DDUCKDB_EXTENSION_PLATFORM_NAME=${PLATFORM} -DDUCKDB_BUILD_LOADABLE_EXTENSION"
INCLUDES="-I${PROJECT_ROOT}/extension/ai/include -I${PROJECT_ROOT}/src/include"

# Source files
SOURCES="${PROJECT_ROOT}/extension/ai/src/ai_extension.cpp ${PROJECT_ROOT}/extension/ai/src/ai_filter.cpp"

# Output file
OUTPUT="${EXT_BUILD_DIR}/ai.duckdb_extension"

echo "Platform: ${PLATFORM}"
echo "Compiling AI Extension (loadable mode)..."
${CXX} ${CXXFLAGS} ${INCLUDES} -shared -undefined dynamic_lookup ${SOURCES} -o ${OUTPUT}

if [ $? -ne 0 ]; then
    echo ""
    echo "❌ Build failed"
    exit 1
fi

# Append metadata to extension
echo "Appending metadata..."
PLATFORM_FILE="${BUILD_DIR}/duckdb_platform_out"
NULL_FILE="${PROJECT_ROOT}/scripts/null.txt"

# Create null.txt if not exists
if [ ! -f "${NULL_FILE}" ]; then
    echo -ne '\x00' > "${NULL_FILE}"
fi

# Use cmake to append metadata
# IMPORTANT: Version must match Python duckdb library version
DUCKDB_VERSION="v1.4.4"
echo "DuckDB version: ${DUCKDB_VERSION}"

cmake -DEXTENSION="${OUTPUT}" \
      -DPLATFORM_FILE="${PLATFORM_FILE}" \
      -DVERSION_FIELD="${DUCKDB_VERSION}" \
      -DEXTENSION_VERSION="0.0.1-mvp" \
      -DABI_TYPE="CPP" \
      -DNULL_FILE="${NULL_FILE}" \
      -P "${PROJECT_ROOT}/scripts/append_metadata.cmake"

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
    echo "⚠️  Build succeeded but metadata append failed - extension may still work"
fi
