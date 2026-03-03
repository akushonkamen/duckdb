-- Comprehensive coverage test for AI extension
-- This script exercises all code paths in a single session to maximize coverage

-- Note: Must run with -unsigned flag

-- 1. Load extension
LOAD '/Users/yp1017/development/daft-duckdb-multimodal/duckdb/build/test/extension/ai.duckdb_extension';

-- Verify extension loaded
SELECT 'Extension loaded successfully' as status;

-- 2. Test ai_filter with valid inputs (main path)
SELECT 'Test 1: Basic ai_filter' as test_name;
SELECT ai_filter('http://example.com/cat.jpg', 'cat', 'clip') as result;

-- 3. Test ai_filter_batch (vectorized batch path)
SELECT 'Test 2: ai_filter_batch' as test_name;
SELECT ai_filter_batch('http://example.com/dog.jpg', 'dog', 'clip') as batch_result;

-- 4. Test NULL inputs (error handling path)
SELECT 'Test 3: NULL image_url' as test_name;
SELECT ai_filter(NULL, 'cat', 'clip') as null_url_result;

SELECT 'Test 4: NULL query' as test_name;
SELECT ai_filter('http://example.com/img.jpg', NULL, 'clip') as null_query_result;

SELECT 'Test 5: NULL model' as test_name;
SELECT ai_filter('http://example.com/img.jpg', 'cat', NULL) as null_model_result;

-- 5. Test empty strings (edge case handling)
SELECT 'Test 6: Empty image_url' as test_name;
SELECT ai_filter('', 'cat', 'clip') as empty_url_result;

SELECT 'Test 7: Empty query' as test_name;
SELECT ai_filter('http://example.com/img.jpg', '', 'clip') as empty_query_result;

SELECT 'Test 8: Empty model (fallback to default)' as test_name;
SELECT ai_filter('http://example.com/img.jpg', 'cat', '') as empty_model_result;

-- 6. Test ai_filter_batch with NULL inputs
SELECT 'Test 9: ai_filter_batch NULL inputs' as test_name;
SELECT ai_filter_batch(NULL, 'cat', 'clip') as batch_null_url;
SELECT ai_filter_batch('http://example.com/img.jpg', NULL, 'clip') as batch_null_query;
SELECT ai_filter_batch('http://example.com/img.jpg', 'cat', NULL) as batch_null_model;

-- 7. Test concurrent-like usage with multiple rows
SELECT 'Test 10: Multiple row processing' as test_name;
CREATE TABLE test_images AS
SELECT * FROM (
    VALUES
        ('http://example.com/img1.jpg', 'cat'),
        ('http://example.com/img2.jpg', 'dog'),
        ('http://example.com/img3.jpg', 'bird'),
        ('http://example.com/img4.jpg', 'car'),
        ('http://example.com/img5.jpg', 'tree')
) AS t(image_url, query);

SELECT image_url, query, ai_filter(image_url, query, 'clip') as score
FROM test_images;

-- 8. Test ai_filter_batch with multiple rows
SELECT 'Test 11: Batch processing multiple rows' as test_name;
SELECT image_url, query, ai_filter_batch(image_url, query, 'clip') as batch_score
FROM test_images;

-- 9. Test with different models (if available)
SELECT 'Test 12: Different model names' as test_name;
SELECT ai_filter('http://example.com/img.jpg', 'cat', 'clip') as clip_model;
SELECT ai_filter('http://example.com/img.jpg', 'cat', 'openclip') as openclip_model;

-- 10. Stress test - many rows to test concurrent path
SELECT 'Test 13: Stress test with 50 rows' as test_name;
CREATE TABLE stress_test AS
SELECT 'http://example.com/img' || i || '.jpg' as url, 'cat' as query
FROM range(50) t(i);

SELECT COUNT(*) as total_rows,
       AVG(ai_filter(url, query, 'clip')) as avg_score
FROM stress_test;

-- 11. Test ai_filter_batch on stress data
SELECT 'Test 14: Batch stress test' as test_name;
SELECT COUNT(*) as total_batch,
       AVG(ai_filter_batch(url, query, 'clip')) as avg_batch_score
FROM stress_test;

-- 12. Edge case - very long query string
SELECT 'Test 15: Long query string' as test_name;
SELECT ai_filter('http://example.com/img.jpg',
                 'a very long description of an image with many words to test handling',
                 'clip') as long_query_result;

-- 13. Edge case - special characters in query
SELECT 'Test 16: Special characters' as test_name;
SELECT ai_filter('http://example.com/img.jpg', 'cat!@#$%^&*()', 'clip') as special_chars_result;

-- 14. Test repeated calls (same URL, different queries)
SELECT 'Test 17: Same URL, different queries' as test_name;
SELECT ai_filter('http://example.com/reuse.jpg', 'cat', 'clip') as cat_score;
SELECT ai_filter('http://example.com/reuse.jpg', 'dog', 'clip') as dog_score;

-- Cleanup
DROP TABLE test_images;
DROP TABLE stress_test;

-- Final verification
SELECT 'All tests completed' as final_status;
SELECT function_name, function_type
FROM duckdb_functions()
WHERE function_name LIKE 'ai_%'
ORDER BY function_name;
