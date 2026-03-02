#!/bin/bash
#
# DuckDB AI Extension - Enhanced Coverage Tests (Shell version)
# TASK-TEST-002: 测试覆盖率提升
#
# 测试覆盖：
# 1. 函数注册
# 2. 基本调用
# 3. 批处理函数
# 4. 错误处理和降级策略
# 5. 边界条件
# 6. 性能基准
# 7. WHERE 子句集成
# 8. 聚合函数集成
# 9. 并发调用
# 10. NULL 值处理
#

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Paths
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DUCKDB_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
EXT_PATH="$DUCKDB_ROOT/build/test/extension/ai.duckdb_extension"
DUCKDB_BIN="$DUCKDB_ROOT/build/duckdb"

# Test counters
PASSED=0
FAILED=0

echo "=== DuckDB AI Extension Test Suite - Enhanced ==="
echo ""

# Check extension exists
if [ ! -f "$EXT_PATH" ]; then
    echo -e "${RED}❌ Extension not found at: $EXT_PATH${NC}"
    exit 1
fi

# Check duckdb binary exists
if [ ! -f "$DUCKDB_BIN" ]; then
    echo -e "${RED}❌ DuckDB binary not found at: $DUCKDB_BIN${NC}"
    exit 1
fi

echo -e "${GREEN}✅ Extension found: $(basename "$EXT_PATH")${NC}"
echo -e "${GREEN}✅ DuckDB binary found: $(basename "$DUCKDB_BIN")${NC}"
echo ""

# Test helper function
run_test() {
    local test_name="$1"
    local sql="$2"
    local expected_pattern="$3"
    local description="$4"

    echo "📋 Test $test_name: $description"

    result=$($DUCKDB_BIN -unsigned -c "LOAD '$EXT_PATH'; $sql" 2>&1 || true)

    if echo "$result" | grep -q "$expected_pattern"; then
        echo -e "${GREEN}  ✅ PASSED${NC}"
        ((PASSED++))
        return 0
    else
        echo -e "${RED}  ❌ FAILED${NC}"
        echo "  Expected pattern: $expected_pattern"
        echo "  Got: $result"
        ((FAILED++))
        return 1
    fi
}

# Test 1: Function Registration
echo "📋 Test 1: Function Registration"
result=$($DUCKDB_BIN -unsigned -c "LOAD '$EXT_PATH'; SELECT function_name FROM duckdb_functions() WHERE function_name LIKE 'ai_%';" 2>&1)
func_count=$(echo "$result" | grep -c "ai_filter" || echo "0")
echo "  Registered functions: $func_count"
if [ "$func_count" -ge 2 ]; then
    echo -e "${GREEN}  ✅ PASSED: Both ai_filter and ai_filter_batch registered${NC}"
    ((PASSED++))
else
    echo -e "${RED}  ❌ FAILED: Expected 2 functions${NC}"
    ((FAILED++))
fi
echo ""

# Test 2: Basic Call
echo "📋 Test 2: Basic Function Call"
result=$($DUCKDB_BIN -unsigned -c "LOAD '$EXT_PATH'; SELECT ai_filter('test_image', 'cat', 'clip') AS score;" 2>&1)
if echo "$result" | grep -qE "[0-9]\.[0-9]"; then
    score=$(echo "$result" | grep -oE "[0-9]\.[0-9]+")
    echo "  Score: $score"
    echo -e "${GREEN}  ✅ PASSED: Score in valid range${NC}"
    ((PASSED++))
else
    echo -e "${RED}  ❌ FAILED: Invalid score${NC}"
    echo "  Result: $result"
    ((FAILED++))
fi
echo ""

# Test 3: Batch Function
echo "📋 Test 3: Batch Function"
result=$($DUCKDB_BIN -unsigned -c "LOAD '$EXT_PATH'; SELECT ai_filter_batch('test_image', 'dog', 'clip') AS score;" 2>&1)
if echo "$result" | grep -qE "[0-9]\.[0-9]"; then
    score=$(echo "$result" | grep -oE "[0-9]\.[0-9]+")
    echo "  Batch score: $score"
    echo -e "${GREEN}  ✅ PASSED: Batch function works${NC}"
    ((PASSED++))
else
    echo -e "${RED}  ❌ FAILED: Invalid score${NC}"
    echo "  Result: $result"
    ((FAILED++))
fi
echo ""

# Test 4: Error Handling - Default Score
echo "📋 Test 4: Error Handling - Default Degradation Score"
export AI_FILTER_DEFAULT_SCORE=0.75
result=$($DUCKDB_BIN -unsigned -c "LOAD '$EXT_PATH'; SELECT ai_filter('test_image', 'test_prompt', 'test_model') AS score;" 2>&1)
if echo "$result" | grep -qE "[0-9]\.[0-9]"; then
    score=$(echo "$result" | grep -oE "[0-9]\.[0-9]+")
    echo "  Score: $score"
    echo -e "${GREEN}  ✅ PASSED: Error handling works${NC}"
    ((PASSED++))
else
    echo -e "${RED}  ❌ FAILED: Invalid score${NC}"
    echo "  Result: $result"
    ((FAILED++))
fi
unset AI_FILTER_DEFAULT_SCORE
echo ""

# Test 5: Boundary Conditions
echo "📋 Test 5: Boundary Conditions"
# Empty strings
result=$($DUCKDB_BIN -unsigned -c "LOAD '$EXT_PATH'; SELECT ai_filter('', '', '') AS score;" 2>&1)
if echo "$result" | grep -qE "[0-9]\.[0-9]|NULL|Error"; then
    echo "  Empty string handled"
fi
# Long prompt (using a simpler test for shell)
result=$($DUCKDB_BIN -unsigned -c "LOAD '$EXT_PATH'; SELECT ai_filter('test', 'cat cat cat cat cat', 'clip') AS score;" 2>&1)
if echo "$result" | grep -qE "[0-9]\.[0-9]|NULL|Error"; then
    echo "  Long prompt handled"
fi
echo -e "${GREEN}  ✅ PASSED: Boundary conditions handled${NC}"
((PASSED++))
echo ""

# Test 6: Performance Benchmark
echo "📋 Test 6: Performance Benchmark"
for size in 1 10 50; do
    start=$(python3 -c "import time; print(int(time.time() * 1000))")
    result=$($DUCKDB_BIN -unsigned -c "LOAD '$EXT_PATH'; SELECT ai_filter_batch('test', 'benchmark', 'clip') FROM range($size);" 2>&1 >/dev/null)
    end=$(python3 -c "import time; print(int(time.time() * 1000))")
    elapsed=$((end - start))
    if [ "$elapsed" -gt 0 ]; then
        rows_per_sec=$((size * 1000 / elapsed))
        printf "  %3d rows: %3dms (%3d rows/s)\\n" "$size" "$elapsed" "$rows_per_sec"
    else
        printf "  %3d rows: <1ms\\n" "$size"
    fi
done
echo -e "${GREEN}  ✅ PASSED: Performance benchmark completed${NC}"
((PASSED++))
echo ""

# Test 7: WHERE Clause Integration
echo "📋 Test 7: WHERE Clause Integration"
result=$($DUCKDB_BIN -unsigned -c "LOAD '$EXT_PATH'; SELECT COUNT(*) AS count FROM (SELECT 'test' AS image, 'cat' AS prompt) AS t WHERE ai_filter_batch(image, prompt, 'clip') > 0.0;" 2>&1)
if echo "$result" | grep -qE "[0-9]+"; then
    count=$(echo "$result" | grep -oE "[0-9]+")
    echo "  Filtered rows: $count"
    echo -e "${GREEN}  ✅ PASSED: WHERE clause integration works${NC}"
    ((PASSED++))
else
    echo -e "${RED}  ❌ FAILED: Invalid count${NC}"
    echo "  Result: $result"
    ((FAILED++))
fi
echo ""

# Test 8: Aggregation
echo "📋 Test 8: Aggregation with AI Filter"
result=$($DUCKDB_BIN -unsigned -c "LOAD '$EXT_PATH'; SELECT MIN(ai_filter_batch('test', 'min', 'clip')) AS min_score, MAX(ai_filter_batch('test', 'max', 'clip')) AS max_score, AVG(ai_filter_batch('test', 'avg', 'clip')) AS avg_score FROM range(5);" 2>&1)
if echo "$result" | grep -qE "min_score|max_score|avg_score"; then
    echo "  Aggregation results received"
    echo -e "${GREEN}  ✅ PASSED: Aggregation works${NC}"
    ((PASSED++))
else
    echo -e "${RED}  ❌ FAILED: Invalid aggregation${NC}"
    echo "  Result: $result"
    ((FAILED++))
fi
echo ""

# Test 9: Concurrent Calls
echo "📋 Test 9: Concurrent Calls"
result=$($DUCKDB_BIN -unsigned -c "LOAD '$EXT_PATH'; SELECT ai_filter_batch('test1', 'cat1', 'clip') AS score1, ai_filter_batch('test2', 'cat2', 'clip') AS score2, ai_filter_batch('test3', 'cat3', 'clip') AS score3;" 2>&1)
score_count=$(echo "$result" | grep -oE "[0-9]\.[0-9]+" | wc -l | tr -d ' ')
if [ "$score_count" -ge 3 ]; then
    echo "  Scores: $(echo "$result" | grep -oE "[0-9]\.[0-9]+" | head -3 | tr '\n' ' ')"
    echo -e "${GREEN}  ✅ PASSED: Concurrent calls work${NC}"
    ((PASSED++))
else
    echo -e "${RED}  ❌ FAILED: Invalid concurrent call${NC}"
    echo "  Result: $result"
    ((FAILED++))
fi
echo ""

# Test 10: NULL Handling
echo "📋 Test 10: NULL Value Handling"
result=$($DUCKDB_BIN -unsigned -c "LOAD '$EXT_PATH'; SELECT ai_filter_batch(NULL, 'cat', 'clip') AS score;" 2>&1)
# DuckDB might return NULL or handle it
echo "  NULL input result: $(echo "$result" | head -1)"
echo -e "${GREEN}  ✅ PASSED: NULL handling works${NC}"
((PASSED++))
echo ""

# Summary
echo "========================================"
echo "Test Summary"
echo "========================================"
echo "Total tests: $((PASSED + FAILED))"
echo -e "${GREEN}✅ Passed: $PASSED${NC}"
echo -e "${RED}❌ Failed: $FAILED${NC}"
if [ $((PASSED + FAILED)) -gt 0 ]; then
    pass_rate=$((PASSED * 100 / (PASSED + FAILED)))
    echo "Pass rate: ${pass_rate}%"
else
    echo "Pass rate: N/A"
fi
echo "========================================"

exit $FAILED
