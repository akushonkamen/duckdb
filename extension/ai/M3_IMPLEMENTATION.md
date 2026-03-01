# M3 Implementation: HTTP-based AI Filter

## Overview
M3 implements HTTP-based AI service integration for the `ai_filter` function, replacing the mock constant values from M1 with simulated HTTP calls.

## Implementation Details

### Architecture
```
DuckDB Extension
    ↓
ai_filter(image, prompt, model)
    ↓
HTTP Client (MVP - simulated)
    ↓
JSON Request/Response Parsing
    ↓
Similarity Score (DOUBLE)
```

### API Signature
```sql
ai_filter(image VARCHAR, prompt VARCHAR, model VARCHAR) -> DOUBLE
```

**Parameters:**
- `image`: Image data (base64 encoded string for MVP)
- `prompt`: Text prompt (e.g., "cat", "dog")
- `model`: Model name (e.g., "clip")

**Returns:**
- Similarity score (0.0 - 1.0+)

### Current Implementation (M3 MVP)

#### HTTP Client
- **Type**: Simulated HTTP client (not real network calls)
- **Purpose**: Demonstrate request/response flow
- **Behavior**: Generates deterministic scores based on prompt
- **Future**: Will be replaced with real httplib in M4

#### Score Generation
```cpp
double generate_mock_score(const std::string& prompt) {
    // Generate deterministic mock score based on prompt
    double score = 0.5;
    for (char c : prompt) {
        score = (score * 31.0 + c) / 32.0;
    }
    return score;
}
```

#### JSON Format

**Request:**
```json
{
  "image": "base64_encoded_image",
  "prompt": "cat",
  "model": "clip"
}
```

**Response:**
```json
{
  "score": 0.95,
  "latency_ms": 50,
  "model": "clip",
  "mock": true
}
```

## Testing

### Basic Usage
```sql
LOAD 'test/extension/ai.duckdb_extension';

SELECT ai_filter('image_data', 'cat', 'clip') AS similarity;
-- Result: ~9.92 (deterministic based on "cat")
```

### Multi-Row Processing
```sql
SELECT prompt, ai_filter('img', prompt, 'clip') AS score
FROM (SELECT 'cat' AS prompt
      UNION ALL SELECT 'dog'
      UNION ALL SELECT 'bird') t;
```

### Deterministic Behavior
```sql
-- Same prompt always returns same score
SELECT ai_filter('img', 'cat', 'clip') FROM range(3);
-- All rows: 9.91951
```

## Limitations (M3 MVP)

1. **No Real HTTP Calls**: Current implementation simulates HTTP
2. **No Network I/O**: All processing is in-memory
3. **Mock Scores**: Scores are deterministic hashes, not real AI inference
4. **No Error Handling**: Network errors not tested
5. **No Timeout**: No timeout handling implemented
6. **No Connection Pooling**: Single client per query

## M4 Enhancements (Planned)

1. **Real HTTP Client**: Integrate httplib for actual network calls
2. **AI Service Integration**: Connect to real CLIP/LLM APIs
3. **Error Handling**: Proper network error handling and retries
4. **Performance**: Connection pooling, batching
5. **Configuration**: Configurable endpoints and timeouts
6. **Authentication**: API key support for AI services

## Files Modified

- `extension/ai/ai_extension_loadable.cpp` - Main implementation
- `test/extension/CMakeLists.txt` - Build configuration

## Performance Notes

- **Function Type**: VOLATILE (scores determined at runtime)
- **Processing**: Row-by-row (no batching in M3)
- **Mock Latency**: Simulated 50ms per request
- **Future**: M4 will add batching for better performance

## Migration from M1 to M3

### M1 (Constant Mock)
```sql
SELECT ai_filter() AS score;  -- Returns 0.42
```

### M3 (HTTP Simulation)
```sql
SELECT ai_filter('image', 'prompt', 'model') AS score;  -- Returns calculated score
```

**Breaking Changes:**
- Function now requires 3 parameters (was 0 in M1)
- Scores vary by prompt (was constant in M1)
- Function is VOLATILE (was CONSISTENT in M1)

---

**Version**: M3 MVP
**Status**: ✅ Complete and Tested
**Date**: 2026-03-01
**Next**: M4 - Real HTTP Integration
