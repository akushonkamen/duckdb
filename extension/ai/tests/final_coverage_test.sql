-- Final comprehensive coverage test
-- Single session covering all code paths
LOAD '/Users/yp1017/development/daft-duckdb-multimodal/duckdb/build/test/extension/ai.duckdb_extension';

SELECT '=== Coverage Test Start ===' as status;

-- Test 1: Success mode (covers successful API response)
SELECT 'Test 1: Success mode' as test;
SELECT ai_filter('url1', 'cat', 'clip') as r1;

-- Test 2: Another success call
SELECT 'Test 2: Second success' as test;
SELECT ai_filter('url2', 'dog', 'clip') as r2;

-- Test 3: NULL handling
SELECT 'Test 3: NULL url' as test;
SELECT ai_filter(NULL, 'cat', 'clip') as r3;

SELECT 'Test 4: NULL query' as test;
SELECT ai_filter('url', NULL, 'clip') as r4;

SELECT 'Test 5: NULL model' as test;
SELECT ai_filter('url', 'cat', NULL) as r5;

-- Test 6: Empty strings
SELECT 'Test 6: Empty url' as test;
SELECT ai_filter('', 'cat', 'clip') as r6;

SELECT 'Test 7: Empty query' as test;
SELECT ai_filter('url', '', 'clip') as r7;

SELECT 'Test 8: Empty model' as test;
SELECT ai_filter('url', 'cat', '') as r8;

-- Test 9: Batch function with NULLs
SELECT 'Test 9: Batch NULL url' as test;
SELECT ai_filter_batch(NULL, 'cat', 'clip') as r9;

SELECT 'Test 10: Batch NULL query' as test;
SELECT ai_filter_batch('url', NULL, 'clip') as r10;

SELECT 'Test 11: Batch NULL model' as test;
SELECT ai_filter_batch('url', 'cat', NULL) as r11;

SELECT 'Test 12: Batch empty url' as test;
SELECT ai_filter_batch('', 'cat', 'clip') as r12;

-- Test 13: Batch normal call
SELECT 'Test 13: Batch normal' as test;
SELECT ai_filter_batch('url13', 'cat', 'clip') as r13;

-- Test 14: Multiple batch calls
SELECT 'Test 14: Multiple batch' as test;
SELECT ai_filter_batch('url14', 'dog', 'clip') as r14;
SELECT ai_filter_batch('url15', 'bird', 'clip') as r15;

-- Test 15: Different models
SELECT 'Test 15: Different models' as test;
SELECT ai_filter('url', 'cat', 'clip') as r_clip;
SELECT ai_filter('url', 'cat', 'openclip') as r_open;
SELECT ai_filter('url', 'cat', 'chatgpt-4o-latest') as r_full;

SELECT '=== Coverage Test Complete ===' as final_status;
