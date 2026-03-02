//===----------------------------------------------------------------------===//
//                         DuckDB
//
// ai_filter.cpp
//
// AI_filter ScalarFunction implementation - Batch Processing
//===----------------------------------------------------------------------===//

#include "ai_functions.hpp"
#include "duckdb/common/exception.hpp"
#include "duckdb/common/vector_operations/vector_operations.hpp"
#include "duckdb/planner/expression/bound_function_expression.hpp"
#include "duckdb/execution/expression_executor.hpp"

#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <sstream>
#include <thread>
#include <mutex>
#include <vector>
#include <atomic>
#include <future>

namespace duckdb {

// ============================================================================
// AI_filter: Real HTTP-based AI Similarity Filter Function
// ============================================================================
// Uses curl command to call OpenAI-compatible API
//
// SQL: ai_filter(image, prompt, model) -> DOUBLE
// Example: SELECT ai_filter('image_data', 'a cat', 'clip') FROM images;
// ============================================================================

// API Configuration
static constexpr char API_KEY[] = "sk-sxWGh4hWeExbe8sqZEkgBi4E9l8E53oaAaoYEzjxbzR5IOgk";
static constexpr char BASE_URL[] = "https://chatapi.littlewheat.com";
static constexpr char DEFAULT_MODEL[] = "chatgpt-4o-latest";

// Execute curl command and return response
static string CallAI_API(const string &image, const string &prompt, const string &model) {
	// Build JSON request body using GPT-4o Vision API format
	// Reference: https://platform.openai.com/docs/guides/vision
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

	// Build curl command
	std::ostringstream curl_cmd;
	curl_cmd << "curl -s '" << BASE_URL << "/v1/chat/completions' "
	         << "-H 'Authorization: Bearer " << API_KEY << "' "
	         << "-H 'Content-Type: application/json' "
	         << "-d '" << json_body.str() << "'";

	// Execute curl command
	FILE *pipe = popen(curl_cmd.str().c_str(), "r");
	if (!pipe) {
		return "0.5"; // Default score on error
	}

	// Read response
	char buffer[4096];
	std::ostringstream response;
	while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
		response << buffer;
	}
	pclose(pipe);

	return response.str();
}

// Extract score from JSON response
static double ExtractScore(const string &json_response) {
	// Try multiple parsing strategies

	// Strategy 1: Look for "content": "number" pattern
	size_t content_pos = json_response.find("\"content\":");
	if (content_pos != string::npos) {
		// Find the colon after "content"
		size_t colon_pos = json_response.find(":", content_pos);
		if (colon_pos != string::npos) {
			// Find the opening quote after colon
			size_t quote_start = json_response.find("\"", colon_pos);
			if (quote_start != string::npos) {
				quote_start++; // Skip the quote

				// Find the closing quote
				size_t quote_end = json_response.find("\"", quote_start);
				if (quote_end != string::npos) {
					string value_str = json_response.substr(quote_start, quote_end - quote_start);

					// Try to parse as number
					try {
						double score = std::stod(value_str);
						// Clamp to [0, 1]
						if (score < 0.0) score = 0.0;
						if (score > 1.0) score = 1.0;
						return score;
					} catch (...) {
						// Fall through to next strategy
					}
				}
			}
		}
	}

	// Strategy 2: Search for any decimal number in the response
	// Look for patterns like 0.75, 0.8, 0.123, etc.
	for (size_t i = 0; i < json_response.length() - 3; i++) {
		// Look for digit followed by decimal point
		if (json_response[i] >= '0' && json_response[i] <= '9' &&
		    json_response[i + 1] == '.') {
			// Found potential number start
			size_t start = i;
			size_t end = i + 2;

			// Find end of number
			while (end < json_response.length() &&
			       ((json_response[end] >= '0' && json_response[end] <= '9') ||
			        json_response[end] == '.')) {
				end++;
			}

			string num_str = json_response.substr(start, end - start);
			try {
				double score = std::stod(num_str);
				if (score >= 0.0 && score <= 1.0) {
					return score;
				}
			} catch (...) {
				// Continue searching
			}
		}
	}

	return 0.5; // Default if parsing fails
}

// Simple deterministic hash for fallback (when HTTP is slow/unavailable)
static double DeterministicScore(const string_t &image, const string_t &prompt) {
	// Simple hash-based scoring
	size_t hash = std::hash<string>{}(prompt.GetString());
	return (hash % 1000) / 1000.0;
}

static void AIFilterFunction(DataChunk &args, ExpressionState &state, Vector &result) {
	auto &image_vector = args.data[0];  // VARCHAR (base64 or placeholder)
	auto &prompt_vector = args.data[1]; // VARCHAR
	auto &model_vector = args.data[2];  // VARCHAR

	// Execute for each row
	TernaryExecutor::Execute<string_t, string_t, string_t, double>(
	    image_vector, prompt_vector, model_vector, result, args.size(),
	    [&](string_t image, string_t prompt, string_t model) {
		    string image_str = image.GetString();
		    string prompt_str = prompt.GetString();
		    string model_str = model.GetString();

		    if (model_str.empty()) {
			    model_str = DEFAULT_MODEL;
		    }

		    // Call AI API with actual image data
		    string response = CallAI_API(image_str, prompt_str, model_str);

		    // Extract score from response
		    double score = ExtractScore(response);

		    return score;
	    });

	// Mark result as volatile (API results can vary)
	result.SetVectorType(VectorType::FLAT_VECTOR);
}

// ============================================================================
// Batch Processing: ai_filter_batch with concurrent requests
// ============================================================================

// Configuration for batch processing
struct BatchConfig {
	static constexpr size_t DEFAULT_BATCH_SIZE = 10;
	static constexpr size_t MAX_CONCURRENT = 5;
	static constexpr int REQUEST_TIMEOUT_SEC = 30;
};

// Single request result
struct BatchResult {
	size_t index;
	double score;
};

// Process single image (thread-safe)
static BatchResult ProcessSingleImage(size_t idx, const string &image, const string &prompt, const string &model) {
	string response = CallAI_API(image, prompt, model);
	double score = ExtractScore(response);
	return BatchResult{idx, score};
}

static void AIFilterBatchFunction(DataChunk &args, ExpressionState &state, Vector &result) {
	auto &image_vector = args.data[0];  // VARCHAR (base64)
	auto &prompt_vector = args.data[1]; // VARCHAR
	auto &model_vector = args.data[2];  // VARCHAR

	size_t row_count = args.size();

	// Unified input vectors for batch processing
	UnifiedVectorFormat image_format;
	UnifiedVectorFormat prompt_format;
	UnifiedVectorFormat model_format;

	image_vector.ToUnifiedFormat(row_count, image_format);
	prompt_vector.ToUnifiedFormat(row_count, prompt_format);
	model_vector.ToUnifiedFormat(row_count, model_format);

	// Prepare data for batch processing
	vector<string> images(row_count);
	vector<string> prompts(row_count);
	vector<string> models(row_count);

	// Extract data from vectors
	for (size_t i = 0; i < row_count; i++) {
		// Get image
		auto image_idx = image_format.sel->get_index(i);
		if (!image_format.validity.RowIsValid(image_idx)) {
			images[i] = "";
		} else {
			auto image_val = ((string_t *)image_format.data)[image_idx];
			images[i] = image_val.GetString();
		}

		// Get prompt
		auto prompt_idx = prompt_format.sel->get_index(i);
		if (!prompt_format.validity.RowIsValid(prompt_idx)) {
			prompts[i] = "";
		} else {
			auto prompt_val = ((string_t *)prompt_format.data)[prompt_idx];
			prompts[i] = prompt_val.GetString();
		}

		// Get model
		auto model_idx = model_format.sel->get_index(i);
		if (!model_format.validity.RowIsValid(model_idx)) {
			models[i] = DEFAULT_MODEL;
		} else {
			auto model_val = ((string_t *)model_format.data)[model_idx];
			string model_str = model_val.GetString();
			models[i] = model_str.empty() ? DEFAULT_MODEL : model_str;
		}
	}

	// Process in batches with concurrency
	vector<double> scores(row_count, 0.5); // Default scores
	vector<future<BatchResult>> futures;

	for (size_t i = 0; i < row_count; i++) {
		// Skip empty images
		if (images[i].empty() || prompts[i].empty()) {
			scores[i] = 0.5;
			continue;
		}

		// Launch async request
		futures.push_back(async(launch::async, ProcessSingleImage, i, images[i], prompts[i], models[i]));

		// Wait if we reached max concurrent
		if (futures.size() >= BatchConfig::MAX_CONCURRENT) {
			for (auto &f : futures) {
				BatchResult res = f.get();
				scores[res.index] = res.score;
			}
			futures.clear();
		}
	}

	// Wait for remaining futures
	for (auto &f : futures) {
		BatchResult res = f.get();
		scores[res.index] = res.score;
	}

	// Write results
	auto result_data = FlatVector::GetData<double>(result);
	for (size_t i = 0; i < row_count; i++) {
		result_data[i] = scores[i];
	}

	result.SetVectorType(VectorType::FLAT_VECTOR);
}

ScalarFunction AIFunctions::GetAIFilterFunction() {
	// ai_filter(image VARCHAR, prompt VARCHAR, model VARCHAR) -> DOUBLE
	ScalarFunction ai_filter_function("ai_filter",
	                                  {LogicalType::VARCHAR, LogicalType::VARCHAR, LogicalType::VARCHAR},
	                                  LogicalType::DOUBLE, AIFilterFunction);

	// Mark as volatile since API results vary
	ai_filter_function.SetStability(FunctionStability::VOLATILE);

	return ai_filter_function;
}

ScalarFunction AIFunctions::GetAIFilterBatchFunction() {
	// ai_filter_batch(image VARCHAR, prompt VARCHAR, model VARCHAR) -> DOUBLE
	// Uses concurrent HTTP requests for better throughput
	ScalarFunction ai_filter_batch_function("ai_filter_batch",
	                                        {LogicalType::VARCHAR, LogicalType::VARCHAR, LogicalType::VARCHAR},
	                                        LogicalType::DOUBLE, AIFilterBatchFunction);

	// Mark as volatile since API results vary
	ai_filter_batch_function.SetStability(FunctionStability::VOLATILE);

	return ai_filter_batch_function;
}

void AIFunctions::RegisterScalarFunctions(ExtensionLoader &loader) {
	// Register AI_filter function (original, sequential)
	loader.RegisterFunction(GetAIFilterFunction());

	// Register AI_filter_batch function (concurrent)
	loader.RegisterFunction(GetAIFilterBatchFunction());
}

} // namespace duckdb
