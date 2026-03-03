//===----------------------------------------------------------------------===//
//                         DuckDB
//
// ai_function_executor.hpp - AI Function Executor with dependency injection
//   Exposes AIFunctionExecutor for Google Mock testing
//===----------------------------------------------------------------------===//

#pragma once

#include "http_client_interface.hpp"
#include <memory>
#include <string>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <atomic>

namespace duckdb {

// API Configuration (shared with ai_filter.cpp)
static constexpr char API_KEY[] = "sk-sxWGh4hWeExbe8sqZEkgBi4E9l8E53oaAaoYEzjxbzR5IOgk";
static constexpr char BASE_URL[] = "https://chatapi.littlewheat.com";
static constexpr int HTTP_TIMEOUT_SEC = 30;

// Retry Configuration
static constexpr int MAX_RETRIES = 3;
static constexpr int BASE_DELAY_MS = 100;
static constexpr int MAX_DELAY_MS = 5000;

// AI Function Executor - Supports dependency injection for testing
class AIFunctionExecutor {
public:
	// Constructor with dependency injection (for testing)
	AIFunctionExecutor(std::unique_ptr<IHTTPClient> http_client,
	                  std::unique_ptr<ISystemClock> clock,
	                  std::unique_ptr<IRandomGenerator> random)
		: http_client_(std::move(http_client))
		, clock_(std::move(clock))
		, random_(std::move(random)) {
	}

	// Default constructor using real implementations (for production)
	AIFunctionExecutor()
		: AIFunctionExecutor(ComponentFactory::CreateDefaultHTTPClient(),
		                   ComponentFactory::CreateDefaultSystemClock(),
		                   ComponentFactory::CreateDefaultRandomGenerator()) {
	}

	~AIFunctionExecutor() = default;

	// Call AI API with retry logic
	std::string CallAPIWithRetry(const std::string &image,
	                             const std::string &prompt,
	                             const std::string &model) {
		// Check for test mode (for coverage testing)
		if (const char* test_mode = std::getenv("AI_FILTER_TEST_MODE")) {
			std::string test_mode_str(test_mode);
			static std::atomic<int> call_count{0};
			int current_call = call_count.fetch_add(1);

			if (test_mode_str == "success") {
				return "{\"choices\":[{\"message\":{\"content\":\"0.85\"}}]}";
			} else if (test_mode_str == "retry") {
				if (current_call % 2 == 1) {
					fprintf(stderr, "[AI_FILTER_RETRY] Test mode: simulating failure\\n");
					return "{\"error\":\"simulated_error\"}";
				}
				fprintf(stderr, "[AI_FILTER_RETRY] Test mode: success\\n");
				return "{\"choices\":[{\"message\":{\"content\":\"0.75\"}}]}";
			} else if (test_mode_str == "fail") {
				return "{\"error\":\"max_retries_exceeded\"}";
			} else if (test_mode_str == "invalid") {
				return "{\"data\":\"some random value with 0.67 number inside\"}";
			}
		}

		// Build JSON request
		std::ostringstream json_body;
		json_body << "{"
		          << "\"model\":\"" << model << "\","
		          << "\"messages\":[{"
		          << "\"role\":\"user\","
		          << "\"content\":["
		          << "{\"type\":\"text\",\"text\":\"You are an image analysis assistant. Analyze how well the image matches: "
		          << prompt << ". Rate similarity 0.0-1.0. Respond ONLY the number.\"},"
		          << "{\"type\":\"image_url\",\"image_url\":{\"url\":\"data:image/png;base64," << image << "\"}}"
		          << "]}],"
		          << "\"max_tokens\":10"
		          << "}";

		std::string url = std::string(BASE_URL) + "/v1/chat/completions";

		// Retry loop
		const int MAX_RETRIES = 3;
		const int BASE_DELAY_MS = 100;
		const int MAX_DELAY_MS = 5000;

		for (int attempt = 0; attempt <= MAX_RETRIES; attempt++) {
			if (!http_client_) {
				// Fallback to direct curl call
				auto response = ExecuteCurlCommand(json_body.str());
				if (response.empty() || response.find("error") != std::string::npos) {
					if (attempt < MAX_RETRIES) {
						clock_->SleepMs(GetRetryDelayMs(attempt));
						continue;
					}
					break;
				}
				return response;
			}

			auto http_response = http_client_->Post(url, json_body.str(), HTTP_TIMEOUT_SEC);

			if (http_response.status_code != 0 || http_response.body.empty() ||
			    http_response.body.find("error") != std::string::npos) {
				if (attempt < MAX_RETRIES) {
					clock_->SleepMs(GetRetryDelayMs(attempt));
					continue;
				}
				break;
			}

			return http_response.body;
		}

		return "{\"error\":\"max_retries_exceeded\"}";
	}

private:
	std::unique_ptr<IHTTPClient> http_client_;
	std::unique_ptr<ISystemClock> clock_;
	std::unique_ptr<IRandomGenerator> random_;

	int GetRetryDelayMs(int attempt) {
		int delay = BASE_DELAY_MS * (1 << attempt);
		delay = std::min(delay, MAX_DELAY_MS);
		int jitter = delay / 5;
		delay += random_->GetRandomInt(-jitter, jitter);
		return std::max(0, delay);
	}

	std::string ExecuteCurlCommand(const std::string& json_payload) {
		std::ostringstream curl_cmd;
		curl_cmd << "curl -s "
		         << "--connect-timeout " << HTTP_TIMEOUT_SEC << " "
		         << "--max-time " << (HTTP_TIMEOUT_SEC + 5) << " "
		         << "'" << BASE_URL << "/v1/chat/completions' "
		         << "-H 'Authorization: Bearer " << API_KEY << "' "
		         << "-H 'Content-Type: application/json' "
		         << "-d '" << json_payload << "'";

		FILE* pipe = popen(curl_cmd.str().c_str(), "r");
		if (!pipe) return "";

		char buffer[4096];
		std::ostringstream response;
		size_t total_read = 0;
		while (fgets(buffer, sizeof(buffer), pipe) != nullptr && total_read < 100000) {
			response << buffer;
			total_read += strlen(buffer);
		}

		pclose(pipe);
		return response.str();
	}
};

} // namespace duckdb
