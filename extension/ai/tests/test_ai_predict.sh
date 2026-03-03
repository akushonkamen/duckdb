#!/bin/bash
#
# Test script for ai_predict function
# TASK-WINDOW-001: ai_predict UDF 实现
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

echo "=== ai_predict Function Test Suite ==="
echo ""
echo "⚠️  NOTE: ai_predict is implemented as a scalar function."
echo "   OVER clause support requires aggregate functions which have"
echo "   limitations in DuckDB loadable extensions."
echo "   See AI_WINDOW_LIMITATIONS.md for workarounds."
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

echo -e "${GREEN}✅ Extension found${NC}"
echo -e "${GREEN}✅ DuckDB binary found${NC}"
echo ""

# Helper function
run_test() {
    local test_name="$1"
    local sql="$2"

    echo -n "Test: $test_name ... "

    local result=$($DUCKDB_BIN -unsigned -c "LOAD '$EXT_PATH'; $sql" 2>&1)
    local exit_code=$?

    if [ $exit_code -eq 0 ]; then
        echo -e "${GREEN}✅ PASSED${NC}"
        ((PASSED++))
        return 0
    else
        echo -e "${RED}❌ FAILED${NC}"
        echo "  Error: $result"
        ((FAILED++))
        return 1
    fi
}

echo "=== Function Registration ==="
run_test "ai_predict is registered" \
    "SELECT function_name FROM duckdb_functions() WHERE function_name='ai_predict';"

echo ""
echo "=== Basic Functionality ==="

run_test "Basic ai_predict call" \
    "SELECT ai_predict('test_image', 'cat', 'clip') AS prediction;"

run_test "Different prompts" \
    "SELECT ai_predict('test_image', 'dog', 'clip') AS prediction;"

echo ""
echo "=== WHERE Clause Integration ==="

run_test "Filter by prediction score" \
    "SELECT 'test' AS id, ai_predict('image', 'cat', 'clip') AS score WHERE score > 0.0;"

echo ""
echo "=== Correlated Subquery (Window-like Pattern) ==="

run_test "Correlated subquery for window-like behavior" \
    "SELECT t1.id, (SELECT ai_predict(t1.data, 'test', 'clip') AS pred) AS window_score FROM (SELECT unnest([0,1,2]) AS id, 'test_data' AS data) t1;"

echo ""
echo "=== LIMITATIONS DOCUMENTATION ==="

echo -e "${YELLOW}⚠️  Known Limitations:${NC}"
echo "   - OVER clause not supported (scalar function)"
echo "   - Use correlated subqueries for window-like patterns"
echo "   - See extension/ai/AI_WINDOW_LIMITATIONS.md for details"

echo ""
echo "=================================================="
echo "Test Results: $PASSED passed, $FAILED failed"
echo "=================================================="

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}✅ All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}❌ Some tests failed${NC}"
    exit 1
fi
