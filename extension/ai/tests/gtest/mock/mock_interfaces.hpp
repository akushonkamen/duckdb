//===----------------------------------------------------------------------===//
//                         DuckDB
//
// mock_interfaces.hpp - Mock classes for Google Mock testing
//   Provides mock implementations of IHTTPClient, ISystemClock, IRandomGenerator
//===----------------------------------------------------------------------===//

#pragma once

#include "http_client_interface.hpp"
#include <gmock/gmock.h>
#include <string>

namespace duckdb {
namespace mock {

// ============================================================================
// Mock HTTP Client
// ============================================================================

class MockHTTPClient : public IHTTPClient {
public:
	MOCK_METHOD(Response, Post,
	            (const std::string& url, const std::string& body, int timeout_sec),
	            (override));

	MOCK_METHOD(std::string, ExecuteCommand,
	            (const std::string& command),
	            (override));
};

// ============================================================================
// Mock System Clock
// ============================================================================

class MockSystemClock : public ISystemClock {
public:
	MOCK_METHOD(void, SleepMs, (int milliseconds), (override));
};

// ============================================================================
// Mock Random Generator
// ============================================================================

class MockRandomGenerator : public IRandomGenerator {
public:
	MOCK_METHOD(int, GetRandomInt, (int min, int max), (override));
};

} // namespace mock
} // namespace duckdb
