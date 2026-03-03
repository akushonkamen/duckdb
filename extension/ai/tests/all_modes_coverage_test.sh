#!/bin/bash
# Run all test modes in sequence to maximize coverage

set -e

BUILD_DIR="/Users/yp1017/development/daft-duckdb-multimodal/duckdb/build"
EXT="${BUILD_DIR}/test/extension/ai.duckdb_extension"

echo "=== Running all test modes for maximum coverage ==="

# Clean coverage data
rm -f ${BUILD_DIR}/test/extension/*.gcda

# Create SQL script that covers all modes
cat > /tmp/all_modes_test.sql << 'EOF'
LOAD '/Users/yp1017/development/daft-duckdb-multimodal/duckdb/build/test/extension/ai.duckdb_extension';

-- Test 1: Success mode (normal API success path)
SELECT '=== Success Mode Tests ===' as section;
SELECT ai_filter('url1', 'cat', 'clip') as s1;
SELECT ai_filter('url2', 'dog', 'clip') as s2;
SELECT ai_filter_batch('url3', 'bird', 'clip') as s3;

-- Test 2: NULL handling
SELECT '=== NULL Handling ===' as section;
SELECT ai_filter(NULL, 'cat', 'clip') as n1;
SELECT ai_filter('url', NULL, 'clip') as n2;
SELECT ai_filter('url', 'cat', NULL) as n3;
SELECT ai_filter_batch(NULL, 'cat', 'clip') as n4;

-- Test 3: Empty strings
SELECT '=== Empty Strings ===' as section;
SELECT ai_filter('', 'cat', 'clip') as e1;
SELECT ai_filter('url', '', 'clip') as e2;
SELECT ai_filter('url', 'cat', '') as e3;
SELECT ai_filter_batch('', 'cat', 'clip') as e4;

-- Test 4: Different models
SELECT '=== Different Models ===' as section;
SELECT ai_filter('url', 'cat', 'clip') as m1;
SELECT ai_filter('url', 'cat', 'openclip') as m2;
SELECT ai_filter('url', 'cat', 'chatgpt-4o-latest') as m3;

SELECT '=== All tests complete ===' as status;
EOF

# Run with success mode
echo "1. Running success mode tests..."
export AI_FILTER_TEST_MODE="success"
${BUILD_DIR}/duckdb -unsigned < /tmp/all_modes_test.sql > /tmp/success_mode.log 2>&1

# Run with retry mode (odd calls fail, even succeed)
echo "2. Running retry mode tests..."
export AI_FILTER_TEST_MODE="retry"
${BUILD_DIR}/duckdb -unsigned -c "LOAD '${EXT}'; SELECT ai_filter('retry1', 'cat', 'clip') as r1; SELECT ai_filter('retry2', 'cat', 'clip') as r2; SELECT ai_filter('retry3', 'cat', 'clip') as r3; SELECT ai_filter('retry4', 'cat', 'clip') as r4;" > /tmp/retry_mode.log 2>&1

# Run with fail mode
echo "3. Running fail mode tests..."
export AI_FILTER_TEST_MODE="fail"
${BUILD_DIR}/duckdb -unsigned -c "LOAD '${EXT}'; SELECT ai_filter('fail1', 'cat', 'clip') as f1; SELECT ai_filter_batch('fail2', 'cat', 'clip') as f2;" > /tmp/fail_mode.log 2>&1

# Run with invalid mode (triggers Strategy 2 parsing)
echo "4. Running invalid mode tests..."
export AI_FILTER_TEST_MODE="invalid"
${BUILD_DIR}/duckdb -unsigned -c "LOAD '${EXT}'; SELECT ai_filter('invalid1', 'cat', 'clip') as i1; SELECT ai_filter_batch('invalid2', 'cat', 'clip') as i2;" > /tmp/invalid_mode.log 2>&1

# Run with custom default score
echo "5. Running custom default score tests..."
export AI_FILTER_TEST_MODE="fail"
export AI_FILTER_DEFAULT_SCORE="0.25"
${BUILD_DIR}/duckdb -unsigned -c "LOAD '${EXT}'; SELECT ai_filter('custom1', 'cat', 'clip') as c1;" > /tmp/custom_score.log 2>&1

unset AI_FILTER_TEST_MODE
unset AI_FILTER_DEFAULT_SCORE

echo ""
echo "=== Coverage Report ==="
cd /Users/yp1017/development/daft-duckdb-multimodal/duckdb
gcov -o build/test/extension build/test/extension/ai.duckdb_extension-ai_filter.gcno 2>&1 | grep "ai_filter.cpp"

# Summary
echo ""
echo "=== Summary ==="
gcov -o build/test/extension build/test/extension/ai.duckdb_extension-ai_filter.gcno 2>&1 | grep "Lines executed" | head -1
gcov -o build/test/extension build/test/extension/ai.duckdb_extension-ai_extension.gcno 2>&1 | grep "Lines executed" | head -1
