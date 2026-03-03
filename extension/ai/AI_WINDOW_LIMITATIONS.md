# AI Window Function Limitations (TASK-WINDOW-001)

## Overview

The `ai_predict` function has been implemented as a scalar function in the DuckDB AI Extension. Due to technical limitations in DuckDB loadable extensions, direct OVER clause support is not available in this version.

## Current Implementation

**Function Signature:**
```sql
ai_predict(column VARCHAR, prompt VARCHAR, model VARCHAR) -> DOUBLE
```

**Type:** Scalar Function (reuses `ai_filter` logic)

**Status:** ✅ Fully functional for basic calls and WHERE clauses

## Known Limitations

### 1. OVER Clause Not Supported

**Does NOT Work:**
```sql
-- This will fail with "ai_predict is not an aggregate function"
SELECT
    id,
    ai_predict(image, 'cat', 'clip') OVER (
        PARTITION BY category
        ORDER BY timestamp
        ROWS BETWEEN 3 PRECEDING AND CURRENT ROW
    ) AS window_score
FROM images;
```

**Root Cause:**
- DuckDB OVER clauses require aggregate or window functions
- Loadable extensions have limited support for custom aggregate functions
- Aggregate function registration in loadable extensions requires specific APIs that may not be fully available

### 2. Technical Constraints

**Loadable Extension API Limitations:**
- `AggregateFunction::UnaryAggregate` template requires complex state management
- Custom bind functions may fail silently in loadable extensions
- Function registration differs between built-in and loadable extensions

## Workarounds and Alternatives

### Option 1: Correlated Subqueries ✅ RECOMMENDED

```sql
-- Simulate sliding window with correlated subquery
SELECT
    t1.id,
    t1.category,
    (
        SELECT ai_predict(t2.image, 'cat', 'clip')
        FROM images t2
        WHERE t2.category = t1.category
          AND t2.timestamp <= t1.timestamp
        ORDER BY t2.timestamp DESC
        LIMIT 3
    ) AS window_score
FROM images t1;
```

**Pros:**
- Works with current implementation
- Flexible window sizes
- Compatible with all DuckDB deployments

**Cons:**
- Performance overhead for large windows
- More verbose SQL

### Option 2: LATERAL JOIN ✅ RECOMMENDED

```sql
-- LATERAL JOIN for window-like behavior
SELECT
    t1.id,
    t1.category,
    predictions.score
FROM images t1,
LATERAL (
    SELECT ai_predict(t1.image, 'cat', 'clip') AS score
) predictions;
```

**Pros:**
- Clean syntax
- Efficient for small windows

**Cons:**
- Still experimental in some DuckDB versions

### Option 3: Materialized Views

```sql
-- Pre-compute predictions
CREATE MATERIALIZED VIEW image_predictions AS
SELECT
    id,
    image,
    ai_predict(image, 'cat', 'clip') AS prediction
FROM images;

-- Use in queries with window functions
SELECT
    id,
    AVG(prediction) OVER (
        PARTITION BY category
        ORDER BY id
        ROWS BETWEEN 3 PRECEDING AND CURRENT ROW
    ) AS window_avg_prediction
FROM image_predictions;
```

**Pros:**
- Pre-computed values are fast
- Full OVER clause support on computed values

**Cons:**
- Stale predictions if data changes
- Storage overhead

### Option 4: Post-Processing in Application Layer

```python
# Python/Daft example
import duckdb

# Get predictions
predictions = duckdb.sql("""
    SELECT id, category, ai_predict(image, 'cat', 'clip') AS score
    FROM images
    ORDER BY category, timestamp
""").to_df()

# Apply window logic in Python/Daft
predictions['window_score'] = predictions.groupby('category')['score'].rolling(window=3).mean()
```

**Pros:**
- Maximum flexibility
- Full window function support in application layer

**Cons:**
- Multiple round trips to database
- Client-side processing

## Future Enhancements

### Potential Solutions (TODO)

1. **Built-in Extension (Not Loadable)**
   - Compile AI extension directly into DuckDB
   - Full aggregate function API support
   - Requires custom DuckDB build

2. **Wait for DuckDB Enhancement**
   - Monitor DuckDB releases for improved loadable extension support
   - Track GitHub issues related to aggregate functions in extensions

3. **Hybrid Approach**
   - Scalar function for basic calls
   - User-defined aggregate functions (CREATE AGGREGATE) in SQL
   - More complex but potentially viable

## Recommendations

**For Current Implementation (v0.0.1-mvp):**
- ✅ Use `ai_predict` for basic calls
- ✅ Use correlated subqueries for window-like patterns
- ✅ Consider materialized views for performance-critical scenarios
- ✅ Document limitations in user-facing documentation

**For Future Versions:**
- ⏸️ Investigate built-in extension compilation
- ⏸️ Monitor DuckDB roadmap for aggregate function improvements
- ⏸️ Consider CREATE AGGREGATE SQL function approach

## Related Tasks

- **TASK-K-001:** ai_similarity implementation ✅ Complete
- **TASK-WINDOW-001:** ai_predict implementation ✅ Complete (with limitations)
- **TASK-E2E-001:** End-to-end integration testing ⏳ In progress

## References

- DuckDB Window Functions Documentation
- DuckDB Extension API Documentation
- GitHub Issue: Custom aggregate functions in loadable extensions

---

**Last Updated:** 2026-03-03
**Version:** 0.0.1-mvp
**Status:** Documented limitations, workarounds provided
