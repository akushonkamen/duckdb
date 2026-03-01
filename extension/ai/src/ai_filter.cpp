//===----------------------------------------------------------------------===//
//                         DuckDB
//
// ai_filter.cpp
//
// AI_filter ScalarFunction implementation - Real HTTP Calls
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
	// Build JSON request body with actual image analysis prompt
	std::ostringstream json_body;
	json_body << "{\"model\":\"" << model << "\","
	          << "\"messages\":[{\"role\":\"user\",\"content\":\""
	          << "You are an image analysis assistant. I will provide you with a base64-encoded image. "
	          << "Analyze how well the image matches the description: '" << prompt << "'. "
	          << "Rate the similarity as a decimal number between 0.0 (not similar) and 1.0 (very similar). "
	          << "Image data (base64): " << image << " "
	          << "Respond with ONLY the number, nothing else."
	          << "\"}],"
	          << "\"max_tokens\":10}";

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

ScalarFunction AIFunctions::GetAIFilterFunction() {
	// ai_filter(image VARCHAR, prompt VARCHAR, model VARCHAR) -> DOUBLE
	ScalarFunction ai_filter_function("ai_filter",
	                                  {LogicalType::VARCHAR, LogicalType::VARCHAR, LogicalType::VARCHAR},
	                                  LogicalType::DOUBLE, AIFilterFunction);

	// Mark as volatile since API results vary
	ai_filter_function.SetStability(FunctionStability::VOLATILE);

	return ai_filter_function;
}

void AIFunctions::RegisterScalarFunctions(ExtensionLoader &loader) {
	// Register AI_filter function
	loader.RegisterFunction(GetAIFilterFunction());
}

} // namespace duckdb
