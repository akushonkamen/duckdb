#!/bin/bash
# Coverage test with test mode enabled
# This script uses AI_FILTER_TEST_MODE to simulate different scenarios

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="/Users/yp1017/development/daft-duckdb-multimodal/duckdb/build"
EXT_DIR="${BUILD_DIR}/test/extension"

echo "=== Coverage Test with Test Mode ==="
echo ""

# Clean old coverage data
echo "Cleaning old coverage data..."
rm -f ${EXT_DIR}/*.gcda
find ${BUILD_DIR} -name "*.gcda" -delete 2>/dev/null || true

# Run tests with different test modes
echo "Running tests with different test modes..."

# Test 1: Success mode (covers successful API response path)
echo "1. Testing success mode..."
export AI_FILTER_TEST_MODE="success"
${BUILD_DIR}/duckdb -unsigned -c "LOAD '${EXT_DIR}/ai.duckdb_extension'; SELECT ai_filter('url1', 'cat', 'clip') as r1; SELECT ai_filter_batch('url2', 'dog', 'clip') as r2;" > /dev/null 2>&1

# Test 2: Retry mode (covers retry logic)
echo "2. Testing retry mode..."
export AI_FILTER_TEST_MODE="retry"
${BUILD_DIR}/duckdb -unsigned -c "LOAD '${EXT_DIR}/ai.duckdb_extension'; SELECT ai_filter('url3', 'cat', 'clip') as r3; SELECT ai_filter('url4', 'dog', 'clip') as r4; SELECT ai_filter('url5', 'cat', 'clip') as r5;" > /dev/null 2>&1

# Test 3: Fail mode (covers max retries exhausted path)
echo "3. Testing fail mode..."
export AI_FILTER_TEST_MODE="fail"
${BUILD_DIR}/duckdb -unsigned -c "LOAD '${EXT_DIR}/ai.duckdb_extension'; SELECT ai_filter('url6', 'cat', 'clip') as r6;" > /dev/null 2>&1

# Test 4: Invalid mode (covers Strategy 2 parsing)
echo "4. Testing invalid JSON mode..."
export AI_FILTER_TEST_MODE="invalid"
${BUILD_DIR}/duckdb -unsigned -c "LOAD '${EXT_DIR}/ai.duckdb_extension'; SELECT ai_filter('url7', 'cat', 'clip') as r7;" > /dev/null 2>&1

# Test 5: NULL handling (with success mode for API call path)
echo "5. Testing NULL handling..."
export AI_FILTER_TEST_MODE="success"
${BUILD_DIR}/duckdb -unsigned -c "LOAD '${EXT_DIR}/ai.duckdb_extension'; SELECT ai_filter(NULL, 'cat', 'clip') as null_url; SELECT ai_filter('url', NULL, 'clip') as null_query; SELECT ai_filter('url', 'cat', NULL) as null_model;" > /dev/null 2>&1

# Test 6: Empty strings
echo "6. Testing empty strings..."
${BUILD_DIR}/duckdb -unsigned -c "LOAD '${EXT_DIR}/ai.duckdb_extension'; SELECT ai_filter('', '', '') as empty_all; SELECT ai_filter('url', '', 'clip') as empty_query; SELECT ai_filter_batch('', 'cat', 'clip') as batch_empty_url;" > /dev/null 2>&1

# Test 7: Batch processing with NULLs
echo "7. Testing batch with NULLs..."
${BUILD_DIR}/duckdb -unsigned -c "LOAD '${EXT_DIR}/ai.duckdb_extension'; CREATE TABLE test_nulls AS SELECT * FROM (VALUES (NULL, 'cat', 'clip'), ('url1', NULL, 'clip'), ('url2', 'dog', NULL)) AS t(img, q, m); SELECT COUNT(*) as cnt FROM test_nulls; SELECT img, q, m, ai_filter_batch(img, q, m) as score FROM test_nulls; DROP TABLE test_nulls;" > /dev/null 2>&1

# Test 8: Concurrent processing (stress test)
echo "8. Testing concurrent processing..."
export AI_FILTER_TEST_MODE="success"
${BUILD_DIR}/duckdb -unsigned -c "LOAD '${EXT_DIR}/ai.duckdb_extension'; SELECT COUNT(*) as cnt, AVG(ai_filter('url' || i, 'cat', 'clip')) as avg_score FROM range(10);" > /dev/null 2>&1

# Test 9: Batch concurrent processing
echo "9. Testing batch concurrent..."
${BUILD_DIR}/duckdb -unsigned -c "LOAD '${EXT_DIR}/ai.duckdb_extension'; SELECT COUNT(*) as cnt, AVG(ai_filter_batch('url' || i, 'cat', 'clip')) as avg_score FROM range(10);" > /dev/null 2>&1

# Test 10: Different model names
echo "10. Testing different models..."
${BUILD_DIR}/duckdb -unsigned -c "LOAD '${EXT_DIR}/ai.duckdb_extension'; SELECT ai_filter('url', 'cat', 'clip') as clip; SELECT ai_filter('url', 'cat', '') as empty_model; SELECT ai_filter('url', 'cat', 'chatgpt-4o-latest') as full_model;" > /dev/null 2>&1

# Test 11: Environment variable for default score
echo "11. Testing custom default score..."
export AI_FILTER_DEFAULT_SCORE="0.25"
export AI_FILTER_TEST_MODE="fail"
${BUILD_DIR}/duckdb -unsigned -c "LOAD '${EXT_DIR}/ai.duckdb_extension'; SELECT ai_filter('url', 'cat', 'clip') as custom_score;" > /dev/null 2>&1

# Unset test mode
unset AI_FILTER_TEST_MODE
unset AI_FILTER_DEFAULT_SCORE

echo ""
echo "=== All tests completed ==="
echo ""

# Generate coverage report
echo "=== Coverage Report ==="
cd /Users/yp1017/development/daft-duckdb-multimodal/duckdb

for gcno in ${EXT_DIR}/ai.duckdb_extension-*.gcno; do
    if [ -f "$gcno" ]; then
        echo ""
        gcov -o "${EXT_DIR}" "$gcno" 2>&1 | grep "ai_filter.cpp\|ai_extension.cpp" | head -5
    fi
done

echo ""
echo "Calculating total coverage..."
gcov -o "${EXT_DIR}" ${EXT_DIR}/ai.duckdb_extension-ai_filter.gcno 2>&1 | grep -E "ai_filter.cpp|Lines executed"
