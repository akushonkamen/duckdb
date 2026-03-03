#!/bin/bash
#
# Test script for ai_similarity function
# TASK-K-001: AI_join/AI_window Extension
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

echo "=== ai_similarity Function Test Suite ==="
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

run_test_error() {
    local test_name="$1"
    local sql="$2"
    local expected_error="$3"

    echo -n "Test: $test_name ... "

    local result=$($DUCKDB_BIN -unsigned -c "LOAD '$EXT_PATH'; $sql" 2>&1)

    if echo "$result" | grep -qi "$expected_error"; then
        echo -e "${GREEN}✅ PASSED${NC}"
        ((PASSED++))
        return 0
    else
        echo -e "${RED}❌ FAILED${NC}"
        echo "  Expected error containing: $expected_error"
        echo "  Got: $result"
        ((FAILED++))
        return 1
    fi
}

echo "=== Function Registration ==="
run_test "ai_similarity is registered" \
    "SELECT function_name FROM duckdb_functions() WHERE function_name='ai_similarity';"

echo ""
echo "=== Basic Similarity Calculation ==="

run_test "Identical vectors (similarity = 1.0)" \
    "SELECT ai_similarity([1.0, 2.0, 3.0], [1.0, 2.0, 3.0], 'clip') AS s;"

run_test "Orthogonal vectors (similarity ≈ 0.0)" \
    "SELECT ai_similarity([1.0, 0.0, 0.0], [0.0, 1.0, 0.0], 'clip') AS s;"

run_test "Similar vectors (high similarity)" \
    "SELECT ai_similarity([1.0, 0.0, 0.0], [0.9, 0.1, 0.0], 'clip') AS s;"

echo ""
echo "=== NULL Handling ==="

run_test "NULL left vector" \
    "SELECT ai_similarity(NULL, [1.0, 2.0, 3.0], 'clip') IS NULL AS s;"

run_test "NULL right vector" \
    "SELECT ai_similarity([1.0, 2.0, 3.0], NULL, 'clip') IS NULL AS s;"

echo ""
echo "=== Dimension Validation ==="

run_test_error "Dimension mismatch (2 vs 3)" \
    "SELECT ai_similarity([1.0, 2.0], [1.0, 2.0, 3.0], 'clip') AS s;" \
    "Invalid Input Error"

echo ""
echo "=== JOIN-like Usage ==="

run_test "CROSS JOIN with similarity" \
    "WITH left_vecs AS (SELECT 1 AS id, [1.0, 0.0, 0.0]::FLOAT[] AS vec), right_vecs AS (SELECT 2 AS id, [0.9, 0.1, 0.0]::FLOAT[] AS vec) SELECT l.id, r.id, ai_similarity(l.vec, r.vec, 'clip') AS similarity FROM left_vecs l CROSS JOIN right_vecs r;"

echo ""
echo "=== WHERE Clause Integration ==="

run_test "Filter by similarity threshold" \
    "WITH vecs AS (SELECT 1 AS id, [1.0, 2.0, 3.0]::FLOAT[] AS vec) SELECT id FROM vecs WHERE ai_similarity(vec, [1.0, 2.0, 3.0]::FLOAT[], 'clip') > 0.9;"

echo ""
echo "=== Multiple Vectors ==="

run_test "Batch processing" \
    "SELECT unnest([1, 2, 3]) AS id, ai_similarity([1.0, 2.0, 3.0], [1.0, 2.0, 3.0], 'clip') AS s FROM range(3);"

echo ""
echo "=== Edge Cases ==="

run_test "Empty vectors" \
    "SELECT ai_similarity([], [], 'clip') IS NULL AS s;"

run_test "Single dimension vectors" \
    "SELECT ai_similarity([1.0], [1.0], 'clip') AS s;"

run_test "High dimensional vectors" \
    "SELECT ai_similarity([0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0], [0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0], 'clip') AS s;"

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
