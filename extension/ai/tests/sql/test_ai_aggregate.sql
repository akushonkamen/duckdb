-- AI Aggregate Functions Test
-- Test ai_similarity_avg aggregate function

-- Load AI extension
LOAD /path/to/ai_extension;

-- Test basic aggregation with mock data (using test mode)
SET AI_FILTER_TEST_MODE=success;

-- Create test table with images
CREATE OR REPLACE TABLE test_products (
    product_id INTEGER,
    product_name VARCHAR,
    image_data BLOB
);

-- Insert test data (simulated base64 encoded images)
INSERT INTO test_products VALUES
    (1, 'Product A', 'fake_base64_image_a'),
    (2, 'Product A', 'fake_base64_image_a'),
    (3, 'Product B', 'fake_base64_image_b'),
    (4, 'Product B', 'fake_base64_image_b');

-- Test ai_similarity_avg - should return average similarity score
SELECT
    product_id,
    ai_similarity_avg(image_data, 'product quality', 'clip') as avg_similarity
FROM test_products
GROUP BY product_id
ORDER BY product_id;

-- Test with empty group (should return NULL)
SELECT
    product_id,
    ai_similarity_avg(image_data, 'product quality', 'clip') as avg_similarity
FROM test_products
WHERE product_id > 100
GROUP BY product_id;

-- Test ai_similarity_avg with NULL handling
CREATE OR REPLACE TABLE test_products_nulls (
    product_id INTEGER,
    image_data BLOB
);

INSERT INTO test_products_nulls VALUES
    (1, 'fake_base64_image_a'),
    (1, NULL),
    (1, 'fake_base64_image_b'),
    (2, NULL),
    (2, 'fake_base64_image_c');

SELECT
    product_id,
    ai_similarity_avg(image_data, 'product quality', 'clip') as avg_similarity
FROM test_products_nulls
GROUP BY product_id
ORDER BY product_id;

-- Clean up
DROP TABLE test_products;
DROP TABLE test_products_nulls;
