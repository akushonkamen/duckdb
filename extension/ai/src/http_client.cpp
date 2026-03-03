//===----------------------------------------------------------------------===//
//                         DuckDB
//
// http_client.cpp - Production implementation of HTTP client interfaces
//   Provides real HTTP/popen execution for AI Filter functions
//===----------------------------------------------------------------------===//

#include "http_client_interface.hpp"
#include <cstdio>
#include <cstring>
#include <sstream>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <ctime>

namespace duckdb {

// ============================================================================
// Real HTTP Client Implementation
// ============================================================================

class RealHTTPClient : public IHTTPClient {
public:
	Response Post(const std::string& url,
	             const std::string& body,
	             int timeout_sec) override {
		// Build curl command
		std::ostringstream cmd;
		cmd << "curl -s "
		    << "--connect-timeout " << timeout_sec << " "
		    << "--max-time " << (timeout_sec + 5) << " "
		    << "'" << url << "' "
		    << "-H 'Content-Type: application/json' "
		    << "-H 'Authorization: Bearer sk-sxWGh4hWeExbe8sqZEkgBi4E9l8E53oaAaoYEzjxbzR5IOgk' "
		    << "-d '" << body << "'";

		// Execute command
		std::string output = ExecuteCommandInternal(cmd.str());

		// Parse status (curl returns 0 on success)
		Response response;
		response.status_code = (output.empty() && !body.empty()) ? -1 : 0;
		response.body = output;

		return response;
	}

	std::string ExecuteCommand(const std::string& command) override {
		return ExecuteCommandInternal(command);
	}

private:
	std::string ExecuteCommandInternal(const std::string& command) {
		FILE* pipe = popen(command.c_str(), "r");
		if (!pipe) {
			return "";
		}

		char buffer[4096];
		std::ostringstream response;
		size_t total_read = 0;
		while (fgets(buffer, sizeof(buffer), pipe) != nullptr && total_read < 100000) {
			response << buffer;
			total_read += strlen(buffer);
		}

		int status = pclose(pipe);
		if (status != 0) {
			return "";
		}

		return response.str();
	}
};

// ============================================================================
// Real System Clock Implementation
// ============================================================================

class RealSystemClock : public ISystemClock {
public:
	void SleepMs(int milliseconds) override {
		std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
	}
};

// ============================================================================
// Real Random Generator Implementation
// ============================================================================

class RealRandomGenerator : public IRandomGenerator {
public:
	RealRandomGenerator() {
		// Seed random number generator
		std::srand(static_cast<unsigned>(std::time(nullptr)));
	}

	int GetRandomInt(int min, int max) override {
		if (max <= min) {
			return min;
		}
		return min + (std::rand() % (max - min + 1));
	}
};

// ============================================================================
// Factory Implementations
// ============================================================================

std::unique_ptr<IHTTPClient> ComponentFactory::CreateDefaultHTTPClient() {
	return std::make_unique<RealHTTPClient>();
}

std::unique_ptr<ISystemClock> ComponentFactory::CreateDefaultSystemClock() {
	return std::make_unique<RealSystemClock>();
}

std::unique_ptr<IRandomGenerator> ComponentFactory::CreateDefaultRandomGenerator() {
	return std::make_unique<RealRandomGenerator>();
}

} // namespace duckdb
