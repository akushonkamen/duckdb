//===----------------------------------------------------------------------===//
//                         DuckDB
//
// http_client_interface.hpp - Abstract interfaces for dependency injection
//   Enables Google Mock testing for AI Filter functions
//===----------------------------------------------------------------------===//

#pragma once

#include <string>
#include <memory>

namespace duckdb {

// ============================================================================
// HTTP Client Interface
// ============================================================================

class IHTTPClient {
public:
	struct Response {
		int status_code;
		std::string body;
	};

	virtual ~IHTTPClient() = default;

	// Execute HTTP POST request
	// Returns response with status code and body
	virtual Response Post(const std::string& url,
	                     const std::string& body,
	                     int timeout_sec) = 0;

	// Execute command via popen (for backward compatibility)
	// Returns the command output as a string
	virtual std::string ExecuteCommand(const std::string& command) = 0;
};

// ============================================================================
// System Clock Interface (for testing retry delays)
// ============================================================================

class ISystemClock {
public:
	virtual ~ISystemClock() = default;

	// Sleep for specified milliseconds
	virtual void SleepMs(int milliseconds) = 0;
};

// ============================================================================
// Random Generator Interface (for testing jitter in retry logic)
// ============================================================================

class IRandomGenerator {
public:
	virtual ~IRandomGenerator() = default;

	// Get random integer in range [min, max]
	virtual int GetRandomInt(int min, int max) = 0;
};

// ============================================================================
// Factory for creating real implementations
// ============================================================================

struct ComponentFactory {
	static std::unique_ptr<IHTTPClient> CreateDefaultHTTPClient();
	static std::unique_ptr<ISystemClock> CreateDefaultSystemClock();
	static std::unique_ptr<IRandomGenerator> CreateDefaultRandomGenerator();
};

} // namespace duckdb
