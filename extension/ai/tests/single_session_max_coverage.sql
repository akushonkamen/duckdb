-- Single session maximum coverage test
-- All tests run in one session to accumulate coverage data
LOAD '/Users/yp1017/development/daft-duckdb-multimodal/duckdb/build/test/extension/ai.duckdb_extension';

SELECT '=== Starting Single Session Coverage Test ===' as status;

-- ============================================================================
-- Part 1: NULL and empty handling (early returns)
-- ============================================================================
SELECT '=== NULL Handling ===' as section;
SELECT ai_filter(NULL, 'cat', 'clip') as null_url;
SELECT ai_filter('url', NULL, 'clip') as null_query;
SELECT ai_filter('url', 'cat', NULL) as null_model;
SELECT ai_filter_batch(NULL, 'cat', 'clip') as batch_null_url;

SELECT '=== Empty Strings ===' as section;
SELECT ai_filter('', 'cat', 'clip') as empty_url;
SELECT ai_filter('url', '', 'clip') as empty_query;
SELECT ai_filter('url', 'cat', '') as empty_model;
SELECT ai_filter_batch('', 'cat', 'clip') as batch_empty_url;

-- ============================================================================
-- Part 2: Success mode (successful API responses)
-- ============================================================================
SELECT '=== Success Mode Tests ===' as section;
SELECT ai_filter('url1', 'cat', 'clip') as success1;
SELECT ai_filter('url2', 'dog', 'clip') as success2;
SELECT ai_filter('url3', 'bird', 'clip') as success3;
SELECT ai_filter_batch('url4', 'cat', 'clip') as batch_success1;
SELECT ai_filter_batch('url5', 'dog', 'clip') as batch_success2;

-- ============================================================================
-- Part 3: Different models (covers model string handling)
-- ============================================================================
SELECT '=== Different Models ===' as section;
SELECT ai_filter('url', 'cat', 'clip') as model_clip;
SELECT ai_filter('url', 'cat', 'openclip') as model_openclip;
SELECT ai_filter('url', 'cat', 'chatgpt-4o-latest') as model_full;
SELECT ai_filter('url', 'cat', '') as model_empty;

-- ============================================================================
-- Part 4: Batch processing with various inputs
-- ============================================================================
SELECT '=== Batch Processing ===' as section;
SELECT ai_filter_batch('url6', 'cat', 'clip') as batch1;
SELECT ai_filter_batch('url7', 'dog', 'clip') as batch2;
SELECT ai_filter_batch('url8', 'bird', 'clip') as batch3;
SELECT ai_filter_batch('url9', 'car', 'clip') as batch4;

-- ============================================================================
-- Part 5: Special query strings
-- ============================================================================
SELECT '=== Special Queries ===' as section;
SELECT ai_filter('url', 'cat!@#$', 'clip') as special_chars;
SELECT ai_filter('url', 'a very long query string with many words', 'clip') as long_query;

-- ============================================================================
-- Part 6: Sequential calls (same URL, different queries)
-- ============================================================================
SELECT '=== Sequential Calls ===' as section;
SELECT ai_filter('same_url', 'cat', 'clip') as seq1;
SELECT ai_filter('same_url', 'dog', 'clip') as seq2;
SELECT ai_filter('same_url', 'bird', 'clip') as seq3;

SELECT '=== Single Session Test Complete ===' as final_status;
