#!/bin/bash
# Build script for Google Test unit tests for AIFunctionExecutor

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
AI_EXT_DIR="${SCRIPT_DIR}/../.."
BUILD_DIR="${SCRIPT_DIR}/build"
GTEST_DIR="/opt/homebrew/include"

echo "=== Building AI Function Executor Unit Tests ==="
echo "Extension dir: ${AI_EXT_DIR}"
echo "Build dir: ${BUILD_DIR}"
echo ""

# Create build directory
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

# Compile only the executor (not the full ai_filter.cpp)
g++ -std=c++17 \
	-I"${AI_EXT_DIR}/include" \
	-I"${SCRIPT_DIR}/../../../../src/include" \
	-I"${GTEST_DIR}" \
	-I/opt/homebrew/opt/gtest/include \
	-fPIC \
	-O0 \
	-g \
	--coverage \
	-o ai_executor_gtest \
	"${SCRIPT_DIR}/ai_filter_test.cpp" \
	${AI_EXT_DIR}/src/http_client.cpp \
	-L/opt/homebrew/lib \
	-L/opt/homebrew/opt/gtest/lib \
	-lgtest \
	-lgmock \
	-lpthread

echo "✅ Build complete!"
echo ""
echo "Running tests..."
./ai_executor_gtest --gtest_color=yes

echo ""
echo "Generating coverage report..."
gcov ../src/http_client.cpp 2>/dev/null | grep "http_client.cpp" || echo "Coverage data not found for http_client.cpp"
