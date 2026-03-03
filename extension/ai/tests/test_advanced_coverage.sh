#!/bin/bash
# Advanced Coverage Tests - 覆盖更多代码分支

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"
DUCKDB="${BUILD_DIR}/duckdb"
EXT="${BUILD_DIR}/test/extension/ai.duckdb_extension"

echo "=== Advanced Coverage Tests ==="

# Test 1: 各种 JSON 响应格式（测试 Strategy 2 回退）
echo "📋 Test 1: Various Response Formats"
${DUCKDB} -unsigned -c "LOAD '${EXT}';" -c "SELECT ai_filter_batch('test', 'num', 'clip') AS score;" > /dev/null 2>&1
${DUCKDB} -unsigned -c "LOAD '${EXT}';" -c "SELECT ai_filter_batch('test', 'complex_json', 'clip') AS score;" > /dev/null 2>&1

# Test 2: 不同并发数量测试 MAX_CONCURRENT 边界
echo "📋 Test 2: MAX_CONCURRENT Boundary Tests"
for count in 9 10 11 12 15; do
    ${DUCKDB} -unsigned -c "LOAD '${EXT}';" -c "SELECT COUNT(*) FROM (SELECT ai_filter_batch('test', 'c$count', 'clip') FROM range(${count}));" > /dev/null 2>&1
done
echo "  ✅ MAX_CONCURRENT boundary tests done"

# Test 3: 超长输入测试
echo "📋 Test 3: Long Input Tests"
LONG_IMAGE=$(python3 -c "print('x' * 50000)")
${DUCKDB} -unsigned -c "LOAD '${EXT}';" -c "SELECT ai_filter_batch('${LONG_IMAGE:0:1000}', 'test', 'clip') AS score;" > /dev/null 2>&1

# Test 4: 特殊字符输入
echo "📋 Test 4: Special Character Inputs"
${DUCKDB} -unsigned -c "LOAD '${EXT}';" -c "SELECT ai_filter_batch('test', 'test\"quote', 'clip') AS score;" > /dev/null 2>&1
${DUCKDB} -unsigned -c "LOAD '${EXT}';" -c "SELECT ai_filter_batch('test', 'test\\slash', 'clip') AS score;" > /dev/null 2>&1
echo "  ✅ Special character tests done"

# Test 5: 模型名称边界测试
echo "📋 Test 5: Model Name Edge Cases"
${DUCKDB} -unsigned -c "LOAD '${EXT}';" -c "SELECT ai_filter_batch('test', 'test', '') AS score;" > /dev/null 2>&1
${DUCKDB} -unsigned -c "LOAD '${EXT}';" -c "SELECT ai_filter_batch('test', 'test', '   ') AS score;" > /dev/null 2>&1
${DUCKDB} -unsigned -c "LOAD '${EXT}';" -c "SELECT ai_filter_batch('test', 'test', 'very_long_model_name_that_exceeds_normal_limits') AS score;" > /dev/null 2>&1
echo "  ✅ Model edge cases done"

# Test 6: 重复调用相同参数（测试缓存）
echo "📋 Test 6: Repeated Calls (Caching)"
for i in {1..10}; do
    ${DUCKDB} -unsigned -c "LOAD '${EXT}';" -c "SELECT ai_filter_batch('test', 'repeat_test', 'clip') AS score;" > /dev/null 2>&1
done
echo "  ✅ Repeated calls done"

# Test 7: 批处理中的 NULL 混合
echo "📋 Test 7: Batch with NULL Mixed"
${DUCKDB} -unsigned -c "LOAD '${EXT}';" -c "
    SELECT ai_filter_batch(
        CASE WHEN i % 2 = 0 THEN 'test' ELSE NULL END,
        'test',
        'clip'
    ) AS score FROM range(10);
" > /dev/null 2>&1
echo "  ✅ Batch NULL mixed done"

# Test 8: 并发 + NULL 组合
echo "📋 Test 8: Concurrent with NULL"
${DUCKDB} -unsigned -c "LOAD '${EXT}';" -c "
    SELECT COUNT(*) FROM (
        SELECT ai_filter_batch(
            CASE WHEN i % 3 = 0 THEN 'test' ELSE NULL END,
            'concurrent_null',
            'clip'
        ) AS score FROM range(15)
    );
" > /dev/null 2>&1
echo "  ✅ Concurrent NULL test done"

# Test 9: 无效 base64 图像数据
echo "📋 Test 9: Invalid Base64 Data"
${DUCKDB} -unsigned -c "LOAD '${EXT}';" -c "SELECT ai_filter_batch('not_valid_base64!!!', 'test', 'clip') AS score;" > /dev/null 2>&1
${DUCKDB} -unsigned -c "LOAD '${EXT}';" -c "SELECT ai_filter_batch('aW52YWxpZGJhc2U2NA==', 'test', 'clip') AS score;" > /dev/null 2>&1
echo "  ✅ Invalid base64 tests done"

# Test 10: 环境变量各种值
echo "📋 Test 10: Environment Variable Values"
for val in "0.0" "1.0" "0.5" "-1.0" "2.0" "invalid"; do
    export AI_FILTER_DEFAULT_SCORE="$val"
    ${DUCKDB} -unsigned -c "LOAD '${EXT}';" -c "SELECT ai_filter_batch('test', 'env_test', 'clip') AS score;" > /dev/null 2>&1
done
unset AI_FILTER_DEFAULT_SCORE
echo "  ✅ Environment variable tests done"

echo ""
echo "=== Advanced Coverage Tests Complete ==="
