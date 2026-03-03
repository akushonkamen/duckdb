#!/bin/bash
# Mock Coverage Tests - 使用 LD_PRELOAD 测试错误处理分支

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"
DUCKDB="${BUILD_DIR}/duckdb"
EXT="${BUILD_DIR}/test/extension/ai.duckdb_extension"
MOCK_LIB="${SCRIPT_DIR}/mock_popen.dylib"

echo "=== Mock Coverage Tests ==="
echo "Mock library: ${MOCK_LIB}"

# 确保使用 coverage 版本的 extension
if [ ! -f "${EXT}" ]; then
    echo "⚠️  Coverage extension not found, building..."
    bash ${SCRIPT_DIR}/../build_with_coverage.sh
fi

# 测试 1: popen 完全失败
echo ""
echo "📋 Test 1: Complete popen failure (MOCK_POPEN_RETURN_NULL)"
export DYLD_INSERT_LIBRARIES="${MOCK_LIB}"
export MOCK_POPEN_RETURN_NULL="1"
${DUCKDB} -unsigned -c "LOAD '${EXT}';" -c "SELECT ai_filter_batch('test', 'mock_fail', 'clip') AS score;" 2>&1 | head -5
unset MOCK_POPEN_RETURN_NULL
unset DYLD_INSERT_LIBRARIES
echo "  ✅ Test 1 complete"

# 测试 2: 前 3 次 popen 失败（测试重试逻辑）
echo ""
echo "📋 Test 2: First 3 popen failures (retry logic)"
export DYLD_INSERT_LIBRARIES="${MOCK_LIB}"
export MOCK_POPEN_FAIL="3"
${DUCKDB} -unsigned -c "LOAD '${EXT}';" -c "SELECT ai_filter_batch('test', 'retry_fail', 'clip') AS score;" 2>&1 | head -10
unset MOCK_POPEN_FAIL
unset DYLD_INSERT_LIBRARIES
echo "  ✅ Test 2 complete"

# 测试 3: pclose 失败
echo ""
echo "📋 Test 3: pclose failure"
export DYLD_INSERT_LIBRARIES="${MOCK_LIB}"
export MOCK_PCLOSE_FAIL="1"
${DUCKDB} -unsigned -c "LOAD '${EXT}';" -c "SELECT ai_filter_batch('test', 'pclose_fail', 'clip') AS score;" 2>&1 | head -5
unset MOCK_PCLOSE_FAIL
unset DYLD_INSERT_LIBRARIES
echo "  ✅ Test 3 complete"

# 测试 4: 混合失败场景
echo ""
echo "📋 Test 4: Mixed failure scenarios"
export DYLD_INSERT_LIBRARIES="${MOCK_LIB}"
export MOCK_POPEN_FAIL="2"
for i in {1..5}; do
    ${DUCKDB} -unsigned -c "LOAD '${EXT}';" -c "SELECT ai_filter_batch('test', 'mixed_$i', 'clip') AS score;" > /dev/null 2>&1
done
unset MOCK_POPEN_FAIL
unset DYLD_INSERT_LIBRARIES
echo "  ✅ Test 4 complete"

# 测试 5: 正常调用（确保 mock 不影响正常流程）
echo ""
echo "📋 Test 5: Normal calls (no mock)"
${DUCKDB} -unsigned -c "LOAD '${EXT}';" -c "SELECT ai_filter_batch('test', 'normal', 'clip') AS score;" > /dev/null 2>&1
echo "  ✅ Test 5 complete"

echo ""
echo "=== Mock Coverage Tests Complete ==="
echo ""
echo "Now running gcov to check updated coverage..."
cd "${SCRIPT_DIR}/../../build/test/extension"
gcov ai.duckdb_extension-ai_filter.gcno 2>&1 | grep "ai_filter.cpp" | head -3
