-- Maximum coverage test - Single session covering ALL paths
LOAD '/Users/yp1017/development/daft-duckdb-multimodal/duckdb/build/test/extension/ai.duckdb_extension';

SELECT '=== Starting comprehensive coverage test ===' as status;

-- Part 1: NULL and empty input handling (early returns)
SELECT 'Test 1: NULL image_url' as test;
SELECT ai_filter(NULL, 'cat', 'clip') as r1;

SELECT 'Test 2: NULL query' as test;
SELECT ai_filter('url', NULL, 'clip') as r2;

SELECT 'Test 3: NULL model (should use default)' as test;
SELECT ai_filter('url', 'cat', NULL) as r3;

SELECT 'Test 4: Empty strings' as test;
SELECT ai_filter('', '', '') as r4;

-- Part 2: ai_filter_batch with NULL handling
SELECT 'Test 5: Batch NULL handling' as test;
SELECT ai_filter_batch(NULL, 'cat', 'clip') as r5;

-- Part 3: API calls that will fail (covers retry paths)
-- The API seems to be returning errors, which is good for coverage
SELECT 'Test 6: Invalid base64 (triggers API error retry)' as test;
SELECT ai_filter('invalid_base64_data', 'cat', 'clip') as r6;

SELECT 'Test 7: Another API call for retry coverage' as test;
SELECT ai_filter('another_invalid', 'dog', 'clip') as r7;

-- Part 8: Multiple concurrent calls to test thread paths
SELECT 'Test 8: Concurrent processing (10 rows)' as test;
SELECT COUNT(*) as cnt, AVG(ai_filter('url' || i, 'cat', 'clip')) as avg_score FROM range(10);

SELECT 'Test 9: Batch processing (10 rows)' as test;
SELECT COUNT(*) as cnt, AVG(ai_filter_batch('url' || i, 'cat', 'clip')) as avg_score FROM range(10);

-- Part 10: Test ai_filter_batch directly for NULL paths
SELECT 'Test 10: Batch with empty inputs' as test;
CREATE TABLE batch_null_test AS
SELECT * FROM (VALUES
    (NULL, 'cat', 'clip'),
    ('url1', NULL, 'clip'),
    ('url2', 'dog', NULL),
    ('', 'cat', 'clip')
) AS t(img, query, model);

SELECT img, query, model, ai_filter_batch(img, query, model) as score
FROM batch_null_test;
DROP TABLE batch_null_test;

-- Part 11: Model parameter variations
SELECT 'Test 11: Different model names' as test;
SELECT ai_filter('url', 'cat', 'clip') as r_clip;
SELECT ai_filter('url', 'cat', 'openclip') as r_openclip;
SELECT ai_filter('url', 'cat', '') as r_empty_model;
SELECT ai_filter('url', 'cat', 'chatgpt-4o-latest') as r_full_model;

-- Part 12: Special query characters
SELECT 'Test 12: Special characters in query' as test;
SELECT ai_filter('url', 'cat!@#$%', 'clip') as r_special;

-- Part 13: Stress test for concurrent paths
SELECT 'Test 13: Stress test (50 rows)' as test;
SELECT COUNT(*) as cnt,
       MIN(ai_filter('url' || i, 'cat', 'clip')) as min_score,
       MAX(ai_filter('url' || i, 'cat', 'clip')) as max_score
FROM range(50);

SELECT 'Test 14: Batch stress test (50 rows)' as test;
SELECT COUNT(*) as cnt,
       MIN(ai_filter_batch('url' || i, 'cat', 'clip')) as min_score,
       MAX(ai_filter_batch('url' || i, 'cat', 'clip')) as max_score
FROM range(50);

SELECT '=== All tests completed ===' as final_status;
