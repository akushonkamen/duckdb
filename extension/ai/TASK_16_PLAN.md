# TASK-16: Real HTTP Implementation Plan

## Goal
Replace mock HTTP client with real httplib-based network calls

## Technical Approach

### Phase 1: Minimal Working HTTP Client
1. Include httplib properly in extension build
2. Create simple POST request function
3. Test with mock server

### Phase 2: Error Handling
1. Timeout configuration
2. Connection error handling
3. Fallback to default score on failure

### Phase 3: Testing
1. Create mock AI service (Python Flask)
2. Verify HTTP calls work
3. Test error scenarios

## Implementation Strategy

### Why Previous Attempt Failed
- httplib.hpp include path issues
- SSL/TLS complexity
- Extension build system constraints

### New Approach
- Use standard C++ socket/CURLOPT for simplicity
- OR fix httplib includes properly
- Start with HTTP (not HTTPS) for MVP

## Files to Modify
1. `ai_extension_loadable.cpp` - Update to use real HTTP
2. `test_mock_ai_server.py` - Create test server
3. `CMakeLists.txt` - Update if needed

## Success Criteria
- ✅ Real HTTP POST to mock server
- ✅ JSON response parsing
- ✅ Error handling (timeout, connection fail)
- ✅ Fallback to default score
